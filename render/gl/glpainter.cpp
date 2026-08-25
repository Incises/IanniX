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

#include <QDebug>
#include <QHash>
#include <QOpenGLContext>
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
// Expand line primitives into screen-space quads in a geometry shader.
const char *kWideLineGeom = R"(
#version 330 core
layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;
uniform vec2 uViewport;
uniform float uLineWidthPx;
void main() {
    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;
    if (p0.w <= 0.0 || p1.w <= 0.0)
        return;
    vec2 ndc0 = p0.xy / p0.w;
    vec2 ndc1 = p1.xy / p1.w;
    vec2 screenDir = (ndc1 - ndc0) * uViewport;
    float len = length(screenDir);
    if (len < 1e-6)
        return;
    vec2 dir = screenDir / len;
    // Half-width offset in NDC: (w/2 px) * (2/viewport px) = w/viewport
    vec2 offsetNdc = vec2(-dir.y, dir.x) * uLineWidthPx / uViewport;
    gl_Position = vec4((ndc0 + offsetNdc) * p0.w, p0.z, p0.w); EmitVertex();
    gl_Position = vec4((ndc0 - offsetNdc) * p0.w, p0.z, p0.w); EmitVertex();
    gl_Position = vec4((ndc1 + offsetNdc) * p1.w, p1.z, p1.w); EmitVertex();
    gl_Position = vec4((ndc1 - offsetNdc) * p1.w, p1.z, p1.w); EmitVertex();
    EndPrimitive();
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
{
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
        m_wideLineProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, kColorVert) &&
        m_wideLineProgram.addShaderFromSourceCode(QOpenGLShader::Geometry, kWideLineGeom) &&
        m_wideLineProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, kColorFrag) &&
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

    // Device-pixel viewport for screen-space wide-line expansion.
    GLint vp[4] = {0, 0, 1, 1};
    glGetIntegerv(GL_VIEWPORT, vp);
    m_viewportSize = QVector2D(float(qMax(1, int(vp[2]))), float(qMax(1, int(vp[3]))));
}

void GlPainter::endFrame()
{
    if (!m_ready)
        return;
    if (m_immActive)
        end();
    unbindTexture();
    m_inFrame = false;
    if (s_current == this)
        s_current = nullptr;
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
        glLineWidth(qMin(w, 1.f));
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

    const bool lineMode = (drawMode == GL_LINES) || (drawMode == GL_LINE_STRIP)
                          || (drawMode == GL_LINE_LOOP);
    if (!textured && lineMode && m_lineWidth > 1.05f && m_wideLineReady) {
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
    const int bytes = vertexCount * int(sizeof(ImmVertex));
    // Orphan previous storage to avoid GPU sync stalls on dynamic uploads.
    m_dynamicVbo.allocate(bytes);
    m_dynamicVbo.write(0, verts.constData(), bytes);

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
