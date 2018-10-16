/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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

//! [0]
U function(const T &t);
//! [0]


//! [1]
QImage scaled(const QImage &image)
{
    return image.scaled(100, 100);
}

QList<QImage> images = ...;
QFuture<QImage> thumbnails = QtConcurrent::mapped(images, scaled);
//! [1]


//! [2]
U function(T &t);
//! [2]


//! [3]
void scale(QImage &image)
{
    image = image.scaled(100, 100);
}

QList<QImage> images = ...;
QFuture<void> future = QtConcurrent::map(images, scale);
//! [3]


//! [4]
V function(T &result, const U &intermediate)
//! [4]


//! [5]
void addToCollage(QImage &collage, const QImage &thumbnail)
{
    QPainter p(&collage);
    static QPoint offset = QPoint(0, 0);
    p.drawImage(offset, thumbnail);
    offset += ...;
}

QList<QImage> images = ...;
QFuture<QImage> collage = QtConcurrent::mappedReduced(images, scaled, addToCollage);
//! [5]


//! [6]
QList<QImage> images = ...;

QFuture<QImage> thumbnails = QtConcurrent::mapped(images.constBegin(), images.constEnd(), scaled);

// Map in-place only works on non-const iterators.
QFuture<void> future = QtConcurrent::map(images.begin(), images.end(), scale);

QFuture<QImage> collage = QtConcurrent::mappedReduced(images.constBegin(), images.constEnd(), scaled, addToCollage);
//! [6]


//! [7]
QList<QImage> images = ...;

// Each call blocks until the entire operation is finished.
QList<QImage> future = QtConcurrent::blockingMapped(images, scaled);

QtConcurrent::blockingMap(images, scale);

QImage collage = QtConcurrent::blockingMappedReduced(images, scaled, addToCollage);
//! [7]


//! [8]
// Squeeze all strings in a QStringList.
QStringList strings = ...;
QFuture<void> squeezedStrings = QtConcurrent::map(strings, &QString::squeeze);

// Swap the rgb values of all pixels on a list of images.
QList<QImage> images = ...;
QFuture<QImage> bgrImages = QtConcurrent::mapped(images, &QImage::rgbSwapped);

// Create a set of the lengths of all strings in a list.
QStringList strings = ...;
QFuture<QSet<int> > wordLengths = QtConcurrent::mappedReduced(strings, &QString::length, &QSet<int>::insert);
//! [8]


//! [9]
// Can mix normal functions and member functions with QtConcurrent::mappedReduced().

// Compute the average length of a list of strings.
extern void computeAverage(int &average, int length);
QStringList strings = ...;
QFuture<int> averageWordLength = QtConcurrent::mappedReduced(strings, &QString::length, computeAverage);

// Create a set of the color distribution of all images in a list.
extern int colorDistribution(const QImage &string);
QList<QImage> images = ...;
QFuture<QSet<int> > totalColorDistribution = QtConcurrent::mappedReduced(images, colorDistribution, QSet<int>::insert);
//! [9]


//! [10]
QImage QImage::scaledToWidth(int width, Qt::TransformationMode) const;
//! [10]

//! [11]
struct ImageTransform
{
    void operator()(QImage &result, const QImage &value);
};

QFuture<QImage> thumbNails =
  QtConcurrent::mappedReduced<QImage>(images,
                                      Scaled(100),
                                      ImageTransform(),
                                      QtConcurrent::SequentialReduce);
//! [11]

//! [13]
QList<QImage> images = ...;
std::function<QImage(const QImage &)> scale = [](const QImage &img) {
    return img.scaledToWidth(100, Qt::SmoothTransformation);
};
QFuture<QImage> thumbnails = QtConcurrent::mapped(images, scale);
//! [13]

//! [14]
struct Scaled
{
    Scaled(int size)
    : m_size(size) { }

    typedef QImage result_type;

    QImage operator()(const QImage &image)
    {
        return image.scaled(m_size, m_size);
    }

    int m_size;
};

QList<QImage> images = ...;
QFuture<QImage> thumbnails = QtConcurrent::mapped(images, Scaled(100));
//! [14]
