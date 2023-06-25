target_sources(RadarEngine
	PRIVATE arpatarget.h radararpa.h kalmanfilter.h arpatarget.cpp radararpa.cpp kalmanfilter.cpp
	PUBLIC FILE_SET HEADERS
	BASE_DIRS ${PROJECT_SOURCE_DIR}/arpa
	FILES arpatarget.h radararpa.h)
