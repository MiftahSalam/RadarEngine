#ifndef RADARIMAGECAPTURE_H
#define RADARIMAGECAPTURE_H

#include "qobject.h"
#include "qobjectdefs.h"
#include "qvariant.h"
#include <QString>

namespace RadarEngine {
class RadarEngine;

class RadarImageCapture: public QObject
{
    Q_OBJECT
public:
    RadarImageCapture(QObject *parent, RadarEngine *re);

    void capture(int width, int height);
    void start();
    void stop();
    void update();
    bool pendingGrabAvailable() const;
    bool isStart() const;

signals:
    void signalSendEcho(const QString echo, const int vp_width, const int vp_height);

private slots:
    void trigger_radarConfigChange(QString key, QVariant val);

private:
    static RadarEngine *m_re;

    void stateChange(int state);

    int currentAngle;
    bool grabStart;
    bool grabPending;

};

}

#endif // RADARIMAGECAPTURE_H
