#ifndef RADARENGINE_GLOBAL_H
#define RADARENGINE_GLOBAL_H

#include <QtCore/qglobal.h>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QColor>

#include <math.h>

namespace RadarEngine {

enum BlobColour {
    BLOB_NONE,
    BLOB_HISTORY_0,
    BLOB_HISTORY_1,
    BLOB_HISTORY_2,
    BLOB_HISTORY_3,
    BLOB_HISTORY_4,
    BLOB_HISTORY_5,
    BLOB_HISTORY_6,
    BLOB_HISTORY_7,
    BLOB_HISTORY_8,
    BLOB_HISTORY_9,
    BLOB_HISTORY_10,
    BLOB_HISTORY_11,
    BLOB_HISTORY_12,
    BLOB_HISTORY_13,
    BLOB_HISTORY_14,
    BLOB_HISTORY_15,
    BLOB_HISTORY_16,
    BLOB_HISTORY_17,
    BLOB_HISTORY_18,
    BLOB_HISTORY_19,
    BLOB_HISTORY_20,
    BLOB_HISTORY_21,
    BLOB_HISTORY_22,
    BLOB_HISTORY_23,
    BLOB_HISTORY_24,
    BLOB_HISTORY_25,
    BLOB_HISTORY_26,
    BLOB_HISTORY_27,
    BLOB_HISTORY_28,
    BLOB_HISTORY_29,
    BLOB_HISTORY_30,
    BLOB_HISTORY_31,
    BLOB_WEAK, //33
    BLOB_INTERMEDIATE, //34
    BLOB_STRONG //35
};

#define BLOB_HISTORY_MAX BLOB_HISTORY_31 //32
#define BLOB_COLOURS (BLOB_STRONG + 1) //36
#define BLOB_HISTORY_COLOURS (BLOB_HISTORY_MAX - BLOB_NONE) //32

typedef enum ControlType
{
    CT_GAIN,
    CT_SEA,
    CT_RAIN,
    CT_INTERFERENCE_REJECTION,
    CT_TARGET_SEPARATION,
    CT_NOISE_REJECTION,
    CT_TARGET_BOOST,
    CT_TARGET_EXPANSION,
    CT_REFRESHRATE,
    CT_SCAN_SPEED,
    CT_BEARING_ALIGNMENT,
    CT_SIDE_LOBE_SUPPRESSION,
    CT_ANTENNA_HEIGHT,
    CT_LOCAL_INTERFERENCE_REJECTION,
    CT_MAX  // Keep this last, see below
} ControlType;


enum RadarReportType
{
    RADAR_STATE = 0,
    RADAR_FILTER,
    RADAR_TYPE,
    RADAR_ALIGN,
    RADAR_SCAN_AND_SIGNAL
};

enum RadarState
{
    RADAR_OFF,
    RADAR_STANDBY,
    RADAR_TRANSMIT,
    RADAR_WAKING_UP,
    RADAR_NO_SPOKE
};

enum RadarFilter
{
    RADAR_GAIN,
    RADAR_RAIN,
    RADAR_SEA,
    RADAR_TARGET_BOOST,
    RADAR_LOCAL_INTERFERENCE_REJECTION,
    RADAR_TARGET_EXPANSION,
    RADAR_RANGE
};

enum RadarAlign
{
    RADAR_BEARING,
    RADAR_ANTENA
};

enum RadarScanSignal
{
    RADAR_SCAN_SPEED,
    RADAR_NOISE_REJECT,
    RADAR_TARGET_SEPARATION,
    RADAR_LOBE_SUPRESION,
    RADAR_INTERFERENT
};


struct RadarRange {
  int meters; //command to radar and display
  uint actual_meters; //based on range feedback
  const char *name;
};

static const QList<int> distanceList = QList<int>()<<40000 //0
                                                <<30000 //1
                                               <<20000 //2
                                              <<10000 //3
                                             <<5000 //4
                                            <<2000 //5
                                           <<1500 //6
                                          <<1000 //7
                                         <<500 //8
                                           ;

}

#endif // RADARENGINE_GLOBAL_H
