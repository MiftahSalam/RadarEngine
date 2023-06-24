#ifndef RADARTRANSMIT_H
#define RADARTRANSMIT_H

#include <QObject>
#include <QUdpSocket>

#include "shared/constants.h"
#include "shared/global.h"

namespace RadarEngine {

class RadarEngine;

class RadarTransmit : public QObject
{
    Q_OBJECT
public:
    RadarTransmit(RadarTransmit& other) = delete;
    void operator=(const RadarTransmit&) = delete;

    static RadarTransmit* getInstance(QObject* parent = nullptr, RadarEngine *engine = nullptr);

    void setControlValue(ControlType controlType, int value);
    void setMulticastData(QString addr,uint port);
    void setRange(int meters);

    QUdpSocket socket;

signals:

public slots:
    void RadarTx();
    void RadarStby();
    void RadarStayAlive();

protected:
    RadarTransmit(QObject *parent = 0, RadarEngine *engine = nullptr);
    ~RadarTransmit();

private:
    static RadarTransmit* instance;

    QString _data;
    uint _data_port;
    RadarEngine *m_re;
};

}

#endif // RADARTRANSMIT_H
