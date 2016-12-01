TEMPLATE      = subdirs
SUBDIRS       = htmlinfo \
                xmlstreamlint

qtHaveModule(widgets) {
    SUBDIRS +=  dombookmarks \
                saxbookmarks

!emscripten: SUBDIRS +=  \
                streambookmarks

    qtHaveModule(network): SUBDIRS += \
                rsslisting
}
