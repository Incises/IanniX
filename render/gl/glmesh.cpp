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
#include "glgeom.h"

#include <QOpenGLContext>

GlMesh::GlMesh()
    : m_initialized(false)
    , m_dirty(true)
    , m_hasUv(false)
    , m_hasThinLine(false)
    , m_thinInitialized(false)
    , m_mode(GL_TRIANGLES)
    , m_vertexCount(0)
    , m_thinMode(GL_LINES)
    , m_thinVertexCount(0)
    , m_vbo(QOpenGLBuffer::VertexBuffer)
    , m_thinVbo(QOpenGLBuffer::VertexBuffer)
{
}

GlMesh::~GlMesh()
{
    destroy();
}

void GlMesh::destroy()
{
    m_linePositions.clear();
    m_projectedLineMask.clear();
    m_projectedAdjacencyValid = false;
    if (!m_initialized && !m_thinInitialized)
        return;
    if (QOpenGLContext::currentContext()) {
        if (m_initialized) {
            m_vao.destroy();
            m_vbo.destroy();
        }
        if (m_thinInitialized) {
            m_thinVao.destroy();
            m_thinVbo.destroy();
        }
    }
    m_initialized = false;
    m_thinInitialized = false;
    m_hasThinLine = false;
    m_vertexCount = 0;
    m_thinVertexCount = 0;
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

void GlMesh::upload(GLenum mode, const QVector<float> &interleavedPosUv, int vertexCount, bool hasUv,
                    bool joinLinePairs)
{
    m_linePositions.clear();
    m_projectedLineMask.clear();
    m_projectedAdjacencyValid = false;
    const int stride = hasUv ? 5 : 3;
    if (vertexCount <= 0 || interleavedPosUv.size() < vertexCount * stride) {
        m_vertexCount = 0;
        m_hasThinLine = false;
        m_dirty = false;
        return;
    }
    if (!hasUv && GlPainter::isLineMode(mode)) {
        m_linePositions = interleavedPosUv.mid(0, vertexCount * stride);
        m_joinLinePairs = joinLinePairs;
        uploadThinLine(mode, interleavedPosUv.constData(), vertexCount);
        // Keep a thin-line VBO for standard GL rasterization, and a separate
        // adjacency VBO for wide capsules selected at draw time.
        const QVector<float> adjacency =
            GlGeom::toLineAdjacency(mode, interleavedPosUv.constData(), stride, vertexCount, joinLinePairs);
        if (adjacency.isEmpty()) {
            m_vertexCount = 0;
            m_hasThinLine = false;
            m_dirty = false;
            return;
        }
        const int adjacencyStride = stride + 6;
        uploadInternal(GL_LINES_ADJACENCY, adjacency.constData(), adjacency.size(),
                       adjacency.size() / adjacencyStride, false);
        return;
    }
    m_hasThinLine = false;
    uploadInternal(mode, interleavedPosUv.constData(), vertexCount * stride, vertexCount, hasUv);
}

void GlMesh::uploadThinLine(GLenum mode, const float *data, int vertexCount)
{
    if (!m_thinInitialized) {
        m_thinVao.create();
        m_thinVbo.create();
        m_thinVbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
        m_thinInitialized = true;
    }

    m_thinMode = mode;
    m_thinVertexCount = vertexCount;
    m_hasThinLine = true;
    m_thinVao.bind();
    m_thinVbo.bind();
    m_thinVbo.allocate(data, vertexCount * 3 * int(sizeof(float)));
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * int(sizeof(float)), nullptr);
    f->glDisableVertexAttribArray(1);
    f->glDisableVertexAttribArray(2);
    f->glDisableVertexAttribArray(3);
    m_thinVbo.release();
    m_thinVao.release();
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

    const bool adjacency = mode == GL_LINES_ADJACENCY;
    const int recordStride = (hasUv ? 5 : 3) + (adjacency ? 6 : 0);
    const int strideBytes = recordStride * int(sizeof(float));
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
    if (adjacency) {
        const int extrasOffset = (hasUv ? 5 : 3) * int(sizeof(float));
        f->glEnableVertexAttribArray(2);
        f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, strideBytes,
                                 reinterpret_cast<const void *>(extrasOffset));
        f->glEnableVertexAttribArray(3);
        f->glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, strideBytes,
                                 reinterpret_cast<const void *>(extrasOffset + 3 * int(sizeof(float))));
    } else {
        f->glDisableVertexAttribArray(2);
        f->glDisableVertexAttribArray(3);
    }

    m_vbo.release();
    m_vao.release();
    m_dirty = false;
}

void GlMesh::draw(GlPainter *painter)
{
    if (!painter || !painter->isReady() || !m_initialized)
        return;

    const bool useThinLine = m_hasThinLine && painter->lineWidth() <= 1.05f;
    if (!useThinLine && m_hasThinLine) {
        const QMatrix4x4 transform = painter->mvp();
        const QVector2D viewport = painter->viewportSize();
        if (!m_projectedAdjacencyValid || transform != m_adjacencyMvp || viewport != m_adjacencyViewport) {
            const QVector<quint8> collapsed = GlGeom::projectedLineMask(
                m_thinMode, m_linePositions.constData(), 3, m_thinVertexCount,
                transform, viewport);
            // MVP changes alone do not change the object-space adjacency.
            // Rebuild only when a segment enters or leaves a collapsed run.
            if (!m_projectedAdjacencyValid || collapsed != m_projectedLineMask) {
                const QVector<float> adjacency = GlGeom::toLineAdjacency(
                    m_thinMode, m_linePositions.constData(), 3, m_thinVertexCount,
                    m_joinLinePairs, nullptr, QVector2D(), &collapsed);
                uploadInternal(GL_LINES_ADJACENCY, adjacency.constData(), adjacency.size(),
                               adjacency.size() / 9, false);
                m_projectedLineMask = collapsed;
            }
            m_adjacencyMvp = transform;
            m_adjacencyViewport = viewport;
            m_projectedAdjacencyValid = true;
        }
    }
    if ((useThinLine ? m_thinVertexCount : m_vertexCount) <= 0)
        return;
    const GLenum drawMode = useThinLine ? m_thinMode : m_mode;
    QOpenGLShaderProgram *prog = painter->bindProgramFor(drawMode, m_hasUv);
    if (!prog)
        return;
    if (m_hasUv)
        prog->setUniformValue("uTexture", 0);

    if (useThinLine)
        m_thinVao.bind();
    else
        m_vao.bind();
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glDrawArrays(drawMode, 0, useThinLine ? m_thinVertexCount : m_vertexCount);
    if (useThinLine)
        m_thinVao.release();
    else
        m_vao.release();
    prog->release();
}
