/*
    This file is part of IanniX, a graphical real-time open-source sequencer for digital art
    Copyright (C) 2010-2015 - IanniX Association (https://www.iannix.org/)
    Copyright (C) 2025-2026 - Hypar.XYZ (https://iannix.hypar.xyz/)

    IanniX is a free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "glpainter.h"
#include "glgeom.h"
#include "misc/application.h"

#include <QDebug>
#include <QHash>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLExtraFunctions>
#include <QRect>
#include <QtMath>

#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE 0x84F5
#endif

GlPainter *GlPainter::s_instance = nullptr;
GlPainter *GlPainter::s_current = nullptr;

namespace {

QHash<QOpenGLContext *, GlPainter *> &painterMap()
{
    static QHash<QOpenGLContext *, GlPainter *> map;
    return map;
}


const char *kColorVert = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
uniform mat4 uMVP;
invariant gl_Position;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

const char *kColorFrag = R"(
#version 330 core
uniform vec4 uColor;
out vec4 fragColor;
void main() {
    fragColor = uColor;
}
)";

const char *kWideLineVert = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec3 aFarPrev;
layout(location = 3) in vec3 aFarNext;
uniform mat4 uMVP;
flat out vec4 vFarPrevClip;
flat out vec4 vFarNextClip;
void main() {
    vFarPrevClip = uMVP * vec4(aFarPrev, 1.0);
    vFarNextClip = uMVP * vec4(aFarNext, 1.0);
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

const char *kTextureVert = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
uniform mat4 uMVP;
out vec2 vTexCoord;
void main() {
    vTexCoord = aTexCoord;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

const char *kTextureFrag = R"(
#version 330 core
in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform vec4 uColor;
out vec4 fragColor;
void main() {
    fragColor = texture(uTexture, vTexCoord) * uColor;
}
)";

// Wide lines were removed from Core profile (glLineWidth > 1 is invalid).
// Lines are drawn as GL_LINES_ADJACENCY (see GlGeom::toLineAdjacency): each
// segment is expanded into a screen-space quad and the fragment shader
// computes coverage from the pixel-to-segment distance, so edges and joints
// antialias without GL_LINE_SMOOTH.
//
// Joints are round. To avoid double blending where neighbouring capsules
// overlap (visible as dark dots on translucent curves), a fragment is kept
// only by the segment nearest to it among (prev, self, next); ties go to the
// earlier segment. Free ends (no neighbour) get butt caps, like legacy
// glLineWidth, so dash lengths stay true.
//
// vEnds per side: 0 = free end, 1 = joined to a projected neighbour that can
// compete, 2 = joined but no usable projected neighbour was found.
const char *kWideLineGeom = R"(
#version 330 core
layout(lines_adjacency) in;
layout(triangle_strip, max_vertices = 4) out;
flat in vec4 vFarPrevClip[];
flat in vec4 vFarNextClip[];
uniform vec2 uViewport;
uniform vec2 uViewportOrigin;
uniform float uLineWidthPx;
flat out vec2 vPrev;
flat out vec2 vP0;
flat out vec2 vP1;
flat out vec2 vNext;
flat out vec2 vEnds;

vec2 toScreen(vec4 clip) {
    vec2 ndc = clip.xy / clip.w;
    return (ndc * 0.5 + 0.5) * uViewport + uViewportOrigin;
}

vec4 fromScreen(vec2 screen, vec4 refClip) {
    vec2 ndc = ((screen - uViewportOrigin) / uViewport) * 2.0 - 1.0;
    return vec4(ndc * refClip.w, refClip.z, refClip.w);
}

float endState(vec4 neighbour, vec4 self, vec4 fallback, vec2 selfScreen,
               out vec2 neighbourScreen) {
    neighbourScreen = selfScreen;
    if (neighbour == self)
        return 0.0;
    if (neighbour.w > 0.0) {
        neighbourScreen = toScreen(neighbour);
        if (length(neighbourScreen - selfScreen) >= 1e-4)
            return 1.0;
    }
    if (fallback.w > 0.0) {
        vec2 fallbackScreen = toScreen(fallback);
        if (length(fallbackScreen - selfScreen) >= 1e-4) {
            neighbourScreen = fallbackScreen;
            return 1.0;
        }
    }
    return 2.0;
}

void main() {
    vec4 c0 = gl_in[1].gl_Position;
    vec4 c1 = gl_in[2].gl_Position;
    if (c0.w <= 0.0 || c1.w <= 0.0)
        return;
    vec2 s0 = toScreen(c0);
    vec2 s1 = toScreen(c1);
    vec2 delta = s1 - s0;
    float len = length(delta);
    if (len < 1e-4)
        return;

    vec2 sPrev, sNext;
    vec2 ends = vec2(endState(gl_in[0].gl_Position, c0, vFarPrevClip[1], s0, sPrev),
                     endState(gl_in[3].gl_Position, c1, vFarNextClip[1], s1, sNext));

    vec2 dir = delta / len;
    vec2 perp = vec2(-dir.y, dir.x);
    float pad = max(uLineWidthPx * 0.5, 0.5) + 1.0;
    vec2 corners[4] = vec2[4](s0 - dir * pad + perp * pad,
                              s0 - dir * pad - perp * pad,
                              s1 + dir * pad + perp * pad,
                              s1 + dir * pad - perp * pad);
    for (int i = 0; i < 4; ++i) {
        vPrev = sPrev;
        vP0 = s0;
        vP1 = s1;
        vNext = sNext;
        vEnds = ends;
        gl_Position = fromScreen(corners[i], i < 2 ? c0 : c1);
        EmitVertex();
    }
    EndPrimitive();
}
)";

const char *kWideLineFrag = R"(
#version 330 core
uniform vec4 uColor;
uniform float uLineWidthPx;
flat in vec2 vPrev;
flat in vec2 vP0;
flat in vec2 vP1;
flat in vec2 vNext;
flat in vec2 vEnds;
out vec4 fragColor;

float distToSeg(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / max(dot(ba, ba), 1e-8), 0.0, 1.0);
    return length(pa - ba * h);
}

void main() {
    // gl_FragCoord (not an interpolated varying) so neighbouring segments
    // evaluate bit-identical distances for the same pixel.
    vec2 p = gl_FragCoord.xy;
    float d = distToSeg(p, vP0, vP1);
    if (vEnds.x > 0.5 && vEnds.x < 1.5 && distToSeg(p, vPrev, vP0) <= d)
        discard;
    if (vEnds.y > 0.5 && vEnds.y < 1.5 && distToSeg(p, vP1, vNext) < d)
        discard;

    vec2 seg = vP1 - vP0;
    float len = length(seg);
    vec2 dir = seg / len;
    float t = dot(p - vP0, dir);
    float perpDist = abs(dot(p - vP0, vec2(-dir.y, dir.x)));
    float capCoverage = 1.0;
    // A short segment's nearest-point capsule must remain its actual coverage:
    // butt-clipping it after the distance competition can leave a pixel that
    // both it and its neighbour discard. Keep round coverage when the segment
    // is shorter than its width; longer free ends retain the butt-cap behavior.
    bool shortSegment = len < max(uLineWidthPx, 1.0);
    if (vEnds.x < 0.5 && !shortSegment) {
        if (t < 0.0)
            d = perpDist;
        capCoverage *= clamp(t + 0.5, 0.0, 1.0);
    }
    if (vEnds.y < 0.5 && !shortSegment) {
        if (t > len)
            d = perpDist;
        capCoverage *= clamp(len - t + 0.5, 0.0, 1.0);
    }

    // Box-filtered coverage: visible width matches uLineWidthPx.
    float halfW = max(uLineWidthPx * 0.5, 0.5);
    float alpha = clamp(halfW + 0.5 - d, 0.0, 1.0) * capCoverage;
    if (alpha <= 0.0)
        discard;
    fragColor = vec4(uColor.rgb, uColor.a * alpha);
}
)";

// Optional ARB rectangle path for Syphon (pixel texcoords). May fail to
// compile on strict Core drivers; Syphon draws are then skipped until a
// TEXTURE_2D upload path lands (C-tier / platform work).
const char *kTextureRectVert = R"(
#version 330 core
#extension GL_ARB_texture_rectangle : enable
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
uniform mat4 uMVP;
out vec2 vTexCoord;
void main() {
    vTexCoord = aTexCoord;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

const char *kTextureRectFrag = R"(
#version 330 core
#extension GL_ARB_texture_rectangle : enable
in vec2 vTexCoord;
uniform sampler2DRect uTexture;
uniform vec4 uColor;
out vec4 fragColor;
void main() {
    fragColor = texture(uTexture, vTexCoord) * uColor;
}
)";

bool linkProgram(QOpenGLShaderProgram &prog, const char *vert, const char *frag, const char *label)
{
    if (!prog.addShaderFromSourceCode(QOpenGLShader::Vertex, vert)) {
        qWarning("GlPainter: %s vertex shader failed: %s", label, qPrintable(prog.log()));
        return false;
    }
    if (!prog.addShaderFromSourceCode(QOpenGLShader::Fragment, frag)) {
        qWarning("GlPainter: %s fragment shader failed: %s", label, qPrintable(prog.log()));
        return false;
    }
    if (!prog.link()) {
        qWarning("GlPainter: %s link failed: %s", label, qPrintable(prog.log()));
        return false;
    }
    return true;
}

} // namespace

GlPainter::GlPainter()
    : m_ready(false)
    , m_inFrame(false)
    , m_textureRectReady(false)
    , m_wideLineReady(false)
    , m_immActive(false)
    , m_immMode(GL_TRIANGLES)
    , m_lineWidth(1.f)
    , m_textureTarget(TextureNone)
    , m_textureId(0)
    , m_dynamicVbo(QOpenGLBuffer::VertexBuffer)
    , m_msaaFbo(nullptr)
    , m_msaaRequestedSamples(0)
    , m_msaaResolveFbo(0)
    , m_msaaResolveReadFbo(0)
    , m_msaaBound(false)
{
    m_msaaResolveViewport[0] = m_msaaResolveViewport[1] = 0;
    m_msaaResolveViewport[2] = m_msaaResolveViewport[3] = 1;
    m_color[0] = m_color[1] = m_color[2] = m_color[3] = 1.f;
    m_immCurrent = {0.f, 0.f, 0.f, 0.f, 0.f};
    m_projection.setToIdentity();
    m_modelViewStack.clear();
    m_modelViewStack.append(QMatrix4x4());
}

GlPainter::~GlPainter()
{
    shutdown();
}

GlPainter *GlPainter::instance()
{
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        if (!s_instance)
            s_instance = new GlPainter();
        return s_instance;
    }

    GlPainter *painter = painterMap().value(ctx, nullptr);
    if (!painter) {
        painter = new GlPainter();
        painterMap().insert(ctx, painter);
        if (!s_instance)
            s_instance = painter;
        QObject::connect(ctx, &QOpenGLContext::aboutToBeDestroyed, ctx, [ctx]() {
            GlPainter *gone = painterMap().take(ctx);
            if (gone) {
                if (s_current == gone)
                    s_current = nullptr;
                if (s_instance == gone)
                    s_instance = painterMap().isEmpty() ? nullptr : painterMap().begin().value();
                delete gone;
            }
        });
    }
    return painter;
}

GlPainter *GlPainter::current()
{
    if (s_current)
        return s_current;
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (ctx)
        return painterMap().value(ctx, s_instance);
    return s_instance;
}

bool GlPainter::initialize()
{
    if (m_ready)
        return true;

    initializeOpenGLFunctions();

    if (!compilePrograms())
        return false;

    if (!m_dynamicVao.create()) {
        qWarning("GlPainter: failed to create VAO");
        return false;
    }
    m_dynamicVao.bind();

    if (!m_dynamicVbo.create()) {
        qWarning("GlPainter: failed to create dynamic VBO");
        m_dynamicVao.release();
        return false;
    }
    m_dynamicVbo.bind();
    m_dynamicVbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    // Allocate a modest initial capacity; end() will orphan/resize as needed.
    m_dynamicVbo.allocate(256 * int(sizeof(ImmVertex)));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ImmVertex),
                          reinterpret_cast<const void *>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImmVertex),
                          reinterpret_cast<const void *>(3 * sizeof(float)));

    m_dynamicVbo.release();
    m_dynamicVao.release();

    m_ready = true;
    return true;
}

void GlPainter::shutdown()
{
    if (!m_ready)
        return;

    m_dynamicVao.destroy();
    m_dynamicVbo.destroy();
    delete m_msaaFbo;
    m_msaaFbo = nullptr;
    m_msaaRequestedSamples = 0;
    m_msaaBound = false;
    m_msaaResolveFbo = 0;
    m_msaaResolveReadFbo = 0;
    m_colorProgram.removeAllShaders();
    m_textureProgram.removeAllShaders();
    m_textureRectProgram.removeAllShaders();
    m_wideLineProgram.removeAllShaders();
    m_textureRectReady = false;
    m_wideLineReady = false;
    m_ready = false;
    if (s_current == this)
        s_current = nullptr;
}

bool GlPainter::compilePrograms()
{
    if (!linkProgram(m_colorProgram, kColorVert, kColorFrag, "color"))
        return false;
    if (!linkProgram(m_textureProgram, kTextureVert, kTextureFrag, "texture"))
        return false;

    m_wideLineReady =
        m_wideLineProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, kWideLineVert) &&
        m_wideLineProgram.addShaderFromSourceCode(QOpenGLShader::Geometry, kWideLineGeom) &&
        m_wideLineProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, kWideLineFrag) &&
        m_wideLineProgram.link();
    if (!m_wideLineReady) {
        qWarning("GlPainter: wide-line program failed (%s); lines fall back to 1px",
                 qPrintable(m_wideLineProgram.log()));
        m_wideLineProgram.removeAllShaders();
    }

    // Optional rectangle sampler for Syphon. Failure is non-fatal under
    // Compatibility: paintBackground can still use fixed-function for that path.
    m_textureRectReady = linkProgram(m_textureRectProgram, kTextureRectVert, kTextureRectFrag, "textureRect");
    if (!m_textureRectReady) {
        qWarning("GlPainter: texture-rectangle program unavailable; "
                 "Syphon/rect textured draws need Compatibility fallback");
        m_textureRectProgram.removeAllShaders();
    }
    return true;
}

void GlPainter::beginFrame()
{
    if (!m_ready)
        return;
    s_current = this;
    m_inFrame = true;
    m_modelViewStack.clear();
    m_modelViewStack.append(QMatrix4x4());
    m_textureTarget = TextureNone;
    m_textureId = 0;
    m_immActive = false;
    m_immVerts.clear();
    glEnable(GL_BLEND);

    GLint vp[4] = {0, 0, 1, 1};
    glGetIntegerv(GL_VIEWPORT, vp);
    m_viewportOrigin = QVector2D(float(vp[0]), float(vp[1]));
    m_viewportSize = QVector2D(float(qMax(1, int(vp[2]))), float(qMax(1, int(vp[3]))));
    if (m_msaaBound)
        resolveMsaa();
    ensureMsaaTarget();
}

void GlPainter::endFrame()
{
    if (!m_ready)
        return;
    if (m_immActive)
        end();
    unbindTexture();
    resolveMsaa();
    m_inFrame = false;
    if (s_current == this)
        s_current = nullptr;
}

void GlPainter::ensureMsaaTarget()
{
    if (!m_ready)
        return;

    const int requestedSamples = Application::msaaSamples;
    if (requestedSamples <= 0)
        return;

    GLint existingSamples = 0;
    glGetIntegerv(GL_SAMPLES, &existingSamples);
    if (existingSamples >= requestedSamples)
        return;

    const int w = int(m_viewportSize.x());
    const int h = int(m_viewportSize.y());
    if (w < 8 || h < 8)
        return;

    GLint resolveDrawFbo = 0;
    GLint resolveReadFbo = 0;
    GLint resolveViewport[4] = {0, 0, w, h};
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &resolveDrawFbo);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &resolveReadFbo);
    glGetIntegerv(GL_VIEWPORT, resolveViewport);

    // Drivers may round the requested sample count up or down. Cache by the
    // request that created the target, not by its negotiated sample count.
    const bool needNew = !m_msaaFbo || m_msaaSize != QSize(w, h) ||
                         m_msaaRequestedSamples != requestedSamples;
    if (needNew) {
        delete m_msaaFbo;
        m_msaaFbo = nullptr;
        const int sampleTries[] = {8, 4, 2};
        for (int want : sampleTries) {
            if (want > requestedSamples)
                continue;
            QOpenGLFramebufferObjectFormat fmt;
            fmt.setSamples(want);
            fmt.setAttachment(QOpenGLFramebufferObject::NoAttachment);
            QOpenGLFramebufferObject *candidate = new QOpenGLFramebufferObject(w, h, fmt);
            if (candidate->isValid() && candidate->format().samples() >= 2) {
                m_msaaFbo = candidate;
                m_msaaSize = QSize(w, h);
                m_msaaRequestedSamples = requestedSamples;
                break;
            }
            delete candidate;
        }
    }

    // QOpenGLFramebufferObject allocation may alter framebuffer bindings.
    // Restore the widget/custom target before binding the MSAA source.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, GLuint(resolveReadFbo));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(resolveDrawFbo));
    glViewport(resolveViewport[0], resolveViewport[1], resolveViewport[2], resolveViewport[3]);

    if (!m_msaaFbo) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            qWarning("GlPainter: could not allocate an MSAA framebuffer (%dx%d)", w, h);
        }
        return;
    }

    m_msaaResolveFbo = GLuint(resolveDrawFbo);
    m_msaaResolveReadFbo = GLuint(resolveReadFbo);
    for (int i = 0; i < 4; ++i)
        m_msaaResolveViewport[i] = resolveViewport[i];
    m_msaaFbo->bind();
    glViewport(0, 0, w, h);
    m_viewportOrigin = QVector2D(0.f, 0.f);
    m_msaaBound = true;
}

void GlPainter::resolveMsaa()
{
    if (!m_msaaBound || !m_msaaFbo)
        return;

    const int w = m_msaaSize.width();
    const int h = m_msaaSize.height();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaaFbo->handle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_msaaResolveFbo);
    QOpenGLContext::currentContext()->extraFunctions()->glBlitFramebuffer(
        0, 0, w, h,
        m_msaaResolveViewport[0], m_msaaResolveViewport[1],
        m_msaaResolveViewport[0] + w, m_msaaResolveViewport[1] + h,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_msaaResolveReadFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_msaaResolveFbo);
    glViewport(m_msaaResolveViewport[0], m_msaaResolveViewport[1],
               m_msaaResolveViewport[2], m_msaaResolveViewport[3]);
    m_viewportOrigin = QVector2D(float(m_msaaResolveViewport[0]), float(m_msaaResolveViewport[1]));
    m_viewportSize = QVector2D(float(qMax(1, m_msaaResolveViewport[2])),
                               float(qMax(1, m_msaaResolveViewport[3])));
    m_msaaBound = false;
}

void GlPainter::ensureModelView()
{
    if (m_modelViewStack.isEmpty())
        m_modelViewStack.append(QMatrix4x4());
}

void GlPainter::setProjection(const QMatrix4x4 &m)
{
    m_projection = m;
}

void GlPainter::setOrtho(float left, float right, float bottom, float top,
                         float nearPlane, float farPlane)
{
    m_projection.setToIdentity();
    m_projection.ortho(left, right, bottom, top, nearPlane, farPlane);
}

void GlPainter::setFrustum(float left, float right, float bottom, float top,
                           float nearPlane, float farPlane)
{
    m_projection.setToIdentity();
    m_projection.frustum(left, right, bottom, top, nearPlane, farPlane);
}

void GlPainter::viewport(int x, int y, int w, int h)
{
    if (m_ready)
        glViewport(x, y, w, h);
}

void GlPainter::pushMatrix()
{
    ensureModelView();
    m_modelViewStack.append(m_modelViewStack.last());
}

void GlPainter::popMatrix()
{
    if (m_modelViewStack.size() > 1)
        m_modelViewStack.removeLast();
}

void GlPainter::loadIdentity()
{
    ensureModelView();
    m_modelViewStack.last().setToIdentity();
}

void GlPainter::translate(float x, float y, float z)
{
    ensureModelView();
    m_modelViewStack.last().translate(x, y, z);
}

void GlPainter::rotate(float angleDegrees, float x, float y, float z)
{
    ensureModelView();
    m_modelViewStack.last().rotate(angleDegrees, x, y, z);
}

void GlPainter::scale(float x, float y, float z)
{
    ensureModelView();
    m_modelViewStack.last().scale(x, y, z);
}

QMatrix4x4 GlPainter::modelView() const
{
    if (m_modelViewStack.isEmpty())
        return QMatrix4x4();
    return m_modelViewStack.last();
}

QMatrix4x4 GlPainter::mvp() const
{
    return m_projection * modelView();
}

void GlPainter::setColor(float r, float g, float b, float a)
{
    m_color[0] = r;
    m_color[1] = g;
    m_color[2] = b;
    m_color[3] = a;
}

void GlPainter::setColor(const QColor &c)
{
    setColor(float(c.redF()), float(c.greenF()), float(c.blueF()), float(c.alphaF()));
}

void GlPainter::setLineWidth(float w)
{
    m_lineWidth = w;
    // Core profile rejects widths > 1 (GL_INVALID_VALUE); wide lines are
    // expanded in the geometry shader instead.
    if (m_ready)
        glLineWidth(1.f);
}

void GlPainter::bindTexture(TextureTarget target, GLuint id)
{
    if (id == 0) {
        unbindTexture();
        return;
    }
    m_textureTarget = target;
    m_textureId = id;
    if (!m_ready)
        return;

    if (target == Texture2D) {
        glBindTexture(GL_TEXTURE_2D, id);
    } else if (target == TextureRectangle) {
        glBindTexture(GL_TEXTURE_RECTANGLE, id);
    }
}

void GlPainter::unbindTexture()
{
    if (m_ready) {
        if (m_textureTarget == Texture2D)
            glBindTexture(GL_TEXTURE_2D, 0);
        else if (m_textureTarget == TextureRectangle)
            glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    }
    m_textureTarget = TextureNone;
    m_textureId = 0;
}

void GlPainter::begin(GLenum mode)
{
    if (m_immActive)
        end();
    m_immActive = true;
    m_immMode = mode;
    m_immVerts.clear();
    m_immCurrent = {0.f, 0.f, 0.f, 0.f, 0.f};
}

void GlPainter::texCoord(float u, float v)
{
    m_immCurrent.u = u;
    m_immCurrent.v = v;
}

void GlPainter::vertex(float x, float y, float z)
{
    if (!m_immActive)
        return;
    ImmVertex v = m_immCurrent;
    v.x = x;
    v.y = y;
    v.z = z;
    m_immVerts.append(v);
}

void GlPainter::end()
{
    if (!m_immActive)
        return;
    m_immActive = false;

    if (!m_ready || m_immVerts.isEmpty()) {
        m_immVerts.clear();
        return;
    }

    QVector<float> interleaved;
    GLenum drawMode = m_immMode;
    const int n = m_immVerts.size();

    auto appendVert = [&](const ImmVertex &v) {
        interleaved.append(v.x);
        interleaved.append(v.y);
        interleaved.append(v.z);
        interleaved.append(v.u);
        interleaved.append(v.v);
    };

    if (m_immMode == GL_QUADS) {
        // 4 verts → 2 triangles (6 verts)
        drawMode = GL_TRIANGLES;
        const int quadCount = n / 4;
        interleaved.reserve(quadCount * 6 * 5);
        for (int q = 0; q < quadCount; ++q) {
            const ImmVertex &a = m_immVerts[q * 4 + 0];
            const ImmVertex &b = m_immVerts[q * 4 + 1];
            const ImmVertex &c = m_immVerts[q * 4 + 2];
            const ImmVertex &d = m_immVerts[q * 4 + 3];
            appendVert(a);
            appendVert(b);
            appendVert(c);
            appendVert(a);
            appendVert(c);
            appendVert(d);
        }
    } else if (m_immMode == GL_POLYGON) {
        drawMode = GL_TRIANGLE_FAN;
        interleaved.reserve(n * 5);
        for (const ImmVertex &v : m_immVerts)
            appendVert(v);
    } else {
        interleaved.reserve(n * 5);
        for (const ImmVertex &v : m_immVerts)
            appendVert(v);
    }

    const int vertexCount = interleaved.size() / 5;
    if (vertexCount > 0)
        flushImmediate(drawMode, interleaved, vertexCount);

    m_immVerts.clear();
}

QOpenGLShaderProgram *GlPainter::bindProgramFor(GLenum drawMode, bool textured)
{
    QOpenGLShaderProgram *prog = &m_colorProgram;

    // The wide-line GS consumes adjacency input; without it, GL draws only
    // the middle edge of each adjacency primitive (plain 1px lines).
    if (!textured && drawMode == GL_LINES_ADJACENCY && m_wideLineReady) {
        prog = &m_wideLineProgram;
    } else if (textured) {
        if (m_textureTarget == TextureRectangle) {
            if (!m_textureRectReady) {
                // TODO(Core): convert Syphon to TEXTURE_2D or ship a rect extension path.
                qWarning("GlPainter: TextureRectangle draw skipped (no rect program)");
                return nullptr;
            }
            prog = &m_textureRectProgram;
        } else {
            prog = &m_textureProgram;
        }
    }

    prog->bind();
    applyUniforms(prog, textured);
    return prog;
}

void GlPainter::flushImmediate(GLenum drawMode, const QVector<float> &verts, int vertexCount)
{
    const bool textured = (m_textureTarget != TextureNone) && (m_textureId != 0);
    if (!textured && isLineMode(drawMode)) {
        if (m_lineWidth <= 1.05f) {
            flushVertices(drawMode, verts, vertexCount, false);
            return;
        }
        const QMatrix4x4 transform = mvp();
        const QVector<float> adjacency =
            GlGeom::toLineAdjacency(drawMode, verts.constData(), 5, vertexCount,
                                    false, &transform, m_viewportSize);
        if (adjacency.isEmpty())
            return;
        const int adjacencyStride = 5 + 6;
        flushVertices(GL_LINES_ADJACENCY, adjacency, adjacency.size() / adjacencyStride, false);
        return;
    }
    flushVertices(drawMode, verts, vertexCount, textured);
}

void GlPainter::flushVertices(GLenum drawMode, const QVector<float> &verts, int vertexCount, bool textured)
{
    QOpenGLShaderProgram *prog = bindProgramFor(drawMode, textured);
    if (!prog)
        return;

    if (textured) {
        prog->setUniformValue("uTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        if (m_textureTarget == Texture2D)
            glBindTexture(GL_TEXTURE_2D, m_textureId);
        else
            glBindTexture(GL_TEXTURE_RECTANGLE, m_textureId);
    }

    m_dynamicVao.bind();
    m_dynamicVbo.bind();
    const int recordStride = vertexCount > 0 ? verts.size() / vertexCount : 0;
    const int bytes = verts.size() * int(sizeof(float));
    // Orphan previous storage to avoid GPU sync stalls on dynamic uploads.
    m_dynamicVbo.allocate(bytes);
    m_dynamicVbo.write(0, verts.constData(), bytes);

    const int strideBytes = recordStride * int(sizeof(float));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, strideBytes, nullptr);
    if (recordStride >= 5) {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, strideBytes,
                              reinterpret_cast<const void *>(3 * sizeof(float)));
    } else {
        glDisableVertexAttribArray(1);
    }
    if (drawMode == GL_LINES_ADJACENCY && recordStride >= 11) {
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, strideBytes,
                              reinterpret_cast<const void *>(5 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, strideBytes,
                              reinterpret_cast<const void *>(8 * sizeof(float)));
    } else {
        glDisableVertexAttribArray(2);
        glDisableVertexAttribArray(3);
    }

    glDrawArrays(drawMode, 0, vertexCount);

    m_dynamicVbo.release();
    m_dynamicVao.release();
    prog->release();
}

void GlPainter::clearColor(const QColor &c)
{
    if (m_ready)
        glClearColor(float(c.redF()), float(c.greenF()), float(c.blueF()), float(c.alphaF()));
}

void GlPainter::clear(GLbitfield mask)
{
    if (m_ready)
        glClear(mask);
}

void GlPainter::applyUniforms(QOpenGLShaderProgram *prog, bool textured) const
{
    Q_UNUSED(textured);
    if (!prog)
        return;
    prog->setUniformValue("uMVP", mvp());
    prog->setUniformValue("uColor", m_color[0], m_color[1], m_color[2], m_color[3]);
    // Wide-line program only; silently ignored (location -1) by the others.
    prog->setUniformValue("uViewport", m_viewportSize);
    prog->setUniformValue("uViewportOrigin", m_viewportOrigin);
    prog->setUniformValue("uLineWidthPx", m_lineWidth);
}

void GlPainter::bindVertexLayout()
{
    if (!m_ready)
        return;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ImmVertex),
                          reinterpret_cast<const void *>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImmVertex),
                          reinterpret_cast<const void *>(3 * sizeof(float)));
}
