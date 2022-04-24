#ifndef RADARENGINE_H
#define RADARENGINE_H

#include <radarconfig.h>
#include "radarengine_global.h"

#include "guardzone.h"
#include "radarreceive.h"
#include "radartransmit.h"
#include "radardraw.h"
#include "radararpa.h"

#include <QTimer>

#define STAYALIVE_TIMEOUT (5000)  // Send data every 5 seconds to ping radar
#define DATA_TIMEOUT (10000)
#define SECONDS_TO_REVOLUTIONS(x) ((x)*2 / 5)
#define TRAIL_MAX_REVOLUTIONS SECONDS_TO_REVOLUTIONS(600) + 1 //241

typedef quint8 TrailRevolutionsAge;

namespace RadarEngine {

class RadarArpa;
class RadarReceive;
class GuardZone;

class RadarEngine : public QObject
{
    Q_OBJECT
public:
    RadarEngine(RadarEngine& other) = delete;
    void operator=(const RadarEngine&) = delete;

    static RadarEngine* getInstance(QObject* parent = nullptr);

    void trigger_ReqTx();
    void trigger_ReqStby();
    void trigger_ReqRangeChange(int range);
    void trigger_ReqControlChange(int ct,int val);
    void trigger_ReqRadarSetting();
    void trigger_clearTrail();
    void trigger_stopRadar();

    struct line_history
    {
      quint8 line[RETURNS_PER_LINE];
      quint64 time;
      double lat;
      double lon;
    };

    line_history m_history[LINES_PER_ROTATION];
    BlobColour m_colour_map[UINT8_MAX + 1];
    QColor m_colour_map_rgb[BLOB_COLOURS];

    RadarDraw *radarDraw;
    RadarArpa *radarArpa;
    GuardZone *guardZone;

signals:
    void signal_updateReport();
    void signal_plotRadarSpoke(int angle, UINT8* data, size_t len);
    void signal_range_change(int range);
    void signal_stay_alive();
//    void signal_sendTx();
//    void signal_sendStby();
//    void signal_state_change();

protected:
    RadarEngine(QObject *parent=nullptr);
    ~RadarEngine() override;

private slots:
    void onRadarConfigChange(QString key, QVariant val);
    void receiveThread_Report(quint8 report_type, quint8 report_field, quint32 value);
    void radarReceive_ProcessRadarSpoke(int, QByteArray, int);
    void timerTimeout();

private:
    static RadarEngine* instance;

    struct TrailBuffer
    {
        TrailRevolutionsAge relative_trails[LINES_PER_ROTATION][RETURNS_PER_LINE];
        TrailRevolutionsAge copy_of_relative_trails[LINES_PER_ROTATION][RETURNS_PER_LINE];

        double lat;
        double lon;
    };
    TrailBuffer m_trails;
    BlobColour m_trail_colour[TRAIL_MAX_REVOLUTIONS + 1];

    RadarReceive *radarReceive;
    RadarTransmit *radarTransmit;

    QTimer *timer;

    quint64 radar_timeout;
    quint64 data_timeout;
    quint64 stay_alive_timeout;

//    uint m_range_meters;
    uint m_old_range;

    RadarState cur_radar_state;
    bool old_draw_trails;
    int old_trail;
    int old_preset;

    void ComputeColourMap();
    void ResetSpokes();
    void ZoomTrails(float zoom_factor);
    void ClearTrails();
    void ComputeTargetTrails();
    void checkRange(uint new_range);
};

}

#endif // RADARENGINE_H
