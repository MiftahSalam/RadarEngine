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
    target.path = /usr/lib/RadarEngine/2022/v1/
    header_base.files = $$HEADERS_BASE
    header_base.path = /usr/include/RadarEngine/2022/v1/

    INSTALLS += target
    INSTALLS += header_base
	
    INCLUDEPATH += /usr/include/RadarConfig/2022/v1/
    DEPENDPATH += /usr/include/RadarConfig/2022/v1/
	
    LIBS += -L/usr/lib/RadarConfig/2022/v1/ -lRadarConfig

} else:win32 {
#    LIBS += -LC:\Users\miftah\RadarConfigLib\lib\2022\v1\ -lRadarConfig
    LIBS += -LC:\Users\ms_tensai\RadarConfigLib\lib\2022\v1\ -lRadarConfig
    LIBS += -lOpenGL32
	
#    INCLUDEPATH += C:\Users\miftah\RadarConfigLib\include\2022\v1\
#    DEPENDPATH += C:\Users\miftah\RadarConfigLib\lib\include\2022\v1\
    INCLUDEPATH += C:\Users\ms_tensai\RadarConfigLib\include\2022\v1\
    DEPENDPATH += C:\Users\ms_tensai\RadarConfigLib\lib\include\2022\v1\
}
