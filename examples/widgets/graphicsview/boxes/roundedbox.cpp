/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

#include "roundedbox.h"

//============================================================================//
//                                P3T2N3Vertex                                //
//============================================================================//

VertexDescription P3T2N3Vertex::description[] = {
    {VertexDescription::Position, GL_FLOAT, SIZE_OF_MEMBER(P3T2N3Vertex, position) / sizeof(float), 0, 0},
    {VertexDescription::TexCoord, GL_FLOAT, SIZE_OF_MEMBER(P3T2N3Vertex, texCoord) / sizeof(float), sizeof(QVector3D), 0},
    {VertexDescription::Normal, GL_FLOAT, SIZE_OF_MEMBER(P3T2N3Vertex, normal) / sizeof(float), sizeof(QVector3D) + sizeof(QVector2D), 0},

    {VertexDescription::Null, 0, 0, 0, 0},
};

//============================================================================//
//                                GLRoundedBox                                //
//============================================================================//

float lerp(float a, float b, float t)
{
    return a * (1.0f - t) + b * t;
}

GLRoundedBox::GLRoundedBox(float r, float scale, int n)
    : GLTriangleMesh<P3T2N3Vertex, unsigned short>((n+2)*(n+3)*4, (n+1)*(n+1)*24+36+72*(n+1))
{
    int vidx = 0, iidx = 0;
    int vertexCountPerCorner = (n + 2) * (n + 3) / 2;

    P3T2N3Vertex *vp = m_vb.lock();
    unsigned short *ip = m_ib.lock();

    if (!vp || !ip) {
        qWarning("GLRoundedBox::GLRoundedBox: Failed to lock vertex buffer and/or index buffer.");
        m_ib.unlock();
        m_vb.unlock();
        return;
    }

    for (int corner = 0; corner < 8; ++corner) {
        QVector3D centre(corner & 1 ? 1.0f : -1.0f,
                corner & 2 ? 1.0f : -1.0f,
                corner & 4 ? 1.0f : -1.0f);
        int winding = (corner & 1) ^ ((corner >> 1) & 1) ^ (corner >> 2);
        int offsX = ((corner ^ 1) - corner) * vertexCountPerCorner;
        int offsY = ((corner ^ 2) - corner) * vertexCountPerCorner;
        int offsZ = ((corner ^ 4) - corner) * vertexCountPerCorner;

        // Face polygons
        if (winding) {
            ip[iidx++] = vidx;
            ip[iidx++] = vidx + offsX;
            ip[iidx++] = vidx + offsY;

            ip[iidx++] = vidx + vertexCountPerCorner - n - 2;
            ip[iidx++] = vidx + vertexCountPerCorner - n - 2 + offsY;
            ip[iidx++] = vidx + vertexCountPerCorner - n - 2 + offsZ;

            ip[iidx++] = vidx + vertexCountPerCorner - 1;
            ip[iidx++] = vidx + vertexCountPerCorner - 1 + offsZ;
            ip[iidx++] = vidx + vertexCountPerCorner - 1 + offsX;
        }

        for (int i = 0; i < n + 2; ++i) {

            // Edge polygons
            if (winding && i < n + 1) {
                ip[iidx++] = vidx + i + 1;
                ip[iidx++] = vidx;
                ip[iidx++] = vidx + offsY + i + 1;
                ip[iidx++] = vidx + offsY;
                ip[iidx++] = vidx + offsY + i + 1;
                ip[iidx++] = vidx;

                ip[iidx++] = vidx + i;
                ip[iidx++] = vidx + 2 * i + 2;
                ip[iidx++] = vidx + i + offsX;
                ip[iidx++] = vidx + 2 * i + offsX + 2;
                ip[iidx++] = vidx + i + offsX;
                ip[iidx++] = vidx + 2 * i + 2;

                ip[iidx++] = (corner + 1) * vertexCountPerCorner - 1 - i;
                ip[iidx++] = (corner + 1) * vertexCountPerCorner - 2 - i;
                ip[iidx++] = (corner + 1) * vertexCountPerCorner - 1 - i + offsZ;
                ip[iidx++] = (corner + 1) * vertexCountPerCorner - 2 - i + offsZ;
                ip[iidx++] = (corner + 1) * vertexCountPerCorner - 1 - i + offsZ;
                ip[iidx++] = (corner + 1) * vertexCountPerCorner - 2 - i;
            }

            for (int j = 0; j <= i; ++j) {
                QVector3D normal = QVector3D(i - j, j, n + 1 - i).normalized();
                QVector3D offset(0.5f - r, 0.5f - r, 0.5f - r);
                QVector3D pos = centre * (offset + r * normal);

                vp[vidx].position = scale * pos;
                vp[vidx].normal = centre * normal;
                vp[vidx].texCoord = QVector2D(pos.x() + 0.5f, pos.y() + 0.5f);

                // Corner polygons
                if (i < n + 1) {
                    ip[iidx++] = vidx;
                    ip[iidx++] = vidx + i + 2 - winding;
                    ip[iidx++] = vidx + i + 1 + winding;
                }
                if (i < n) {
                    ip[iidx++] = vidx + i + 1 + winding;
                    ip[iidx++] = vidx + i + 2 - winding;
                    ip[iidx++] = vidx + 2 * i + 4;
                }

                ++vidx;
            }
        }

    }

    m_ib.unlock();
    m_vb.unlock();
}

