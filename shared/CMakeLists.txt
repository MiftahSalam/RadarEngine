target_sources(RadarEngine
	PRIVATE utils.cpp
	PUBLIC FILE_SET HEADERS
	BASE_DIRS ${PROJECT_SOURCE_DIR}
	FILES constants.h global.h utils.h)
