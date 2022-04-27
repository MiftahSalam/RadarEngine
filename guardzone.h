#ifndef GUARDZONE_H
#define GUARDZONE_H

#include <QObject>
#include <QMutex>

#include <radarconfig_global.h>

namespace RadarEngine {

typedef enum GZType { GZ_ARC, GZ_CIRCLE } GZType;
class RadarEngine;

class GuardZone : public QObject
{
    Q_OBJECT
public:
    GuardZone(QObject* parent, RadarEngine* re);

    quint64 arpa_update_time[LINES_PER_ROTATION];

    virtual ~GuardZone() override {}

    void ResetBogeys();
    void ProcessSpoke(int angle, UINT8 *data);
//    void autoTrack();

    int GetBogeyCount();

signals:
//    void signal_autoTrack(int angle, int range);

private slots:
//    void trigger_autoTrack(int angle, int range);
    void trigger_configChange(const QString key, const QVariant val);


private:
    RadarEngine *m_re;

    QString m_log_name;
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
    float m_range_start;
    float m_range_end;
    int m_current_range;
    bool m_show;
};
} // namespace RadarEngine

#endif // GUARDZONE_H
