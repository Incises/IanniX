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

#ifndef GLGEOM_H
#define GLGEOM_H

#include <QVector>
#include <QMatrix4x4>
#include <QVector2D>
#include <QtMath>
#include <QtGui/qopengl.h>

/**
 * Small CPU geometry helpers for GlPainter / GlMesh (Bezier sample, line dash).
 */
namespace GlGeom {

inline void appendXYZ(QVector<float> &v, float x, float y, float z)
{
    v.append(x);
    v.append(y);
    v.append(z);
}

inline void appendLine(QVector<float> &v,
                       float x0, float y0, float z0,
                       float x1, float y1, float z1)
{
    appendXYZ(v, x0, y0, z0);
    appendXYZ(v, x1, y1, z1);
}

/** Sample a cubic Bezier into xyz points (not closed). */
inline void sampleCubicBezier(QVector<float> &out,
                              float p1x, float p1y, float p1z,
                              float c1x, float c1y, float c1z,
                              float c2x, float c2y, float c2z,
                              float p2x, float p2y, float p2z,
                              float step = 0.008f)
{
    // Integer stepping so t hits exactly 0 and 1: the first/last samples are
    // then bit-identical to p1/p2, letting GlPainter join consecutive curve
    // segments without a seam.
    const int steps = qMax(1, qRound(1.f / step));
    for (int i = 0; i <= steps; ++i) {
        const float t = float(i) / float(steps);
        const float u = 1.f - t;
        const float uu = u * u;
        const float tt = t * t;
        const float uuu = uu * u;
        const float ttt = tt * t;
        const float x = uuu * p1x + 3.f * uu * t * c1x + 3.f * u * tt * c2x + ttt * p2x;
        const float y = uuu * p1y + 3.f * uu * t * c1y + 3.f * u * tt * c2y + ttt * p2y;
        const float z = uuu * p1z + 3.f * uu * t * c1z + 3.f * u * tt * c2z + ttt * p2z;
        appendXYZ(out, x, y, z);
    }
}

/** Convert an xyz polyline strip into GL_LINES pairs. */
inline void stripToLines(QVector<float> &outLines, const QVector<float> &strip, bool closed = false)
{
    const int n = strip.size() / 3;
    if (n < 2)
        return;
    for (int i = 0; i < n - 1; ++i) {
        appendLine(outLines,
                   strip[i * 3], strip[i * 3 + 1], strip[i * 3 + 2],
                   strip[(i + 1) * 3], strip[(i + 1) * 3 + 1], strip[(i + 1) * 3 + 2]);
    }
    if (closed && n >= 3) {
        appendLine(outLines,
                   strip[(n - 1) * 3], strip[(n - 1) * 3 + 1], strip[(n - 1) * 3 + 2],
                   strip[0], strip[1], strip[2]);
    }
}

/**
 * Approximate GL_LINE_STIPPLE along a polyline (world-space length, not pixels).
 * pattern 0xFFFF (default) copies the strip as solid GL_LINES.
 */
inline void dashifyStrip(QVector<float> &outLines, const QVector<float> &strip,
                         int factor, quint16 pattern, bool closed = false)
{
    const int n = strip.size() / 3;
    if (n < 2)
        return;

    if (pattern == 0xFFFF) {
        stripToLines(outLines, strip, closed);
        return;
    }

    const int fac = qMax(1, factor);
    auto pointAt = [&](int i) -> const float * {
        const int idx = ((i % n) + n) % n;
        return &strip[idx * 3];
    };

    const int segCount = closed ? n : (n - 1);
    qreal dist = 0.;
    for (int s = 0; s < segCount; ++s) {
        const float *a = pointAt(s);
        const float *b = pointAt(s + 1);
        const qreal dx = b[0] - a[0], dy = b[1] - a[1], dz = b[2] - a[2];
        const qreal len = qSqrt(dx * dx + dy * dy + dz * dz);
        if (len < 1e-8)
            continue;

        // Subdivide segment into short pieces (~factor units) and keep "on" bits.
        const qreal piece = qMax(qreal(fac) * 0.05, len / 32.);
        qreal t0 = 0.;
        while (t0 < len - 1e-8) {
            const qreal t1 = qMin(len, t0 + piece);
            const int bit = int(dist / fac) % 16;
            if ((pattern >> bit) & 1) {
                // Reuse the exact vertices at segment ends so a dash spanning
                // a strip vertex stays joined (see toLineAdjacency).
                auto lerp = [&](qreal t, int k) {
                    if (t <= 0.)
                        return a[k];
                    if (t >= len)
                        return b[k];
                    const qreal d[3] = {dx, dy, dz};
                    return float(a[k] + d[k] * (t / len));
                };
                appendLine(outLines,
                           lerp(t0, 0), lerp(t0, 1), lerp(t0, 2),
                           lerp(t1, 0), lerp(t1, 1), lerp(t1, 2));
            }
            dist += (t1 - t0);
            t0 = t1;
        }
    }
}

// Share segment ordering between projection classification and adjacency.
template<typename Visitor>
inline void forEachLineSegment(GLenum mode, const float *data, int stride,
                               int vertexCount, Visitor visit)
{
    auto add = [&](int a, int b) {
        const float *p = data + a * stride, *q = data + b * stride;
        if (p[0] != q[0] || p[1] != q[1] || p[2] != q[2])
            visit(a, b);
    };
    if (mode == GL_LINES) {
        for (int i = 0; i + 1 < vertexCount; i += 2)
            add(i, i + 1);
    } else {
        for (int i = 0; i + 1 < vertexCount; ++i)
            add(i, i + 1);
        if (mode == GL_LINE_LOOP && vertexCount >= 3)
            add(vertexCount - 1, 0);
    }
}

/** Classify projected segments without constructing expanded adjacency data. */
inline QVector<quint8> projectedLineMask(GLenum mode, const float *data, int stride,
                                         int vertexCount, const QMatrix4x4 &mvp,
                                         const QVector2D &viewport)
{
    QVector<quint8> collapsed;
    if (!data || stride < 3 || vertexCount < 2)
        return collapsed;
    collapsed.reserve(mode == GL_LINES ? vertexCount / 2 : vertexCount);
    auto project = [&](int i) {
        const float *p = data + i * stride;
        return mvp * QVector4D(p[0], p[1], p[2], 1.f);
    };
    forEachLineSegment(mode, data, stride, vertexCount, [&](int a, int b) {
        const QVector4D ca = project(a), cb = project(b);
        bool skip = false;
        // Do not collapse segments crossing or behind the camera plane.
        if (ca.w() > 0.f && cb.w() > 0.f) {
            const QVector2D delta((cb.x()/cb.w() - ca.x()/ca.w()) * viewport.x() * .5f,
                                  (cb.y()/cb.w() - ca.y()/ca.w()) * viewport.y() * .5f);
            skip = delta.length() < 1e-4f;
        }
        collapsed.append(quint8(skip));
    });
    return collapsed;
}

/**
 * Rewrite a GL_LINES / GL_LINE_STRIP / GL_LINE_LOOP vertex array as
 * GL_LINES_ADJACENCY (prev, a, b, next per segment) for the wide-line
 * geometry shader. Each output vertex contains its original `stride` floats
 * (xyz first), followed by xyz for one farther point on each side. The
 * geometry shader uses those farther points to skip one screen-degenerate
 * neighbour while preserving distance competition across the joint.
 *
 * Segments with matching endpoints are treated as joined, including a closing
 * last-to-first segment. GL_LINES pairs remain independent unless joinLinePairs
 * is enabled for ordered curve fragments; gaps still mark free ends.
 * Zero-length segments are dropped.
 * With an MVP and viewport, skip entire runs of screen-degenerate segments
 * while following the original connections. A caller may supply a mask from
 * projectedLineMask for the same geometry to avoid classifying it twice.
 */
inline QVector<float> toLineAdjacency(GLenum mode, const float *data, int stride, int vertexCount,
                                     bool joinLinePairs = false,
                                     const QMatrix4x4 *mvp = nullptr,
                                     const QVector2D &viewport = QVector2D(),
                                     const QVector<quint8> *projectedMask = nullptr)
{
    QVector<float> out;
    if (!data || stride < 3 || vertexCount < 2)
        return out;

    struct Seg { int a, b; };
    QVector<Seg> segs;
    auto samePos = [&](int i, int j) {
        const float *p = data + i * stride;
        const float *q = data + j * stride;
        return p[0] == q[0] && p[1] == q[1] && p[2] == q[2];
    };
    forEachLineSegment(mode, data, stride, vertexCount,
                       [&](int a, int b) { segs.append({a, b}); });

    const int n = segs.size();
    if (n == 0)
        return out;
    const bool closed = n >= 3 && samePos(segs[n - 1].b, segs[0].a);

    // Keep topology in object space: coincident screen positions alone must
    // never connect independent fragments or bridge a dash gap.
    const QVector<quint8> collapsed = projectedMask ? *projectedMask
        : mvp ? projectedLineMask(mode, data, stride, vertexCount, *mvp, viewport)
              : QVector<quint8>(n, 0);
    Q_ASSERT(collapsed.size() == n);
    auto neighbour = [&](int j, int step) {
        if (mode == GL_LINES && !joinLinePairs)
            return -1;
        int current = j;
        for (int count = 0; count < n; ++count) {
            int candidate = current + step;
            if (candidate < 0 || candidate >= n) {
                if (!closed)
                    return -1;
                candidate = (candidate + n) % n;
            }
            if (candidate == j)
                return -1;
            const bool connected = step < 0
                ? samePos(segs[candidate].b, segs[current].a)
                : samePos(segs[current].b, segs[candidate].a);
            if (!connected)
                return -1;
            if (!collapsed[candidate])
                return candidate;
            current = candidate;
        }
        return -1;
    };

    out.reserve(n * 4 * (stride + 6));
    auto appendVert = [&](int i, int farPrev, int farNext) {
        const float *p = data + i * stride;
        for (int k = 0; k < stride; ++k)
            out.append(p[k]);
        const float *pFarPrev = data + farPrev * stride;
        const float *pFarNext = data + farNext * stride;
        for (int k = 0; k < 3; ++k)
            out.append(pFarPrev[k]);
        for (int k = 0; k < 3; ++k)
            out.append(pFarNext[k]);
    };
    for (int j = 0; j < n; ++j) {
        if (collapsed[j])
            continue;
        const Seg &s = segs[j];
        int prev = s.a, next = s.b;
        int farPrev = prev, farNext = next;
        const int prevSeg = neighbour(j, -1);
        if (prevSeg >= 0) {
            prev = segs[prevSeg].a;
            int farPrevSeg = prevSeg - 1;
            if (farPrevSeg < 0 && closed)
                farPrevSeg = n - 1;
            if (farPrevSeg >= 0 && samePos(segs[farPrevSeg].b, segs[prevSeg].a))
                farPrev = segs[farPrevSeg].a;
            else
                farPrev = prev;
        }

        const int nextSeg = neighbour(j, 1);
        if (nextSeg >= 0) {
            next = segs[nextSeg].b;
            int farNextSeg = nextSeg + 1;
            if (farNextSeg >= n && closed)
                farNextSeg = 0;
            if (farNextSeg < n && samePos(segs[farNextSeg].a, segs[nextSeg].b))
                farNext = segs[farNextSeg].b;
            else
                farNext = next;
        }
        appendVert(prev, farPrev, farNext);
        appendVert(s.a, farPrev, farNext);
        appendVert(s.b, farPrev, farNext);
        appendVert(next, farPrev, farNext);
    }
    return out;
}

} // namespace GlGeom

#endif // GLGEOM_H
