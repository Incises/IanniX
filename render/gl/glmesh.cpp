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

#include "glmesh.h"
#include "glpainter.h"

#include <QOpenGLContext>

GlMesh::GlMesh()
    : m_initialized(false)
    , m_dirty(true)
    , m_hasUv(false)
    , m_mode(GL_TRIANGLES)
    , m_vertexCount(0)
    , m_vbo(QOpenGLBuffer::VertexBuffer)
{
}

GlMesh::~GlMesh()
{
    destroy();
}

void GlMesh::destroy()
{
    if (!m_initialized)
        return;
    if (QOpenGLContext::currentContext()) {
        m_vao.destroy();
        m_vbo.destroy();
    }
    m_initialized = false;
    m_vertexCount = 0;
    m_dirty = true;
}

void GlMesh::ensureCreated()
{
    if (m_initialized)
        return;
    m_vao.create();
    m_vbo.create();
    m_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    m_initialized = true;
}

void GlMesh::upload(GLenum mode, const QVector<float> &interleavedPos, int vertexCount)
{
    upload(mode, interleavedPos, vertexCount, false);
}

void GlMesh::upload(GLenum mode, const QVector<float> &interleavedPosUv, int vertexCount, bool hasUv)
{
    const int stride = hasUv ? 5 : 3;
    if (vertexCount <= 0 || interleavedPosUv.size() < vertexCount * stride) {
        m_vertexCount = 0;
        m_dirty = false;
        return;
    }
    uploadInternal(mode, interleavedPosUv.constData(), vertexCount * stride, vertexCount, hasUv);
}

void GlMesh::uploadInternal(GLenum mode, const float *data, int floatCount, int vertexCount, bool hasUv)
{
    ensureCreated();
    m_mode = mode;
    m_hasUv = hasUv;
    m_vertexCount = vertexCount;

    m_vao.bind();
    m_vbo.bind();
    m_vbo.allocate(data, floatCount * int(sizeof(float)));

    const int strideBytes = (hasUv ? 5 : 3) * int(sizeof(float));
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, strideBytes, reinterpret_cast<const void *>(0));
    if (hasUv) {
        f->glEnableVertexAttribArray(1);
        f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, strideBytes,
                                 reinterpret_cast<const void *>(3 * sizeof(float)));
    } else {
        f->glDisableVertexAttribArray(1);
    }

    m_vbo.release();
    m_vao.release();
    m_dirty = false;
}

void GlMesh::draw(GlPainter *painter) const
{
    if (!painter || !painter->isReady() || m_vertexCount <= 0 || !m_initialized)
        return;

    QOpenGLShaderProgram *prog = painter->bindProgramFor(m_mode, m_hasUv);
    if (!prog)
        return;
    if (m_hasUv)
        prog->setUniformValue("uTexture", 0);

    m_vao.bind();
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glDrawArrays(m_mode, 0, m_vertexCount);
    m_vao.release();
    prog->release();
}
