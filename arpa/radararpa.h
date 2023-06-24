#ifndef RADARARPA_H
#define RADARARPA_H

#include <QObject>

#include "shared/constants.h"
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

    int m_number_of_targets/*,range_meters*/;
    ARPATarget *m_target[MAX_NUMBER_OF_TARGETS];

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
    void signal_LostTarget(int id);

protected:
    RadarArpa(QObject *parent = nullptr, RadarEngine *ri=nullptr);
    ~RadarArpa();

private:
    static RadarArpa* instance;

    bool Pix(int ang, int rad);
    void AcquireOrDeleteMarpaTarget(Position target_pos, int status);
    RadarEngine *m_ri;
};

}
#endif // RADARARPA_H
