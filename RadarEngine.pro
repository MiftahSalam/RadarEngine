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
    target.path = /usr/lib
    header_base.files = $$HEADERS_BASE
    header_base.path = /usr/include/RadarEngine
    INSTALLS += target
    INSTALLS += header_base
	
    INCLUDEPATH += /usr/include/RadarConfigLib/
    DEPENDPATH += /usr/include/RadarConfigLib/
	
    LIBS += -L/usr/lib/RadarConfigLib/ -lRadarConfig

} else:win32 {
        LIBS += -LC:\Users\miftah\RadarConfigLib\lib\ -lRadarConfig
	
        INCLUDEPATH += C:\Users\miftah\RadarConfigLib\include
        DEPENDPATH += C:\Users\miftah\RadarConfigLib\lib\include

}
