#ifndef RADARENGINE_H
#define RADARENGINE_H

#include "shared/constants.h"
#include "guardzone.h"
#include "stream/radarreceive.h"
#include "stream/radartransmit.h"
#include "radardraw.h"
#include "arpa/radararpa.h"

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

    static RadarEngine* GetInstance(QObject* parent = nullptr);

    void TriggerReqTx();
    void TriggerReqStby();
    void TriggerReqRangeChange(int range);
    void TriggerReqControlChange(int ct,int val);
    void TriggerReqRadarSetting();
    void TriggerClearTrail();
    void TriggerStopRadar();

    struct line_history
    {
      quint8 line[RETURNS_PER_LINE];
      quint64 time;
      double lat;
      double lon;
    };

    line_history history[LINES_PER_ROTATION];
    BlobColour colourMap[UINT8_MAX + 1];
    QColor colourMapRGB[BLOB_COLOURS];

    RadarDraw *radarDraw;
    RadarArpa *radarArpa;
    QMap<QString,GuardZone *> guardZones;

signals:
    void SignalUpdateReport();
    void SignalPlotRadarSpoke(int angle, UINT8* data, size_t len);
    void SignalRange_change(int range);
    void SignalStayAlive();

protected:
    RadarEngine(QObject *parent=nullptr);
    ~RadarEngine() override;

private slots:
    void onRadarConfigChange(QString key, QVariant val);
    void receiveThreadReport(quint8 report_type, quint8 report_field, quint32 value);
    void radarReceiveProcessRadarSpoke(int, QByteArray, int);
    void timerTimeout();

private:
    static RadarEngine* m_instance;

    struct TrailBuffer
    {
        TrailRevolutionsAge relative_trails[LINES_PER_ROTATION][RETURNS_PER_LINE];
        TrailRevolutionsAge copy_of_relative_trails[LINES_PER_ROTATION][RETURNS_PER_LINE];

        double lat;
        double lon;
    };
    TrailBuffer m_trails;
    BlobColour m_trail_colour[TRAIL_MAX_REVOLUTIONS + 1];

    RadarReceive *m_radar_receive;
    RadarTransmit *m_radar_transmit;

    QTimer *timer;

    quint64 radar_timeout;
    quint64 data_timeout;
    quint64 stay_alive_timeout;

    uint m_old_range;

    RadarState m_cur_radar_state;
    bool m_old_draw_trails;
    int m_old_trail;
    int m_old_preset;

    void computeColourMap();
    void resetSpokes();
    void zoomTrails(float zoom_factor);
    void clearTrails();
    void computeTargetTrails();
    void checkRange(uint new_range);
    void setupGZ();
};

}

#endif // RADARENGINE_H
