TEMPLATE=subdirs

SUBDIRS = \
    kernel

!uikit: SUBDIRS += \
    image \
    math3d \
    painting \
    qopenglconfig \
    qopengl \
    qvulkan \
    text \
    util \
    itemmodels \

!qtConfig(opengl)|winrt: SUBDIRS -= qopengl qopenglconfig

!qtConfig(vulkan): SUBDIRS -= qvulkan
