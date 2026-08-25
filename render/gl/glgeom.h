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
#include <QtMath>

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
                              float step = 0.02f)
{
    for (float t = 0.f; t <= 1.f + step * 0.5f; t += step) {
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
                const qreal u0 = t0 / len, u1 = t1 / len;
                appendLine(outLines,
                           float(a[0] + dx * u0), float(a[1] + dy * u0), float(a[2] + dz * u0),
                           float(a[0] + dx * u1), float(a[1] + dy * u1), float(a[2] + dz * u1));
            }
            dist += (t1 - t0);
            t0 = t1;
        }
    }
}

} // namespace GlGeom

#endif // GLGEOM_H
