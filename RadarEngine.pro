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
    target.path = /usr/lib/RadarEngine/pjs-2024

    header_base.files = $$HEADERS_BASE
    header_base.path = /usr/include/RadarEngine/pjs-2024

    INSTALLS += target
    INSTALLS += header_base
}

DISTFILES += \
    CMakeLists.txt
