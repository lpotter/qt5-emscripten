TEMPLATE = subdirs
CONFIG += no_docs_target

SUBDIRS = \
    ipc \
    json \
    mimetypes \
    tools

!emscripten: threads

