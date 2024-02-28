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
    target.path = /usr/lib/pjs-2024/RadarEngine

    header_base.files = $$HEADERS_BASE
    header_base.path = /usr/include/pjs-2024/RadarEngine

    INSTALLS += target
    INSTALLS += header_base
}

DISTFILES += \
    CMakeLists.txt
