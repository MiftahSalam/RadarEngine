#include "shared/utils.h"
#include "shared/constants.h"

using namespace RadarEngine;

Position Polar2Pos(Polar pol, Position own_ship, double range)
{
    // The "own_ship" in the fumction call can be the position at an earlier time than the current position
    // converts in a radar image angular data r ( 0 - 512) and angle (0 - 2096) to position (lat, lon)
    // based on the own ship position own_ship
    Position pos;
    pos.lat = own_ship.lat +
            static_cast<double>(pol.r) / static_cast<double>(RETURNS_PER_LINE) * range * cos(deg2rad(SCALE_RAW_TO_DEGREES2048(pol.angle))) / 60. / 1852.;
    pos.lon = own_ship.lon +
            static_cast<double>(pol.r) / static_cast<double>(RETURNS_PER_LINE) * range * sin(deg2rad(SCALE_RAW_TO_DEGREES2048(pol.angle))) /
            cos(deg2rad(own_ship.lat)) / 60. / 1852.;
    return pos;
}

Polar Pos2Polar(Position p, Position own_ship, int range)
{
    // converts in a radar image a lat-lon position to angular data
    Polar pol;
    double dif_lat = p.lat;
    dif_lat -= own_ship.lat;
    double dif_lon = (p.lon - own_ship.lon) * cos(deg2rad(own_ship.lat));
    pol.r = static_cast<int>(sqrt(dif_lat * dif_lat + dif_lon * dif_lon) * 60. * 1852. * static_cast<double>(RETURNS_PER_LINE) / static_cast<double>(range) + 1);
    pol.angle = static_cast<int>((atan2(dif_lon, dif_lat)) * static_cast<double>(LINES_PER_ROTATION) / (2. * M_PI) + 1);  // + 1 to minimize rounding errors
    if (pol.angle < 0) pol.angle += LINES_PER_ROTATION;
    return pol;
}
