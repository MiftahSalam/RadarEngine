#include "guardzone.h"
#include <radarconfig.h>

#include <qmath.h>

#define SCAN_MARGIN (150)            // number of lines that a next scan of the target may have moved
#define SCAN_MARGIN2 (1000)          // if target is refreshed after this time you will be shure it is the next sweep

using namespace RadarEngine;

GuardZone::GuardZone(QObject *parent, RadarEngine *re): QObject(parent), m_re(re)
{
    for (int angle = 0; angle < LINES_PER_ROTATION; angle++)
    {
        arpaUpdateTime[angle] = 0;
    }

    /*
    const int curRange =  RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_PPI_DISPLAY_LAST_SCALE).toInt();

    m_type = static_cast<GZType>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_MODE).toInt());
    m_show = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_PPI_DISPLAY_SHOW_GZ).toBool();
    m_start_bearing = SCALE_DEGREES_TO_RAW2048(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_START_BEARING).toDouble());
    m_end_bearing = SCALE_DEGREES_TO_RAW2048(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_END_BEARING).toDouble());
    m_inner_range =  RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_START_RANGE).toInt()*RETURNS_PER_LINE/curRange;
    m_outer_range =  RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_END_RANGE).toInt()*RETURNS_PER_LINE/curRange;

    ResetBogeys();
    */
}

void GuardZone::ProcessSpoke(int angle, UINT8* data/*, UINT8* hist, int range*/)
{
    if(!m_show) return;

    //    qDebug()<<Q_FUNC_INFO<<"inner "<<m_inner_range<<"outter "<<m_outer_range;

    double curRange = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_LAST_SCALE).toDouble();
    int range_start = m_inner_range*RETURNS_PER_LINE/curRange;  // Convert from meters to 0..511
    int range_end = m_outer_range*RETURNS_PER_LINE/curRange;  // Convert from meters to 0..511
//    int range_end = m_outer_range;    // Convert from meters to 0..511
    bool in_guard_zone = false;
    const bool is_heading_up =  RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_HEADING_UP).toBool();
    const double hdt = is_heading_up ? RadarConfig::getInstance("")->getConfig(NON_VOLATILE_NAV_DATA_LAST_HEADING).toDouble() : 0.;
    const double orientation = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_ORIENTATION).toDouble();
    const double hdtCorr = hdt + orientation;

    m_start_bearing = SCALE_DEGREES_TO_RAW2048(m_start_bearing_deg-hdtCorr);
    m_end_bearing = SCALE_DEGREES_TO_RAW2048(m_end_bearing_deg-hdtCorr);

    switch (m_type)
    {
    case GZ_ARC:
        if ((angle >= m_start_bearing && angle < m_end_bearing) ||
                (m_start_bearing >= m_end_bearing && (angle >= m_start_bearing || angle < m_end_bearing)))
        {
            if (range_start < RETURNS_PER_LINE)
            {
                if (range_end > RETURNS_PER_LINE)
                {
                    range_end = RETURNS_PER_LINE;
                }
                for (int r = range_start; r <= range_end; r++)
                {
                    if (data[r] >= 50/*m_pi->m_settings.threshold_blue*/)
                    {
                        m_running_count++;
                    }
#ifdef TEST_GUARD_ZONE_LOCATION
                    // Zap guard zone computation location to green so this is visible on screen
                    else {
                        data[r] = m_pi->m_settings.threshold_green;
                    }
#endif
                }
            }
            in_guard_zone = true;
        }
        break;

    case GZ_CIRCLE:
        if (range_start < RETURNS_PER_LINE)
        {
            if (range_end > RETURNS_PER_LINE)
            {
                range_end = RETURNS_PER_LINE;
            }

            for (int r = range_start; r <= range_end; r++)
            {
                if (data[r] >= 50/*m_pi->m_settings.threshold_blue*/)
                {
                    m_running_count++;
                }
#ifdef TEST_GUARD_ZONE_LOCATION
                // Zap guard zone computation location to green so this is visible on screen
                else {
                    data[r] = m_pi->m_settings.threshold_green;
                }
#endif
            }
            if (angle > m_last_angle)
            {
                in_guard_zone = true;
            }
        }
        break;
    }

    if (m_last_in_guard_zone && !in_guard_zone)
    {
        // last bearing that could add to m_running_count, so store as bogey_count;
        m_bogey_count = m_running_count;
        m_running_count = 0;

        qDebug()<<Q_FUNC_INFO<<"inner "<<m_inner_range<<"outter "<<m_outer_range<<"m_last_angle"<<m_last_angle<<"range_start"<<range_start<<"range_end"<<range_end<<"m_bogey_count"<<m_bogey_count;

        // When debugging with a static ship it is hard to find moving targets, so move
        // the guard zone instead. This slowly rotates the guard zone.
        /*
       if (m_pi->m_settings.guard_zone_debug_inc && m_type == GZ_ARC) {
         m_start_bearing += LINES_PER_ROTATION - m_pi->m_settings.guard_zone_debug_inc;
         m_end_bearing += LINES_PER_ROTATION - m_pi->m_settings.guard_zone_debug_inc;
         m_start_bearing %= LINES_PER_ROTATION;
         m_end_bearing %= LINES_PER_ROTATION;
       }
       */
    }

    m_last_in_guard_zone = in_guard_zone;
    m_last_angle = angle;
}
/*
void GuardZone::trigger_configChange(const QString key, const QVariant val)
{
    //    qDebug()<<Q_FUNC_INFO<<"key"<<key<<"val"<<val;
    if(key == NON_VOLATILE_GZ_START_RANGE)
    {
    }
    else if(key == NON_VOLATILE_GZ_START_BEARING)
    {
        const double hdt =  RadarConfig::getInstance("")->getConfig(NON_VOLATILE_NAV_DATA_LAST_HEADING).toDouble();
        m_start_bearing = SCALE_DEGREES_TO_RAW2048(val.toDouble()+hdt);
        ResetBogeys();
    }
    else if(key == NON_VOLATILE_GZ_END_RANGE)
    {
        const int curRange =  RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_LAST_SCALE).toInt();
        m_outer_range = val.toInt()*RETURNS_PER_LINE/curRange;
        ResetBogeys();
    }
    else if(key == NON_VOLATILE_GZ_END_BEARING)
    {
        const double hdt =  RadarConfig::getInstance("")->getConfig(NON_VOLATILE_NAV_DATA_LAST_HEADING).toDouble();
        m_end_bearing = SCALE_DEGREES_TO_RAW2048(val.toDouble()+hdt);
        ResetBogeys();
    }
    else if(key == NON_VOLATILE_GZ_MODE)
    {
        m_type = static_cast<GZType>(val.toInt());
        ResetBogeys();
    }
    else if(key == NON_VOLATILE_PPI_DISPLAY_SHOW_GZ)
    {
        m_show = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_SHOW_GZ).toBool();
        ResetBogeys();
    }
}
*/
void GuardZone::SetInnerRange(const int range)
{
//    double curRange = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_LAST_SCALE).toDouble();
//    const quint8 unit = static_cast<quint8>(RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_UNIT).toUInt());

//    switch (unit) {
//    case 1:
//        curRange *= KM_TO_NM;
//        break;
//    default:
//        break;
//    }

    m_inner_range = range;
//    m_inner_range = range*RETURNS_PER_LINE/curRange;
    ResetBogeys();
}

void GuardZone::SetOutterRange(const int range)
{
//    double curRange = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_LAST_SCALE).toDouble();
//    const quint8 unit = static_cast<quint8>(RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_UNIT).toUInt());

//    switch (unit) {
//    case 1:
//        curRange *= KM_TO_NM;
//        break;
//    default:
//        break;
//    }

    m_outer_range = range;
//    m_outer_range = range*RETURNS_PER_LINE/curRange;
    ResetBogeys();
}

void GuardZone::SetStartBearing(const double deg)
{
//    const double hdt =  RadarConfig::getInstance("")->getConfig(NON_VOLATILE_NAV_DATA_LAST_HEADING).toDouble();
    const double hdt =  0.;
    m_start_bearing_deg = deg;
    m_start_bearing = SCALE_DEGREES_TO_RAW2048(deg+hdt);
    ResetBogeys();
}

void GuardZone::SetEndBearing(const double deg)
{
//    const double hdt =  RadarConfig::getInstance("")->getConfig(NON_VOLATILE_NAV_DATA_LAST_HEADING).toDouble();
    const double hdt =  0.;
    m_end_bearing_deg = deg;
    m_end_bearing = SCALE_DEGREES_TO_RAW2048(deg+hdt);
    ResetBogeys();
}

void GuardZone::SetType(const GZType type)
{
    m_type = type;
    ResetBogeys();
}

void GuardZone::SetShown(const bool show)
{
    m_show = show;
    ResetBogeys();
}

int GuardZone::GetBogeyCount()
{
    return m_bogey_count;
}
void GuardZone::ResetBogeys()
{
    m_bogey_count = -1;
    m_running_count = 0;
    m_last_in_guard_zone = false;
    m_last_angle = 0;
}

/*
void GZ::autoTrack()
{
    if (m_current_range == 0 || !gz_settings.show || state_radar != RADAR_TRANSMIT)
      return;

//    bool stop = false; //tes

    size_t range_start = (size_t)m_range_start*RETURNS_PER_LINE/m_current_range;
    size_t range_end = (size_t)m_range_end*RETURNS_PER_LINE/m_current_range;
    int hdt = SCALE_DEGREES_TO_RAW2048(currentHeading);
    int start_bearing = m_start_bearing+hdt;
    int end_bearing = m_end_bearing+hdt;
    start_bearing = MOD_ROTATION2048(start_bearing);
    end_bearing = MOD_ROTATION2048(end_bearing);
    if (start_bearing > end_bearing)
        end_bearing += LINES_PER_ROTATION;


    qDebug()<<Q_FUNC_INFO<<start_bearing<<end_bearing
           <<range_start<<range_end
             <<GetP2CLookupTable()->intx[start_bearing][range_start]<<GetP2CLookupTable()->inty[start_bearing][range_start]
               <<GetP2CLookupTable()->intx[end_bearing][range_end]<<GetP2CLookupTable()->inty[end_bearing][range_end];

    qDebug()<<Q_FUNC_INFO<<m_range_start<<m_range_end<<range_start<<range_end<<m_current_range;

    if (start_bearing > end_bearing)
        end_bearing += LINES_PER_ROTATION;

    if (range_end > RETURNS_PER_LINE)
        range_end = RETURNS_PER_LINE;

    if (range_end < range_start) return;

    for (int angle = start_bearing; angle < end_bearing; angle += 2)
    {
//        if(stop)
//            break;
        // check if this angle has been updated by the beam since last time
        // and if possible targets have been refreshed

        quint64 time1 = m_ri->m_history[MOD_ROTATION2048(angle)].time;
        // next one must be timed later than the pass 2 in refresh, otherwise target may be found multiple times
        quint64 time2 = m_ri->m_history[MOD_ROTATION2048(angle + 3 * SCAN_MARGIN)].time;

        // check if target has been refreshed since last time
        // and if the beam has passed the target location with SCAN_MARGIN spokes
        if ((time1 > (arpa_update_time[MOD_ROTATION2048(angle)] + SCAN_MARGIN2) &&
             time2 >= time1))
        {  // the beam sould have passed our "angle" AND a point SCANMARGIN further
            // set new refresh time
            arpa_update_time[MOD_ROTATION2048(angle)] = time1;
            for (int rrr = (int)range_start; rrr < (int)range_end; rrr++)
            {
//                qDebug()<<Q_FUNC_INFO<<"angle"<<angle<<"range"<<rrr;
                if (m_ri->m_arpa->MultiPix(angle, rrr))
                {
                    // pixel found that does not belong to a known target
                    Polar pol;
                    QPoint tes_point;

                    pol.angle = angle;
                    pol.r = rrr/2;
                    tes_point.setX(GetP2CLookupTable()->intx[pol.angle][rrr]);
                    tes_point.setY(GetP2CLookupTable()->inty[pol.angle][rrr]);


                    if(m_arpa_polygon.containsPoint(tes_point,Qt::WindingFill))
                    {
                        Position own_pos;
                        own_pos.lat = currentOwnShipLat;
                        own_pos.lon = currentOwnShipLon;
                        Position x;
                        x = Polar2Pos(pol, own_pos, m_current_range);

                        int target_i;
                        target_i = m_ri->m_arpa->AcquireNewARPATarget(pol,0);
                        qDebug()<<Q_FUNC_INFO<<"angle"<<angle<<"range"<<rrr;
                        qDebug()<<Q_FUNC_INFO<<"X"<<tes_point.x()<<"Y"<<tes_point.y();
                        qDebug()<<Q_FUNC_INFO<<"m_arpa_polygon"<<m_arpa_polygon;

//                        qDebug()<<Q_FUNC_INFO<<"in guargzone. polar angle"<<pol.angle<<"polar range"<<pol.r;
//                        qDebug()<<Q_FUNC_INFO<<"target_i"<<target_i;
    //                    if (target_i == -1) break;  // TODO: how to handle max targets exceeded
                    }
//                    else
//                        qDebug()<<Q_FUNC_INFO<<"not in guargzone";

                }
            }
        }
    }
}
*/
