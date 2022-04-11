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
    header_base.files = $$HEADERS_BASE
    header_base.path = /usr/include/RadarEngine
    INSTALLS += target
    INSTALLS += header_base
	
    INCLUDEPATH += /usr/include/RadarConfig/
    DEPENDPATH += /usr/include/RadarConfig/
	
    LIBS += -L/usr/lib/RadarConfig/ -lRadarConfig

} else:win32 {
    LIBS += -LC:\Users\miftah\RadarConfigLib\lib\ -lRadarConfig
    LIBS += -lOpenGL32
	
    INCLUDEPATH += C:\Users\miftah\RadarConfigLib\include
    DEPENDPATH += C:\Users\miftah\RadarConfigLib\lib\include
}
