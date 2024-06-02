QT       += network opengl

include(RadarEngine.pri)

CONFIG += staticlib

TARGET = RadarEngine
TEMPLATE = lib
DESTDIR = bin
DEFINES += RADAR_ENGINE_LIBRARY
#DEFINES += SAVE_CAPTURE
#DEFINES += DISPLAY_ONLY
MOC_DIR = tmp
OBJECTS_DIR = obj

unix: {
    target.path = /usr/lib/RadarEngine/v3

    header_base.files = $$HEADERS_BASE
    header_base.path = /usr/include/RadarEngine/v3/RadarEngine

    INSTALLS += target
    INSTALLS += header_base
}

DISTFILES += \
    CMakeLists.txt
