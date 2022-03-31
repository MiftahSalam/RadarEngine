#include "guardzone.h"
#include <radarconfig.h>

#include <qmath.h>

#define SCAN_MARGIN (150)            // number of lines that a next scan of the target may have moved
#define SCAN_MARGIN2 (1000)          // if target is refreshed after this time you will be shure it is the next sweep

using namespace RadarEngine;

GuardZone::GuardZone(RadarEngine *re):m_re(re)
{
//    zero_angle_count = 0;

//    for(int i=0;i<gz_settings.polygon.size();i++)
//        m_polygon<<gz_settings.polygon.at(i).toPointF()*2.0;

//    m_current_range = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_PPI_DISPLAY_LAST_SCALE).toInt();
//    SetPolygon(m_polygon);

//    qDebug()<<Q_FUNC_INFO<<"initial m_polygon"<<m_polygon;

//    for (int angle = 0; angle < LINES_PER_ROTATION; angle++)
//    {
//        arpa_update_time[angle] = 0;
//    }

    m_type = static_cast<GZType>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_MODE).toInt());
        m_start_bearing = static_cast<int>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_START_BEARING).toInt());
        m_end_bearing =  static_cast<int>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_END_BEARING).toInt());
        m_inner_range =  static_cast<int>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_START_RANGE).toInt());
        m_outer_range =  static_cast<int>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_END_RANGE).toInt());
        ResetBogeys();
}


void GuardZone::ProcessSpoke(int angle, UINT8* data, UINT8* hist, int range)
{

    //    qDebug()<<Q_FUNC_INFO<<"inner "<<m_inner_range<<"outter "<<m_outer_range;
    size_t range_start =  static_cast<size_t>( m_inner_range * RETURNS_PER_LINE / range / 2);  // Convert from meters to 0..511
    size_t range_end =  static_cast<size_t>(m_outer_range * RETURNS_PER_LINE / range / 2);    // Convert from meters to 0..511
    bool in_guard_zone = false;

    if(m_inner_range != static_cast<int>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_START_RANGE).toInt()))
    {
        m_inner_range = static_cast<int>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_START_RANGE).toInt());
        ResetBogeys();
    }
    if(m_outer_range != static_cast<int>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_END_RANGE).toInt()))
    {
        m_outer_range = static_cast<int>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_END_RANGE).toInt());
        ResetBogeys();
    }
    if(m_start_bearing != static_cast<int>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_START_BEARING).toInt()))
    {
        m_start_bearing = static_cast<int>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_START_BEARING).toInt());
        ResetBogeys();
    }
    if(m_end_bearing != static_cast<int>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_END_BEARING).toInt()))
    {
        m_end_bearing = static_cast<int>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_END_BEARING).toInt());
        ResetBogeys();
    }
    if(m_type != static_cast<GZType>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_MODE).toInt()))
    {
        m_type = static_cast<GZType>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_GZ_MODE).toInt());
        ResetBogeys();
    }

    int start_brn_2048 = SCALE_DEGREES_TO_RAW2048(m_start_bearing);
    int end_brn_2048 = SCALE_DEGREES_TO_RAW2048(m_end_bearing);

    switch (m_type)
    {
    case GZ_ARC:
        if ((angle >= start_brn_2048 && angle < end_brn_2048) ||
                (start_brn_2048 >= end_brn_2048 && (angle >= start_brn_2048 || angle < end_brn_2048)))
        {
            if (range_start < RETURNS_PER_LINE)
            {
                if (range_end > RETURNS_PER_LINE)
                    range_end = RETURNS_PER_LINE;

                for (size_t r = range_start; r <= range_end; r++)
                {
//                    uint8_t atad = data[r];
//                    if (/*!m_multi_sweep_filter ||*/ HISTORY_FILTER_ALLOW(hist[r]))
                    {
                        if (data[r] >= 50)
//                            if (data[r] >= m_ri->m_settings.threshold_blue)
                            m_running_count++;

                    }
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

            for (size_t r = range_start; r <= range_end; r++)
            {
//                if (/*!m_multi_sweep_filter ||*/ HISTORY_FILTER_ALLOW(hist[r]))
                {
                    if (data[r] >= 50)
//                        if (data[r] >= m_pi->m_settings.threshold_blue)
                        m_running_count++;

                }
            }
            if (angle > m_last_angle)
                in_guard_zone = true;

        }
        break;
    default:
        in_guard_zone = false;
        break;
    }

    if (m_last_in_guard_zone && !in_guard_zone)
    {
        // last bearing that could add to m_running_count, so store as bogey_count;
        m_bogey_count = m_running_count;
        m_running_count = 0;

//        qDebug()<<Q_FUNC_INFO<<m_log_name<<"angle"<<angle<<"last_angle"<<m_last_angle<<"range"<<range<<"guardzone"<<range_start
//               <<"..."<<range_end<<"("<<m_inner_range<<"-"<<m_outer_range<<")"<<"bogey_count"<<m_bogey_count;
        // When debugging with a static ship it is hard to find moving targets, so move
        // the guard zone instead. This slowly rotates the guard zone.
        if (0 && m_type == GZ_ARC)
        {
            m_start_bearing += LINES_PER_ROTATION - 0;
            m_end_bearing += LINES_PER_ROTATION - 0;
            m_start_bearing %= LINES_PER_ROTATION;
            m_end_bearing %= LINES_PER_ROTATION;
        }
    }

    m_last_in_guard_zone = in_guard_zone;
    m_last_angle = angle;
}

/*
void GuardZone::countBogey(const int angle, const double heading, const quint8 *data, const int range, const QPolygonF polygon)
{
    QPointF tes_point;
    float range_res = 2.0*(float)range/RETURNS_PER_LINE;
    float x = 0;
    float y = 0;
    float theta = SCALE_RAW_TO_DEGREES2048(angle)+heading;
    float cos_theta = qCos(deg2rad(90.0-theta));
    float sin_theta = qSin(deg2rad(90.0-theta));
//    int current_bogey = 0;
//    qDebug()<<Q_FUNC_INFO<<range_res;

    for(int i=0; i<RETURNS_PER_LINE; i++)
    {
        tes_point.setX(x);
        tes_point.setY(y);

        if(data[i] > 50)
        {
            if(polygon.containsPoint(tes_point,Qt::WindingFill))
            {
//                qDebug()<<Q_FUNC_INFO<<sqrt(pow(tes_point.x(),2)+pow(tes_point.y(),2))<<theta<<range<<data[i]<<i;
//                qDebug()<<Q_FUNC_INFO<<"angle"<<angle<<"range"<<i<<"data"<<data[i];
//                qDebug()<<Q_FUNC_INFO<<tes_point<<m_bogey_count;
//                qDebug()<<Q_FUNC_INFO<<polygon;
                mutex.lock();
                m_running_count++;
                mutex.unlock();
            }
        }
        x += (range_res*cos_theta);
        y += (range_res*sin_theta);
    }
}

void GuardZone::ProcessSpokePoly(int angle, UINT8* data, int range)
{
    m_current_range = range;
    if(angle == 0)
    {
        zero_angle_count++;
        if(zero_angle_count == 2)
        {
            zero_angle_count = 0;
            mutex.lock();
//            qDebug()<<Q_FUNC_INFO<<m_bogey_count<<m_running_count;
            m_bogey_count = m_running_count;
            m_running_count = 0;
            mutex.unlock();
        }
    }

//    QtConcurrent::run(this,&GZ::countBogey,angle,currentHeading,data,range,m_polygon);
//    countBogey(angle,currentHeading,data,range,m_polygon);

}

void GZ::SetPolygon(const QPolygonF polyF)
{
    m_polygon = polyF;
    m_start_bearing = 0;
    m_range_start = 0;
    m_end_bearing = 0;
    m_range_end = 0;
    m_arpa_polygon.clear();
    gz_settings.polygon.clear();

    QList<float> range_list;
    QList<float> bearing_list;
    QPointF buf_point_arpa;
    float range_scale = (float)RETURNS_PER_LINE/m_current_range;

    if(m_polygon.isEmpty()) return;

    range_list.clear();
    bearing_list.clear();

    for(int i=0;i<m_polygon.size();i++)
    {
        QPointF point = m_polygon.at(i);
        buf_point_arpa = point*range_scale;
        QPoint point_arpa((int)buf_point_arpa.x(),(int)buf_point_arpa.y());
        float buf_range = sqrt(pow(point.x(),2)+pow(point.y(),2));
        float buf_bearing = atan2f(point.y(),point.x());
        buf_bearing = 90.0-rad2deg(buf_bearing);

        range_list.append(buf_range);
        bearing_list.append(buf_bearing);
        m_arpa_polygon.append(point_arpa);

        gz_settings.polygon<<point;
    }
    qSort(range_list.begin(),range_list.end());
    qSort(bearing_list.begin(),bearing_list.end());

    qDebug()<<Q_FUNC_INFO<<"m_current_range"<<m_current_range;
    qDebug()<<Q_FUNC_INFO<<"range scale"<<range_scale;
    qDebug()<<Q_FUNC_INFO<<"polyF"<<polyF;
    qDebug()<<Q_FUNC_INFO<<"buf_point_arpa"<<buf_point_arpa;
    qDebug()<<Q_FUNC_INFO<<"m_arpa_polygon"<<m_arpa_polygon;
    qDebug()<<Q_FUNC_INFO<<"range list"<<range_list;
    qDebug()<<Q_FUNC_INFO<<"bearing list"<<bearing_list;
    qDebug()<<Q_FUNC_INFO<<"range start"<<range_list.at(0)<<"range end"<<range_list.last();
    qDebug()<<Q_FUNC_INFO<<"bearing start"<<bearing_list.at(0)<<"bearing end"<<bearing_list.last();

    m_range_start = range_list.at(0);
    m_range_end = range_list.last();
    m_start_bearing = SCALE_DEGREES_TO_RAW2048(bearing_list.at(0));
    m_end_bearing = SCALE_DEGREES_TO_RAW2048(bearing_list.last());

    qDebug()<<Q_FUNC_INFO<<"m_range_start"<<m_range_start<<"m_range_end"<<m_range_end;
    qDebug()<<Q_FUNC_INFO<<"m_start_bearing"<<m_start_bearing<<"m_end_bearing"<<m_end_bearing;

}

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
