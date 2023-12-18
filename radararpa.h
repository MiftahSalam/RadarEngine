#ifndef RADARARPA_H
#define RADARARPA_H

#include <QObject>

//#include "RadarEngine/shared/utils.h"
#include "constants.h"
#include "arpatarget.h"

namespace RadarEngine {

class RadarEngine;

class RadarArpa : public QObject
{
    Q_OBJECT
public:
    RadarArpa(RadarArpa& other) = delete;
    void operator=(const RadarArpa&) = delete;

    static RadarArpa* getInstance(QObject* parent = nullptr, RadarEngine *engine = nullptr);

    int targetNumber;
    ARPATarget *targets[MAX_NUMBER_OF_TARGETS];

    bool MultiPix(int ang, int rad);
    void AcquireNewMARPATarget(Position p);
    int AcquireNewARPATarget(Polar pol, int status);
    void RefreshArpaTargets();
    void DeleteTarget(Position p);
    void DeleteAllTargets();
    void RadarLost()
    {
        DeleteAllTargets();
    }

signals:
    void Signal_LostTarget(int id);

protected:
    RadarArpa(QObject *parent = nullptr, RadarEngine *ri=nullptr);
    ~RadarArpa(){}

private:
    static RadarArpa* instance;
    RadarEngine *m_ri;

    bool pix(int ang, int rad);
    void acquireOrDeleteMarpaTarget(Position target_pos, int status);
};

}
#endif // RADARARPA_H
