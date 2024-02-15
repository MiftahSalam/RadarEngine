HEADERS_BASE += \
                $$PWD/radarconfig.h \
                $$PWD/radarengine.h \
                $$PWD/radardraw.h \
                $$PWD/guardzone.h \

HEADERS_SHARED += \
                $$PWD/shared/global.h \
                $$PWD/shared/constants.h

HEADERS_ARPA += \
                $$PWD/arpa/kalmanfilter.h \
                $$PWD/arpa/arpatarget.h \
                $$PWD/arpa/radararpa.h

HEADERS_STREAM += \
                $$PWD/stream/radartransmit.h \
                $$PWD/stream/radarreceive.h

SOURCES += \
           $$PWD/radarconfig.cpp \
           $$PWD/radarengine.cpp \
           $$PWD/stream/radarreceive.cpp \
           $$PWD/stream/radartransmit.cpp \
           $$PWD/radardraw.cpp \
           $$PWD/arpa/arpatarget.cpp \
           $$PWD/arpa/kalmanfilter.cpp \
           $$PWD/guardzone.cpp \
           $$PWD/arpa/radararpa.cpp

HEADERS += $$HEADERS_BASE \
           $$HEADERS_SHARED \
           $$HEADERS_ARPA \
           $$HEADERS_STREAM
