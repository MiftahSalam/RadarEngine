QT       += network opengl

include(RadarEngine.pri)

CONFIG += staticlib

TARGET = RadarEngine
TEMPLATE = lib
DESTDIR = bin
DEFINES += RADAR_ENGINE_LIBRARY
MOC_DIR = tmp
OBJECTS_DIR = obj

unix: {
    target.path = /usr/lib/RadarEngine
    header.files = $$HEADERS_BASE \
                        $$HEADERS_SHARED \
                        $$HEADERS_ARPA \
                        $$HEADERS_STREAM

    header.path = /usr/include/RadarEngine
    INSTALLS += target
    INSTALLS += header
}

DISTFILES += \
    CMakeLists.txt
