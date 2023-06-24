#ifndef UTILS_H
#define UTILS_H

#include "shared/global.h"

namespace RadarEngine {
class Polar {
 public:
  int angle;
  int r;
  quint64 time;  // wxGetUTCTimeMillis
};

class Position
{
public:
    double lat;
    double lon;
    double dlat_dt;   // m / sec
    double dlon_dt;   // m / sec
    quint64 time;  // millis
    double speed_kn;
    double sd_speed_kn;  // standard deviation of the speed in knots
};
}

RadarEngine::Position Polar2Pos(RadarEngine::Polar pol, RadarEngine::Position own_ship, double range);
RadarEngine::Polar Pos2Polar(RadarEngine::Position p, RadarEngine::Position own_ship, int range);

#endif // UTILS_H
