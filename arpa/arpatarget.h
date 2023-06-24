#ifndef ARPATARGET_H
#define ARPATARGET_H

#include <QObject>

#include "shared/constants.h"
#include "kalmanfilter.h"

//#define SCAN_MARGIN (750)
#define SCAN_MARGIN (150)
//#define SCAN_MARGIN2 (5000)
#define SCAN_MARGIN2 (1000)

#define MAX_NUMBER_OF_TARGETS (200)
#define MAX_CONTOUR_LENGTH (601)
#define SPEED_HISTORY (8)

#define MAX_LOST_COUNT (3)
#define FOR_DELETION (-2)
#define LOST (-1)
#define ACQUIRE0 (0)
#define ACQUIRE1 (1)
#define ACQUIRE2 (2)
#define ACQUIRE3 (3)

#define Q_NUM (4)
#define T_NUM (6)
#define TARGET_SPEED_DIV_SDEV 2.
#define STATUS_TO_OCPN (5)            // First status to be send track info
#define START_UP_SPEED (0.5)          // maximum allowed speed (m/sec) for new target, real format with .
#define DISTANCE_BETWEEN_TARGETS (4)  // minimum separation between targets. configurable?

namespace RadarEngine {

class RadarEngine;
class RadarArpa;

typedef int target_status;
enum TargetProcessStatus { UNKNOWN, NOT_FOUND_IN_PASS1 };
enum PassN { PASS1, PASS2 };

struct SpeedHistory
{
    double av;
    double hist[SPEED_HISTORY];
    double dif[SPEED_HISTORY];
    double sd;
    int nr;
};

class ARPATarget : public QObject
{
    Q_OBJECT
    friend class RadarArpa;

public:
    explicit ARPATarget(QObject *parent = nullptr, RadarEngine *re = nullptr);
    ~ARPATarget() override;

    void RefreshTarget(int dist);
    void SetStatusLost();
    target_status GetStatus() { return m_status; }
    QPointF BlobPixelPosition();

    int targetId;
    Polar maxAngle, minAngle, maxR, minR;
    Polar maxAngleFuture, minAngleFuture, maxRFuture, minRFuture;
    bool futureFirst;
    double speedKts;
    double course;
    int stationary;
    Position position;

signals:
    void Signal_LostTarget(int id);

private:
    bool getTarget(Polar* pol, int dist1);
    void resetPixels();
    bool findContourFromInside(Polar* pol);
    bool findNearestContour(Polar* pol, int dist);
    bool multiPix(int ang, int rad);
    bool pix(int ang, int rad);
    int getContour(Polar* p);

    KalmanFilter* m_kalman;
    RadarEngine *m_ri;
    target_status m_status;
    TargetProcessStatus m_pass1_result;
    SpeedHistory m_speeds;
    PassN m_pass_nr;
    int m_contour_length;
    quint64 m_refresh;  // time of last refresh
    quint64 m_time_future;
    int m_lost_count;
    bool m_check_for_duplicate;
    Polar m_contour[MAX_CONTOUR_LENGTH + 1];
    Polar m_expected;
    bool m_automatic;
};

}

#endif // ARPATARGET_H
