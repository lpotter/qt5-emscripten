/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "glwidget.h"
#include <qmath.h>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    startTimer(40);
    setWindowTitle(tr("Sample Buffers"));
}

void GLWidget::initializeGL()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-.5, .5, .5, -.5, -1000, 1000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    makeObject();
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void GLWidget::paintGL()
{
    static float rot = 0.0;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glEnable(GL_MULTISAMPLE);
    glTranslatef(-0.25f, -0.10f, 0.0f);
    glScalef(0.75f, 1.15f, 0.0f);
    glRotatef(rot, 0.0f, 0.f, 1.0f);
    glCallList(list);
    glPopMatrix();

    glPushMatrix();
    glDisable(GL_MULTISAMPLE);
    glTranslatef(0.25f, -0.10f, 0.0f);
    glScalef(0.75f, 1.15f, 0.0f);
    glRotatef(rot, 0.0f, 0.0f, 1.0f);
    glCallList(list);
    glPopMatrix();

    rot += 0.2f;

    qglColor(Qt::black);
    renderText(-0.35, 0.4, 0.0, "Multisampling enabled");
    renderText(0.15, 0.4, 0.0, "Multisampling disabled");
}

void GLWidget::timerEvent(QTimerEvent *)
{
    update();
}

void GLWidget::makeObject()
{
    QColor qtGreen(QColor::fromCmykF(0.40, 0.0, 1.0, 0.0));
    const int NumSectors = 15;
    GLdouble x1 = +0.06;
    GLdouble y1 = -0.14;
    GLdouble x2 = +0.14;
    GLdouble y2 = -0.06;
    GLdouble x3 = +0.08;
    GLdouble y3 = +0.00;
    GLdouble x4 = +0.30;
    GLdouble y4 = +0.22;

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    {
        const double sectorAngle = 2 * M_PI / NumSectors;
        for (int i = 0; i < NumSectors; ++i) {
            double angle = i * sectorAngle;
            GLdouble x5 = 0.30 * sin(angle);
            GLdouble y5 = 0.30 * cos(angle);
            GLdouble x6 = 0.20 * sin(angle);
            GLdouble y6 = 0.20 * cos(angle);

            angle += sectorAngle;
            GLdouble x7 = 0.20 * sin(angle);
            GLdouble y7 = 0.20 * cos(angle);
            GLdouble x8 = 0.30 * sin(angle);
            GLdouble y8 = 0.30 * cos(angle);

            qglColor(qtGreen);
            quad(GL_QUADS, x5, y5, x6, y6, x7, y7, x8, y8);
            qglColor(Qt::black);
            quad(GL_LINE_LOOP, x5, y5, x6, y6, x7, y7, x8, y8);
        }

        qglColor(qtGreen);
        quad(GL_QUADS, x1, y1, x2, y2, y2, x2, y1, x1);
        quad(GL_QUADS, x3, y3, x4, y4, y4, x4, y3, x3);

        qglColor(Qt::black);
        quad(GL_LINE_LOOP, x1, y1, x2, y2, y2, x2, y1, x1);
        quad(GL_LINE_LOOP, x3, y3, x4, y4, y4, x4, y3, x3);
    }
    glEndList();
}

void GLWidget::quad(GLenum primitive, GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2,
                    GLdouble x3, GLdouble y3, GLdouble x4, GLdouble y4)
{
    glBegin(primitive);
    {
        glVertex2d(x1, y1);
        glVertex2d(x2, y2);
        glVertex2d(x3, y3);
        glVertex2d(x4, y4);
    }
    glEnd();
}
