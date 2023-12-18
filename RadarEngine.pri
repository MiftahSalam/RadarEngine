HEADERS_BASE += \
                $$PWD/radarconfig.h \
                $$PWD/radarengine.h \
                $$PWD/radardraw.h \
                $$PWD/guardzone.h \
                $$PWD/global.h \
                $$PWD/constants.h \
                $$PWD/kalmanfilter.h \
                $$PWD/arpatarget.h \
                $$PWD/radararpa.h \
                $$PWD/radartransmit.h \
                $$PWD/radarreceive.h

SOURCES += \
           $$PWD/radarconfig.cpp \
           $$PWD/radarengine.cpp \
#           $$PWD/shared/utils.cpp \
           $$PWD/radarreceive.cpp \
           $$PWD/radartransmit.cpp \
           $$PWD/radardraw.cpp \
           $$PWD/arpatarget.cpp \
           $$PWD/kalmanfilter.cpp \
           $$PWD/guardzone.cpp \
           $$PWD/radararpa.cpp

HEADERS += $$HEADERS_BASE
