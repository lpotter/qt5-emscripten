TEMPLATE = subdirs
SUBDIRS += src

# We do it this way instead of letting load(qt_module) handle it for two reasons:
#  1) qt_module_headers assumes the TARGET is the same as the include directory (eg: libGLESv2 != GLES2)
#  2) If we made a 'QtANGLE' module, the include directory would be flattened which won't work since
#     we need to support "#include <GLES2/gl2.h>"
CONFIG += minimal_syncqt
MODULE_INCNAME = QtANGLE/KHR QtANGLE/EGL QtANGLE/GLES2 QtANGLE/GLES3
VERSION = none
load(qt_module_headers)
