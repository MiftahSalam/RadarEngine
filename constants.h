#ifndef RADARENGINE_CONSTANTS_H
#define RADARENGINE_CONSTANTS_H

#include <QtCore/qglobal.h>
#include <QDebug>
#include <math.h>

#ifndef UINT8
#ifdef Q_OS_WIN
#define UINT8 quint8
#elif defined(Q_OS_LINUX)
#define UINT8 u_int8_t
#endif
#endif

#ifndef UINT8_MAX
#define UINT8_MAX (255)
#endif

#ifndef UINT16
#ifdef Q_OS_WIN
#define UINT16 quint16
#elif defined(Q_OS_LINUX)
#define UINT16 u_int16_t
#endif
#endif

#ifndef UINT32
#ifdef Q_OS_WIN
#define UINT32 quint32
#elif defined(Q_OS_LINUX)
#define UINT32 u_int32_t
#endif
#endif

#ifndef PI
#define PI (3.1415926535897931160E0)
#endif

#ifndef KM_TO_NM
#define KM_TO_NM 0.539957
#endif

#ifndef deg2rad
#define deg2rad(x) ((x)*2 * PI / 360.0)
#endif
#ifndef rad2deg
#define rad2deg(x) ((x)*360.0 / (2 * PI))
#endif

#define RING_COUNT (5)
#define WATCHDOG_TIMEOUT (10000)  // After 10s assume GPS and heading data is invalid
#define TIMED_OUT(t, timeout) (t >= timeout)

#define SPOKES (4096)               //  radars can generate up to 4096 spokes per rotation,
#define LINES_PER_ROTATION (2048)   // but use only half that in practice
#define RETURNS_PER_LINE (512)      //  radars generate 512 separate values per range, at 8 bits each
#define DEGREES_PER_ROTATION (360)  // Classical math

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define MAX_OVERLAY_TRANSPARENCY (10)

// Use the above to convert from 'raw' headings sent by the radar (0..4095) into classical degrees
// (0..359) and back
#define SCALE_RAW_TO_DEGREES(raw) ((raw) * static_cast<double>(DEGREES_PER_ROTATION) / static_cast<double>(SPOKES))
#define SCALE_RAW_TO_DEGREES2048(raw) ((raw) * static_cast<double>(DEGREES_PER_ROTATION) / static_cast<double>(LINES_PER_ROTATION))
#define SCALE_DEGREES_TO_RAW(angle) (static_cast<int>((angle) * static_cast<double>(SPOKES) / static_cast<double>(DEGREES_PER_ROTATION)))
#define SCALE_DEGREES_TO_RAW2048(angle) (static_cast<int>((angle) * static_cast<double>(LINES_PER_ROTATION) / static_cast<double>(DEGREES_PER_ROTATION)))
#define MOD_DEGREES(angle) (fmod(angle + 2 * DEGREES_PER_ROTATION, DEGREES_PER_ROTATION))
#define MOD_ROTATION(raw) (((raw) + 2 * SPOKES) % SPOKES)
#define MOD_ROTATION2048(raw) (((raw) + 2 * LINES_PER_ROTATION) % LINES_PER_ROTATION)

#define BLOB_HISTORY_MAX BLOB_HISTORY_31 //32
#define BLOB_COLOURS (BLOB_STRONG + 1) //36
#define BLOB_HISTORY_COLOURS (BLOB_HISTORY_MAX - BLOB_NONE) //32

static const QList<int> distanceList = QList<int>()<<60000 //0
                                                <<50000 //1
                                               <<40000 //2
                                              <<30000 //3
                                             <<20000 //4
                                            <<10000 //5
                                           <<5000 //6
                                          <<2000 //7
                                         <<1500 //8
                                        <<1000 //9
                                       <<500 //10
                                           ;
static const QList<int> distanceListNautical = QList<int>()<<32000/KM_TO_NM //0
                                                <<27000/KM_TO_NM //1
                                               <<22000/KM_TO_NM //2
                                              <<16000/KM_TO_NM //3
                                             <<10000/KM_TO_NM //4
                                            <<5000/KM_TO_NM //5
                                           <<2000/KM_TO_NM //6
                                          <<1000/KM_TO_NM //7
                                         <<800/KM_TO_NM //8
                                        <<500/KM_TO_NM //9
                                       <<250/KM_TO_NM //10
                                           ;
#endif // RADARENGINE_CONSTANTS_H
