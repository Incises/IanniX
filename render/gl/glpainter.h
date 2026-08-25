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

#ifndef GLPAINTER_H
#define GLPAINTER_H

#include <QColor>
#include <QMatrix4x4>
#include <QVector2D>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QVector>

/**
 * Paint-time OpenGL helper (B-tier foundation).
 *
 * Owns color/textured shader programs, a CPU matrix stack, and an
 * immediate-mode batcher that replaces glBegin/glEnd for migrated call sites.
 *
 * Targets OpenGL 3.3 Core. Matrix state lives only on the CPU stack.
 *
 * Extension point for C-tier: colorProgram()/textureProgram()/applyUniforms()
 * let GlMesh and future batchers share the same shaders without rebinding.
 */
class GlPainter : protected QOpenGLFunctions {
public:
    static GlPainter *instance();
    static GlPainter *current(); // alias during an active frame

    bool initialize();
    void shutdown();
    bool isReady() const { return m_ready; }

    void beginFrame();
    void endFrame();

    void setProjection(const QMatrix4x4 &m);
    void setOrtho(float left, float right, float bottom, float top,
                  float nearPlane, float farPlane);
    void setFrustum(float left, float right, float bottom, float top,
                    float nearPlane, float farPlane);
    void viewport(int x, int y, int w, int h);

    void pushMatrix();
    void popMatrix();
    void loadIdentity();
    void translate(float x, float y, float z);
    void rotate(float angleDegrees, float x, float y, float z);
    void scale(float x, float y, float z);
    QMatrix4x4 modelView() const;
    QMatrix4x4 projection() const { return m_projection; }
    QMatrix4x4 mvp() const;

    void setColor(float r, float g, float b, float a);
    void setColor(const QColor &c);
    void setLineWidth(float w);

    enum TextureTarget { TextureNone, Texture2D, TextureRectangle };
    void bindTexture(TextureTarget target, GLuint id); // 0 unbinds
    void unbindTexture();

    // Immediate mode — converts GL_QUADS→triangles, GL_POLYGON→triangle fan
    void begin(GLenum mode);
    void texCoord(float u, float v);
    void vertex(float x, float y, float z = 0.f);
    void end();

    void clearColor(const QColor &c);
    void clear(GLbitfield mask = GL_COLOR_BUFFER_BIT);

    // For GlMesh and future C-tier batching:
    QOpenGLShaderProgram *colorProgram() { return &m_colorProgram; }
    QOpenGLShaderProgram *textureProgram() { return &m_textureProgram; }
    QOpenGLShaderProgram *textureRectProgram() { return m_textureRectReady ? &m_textureRectProgram : nullptr; }
    void applyUniforms(QOpenGLShaderProgram *prog, bool textured) const;

    // Binds the right program for a draw (wide-line GS for line modes with
    // width > 1, texture programs when textured) and applies uniforms.
    // Returns nullptr if the draw should be skipped.
    QOpenGLShaderProgram *bindProgramFor(GLenum drawMode, bool textured);

    // Bind dynamic VAO attrib layout (pos @0, uv @1) for callers that share the VBO format.
    void bindVertexLayout();

private:
    GlPainter();
    ~GlPainter();
    GlPainter(const GlPainter &) = delete;
    GlPainter &operator=(const GlPainter &) = delete;

    bool compilePrograms();
    void ensureModelView();
    void flushImmediate(GLenum drawMode, const QVector<float> &verts, int vertexCount);

    struct ImmVertex {
        float x, y, z, u, v;
    };

    static GlPainter *s_instance;
    static GlPainter *s_current;

    bool m_ready;
    bool m_inFrame;
    bool m_textureRectReady;
    bool m_wideLineReady;
    bool m_immActive;
    GLenum m_immMode;
    ImmVertex m_immCurrent;
    QVector<ImmVertex> m_immVerts;

    QMatrix4x4 m_projection;
    QVector<QMatrix4x4> m_modelViewStack;

    float m_color[4];
    float m_lineWidth;
    TextureTarget m_textureTarget;
    GLuint m_textureId;

    QOpenGLShaderProgram m_colorProgram;
    QOpenGLShaderProgram m_textureProgram;
    QOpenGLShaderProgram m_textureRectProgram;
    QOpenGLShaderProgram m_wideLineProgram;
    QVector2D m_viewportSize;
    QOpenGLBuffer m_dynamicVbo;
    QOpenGLVertexArrayObject m_dynamicVao;
};

#endif // GLPAINTER_H
