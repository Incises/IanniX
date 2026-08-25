/*
    This file is part of IanniX, a graphical real-time open-source sequencer for digital art
    Copyright (C) 2010-2015 - IanniX Association (https://www.iannix.org/)
    Copyright (C) 2025-2026 - Hypar.XYZ (https://iannix.hypar.xyz/)

    This file was originally written by Guillaume Jacquemin.
    
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

#include "uirenderpreview.h"
#include "gl/glpainter.h"

#include <QtGlobal>

#ifdef USE_GLWIDGET
UiRenderPreview::UiRenderPreview(QWidget *parent, void *shared) :
    QGLWidget(QGLFormat(QGL::DoubleBuffer | QGL::DirectRendering), parent, (QGLWidget*)shared) {
#else
UiRenderPreview::UiRenderPreview(QWidget *parent, void *shared) :
    QOpenGLWidget(parent) {
    Q_UNUSED(shared);
    QSurfaceFormat sf;
    // Match main render: OpenGL 3.3 Core (MIGRATION.md §14).
    sf.setVersion(3, 3);
    sf.setProfile(QSurfaceFormat::CoreProfile);
    sf.setSamples(qMax(0, int(4. / devicePixelRatioFScale())));
    setFormat(sf);
#endif
    setFocusPolicy(Qt::StrongFocus);
    render = 0;
}

void UiRenderPreview::initializeGL() {
    if (!GlPainter::instance()->initialize())
        qWarning("UiRenderPreview: GlPainter initialization failed");
}

void UiRenderPreview::resizeGL(int width, int height) {
    GlPainter *g = GlPainter::instance();
    if (g && g->isReady()) {
        g->viewport(0, 0, width, height);
        g->setOrtho(0, 1, 0, 1, 1, -1);
        g->loadIdentity();
    }
}

void UiRenderPreview::paintPreview(NxEventsPropagation *_render, GLuint _renderPreviewTexture, QSizeF _renderSize) {
    makeCurrent();
    render               = _render;
    renderSize           = _renderSize;
    renderPreviewTexture = _renderPreviewTexture;
    updateGL();
}

void UiRenderPreview::paintGL() {
    GlPainter *g = GlPainter::instance();
    if (!g || !g->isReady())
        return;

    g->beginFrame();
    g->clearColor(QColor(0, 0, 0, 255));
    g->clear(GL_COLOR_BUFFER_BIT);
    g->setOrtho(0, 1, 0, 1, 1, -1);
    g->loadIdentity();

    qreal scaleX = 1, scaleY = 1;
    qreal ratioRender  = (qreal)width()            / (qreal)height();
    qreal ratioTexture = (qreal)renderSize.width() / (qreal)renderSize.height();

    if(ratioRender >= ratioTexture) scaleX = ratioTexture / ratioRender;
    else                            scaleY = ratioRender  / ratioTexture;

    g->pushMatrix();
    g->translate(float((1-scaleX)/2), float((1-scaleY)/2), 0);
    g->setColor(1, 1, 1, 1);
    g->bindTexture(GlPainter::Texture2D, renderPreviewTexture);
    g->begin(GL_QUADS);
    g->texCoord(0, 0); g->vertex(0, 0);
    g->texCoord(1, 0); g->vertex(float(scaleX), 0);
    g->texCoord(1, 1); g->vertex(float(scaleX), float(scaleY));
    g->texCoord(0, 1); g->vertex(0, float(scaleY));
    g->end();
    g->unbindTexture();
    g->popMatrix();
    g->endFrame();
}
