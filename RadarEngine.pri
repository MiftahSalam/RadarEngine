HEADERS_BASE += \
                $$PWD/shared/global.h \
                $$PWD/shared/constants.h \
                $$PWD/radarconfig.h \
                $$PWD/radarengine.h \
                $$PWD/radarreceive.h \
                $$PWD/radartransmit.h \
                $$PWD/radardraw.h \
                $$PWD/arpatarget.h \
                $$PWD/kalmanfilter.h \
                $$PWD/guardzone.h \
                $$PWD/radararpa.h

SOURCES += \
           $$PWD/radarconfig.cpp \
           $$PWD/radarengine.cpp \
           $$PWD/radarreceive.cpp \
           $$PWD/radartransmit.cpp \
           $$PWD/radardraw.cpp \
           $$PWD/arpatarget.cpp \
           $$PWD/kalmanfilter.cpp \
           $$PWD/guardzone.cpp \
           $$PWD/radararpa.cpp

HEADERS += $$HEADERS_BASE \
