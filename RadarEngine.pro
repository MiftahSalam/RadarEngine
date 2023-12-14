QT       += core network opengl

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

    header_base.files = $$HEADERS_BASE
    header_base.path = /usr/include/RadarEngine

    header_arpa.files = $$HEADERS_ARPA
    header_arpa.path = /usr/include/RadarEngine/arpa

    header_shared.files = $$HEADERS_SHARED
    header_shared.path = /usr/include/RadarEngine/shared

    header_stream.files = $$HEADERS_STREAM
    header_stream.path = /usr/include/RadarEngine/stream

    INSTALLS += target
    INSTALLS += header_base header_arpa header_shared header_stream
}

DISTFILES += \
    CMakeLists.txt
