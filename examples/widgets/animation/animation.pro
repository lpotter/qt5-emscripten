TEMPLATE         = \
                 subdirs
SUBDIRS          += \
                 animatedtiles \
                 easing \
                 moveblocks \
                 states \
                 stickman

!emscripten: SUBDIRS += \
                 animatedtiles \
                 sub-attaq
