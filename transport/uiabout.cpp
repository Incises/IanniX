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

#include "uiabout.h"
#include "ui_uiabout.h"
#include <QGuiApplication>
#include <QScreen>

UiAbout::UiAbout(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UiAbout) {
    ui->setupUi(this);
    ui->version->setText(tr("version") + " " + QCoreApplication::applicationVersion() + " " + tr("beta"));
    QRect screen = QGuiApplication::primaryScreen()->geometry();
    move(screen.center() - rect().center());
}

UiAbout::~UiAbout() {
    delete ui;
}

void UiAbout::changeEvent(QEvent *e) {
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
