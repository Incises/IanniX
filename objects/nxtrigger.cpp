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

#include "nxtrigger.h"
#include "render/gl/glpainter.h"
#include "render/gl/glgeom.h"

#include <QtMath>

GlMesh NxTrigger::s_meshFill;
GlMesh NxTrigger::s_meshOutline;

void NxTrigger::ensureMeshes()
{
    if (!s_meshFill.isEmpty() && !s_meshOutline.isEmpty() && !s_meshFill.isDirty())
        return;

    QVector<float> fill;
    QVector<float> outline;
    for (qreal drawAngle = 0; drawAngle < 2 * M_PI; drawAngle += 0.1) {
        const float c = float(qCos(drawAngle));
        const float s = float(qSin(drawAngle));
        GlGeom::appendXYZ(fill, 0.5f * c, 0.5f * s, 0.f);
        GlGeom::appendXYZ(outline, 1.2f * c, 1.2f * s, 0.f);
    }
    s_meshFill.upload(GL_TRIANGLE_FAN, fill, fill.size() / 3);
    s_meshOutline.upload(GL_LINE_LOOP, outline, outline.size() / 3);
}

NxTrigger::NxTrigger(ApplicationCurrent *parent, QTreeWidgetItem *ccParentItem) :
    NxObject(parent, ccParentItem) {
    cacheSize = 0;
    cursorTrigged = 0;
    lastTrigTime = 0;
    setText(0, tr("TRIGGER"));

    initializeCustom();
}

void NxTrigger::initializeCustom() {
    setSize(1);
    setColorActive  ("_trigger_active");
    setColorInactive("_trigger_inactive");
    setTextureActive  ("trigger_active");
    setTextureInactive("trigger_inactive");
    setTriggerOff(1);
    setMessagePatterns("1," + Application::defaultMessageTrigger.val());
}

void NxTrigger::paint() {
    //Color
    if(cursorTrigged)
        color = colorTrigged;
    else if(active) {
        if(colorActive.isEmpty())                                                                                   color = colorActiveColor;
        else if((colorActive.startsWith("_")) && (Render::colors->contains(Application::colorsPrefix() + colorActive)))  color = Render::colors->value(Application::colorsPrefix() + colorActive);
        else if(Render::colors->contains(colorActive))                                                              color = Render::colors->value(colorActive);
        else                                                                                                        color = Qt::gray;
    }
    else {
        if(colorInactive.isEmpty())                                                                                     color = colorInactiveColor;
        else if((colorInactive.startsWith("_")) && (Render::colors->contains(Application::colorsPrefix() + colorInactive)))  color = Render::colors->value(Application::colorsPrefix() + colorInactive);
        else if(Render::colors->contains(colorInactive))                                                                color = Render::colors->value(colorInactive);
        else                                                                                                            color = Qt::gray;
    }
    color.setRgb (qBound(qreal(0.), color.red()   * colorMultiplyColor.redF(),   qreal(255.)),
                  qBound(qreal(0.), color.green() * colorMultiplyColor.greenF(), qreal(255.)),
                  qBound(qreal(0.), color.blue()  * colorMultiplyColor.blueF(),  qreal(255.)),
                  qBound(qreal(0.), color.alpha() * colorMultiplyColor.alphaF(), qreal(255.)));

    //Size of trigger
    if(cacheSize != Render::objectSize*size) {
        cacheSize = Render::objectSize*size;
        calcBoundingRect();
    }

    if((color.alpha() > 0) && (cacheSize > 0)) {
        if(selectedHover)   color = Render::colors->value(Application::colorsPrefix() + "_gui_object_hover");
        if(selected)        color = Render::colors->value(Application::colorsPrefix() + "_gui_object_selection");

        //Start
        if(!Application::allowSelectionTriggers)
            color.setAlphaF(color.alphaF()/3);

        GlPainter *g = GlPainter::current();
        if (!g || !g->isReady())
            return;

        if(Render::paintThisGroup)
            g->setColor(color);
        else
            g->setColor(float(color.redF()), float(color.greenF()), float(color.blueF()), 0.1f);

        g->pushMatrix();
        g->translate(float(pos.x()), float(pos.y()), float(pos.z()));
        g->rotate(float(Render::rotation.z()), 0, 0, -1);
        g->rotate(float(Render::rotation.x()), 0, -1, 0);
        g->rotate(float(Render::rotation.y()), -1, 0, 0);

        //Label
        if((Render::paintThisGroup) && (Application::paintLabel || selectedHover) && (!label.isEmpty()))
            Application::render->renderText(cacheSize * 1.8, cacheSize * 1.8, 0, label.toUpper(), Application::renderFont, false);
        else if(selectedHover)
            Application::render->renderText(cacheSize * 1.8, cacheSize * 1.8, 0, QString::number(id), Application::renderFont, false);
        if((selectedHover) && (!isDrag)) {
            qreal startY = 0.1 - cacheSize * 1.2;
            foreach(const QString & messageLabelItem, messageLabel) {
                Application::render->renderText(cacheSize * 1.8, startY, 0, messageLabelItem.trimmed(), Application::renderFont, false);
                startY -= 0.2 * Render::zoomLinear;
            }
        }

        //Draw
        bool textureOk = false;
        QString textureName = (active)?(textureActive):(textureInactive);
        if(Render::textures->contains(textureName)) {
            UiRenderTexture *texture = Render::textures->value(textureName);
            if((texture) && (texture->loaded) && (texture->mapping.width() != 0 ) && (texture->mapping.height() != 0)) {
                textureOk = true;
                const qreal widthRatio = cacheSize * texture->originalSize.width() / texture->originalSize.height();
                if(texture->isSyphon) {
                    g->bindTexture(GlPainter::TextureRectangle, texture->texture);
                    g->begin(GL_QUADS);
                    g->texCoord(0, 0);
                    g->vertex(float(widthRatio * texture->mapping.left()), float(cacheSize * texture->mapping.bottom()));
                    g->texCoord(float(texture->originalSize.width()), 0);
                    g->vertex(float(widthRatio * texture->mapping.right()), float(cacheSize * texture->mapping.bottom()));
                    g->texCoord(float(texture->originalSize.width()), float(texture->originalSize.height()));
                    g->vertex(float(widthRatio * texture->mapping.right()), float(cacheSize * texture->mapping.top()));
                    g->texCoord(0, float(texture->originalSize.height()));
                    g->vertex(float(widthRatio * texture->mapping.left()), float(cacheSize * texture->mapping.top()));
                    g->end();
                    g->unbindTexture();
                }
                else {
                    g->bindTexture(GlPainter::Texture2D, texture->texture);
                    g->begin(GL_QUADS);
                    g->texCoord(0, 0);
                    g->vertex(float(widthRatio * texture->mapping.left()), float(cacheSize * texture->mapping.bottom()));
                    g->texCoord(1, 0);
                    g->vertex(float(widthRatio * texture->mapping.right()), float(cacheSize * texture->mapping.bottom()));
                    g->texCoord(1, 1);
                    g->vertex(float(widthRatio * texture->mapping.right()), float(cacheSize * texture->mapping.top()));
                    g->texCoord(0, 1);
                    g->vertex(float(widthRatio * texture->mapping.left()), float(cacheSize * texture->mapping.top()));
                    g->end();
                    g->unbindTexture();
                }
            }
        }
        if(!textureOk) {
            ensureMeshes();
            g->scale(float(cacheSize), float(cacheSize), float(cacheSize));
            g->setLineWidth(float(OpenGlDrawing::dpi * size));
            s_meshFill.draw(g);
            g->setLineWidth(float(OpenGlDrawing::dpi * 1.5));
            s_meshOutline.draw(g);
            g->setLineWidth(float(OpenGlDrawing::dpi));
        }

        g->popMatrix();
    }
}

void NxTrigger::trig(NxObject *cursor) {
    if(cursor) {
        colorTrigged = cursor->getCurrentColor();
        colorTrigged.setAlpha(255);
    }
    cursorTrigged = cursor;
    MessageManager::outgoingMessage(MessageManagerDestination(this, this, cursorTrigged));
    if(triggerOff > 0)  QTimer::singleShot(triggerOff*1000, this, &NxTrigger::trigEnd);
    else                trigEnd();
}
void NxTrigger::trigEnd() {
    NxObject *cursorTriggedTmp = cursorTrigged;
    cursorTrigged = 0;
    if(triggerOff > 0) {
        bool sendMessage = false;
        foreach(const QVector<QByteArray> & messagePattern, this->getMessagePatterns()) {
            foreach(const QByteArray &messageArgument, messagePattern) {
                if(messageArgument == "trigger_value")
                    sendMessage = true;
            }
        }
        if(sendMessage)
            MessageManager::outgoingMessage(MessageManagerDestination(this, this, cursorTriggedTmp));
    }
}
