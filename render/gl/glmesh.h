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

#ifndef GLMESH_H
#define GLMESH_H

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector>
#include <QtGui/qopengl.h>

class GlPainter;

/**
 * Cached GPU geometry replacing OpenGL display lists.
 *
 * Upload interleaved position (or pos+uv) data once; draw() binds the VBO/VAO
 * and applies GlPainter uniforms. C-tier batching can later share a single
 * draw pass across many GlMesh instances.
 */
class GlMesh {
public:
    GlMesh();
    ~GlMesh();

    void destroy(); // safe if never initialized
    void setDirty(bool dirty = true) { m_dirty = dirty; }
    bool isDirty() const { return m_dirty; }
    bool isEmpty() const { return m_vertexCount == 0; }

    // Upload position-only mesh (stride 3 floats)
    void upload(GLenum mode, const QVector<float> &interleavedPos, int vertexCount);
    // Upload pos or pos+uv mesh (stride 3 or 5 floats)
    void upload(GLenum mode, const QVector<float> &interleavedPosUv, int vertexCount, bool hasUv);

    void draw(GlPainter *painter) const;

private:
    void ensureCreated();
    void uploadInternal(GLenum mode, const float *data, int floatCount, int vertexCount, bool hasUv);

    mutable bool m_initialized;
    bool m_dirty;
    bool m_hasUv;
    GLenum m_mode;
    int m_vertexCount;
    mutable QOpenGLBuffer m_vbo;
    mutable QOpenGLVertexArrayObject m_vao;
};

#endif // GLMESH_H
