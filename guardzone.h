#ifndef GUARDZONE_H
#define GUARDZONE_H

#include <QObject>
#include <QMutex>

//#include "radarengine.h"
#include <radarconfig_global.h>

namespace RadarEngine {

typedef enum GZType { GZ_ARC, GZ_CIRCLE } GZType;
class RadarEngine;

class GuardZone : public QObject
{
    Q_OBJECT
public:
    GuardZone(RadarEngine* re);

    quint64 arpa_update_time[LINES_PER_ROTATION];

    virtual ~GuardZone() {}

    void ResetBogeys()
    {
        m_bogey_count = 0;
                m_running_count = 0;
                m_last_in_guard_zone = false;
                m_last_angle = 0;
    }
    void ProcessSpoke(int angle, UINT8 *data, UINT8 *hist, int range);
//    void ProcessSpokePoly(int angle, UINT8 *data, int range);
//    void autoTrack();
    void setCurRange(int range){m_current_range = range;}

    int GetBogeyCount()
    {
        return m_bogey_count;
    }
    void resetPolygon()
    {
//        m_polygon.clear();
        m_start_bearing = 0;
        m_range_start = 0;
        m_end_bearing = 0;
        m_range_end = 0;
    }

//    void SetPolygon(const QPolygonF polyF);

//    QPolygonF getPolygon()
//    {
//        return m_polygon;
//    }
/*
signals:
    void signal_autoTrack(int angle, int range);

private slots:
    void trigger_autoTrack(int angle, int range);
    */

private:
    RadarEngine *m_re;

    QString m_log_name;
//    QPolygonF m_polygon;
//    QPolygon m_arpa_polygon;
    QMutex mutex;
    bool m_last_in_guard_zone;
    int m_last_angle;
    int m_bogey_count;    // complete cycle
    int m_running_count;  // current swipe
    GZType m_type;
    int m_start_bearing;
    int m_end_bearing;
    int m_inner_range;  // start in meters
    int m_outer_range;  // end   in meters
//    int zero_angle_count;
    float m_range_start;
    float m_range_end;
    int m_current_range;
//    void UpdateSettings();
//    void countBogey(const int angle, const double heading, const quint8 *data, const int range, const QPolygonF polygon);

};
} // namespace RadarEngine

#endif // GUARDZONE_H
