TEMPLATE = subdirs

SUBDIRS += minimal

contains(QT_CONFIG, xcb) {
    SUBDIRS += xcb
}

mac:!ios: SUBDIRS += cocoa

win32: SUBDIRS += windows

qnx {
    SUBDIRS += qnx
}

contains(QT_CONFIG, eglfs) {
    SUBDIRS += eglfs
    SUBDIRS += minimalegl
}

contains(QT_CONFIG, directfb) {
    SUBDIRS += directfb
}

contains(QT_CONFIG, kms) {
    SUBDIRS += kms
}

contains(QT_CONFIG, linuxfb): SUBDIRS += linuxfb
