#include "arpatarget.h"
#include "radarconfig.h"
#include "radarengine.h"

#include <qmath.h>

static int target_id_count = 0;

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

ARPATarget::ARPATarget(QObject *parent, RadarEngine *re) :
    QObject(parent),m_ri(re)
{
    qDebug()<<Q_FUNC_INFO;
    m_kalman = nullptr;
    m_status = LOST;
    m_contour_length = 0;
    m_lost_count = 0;
    targetId = 0;
    m_refresh = 0;
    m_time_future = 0;
    m_automatic = false;
    speedKts = 0.;
    course = 0.;
    stationary = 0;
    position.dlat_dt = 0.;
    position.dlon_dt = 0.;
    m_speeds.nr = 0;
    m_pass1_result = UNKNOWN;
    m_pass_nr = PASS1;
}

ARPATarget::~ARPATarget()
{
    if (m_kalman)
    {
        delete m_kalman;
        m_kalman = nullptr;
    }
}

bool ARPATarget::pix(int ang, int rad)
{
    if (rad <= 1 || rad >= RETURNS_PER_LINE - 1) //  avoid range ring
        return false;

    if (m_check_for_duplicate)        // check bit 1
        return ((m_ri->history[MOD_ROTATION2048(ang)].line[rad] & 64) != 0);
    else        // check bit 0
        return ((m_ri->history[MOD_ROTATION2048(ang)].line[rad] & 128) != 0);

}

bool ARPATarget::findContourFromInside(Polar* pol)
{  // moves pol to contour of blob
    // true if success
    // false when failed
    //    qDebug()<<Q_FUNC_INFO;
    int ang = pol->angle;
    int rad = pol->r;
    if (rad >= RETURNS_PER_LINE - 1 || rad < 3)
        return false;

    if (!(pix(ang, rad)))
        return false;

    while (pix(ang, rad))
        ang--;

    ang++;
    pol->angle = ang;
    // check if the blob has the required min contour length
    if (multiPix(ang, rad))
        return true;
    else
        return false;

}

bool ARPATarget::multiPix(int ang, int rad)
{  // checks the blob has a contour of at least length pixels
    // pol must start on the contour of the blob
    // false if not
    // if false clears out pixels of th blob in hist
    int length = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_ARPA_PARAMS_MIN_CONTOUR_LEN).toInt();
    Polar start;
    start.angle = ang;
    start.r = rad;
    if (!pix(start.angle, start.r))
        return false;

    Polar current = start;  // the 4 possible translations to move from a point on the contour to the next
    Polar max_angle;
    Polar min_angle;
    Polar max_r;
    Polar min_r;
    Polar transl[4];  //   = { 0, 1,   1, 0,   0, -1,   -1, 0 };
    transl[0].angle = 0;
    transl[0].r = 1;
    transl[1].angle = 1;
    transl[1].r = 0;
    transl[2].angle = 0;
    transl[2].r = -1;
    transl[3].angle = -1;
    transl[3].r = 0;
    int count = 0;
    int aa = 0;
    int rr = 0;
    bool succes = false;
    int index = 0;
    max_r = current;
    max_angle = current;
    min_r = current;
    min_angle = current;  // check if p inside blob
    if (start.r >= RETURNS_PER_LINE - 1)
    {
        qDebug()<<Q_FUNC_INFO<<"r too large";
        return false;  //  r too large
    }
    if (start.r < 3)
    {
        qDebug()<<Q_FUNC_INFO<<"r too small";
        return false;  //  r too small
    }
    // first find the orientation of border point p
    for (int i = 0; i < 4; i++)
    {
        index = i;
        aa = current.angle + transl[index].angle;
        rr = current.r + transl[index].r;
        succes = !pix(aa, rr);
        if (succes) break;
    }
    if (!succes)
    {
        qDebug()<<Q_FUNC_INFO<<" Error starting point not on contour";
        return false;
    }
    index += 1;  // determines starting direction
    if (index > 3) index -= 4;
    while (current.r != start.r || current.angle != start.angle || count == 0)
    {  // try all translations to find the next point  // start with the "left most" translation relative to the
        // previous one
        index += 3;         // we will turn left all the time if possible
        for (int i = 0; i < 4; i++)
        {
            if (index > 3) index -= 4;
            aa = current.angle + transl[index].angle;
            rr = current.r + transl[index].r;
            succes = pix(aa, rr);
            if (succes)  // next point found
                break;

            index += 1;
        }
        if (!succes)
        {
            qDebug()<<Q_FUNC_INFO<<" no next point found count="<<count;
            return false;  // return code 7, no next point found
        }                // next point found
        current.angle = aa;
        current.r = rr;
        if (count >= length)
            return true;

        count++;
        if (current.angle > max_angle.angle)
            max_angle = current;

        if (current.angle < min_angle.angle)
            min_angle = current;

        if (current.r > max_r.r)
            max_r = current;

        if (current.r < min_r.r)
            min_r = current;

    }  // contour length is less than m_min_contour_length
    // before returning false erase this blob so we do not have to check this one again
    qDebug()<<Q_FUNC_INFO<<" contour length less than minimal ="<<length;

    if (min_angle.angle < 0)
    {
        min_angle.angle += LINES_PER_ROTATION;
        max_angle.angle += LINES_PER_ROTATION;
    }
    for (int a = min_angle.angle; a <= max_angle.angle; a++)
    {
        for (int r = min_r.r; r <= max_r.r; r++)
            m_ri->history[MOD_ROTATION2048(a)].line[r] &= 63;

    }
    return false;
}

void ARPATarget::SetStatusLost()
{
    qDebug()<<Q_FUNC_INFO<<"id"<<targetId;

    int cur_target_id = targetId;
    m_contour_length = 0;
    m_lost_count = 0;
    if (m_kalman)         // reset kalman filter, don't delete it, too  expensive
        m_kalman->ResetFilter();

    if (m_status >= STATUS_TO_OCPN)
    {
        Polar p;
        p.angle = 0;
        p.r = 0;
        //    PassARPAtoOCPN(&p, L);
    }

    m_status = LOST;
    targetId = 0;
    m_automatic = false;
    m_time_future = 0;
    m_refresh = 0;
    speedKts = 0.;
    course = 0.;
    stationary = 0;
    position.dlat_dt = 0.;
    position.dlon_dt = 0.;
    m_speeds.nr = 0;
    m_pass_nr = PASS1;

    emit Signal_LostTarget(cur_target_id);
}

#define PIX(aa, rr)       \
    if (rr > 510) continue; \
    if (multiPix(aa, rr)) { \
    pol->angle = aa;      \
    pol->r = rr;          \
    return true;          \
    }

bool ARPATarget::findNearestContour(Polar* pol, int dist)
{
    // make a search pattern along a square
    // returns the position of the nearest blob found in pol
    // dist is search radius (1 more or less)
    //    qDebug()<<Q_FUNC_INFO;

    int a = pol->angle;
    int r = pol->r;
    if (dist < 2) dist = 2;
    for (int j = 1; j <= dist; j++)
    {
        int dist_r = j;
        int dist_a = static_cast<int>((326. / static_cast<double>(r) * j));  // 326/r: conversion factor to make squares
        //        qDebug()<<Q_FUNC_INFO<<"dist_a "<<dist_a;
        if (dist_a == 0) dist_a = 1;
        for (int i = 0; i <= dist_a; i++)
        {  // "upper" side
            PIX(a - i, r + dist_r)           // search starting from the middle
            PIX(a + i, r + dist_r)
        }
        for (int i = 0; i < dist_r; i++)
        {  // "right hand" side
            PIX(a + dist_a, r + i)
            PIX(a + dist_a, r - i)
        }
        for (int i = 0; i <= dist_a; i++)
        {  // "lower" side
            PIX(a + i, r - dist_r)
            PIX(a - i, r - dist_r)
        }
        for (int i = 0; i < dist_r; i++)
        {  // "left hand" side
            PIX(a - dist_a, r + i)
            PIX(a - dist_a, r - i)
        }
    }
    return false;
}

bool ARPATarget::getTarget(Polar* pol, int dist1)
{
    // general target refresh
    bool contour_found = false;
    int dist = dist1;
    if (m_status == ACQUIRE0 || m_status == ACQUIRE1)
        dist *= 2;

    if (dist > pol->r - 5) dist = pol->r - 5;  // don't search close to origin
    int a = pol->angle;
    int r = pol->r;

    if (pix(a, r))
        contour_found = findContourFromInside(pol);
    else
        contour_found = findNearestContour(pol, dist);

    if (!contour_found)
        return false;

    int cont = getContour(pol);
    if (cont != 0)
    {
        // reset pol
        pol->angle = a;
        pol->r = r;
        return false;
    }
    return true;
}

void ARPATarget::resetPixels()
{
    //    qDebug()<<Q_FUNC_INFO;
    // resets the pixels of the current blob (plus a little margin) so that blob will no be found again in the same sweep
    for (int r = minR.r - DISTANCE_BETWEEN_TARGETS; r <= maxR.r + DISTANCE_BETWEEN_TARGETS; r++)
    {
        if (r >= LINES_PER_ROTATION || r < 0) continue;
        for (int a = minAngle.angle - DISTANCE_BETWEEN_TARGETS; a <= maxAngle.angle + DISTANCE_BETWEEN_TARGETS; a++)
            m_ri->history[MOD_ROTATION2048(a)].line[r] = m_ri->history[MOD_ROTATION2048(a)].line[r] & 127;
    }
}

QPointF ARPATarget::BlobPixelPosition()
{
    double m_range = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_LAST_SCALE).toDouble();
//    const quint8 unit = static_cast<quint8>(RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_UNIT).toUInt());

//    switch (unit) {
//    case 1:
//        m_range *= KM_TO_NM;
//        break;
//    default:
//        break;
//    }

    const double currentOwnShipLon = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_NAV_DATA_LAST_LONGITUDE).toDouble();
    const double currentOwnShipLat = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_NAV_DATA_LAST_LATITUDE).toDouble();
    double y_max = currentOwnShipLat +
            static_cast<double>(maxR.r) / static_cast<double>RETURNS_PER_LINE * m_range * cos(deg2rad(SCALE_RAW_TO_DEGREES2048(maxAngle.angle))) / 60. / 1852.;
    double x_max = currentOwnShipLon +
            static_cast<double>(maxR.r) / static_cast<double>RETURNS_PER_LINE * m_range * sin(deg2rad(SCALE_RAW_TO_DEGREES2048(maxAngle.angle))) /
            cos(deg2rad(currentOwnShipLat)) / 60. / 1852.;

    double y_min = currentOwnShipLat +
            static_cast<double>(minR.r) / static_cast<double>RETURNS_PER_LINE * m_range * cos(deg2rad(SCALE_RAW_TO_DEGREES2048(minAngle.angle))) / 60. / 1852.;
    double x_min = currentOwnShipLon +
            static_cast<double>(minR.r) / static_cast<double>RETURNS_PER_LINE * m_range * sin(deg2rad(SCALE_RAW_TO_DEGREES2048(minAngle.angle))) /
            cos(deg2rad(currentOwnShipLat)) / 60. / 1852.;
    double x_avg = (x_min+x_max)/2;
    double y_avg = (y_max+y_min)/2;

    qDebug()<<Q_FUNC_INFO<<"m_range"<<m_range<<"id"<<targetId<<"maxR"<<maxR.r<<"minR"<<minR.r;

    return QPointF(x_avg,y_avg);
}

void ARPATarget::RefreshTarget(int dist)
{
    const double m_range = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_LAST_SCALE).toDouble();
    Position prev_X;
    Position prev2_X;
    Position own_pos;
    Polar pol;
    double delta_t;
    LocalPosition x_local;
    quint64 prev_refresh = m_refresh;

    // refresh may be called from guard directly, better check
    if (m_status == LOST)
        return;

    const double currentOwnShipLon = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_NAV_DATA_LAST_LONGITUDE).toDouble();
    const double currentOwnShipLat = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_NAV_DATA_LAST_LATITUDE).toDouble();

    own_pos.lat = currentOwnShipLat;
    own_pos.lon = currentOwnShipLon;

    pol = Pos2Polar(position, own_pos, static_cast<int>(m_range));
    //    qDebug()<<"old pol "<<pol.angle<<pol.r;
    //    qDebug()<<"old pos "<<currentOwnShipLat<<currentOwnShipLon<<m_position.lat<<m_position.lon<<m_range;

    quint64 time1 = m_ri->history[MOD_ROTATION2048(pol.angle)].time;
    int margin = SCAN_MARGIN;
    if (m_pass_nr == PASS2)
    {
        margin += 100;
//        qDebug()<<Q_FUNC_INFO<<"try pass2";
    }
    quint64 time2 = m_ri->history[MOD_ROTATION2048(pol.angle + margin)].time;
    if ((time1 < (m_refresh + SCAN_MARGIN2) || time2 < time1) && m_status != 0)
    {
        quint64 now = static_cast<quint64>(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());  // millis
        quint64 diff = now - m_refresh;
        if (diff > 28000)
//            if (diff > 40000)
        {
            qDebug()<<"target not refreshed, missing spokes, set lost, status="<<m_status<<", target_id="<<targetId<<" timediff="<<diff;
            SetStatusLost();
        }
        else if ((diff < 28000) && (diff > 4000))
//            else if ((diff < 40000) && (diff > 20000))
        {
            /*arpa future pos prediction
            */
            if(m_status > 1)
            {
                if(futureFirst)
                {
                    m_time_future = now;
                    maxAngleFuture = maxAngle;
                    minAngleFuture = minAngle;
                    maxRFuture = maxR;
                    minRFuture = minR;
                    futureFirst = false;
                }
                else
                {
                    double dt = static_cast<double>(now - m_time_future)/1000.;
                    if(dt > 1)
                    {
                        m_time_future = now;
                        int buf_deg = MOD_ROTATION2048(maxAngleFuture.angle);
                        buf_deg = static_cast<int>SCALE_RAW_TO_DEGREES2048(buf_deg);
                        int max_r_xA = maxRFuture.r*static_cast<int>(qSin(deg2rad(buf_deg)));
                        int max_r_yA = maxRFuture.r*static_cast<int>(qCos(deg2rad(buf_deg)));
                        double dx = 8*(position.dlon_dt*dt*RETURNS_PER_LINE)/m_range;
                        double dy = 9*(position.dlat_dt*dt*RETURNS_PER_LINE)/m_range;
                        int max_r_xB = max_r_xA+static_cast<int>(dx);
                        int max_r_yB = max_r_yA+static_cast<int>(dy);

                        buf_deg = MOD_ROTATION2048(minAngleFuture.angle);
                        buf_deg = static_cast<int>SCALE_RAW_TO_DEGREES2048(buf_deg);
                        int min_r_xA = minRFuture.r*static_cast<int>(qSin(deg2rad(buf_deg)));
                        int min_r_yA = minRFuture.r*static_cast<int>(qCos(deg2rad(buf_deg)));
                        int min_r_xB = min_r_xA+static_cast<int>(dx);
                        int min_r_yB = min_r_yA+static_cast<int>(dy);

                        maxRFuture.r = static_cast<int>(sqrt(pow(max_r_xB,2)+pow(max_r_yB,2)));
                        maxAngleFuture.angle = static_cast<int>rad2deg(atan2(max_r_xB,max_r_yB));
                        maxAngleFuture.angle = SCALE_DEGREES_TO_RAW2048(maxAngleFuture.angle);
                        maxAngleFuture.angle = MOD_ROTATION2048(maxAngleFuture.angle);

                        minRFuture.r = static_cast<int>(sqrt(pow(min_r_xB,2)+pow(min_r_yB,2)));
                        minAngleFuture.angle = static_cast<int>rad2deg(atan2(min_r_xB,min_r_yB));
                        minAngleFuture.angle = SCALE_DEGREES_TO_RAW2048(minAngleFuture.angle);
                        minAngleFuture.angle = MOD_ROTATION2048(minAngleFuture.angle);

                        /*
                        qDebug()<<"dx"<<dx<<"dy"<<dy<<"dt"<<dt<<"dlat"<<m_position.dlat_dt<<"dlon"
                               <<m_position.dlon_dt<<"m_range"<<m_range<<"speedKts"<<speedKts
                              <<"course"<<course<<"r max futur"<<maxRFuture.r<<"r min futur"<<minRFuture.r;
                        qDebug()<<"dx"<<dx<<"dy"<<dy<<"dt"<<dt<<"speedKts"<<speedKts<<"course"<<course
                               <<"r max futur"<<maxRFuture.r<<"r min futur"<<minRFuture.r
                              <<"max_r_xB"<<max_r_xB<<"max_r_yB"<<max_r_yB<<"min_r_xB"<<min_r_xB<<"min_r_yB"<<min_r_yB
                             <<"max_r_xA"<<max_r_xA<<"max_r_yA"<<max_r_yA<<"min_r_xA"<<min_r_xA<<"min_r_yA"<<min_r_yA
                                ;
                        */

                    }
                }

            }

        }
        else if(diff < 4000)
//            else if(diff < 20000)
        {
            m_time_future = now;
            maxAngleFuture = maxAngle;
            minAngleFuture = minAngle;
            maxRFuture = maxR;
            minRFuture = minR;
            futureFirst = true;
        }

        return;
    }
    // set new refresh time
    m_refresh = time1;
    prev_X = position;  // save the previous target position
    prev2_X = prev_X;

    // PREDICTION CYCLE
    position.time = time1;                                                // estimated new target time
    delta_t = (static_cast<double>((position.time - prev_X.time))) / 1000.;  // in seconds
    //    qDebug()<<Q_FUNC_INFO<<"m_pos time"<<position.time<<"prev time"<<prev_X.time;
    if (m_status == 0)
        delta_t = 0.;

    if (position.lat > 90.)
    {
        qDebug()<<Q_FUNC_INFO<<"lat >90";
        SetStatusLost();
        return;
    }
    x_local.lat = (position.lat - own_pos.lat) * 60. * 1852.;                              // in meters
    x_local.lon = (position.lon - own_pos.lon) * 60. * 1852. * cos(deg2rad(own_pos.lat));  // in meters
    x_local.dlat_dt = position.dlat_dt;                                                    // meters / sec
    x_local.dlon_dt = position.dlon_dt;                                                    // meters / sec
    //    qDebug()<<Q_FUNC_INFO<<"111..x_local.lat"<<x_local.lat<<"x_local.lon"<<x_local.lon;
    m_kalman->Predict(&x_local, delta_t);  // x_local is new estimated local position of the target
    // now set the polar to expected angular position from the expected local position
    pol.angle = static_cast<int>(atan2(x_local.lon, x_local.lat) * LINES_PER_ROTATION / (2. * M_PI));
    if (pol.angle < 0) pol.angle += LINES_PER_ROTATION;
    pol.r =static_cast<int>(sqrt(x_local.lat * x_local.lat + x_local.lon * x_local.lon) * static_cast<double>RETURNS_PER_LINE / m_range);
    // zooming and target movement may  cause r to be out of bounds
    if (pol.r >= RETURNS_PER_LINE || pol.r <= 0)
    {
        qDebug()<<Q_FUNC_INFO<<" r out of area"<<pol.r;
        SetStatusLost();
        return;
    }
    m_expected = pol;  // save expected polar position

    // Measurement cycle
    int dist1 = dist;
    Polar back = pol;
    if (getTarget(&pol, dist1))
    {
        const int max_target_size = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_ARPA_PARAMS_MAX_TARGET_SIZE).toInt();
        resetPixels();
        // target too large? (land masses?) get rid of it
        if (abs(back.r - pol.r) > max_target_size || abs(maxR.r - minR.r) > max_target_size ||
                abs(minAngle.angle - maxAngle.angle) > max_target_size)
        {
            qDebug()<<Q_FUNC_INFO<<"target too large? (land masses?) get rid of it";
            SetStatusLost();
            return;
        }

        // target refreshed, measured position in pol
        // check if target has a new later time than previous target
        if (pol.time <= prev_X.time && m_status > 1)
        {
            // found old target again, reset what we have done
            qDebug()<<Q_FUNC_INFO<<" Error Gettarget same time found";
            position = prev_X;
            prev_X = prev2_X;
            return;
        }

        m_lost_count = 0;
        if (m_status == ACQUIRE0)
        {
            // as this is the first measurement, move target to measured position
            Position p_own;
            p_own.lat = m_ri->history[MOD_ROTATION2048(pol.angle)].lat;  // get the position at receive time
            p_own.lon = m_ri->history[MOD_ROTATION2048(pol.angle)].lon;
            position = Polar2Pos(pol, p_own, m_range);  // using own ship location from the time of reception
            position.dlat_dt = 0.;
            position.dlon_dt = 0.;
            m_expected = pol;
            position.sd_speed_kn = 0.;
        }

        m_status++;
        qDebug()<<Q_FUNC_INFO<<"track status"<<m_status;
        if(m_status > 10)
            m_status = 10;
        // target gets an id when status  == STATUS_TO_OCPN
        if (m_status == STATUS_TO_OCPN)
        {
            target_id_count++;
            if (target_id_count >= 10000) target_id_count = 1;
            targetId = target_id_count;
        }

        // Kalman filter to  calculate the apostriori local position and speed based on found position (pol)
        if (m_status > 1)
        {
            m_kalman->Update_P();
            m_kalman->SetMeasurement(&pol, &x_local, &m_expected, static_cast<int>(m_range));  // pol is measured position in polar coordinates
        }

        // x_local expected position in local coordinates

        position.time = pol.time;  // set the target time to the newly found time
    }                              // end of target found
    // target not found
    else
    {
        // target not found
        if (m_pass_nr == PASS1) m_kalman->Update_P();
        // check if the position of the target has been taken by another target, a duplicate
        // if duplicate, handle target as not found but don't do pass 2 (= search in the surroundings)
        bool duplicate = false;
        m_check_for_duplicate = true;
        qDebug()<<Q_FUNC_INFO<<"m_check_for_duplicate"<<m_check_for_duplicate;
        if (m_pass_nr == PASS1 && getTarget(&pol, dist1))
        {
            m_pass1_result = UNKNOWN;
            duplicate = true;
            qDebug()<<Q_FUNC_INFO<<"found duplicate";
        }
        m_check_for_duplicate = false;

        // not found in pass 1
        // try again later in pass 2 with a larger distance
        if (m_pass_nr == PASS1 && !duplicate)
        {
            qDebug()<<Q_FUNC_INFO<<"NOT_FOUND_IN_PASS1";
            m_pass1_result = NOT_FOUND_IN_PASS1;
            // reset what we have done
            pol.time = prev_X.time;
            m_refresh = prev_refresh;
            position = prev_X;
            prev_X = prev2_X;
            return;
        }
        else if (m_pass_nr == PASS2 && !duplicate)
            qDebug()<<Q_FUNC_INFO<<"NOT_FOUND_IN_PASS2";


        // delete low status targets immediately when not found
        if (m_status == ACQUIRE0 || m_status == ACQUIRE1 || m_status == 2)
        {
            SetStatusLost();
            return;
        }

        m_lost_count++;

        // delete if not found too often
        qDebug()<<Q_FUNC_INFO<<"id"<<targetId<<"m_lost_count"<<m_lost_count;
        if (m_lost_count > MAX_LOST_COUNT)
        {
            qDebug()<<Q_FUNC_INFO<<"not found often";
            SetStatusLost();
            return;
        }
    }  // end of target not found
    // set pass1_result ready for next sweep
    m_pass1_result = UNKNOWN;
    if (m_status != ACQUIRE1)
    {
        // if status == 1, then this was first measurement, keep position at measured position
        position.lat = own_pos.lat + x_local.lat / 60. / 1852.;
        position.lon = own_pos.lon + x_local.lon / 60. / 1852. / cos(deg2rad(own_pos.lat));
        position.dlat_dt = x_local.dlat_dt;  // meters / sec
        position.dlon_dt = x_local.dlon_dt;  // meters /sec
        position.sd_speed_kn = x_local.sd_speed_m_s * 3600. / 1852.;

//        double dif_lat = position.lat*M_PI/180.;
//        double dif_lon = ((position.lon*M_PI/180.)-(currentOwnShipLon*M_PI/180.))*cos(((currentOwnShipLat+position.lat)/2.)*M_PI/180.);
//        double R = 6371.;

//        dif_lat =  dif_lat - (currentOwnShipLat*M_PI/180.);

//        position.rng = sqrt(dif_lat * dif_lat + dif_lon * dif_lon)*R;
//        position.rng *= 1.5;
//        qreal bearing = atan2(dif_lon,dif_lat)*180./M_PI;

//        while(bearing < 0.0)
//        {
//            bearing += 360.0;
//        }

//        position.brn = bearing;

    }

    // set refresh time to the time of the spoke where the target was found
    m_refresh = position.time;
    if (m_status >= 1)
    {
        if (m_status == 2)
        {
            // avoid extreme start-up speeds
            if (position.dlat_dt > START_UP_SPEED) position.dlat_dt = START_UP_SPEED;
            if (position.dlat_dt < -START_UP_SPEED) position.dlat_dt = -START_UP_SPEED;
            if (position.dlon_dt > START_UP_SPEED) position.dlon_dt = START_UP_SPEED;
            if (position.dlon_dt < -START_UP_SPEED) position.dlon_dt = -START_UP_SPEED;
        }
        if (m_status == 3)
        {
            // avoid extreme start-up speeds
            if (position.dlat_dt > 2 * START_UP_SPEED) position.dlat_dt = 2 * START_UP_SPEED;
            if (position.dlat_dt < -2 * START_UP_SPEED) position.dlat_dt = -2 * START_UP_SPEED;
            if (position.dlon_dt > 2 * START_UP_SPEED) position.dlon_dt = 2 * START_UP_SPEED;
            if (position.dlon_dt < -2 * START_UP_SPEED) position.dlon_dt = -2 * START_UP_SPEED;
        }
        double s1 = position.dlat_dt;                          // m per second
        double s2 = position.dlon_dt;                          // m  per second
        speedKts = (sqrt(s1 * s1 + s2 * s2)) * 3600. / 1852.;  // and convert to nautical miles per hour
        course = rad2deg(atan2(s2, s1));
        if (course < 0) course += 360.;
        if (speedKts > 20.)
            pol = Pos2Polar(position, own_pos, static_cast<int>(m_range));


        if (speedKts < static_cast<double>(TARGET_SPEED_DIV_SDEV) * position.sd_speed_kn)
        {
            speedKts = 0.;
            course = 0.;
            if (stationary < 2)
                stationary++;
        }
        else if (stationary > 0)
            stationary--;

    }
    return;
}

int ARPATarget::getContour(Polar* pol)
{  // sets the measured_pos if succesfull
    // pol must start on the contour of the blob
    // follows the contour in a clockwise direction
    // returns metric position of the blob in Z
    // the 4 possible translations to move from a point on the contour to the next
    Polar transl[4];  //   = { 0, 1,   1, 0,   0, -1,   -1, 0 };
    transl[0].angle = 0;
    transl[0].r = 1;

    transl[1].angle = 1;
    transl[1].r = 0;

    transl[2].angle = 0;
    transl[2].r = -1;

    transl[3].angle = -1;
    transl[3].r = 0;

    int const MAX_BREAK_FIND_COUNTOUR_ANGLE = 3*LINES_PER_ROTATION; //tambahan

    int count = 0;
    Polar start = *pol;
    Polar current = *pol;
    int aa = 0;
    int rr = 0;

    bool succes = false;
    int index = 0;
    maxR = current;
    maxAngle = current;
    minR = current;
    minAngle = current;
    // check if p inside blob
    if (start.r >= RETURNS_PER_LINE - 1)
    {
        qDebug()<<Q_FUNC_INFO<<"return code 1, r too large";
        return 1;  // return code 1, r too large
    }
    if (start.r < 4)
    {
        qDebug()<<Q_FUNC_INFO<<"return code 2, r too small";
        return 2;  // return code 2, r too small
    }
    if (!pix(start.angle, start.r))
    {
        qDebug()<<Q_FUNC_INFO<<"return code 3, starting point outside blob";
        return 3;  // return code 3, starting point outside blob
    }
    // first find the orientation of border point p
    for (int i = 0; i < 4; i++)
    {
        index = i;
        aa = current.angle + transl[index].angle;
        rr = current.r + transl[index].r;
        //  if (rr > 511) return 13;  // r too large
        succes = !pix(aa, rr);
        if (succes) break;
    }
    if (!succes)
    {
        qDebug()<<Q_FUNC_INFO<<"return code 4, starting point not on contour";
        return 4;  // return code 4, starting point not on contour
    }
    index += 1;  // determines starting direction
    if (index > 3) index -= 4;

    while (current.r != start.r || current.angle != start.angle || count == 0)
    {
        // try all translations to find the next point
        // start with the "left most" translation relative to the previous one
        index += 3;  // we will turn left all the time if possible
        for (int i = 0; i < 4; i++)
        {
            if (index > 3) index -= 4;
            aa = current.angle + transl[index].angle;
            rr = current.r + transl[index].r;
            succes = pix(aa, rr);
            if (succes)                // next point found
                break;

            index += 1;
        }
        if (!succes)
        {
            qDebug()<<Q_FUNC_INFO<<" no next point found count= "<<count;
            return 7;  // return code 7, no next point found
        }
        // next point found
        current.angle = aa;
        current.r = rr;
        if (count < MAX_CONTOUR_LENGTH - 2)
            m_contour[count] = current;

        if (count == MAX_CONTOUR_LENGTH - 2)
            m_contour[count] = start;  // shortcut to the beginning for drawing the contour

        if (count < MAX_CONTOUR_LENGTH - 1)
            count++;

        if (current.angle > maxAngle.angle)
            maxAngle = current;

        if (current.angle < minAngle.angle)
            minAngle = current;

        if (current.r > maxR.r)
            maxR = current;

        if (current.r < minR.r)
            minR = current;

        if(current.angle > MAX_BREAK_FIND_COUNTOUR_ANGLE)
            break;
    }

    m_contour_length = count;
    //  CalculateCentroid(*target);    we better use the real centroid instead of the average, todo
    if (minAngle.angle < 0)
    {
        minAngle.angle += LINES_PER_ROTATION;
        maxAngle.angle += LINES_PER_ROTATION;
    }

    pol->angle = (maxAngle.angle + minAngle.angle) / 2; //av angle of centroid
    if (maxR.r > RETURNS_PER_LINE - 1 || minR.r > RETURNS_PER_LINE - 1)
        return 10;  // return code 10 r too large

    if (maxR.r < 2 || minR.r < 2)
        return 11;  // return code 11 r too small

    if (pol->angle >= LINES_PER_ROTATION)
        pol->angle -= LINES_PER_ROTATION;

    pol->r = (maxR.r + minR.r) / 2; //av radius of centroid
    pol->time = m_ri->history[MOD_ROTATION2048(pol->angle)].time;
    return 0;  //  succes, blob found
}

