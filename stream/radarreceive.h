#ifndef RADARRECEIVE_H
#define RADARRECEIVE_H

#include <QObject>
#include <QThread>
#include <QMutex>

#include "shared/constants.h"

namespace RadarEngine {

class RadarEngine;

class RadarReceive : public QThread
{
    Q_OBJECT
public:
    RadarReceive(RadarReceive& other) = delete;
    void operator=(const RadarReceive&) = delete;

    static RadarReceive* getInstance(QObject* parent = nullptr, RadarEngine *engine = nullptr);

    void ExitReq();
    void SetMulticastData(QString addr,uint port);
    void SetMulticastReport(QString addr,uint port);

signals:
    void ProcessRadarSpoke(int angle_raw, QByteArray data,int dataSize);
    void UpdateReport(quint8 report_type,quint8 report_field,quint32 value);

protected:
    void run();
    RadarReceive(QObject *parent = 0, RadarEngine *engine = nullptr);
    ~RadarReceive();

private:
    static RadarReceive* instance;

    void processFrame(QByteArray data, int len);
    void processReport(QByteArray data, int len);

    bool exit_req;
    QMutex mutex;
    RadarEngine *m_engine;
};

}

#endif // RADARRECEIVE_H
