#include "radarengine.h"
#include <radarconfig_global.h>

#include <QtOpenGL/qgl.h>
#include <unistd.h>

GLubyte old_strength_info[2048][512];
GLubyte new_strength_info[2048][512];

#define MARGIN (100)
#define TRAILS_SIZE (RETURNS_PER_LINE * 2 + MARGIN * 2)

enum {
    TRAIL_15SEC, //6 rev
    TRAIL_30SEC, //12 rev
    TRAIL_1MIN,  //24 rev
    TRAIL_3MIN,  //72 rev
    TRAIL_5MIN, //120 rev
    TRAIL_10MIN, //240 rev
    TRAIL_CONTINUOUS,
    TRAIL_ARRAY_SIZE
};

RadarEngine::RadarEngine* RadarEngine::RadarEngine::instance{nullptr};
RadarEngine::RadarEngine* RadarEngine::RadarEngine::getInstance(QObject* parent)
{
    if(instance == nullptr) instance = new RadarEngine(parent);
    return  instance;
}

RadarEngine::RadarEngine::RadarEngine(QObject *parent):
    QObject(parent)
{
    qDebug()<<Q_FUNC_INFO;

    radar_timeout = 0;
//    m_range_meters = 0;

    cur_radar_state = static_cast<RadarState>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::VOLATILE_RADAR_STATUS).toInt());
    old_draw_trails = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_RADAR_TRAIL_ENABLE).toBool();
    old_trail = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_RADAR_TRAIL_TIME).toInt();
    old_preset = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::VOLATILE_DISPLAY_PRESET_COLOR).toInt();

    ComputeColourMap();
    ComputeTargetTrails();

    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(timerTimeout()));

    radarReceive = new RadarReceive(this,this);
    radarTransmit = new RadarTransmit(this,this);
    radarDraw = RadarDraw::make_Draw(this,0);
    radarArpa = new RadarArpa(this,this);
    guardZone = new GuardZone(this,this);

    RadarConfig::RadarConfig *instance = RadarConfig::RadarConfig::getInstance("");

    connect(radarReceive,&RadarReceive::updateReport,
            this,&RadarEngine::receiveThread_Report);
    connect(radarReceive,&RadarReceive::ProcessRadarSpoke,
            this,&RadarEngine::radarReceive_ProcessRadarSpoke);

//    connect(this,SIGNAL(signal_sendStby()),radarTransmit,SLOT(RadarStby()));
//    connect(this,SIGNAL(signal_sendTx()),this,SLOT(trigger_ReqTx()));
    connect(this,SIGNAL(signal_stay_alive()),radarTransmit,SLOT(RadarStayAlive()));
    connect(instance,&RadarConfig::RadarConfig::configValueChange,this,&RadarEngine::onRadarConfigChange);

    trigger_ReqRadarSetting();
    timer->start(1000);
}

void RadarEngine::RadarEngine::onRadarConfigChange(QString key, QVariant val)
{
//    qDebug()<<Q_FUNC_INFO<<"key"<<key<<"val"<<val;
    if(key == RadarConfig::NON_VOLATILE_PPI_DISPLAY_LAST_SCALE) trigger_ReqRangeChange(val.toInt());
    else if(key == RadarConfig::NON_VOLATILE_RADAR_TRAIL_TIME || key == RadarConfig::NON_VOLATILE_RADAR_TRAIL_ENABLE) trigger_clearTrail();
    else if(key == RadarConfig::NON_VOLATILE_RADAR_NET_IP_DATA) trigger_ReqRadarSetting();
}

void RadarEngine::RadarEngine::trigger_ReqTx()
{
    radarTransmit->RadarTx();
}
void RadarEngine::RadarEngine::trigger_ReqStby()
{
    radarTransmit->RadarStby();
}
RadarEngine::RadarEngine::~RadarEngine()
{
    radarReceive->exitReq();
}

void RadarEngine::RadarEngine::timerTimeout()
{
    quint64 now = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());

    const RadarState state_radar = static_cast<RadarState>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::VOLATILE_RADAR_STATUS).toInt());
    const bool is_trail_enable = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_RADAR_TRAIL_ENABLE).toBool();
    const int trail = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_RADAR_TRAIL_TIME).toInt();

//    qDebug()<<Q_FUNC_INFO<<"state_radar"<<(int)state_radar;

    if(state_radar == RADAR_TRANSMIT && TIMED_OUT(now,stay_alive_timeout))
    {
        emit signal_stay_alive();
        stay_alive_timeout = now + STAYALIVE_TIMEOUT;
    }
    if(((state_radar == RADAR_STANDBY) | (state_radar == RADAR_TRANSMIT)) && TIMED_OUT(now,radar_timeout))
    {
        RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_STATUS,RADAR_OFF);
        ResetSpokes();
    }

//    state_radar = RADAR_STANDBY; //temporary
//    state_radar = RADAR_TRANSMIT; //temporary

    if(cur_radar_state != state_radar)
    {
        cur_radar_state = state_radar;
        if(state_radar == RADAR_STANDBY)
        {
            ResetSpokes();
        }
//        emit signal_state_change();
    }

    const int preset_color = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::VOLATILE_DISPLAY_PRESET_COLOR).toInt();
    if(old_preset != preset_color)
    {
        ClearTrails();
        ComputeColourMap();
        ComputeTargetTrails();
        old_preset = preset_color;
    }

    if(old_draw_trails != is_trail_enable)
    {
        if(!is_trail_enable)
            ClearTrails();

        ComputeColourMap();
        ComputeTargetTrails();
        old_draw_trails = is_trail_enable;
    }
    if(old_trail != trail)
    {
        ClearTrails();
        ComputeColourMap();
        ComputeTargetTrails();
        old_trail = trail;
    }
}

void RadarEngine::RadarEngine::trigger_ReqControlChange(int ct, int val)
{
    radarTransmit->setControlValue((ControlType)ct,val);
}

void RadarEngine::RadarEngine::radarReceive_ProcessRadarSpoke(int angle_raw, QByteArray data, int dataSize)

{
//    qDebug()<<Q_FUNC_INFO<<"radarId"<<m_range_meters<<m_old_range<<angle_raw;
    quint64 now = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    radar_timeout = now + WATCHDOG_TIMEOUT;
    data_timeout = now + DATA_TIMEOUT;
//    state_radar = RADAR_TRANSMIT; //need for offline mode


    RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_STATUS,RADAR_TRANSMIT);

    const bool heading_up = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_PPI_DISPLAY_HEADING_UP).toBool();
    const bool mti_enable = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::VOLATILE_RADAR_PARAMS_FILTER_CONTROL_MTI).toBool();
    const int mti = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::VOLATILE_RADAR_PARAMS_FILTER_DATA_MTI).toInt();
    const bool is_trail_enable = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_RADAR_TRAIL_ENABLE).toBool();
    const double currentHeading = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_NAV_DATA_LAST_HEADING).toDouble();
    const double currentOwnShipLat = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_NAV_DATA_LAST_LATITUDE).toDouble();
    const double currentOwnShipLon = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_NAV_DATA_LAST_LONGITUDE).toDouble();

    short int hdt_raw = heading_up ? 0 : SCALE_DEGREES_TO_RAW(currentHeading);
    int bearing_raw = angle_raw + hdt_raw;

    int angle = MOD_ROTATION2048(angle_raw / 2);
    int bearing = MOD_ROTATION2048(bearing_raw / 2);

    quint8 weakest_normal_blob = 50; //next load from configuration file
    quint8 *hist_data = m_history[bearing].line;
    UINT8 *raw_data = reinterpret_cast<UINT8*>(data.data());

    m_history[bearing].time = now;
    m_history[bearing].lat = currentOwnShipLat;
    m_history[bearing].lon = currentOwnShipLon;

    for (int radius = 0; radius < data.size(); radius++)
    {
        if(mti_enable)
        {
            new_strength_info[bearing][radius] = raw_data[radius];
            if(abs(static_cast<int>((old_strength_info[bearing][radius] - new_strength_info[bearing][radius]))) < mti)
                raw_data[radius]=0;
        }

        hist_data[radius] = hist_data[radius] << 1;  // shift left history byte 1 bit
        // clear leftmost 2 bits to 00 for ARPA
        hist_data[radius] = hist_data[radius] & 63;
        if (raw_data[radius] >= weakest_normal_blob)
        {
            // and add 1 if above threshold and set the left 2 bits, used for ARPA
                hist_data[radius] = hist_data[radius] | 192;
        }

        /*
        */
        if(mti_enable)
            old_strength_info[bearing][radius] = new_strength_info[bearing][radius];
    }
    raw_data[RETURNS_PER_LINE-1] = 255; //range rings

    /*check Guardzone*/
    //    if(gz_settings.show && gz_settings.enable_alarm)
    //        m_gz->ProcessSpokePoly(angle, raw_data, rng_gz);
    //    m_gz->ProcessSpoke(angle, raw_data, m_history[bearing].line, rng_gz);


    /*Trail handler*/
    if(is_trail_enable)
    {
        const uint cur_range = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_PPI_DISPLAY_LAST_SCALE).toUInt();
        if (m_old_range != cur_range && m_old_range != 0 && cur_range != 0)
        {
            // zoom trails
            float zoom_factor = static_cast<float>(m_old_range) / static_cast<float>(cur_range);
            ZoomTrails(zoom_factor);
        }
        if (m_old_range == 0 || cur_range == 0)
        {
            ClearTrails();
        }
        m_old_range = cur_range;

        // Relative trails
        quint8 *trail = m_trails.relative_trails[angle];
        for (int radius = 0; radius < dataSize - 1; radius++)
        {
            if (raw_data[radius] >= weakest_normal_blob)
                *trail = 1;
            else
            {
                if (*trail > 0 && *trail < TRAIL_MAX_REVOLUTIONS)
                    (*trail)++;

                raw_data[radius] = m_trail_colour[*trail];
            }
//            qDebug()<<Q_FUNC_INFO<<"trail"<<radius<<*trail;
            trail++;
        }
    }

    guardZone->ProcessSpoke(bearing,raw_data);
    emit signal_plotRadarSpoke(bearing,raw_data,static_cast<size_t>(dataSize));
}

void RadarEngine::RadarEngine::ResetSpokes()
{
    quint8 zap[RETURNS_PER_LINE];

    qDebug()<<Q_FUNC_INFO<<"reset spokes, history and trails";

    memset(zap, 0, sizeof(zap));
    memset(m_history, 0, sizeof(m_history));

    for (size_t r = 0; r < LINES_PER_ROTATION; r++)
        emit signal_plotRadarSpoke(r,zap,sizeof(zap));

}

void RadarEngine::RadarEngine::ZoomTrails(float zoom_factor)
{
    // zoom relative trails
    memset(&m_trails.copy_of_relative_trails, 0, sizeof(m_trails.copy_of_relative_trails));
    for (int i = 0; i < LINES_PER_ROTATION; i++)
    {
        for (int j = 0; j < RETURNS_PER_LINE; j++)
        {
            int index_j = (int((float)j * zoom_factor));
            if (index_j >= RETURNS_PER_LINE) break;
            if (m_trails.relative_trails[i][j] != 0)
            {
                m_trails.copy_of_relative_trails[i][index_j] = m_trails.relative_trails[i][j];
            }
        }
    }
    memcpy(&m_trails.relative_trails[0][0], &m_trails.copy_of_relative_trails[0][0], sizeof(m_trails.copy_of_relative_trails));
}
void RadarEngine::RadarEngine::ClearTrails()
{
    memset(&m_trails, 0, sizeof(m_trails));
}
void RadarEngine::RadarEngine::ComputeColourMap()
{
    for (int i = 0; i <= UINT8_MAX; i++)
    {
        m_colour_map[i] = (i >= 200 /*red strong threshold*/) ? BLOB_STRONG
                                                              : (i >= 100 /*green strong threshold*/)
                                                                ? BLOB_INTERMEDIATE
                                                                : (i >= 50 /*blue strong threshold*/) ? BLOB_WEAK : BLOB_NONE; //next implementation load from conf file
        //        qDebug()<<Q_FUNC_INFO<<"color map "<<i<<m_colour_map[i];
    }

    for (int i = 0; i < BLOB_COLOURS; i++)
        m_colour_map_rgb[i] = QColor(0, 0, 0);

    const int preset_color = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::VOLATILE_DISPLAY_PRESET_COLOR).toInt();
    QColor color;

    if(preset_color == 0)
        color = QColor(255, 255, 0);
    else if(preset_color == 1)
        color = QColor(0, 255, 0);

    m_colour_map_rgb[BLOB_STRONG] = color;
    m_colour_map_rgb[BLOB_INTERMEDIATE] = color;
    m_colour_map_rgb[BLOB_WEAK] = color;

    const bool is_trail_enable = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_RADAR_TRAIL_ENABLE).toBool();
    if (is_trail_enable)
    {
        /*
        int a1 = 255;
        int a2 = 0;
        float delta_a = (float)((a2 - a1) / BLOB_HISTORY_COLOURS);

        for (BlobColour history = BLOB_HISTORY_0;
             history <= BLOB_HISTORY_MAX;
             history = (BlobColour)(history + 1))
        {
            m_colour_map[history] = history;
            m_colour_map_rgb[history] = QColor(0, 255, 0,a1);
            a1 += (int)delta_a;
        }
        */
        int r1 = 255.0;
        int g1 = 255.0;
        int b1 = 255.0;
        int a1 = 255.0;
        int r2 = 0.0;
        int g2 = 0.0;
        int b2 = 0.0;
        int a2 = 0.0;
        float delta_r = static_cast<float>(((r2 - r1) / BLOB_HISTORY_COLOURS));
        float delta_g = static_cast<float>((g2 - g1) / BLOB_HISTORY_COLOURS);
        float delta_b = static_cast<float>((b2 - b1) / BLOB_HISTORY_COLOURS);
        float delta_a = static_cast<float>((a2 - a1) / BLOB_HISTORY_COLOURS);

        if(preset_color == 0)
        {
            r1 = 255.0;
            g1 = 255.0;
            b1 = 255.0;
//            a1 = 255.0;
        }
        else if(preset_color == 1)
        {
            r1 = .0;
            g1 = .0;
            b1 = .0;
//            a1 = .0;
        }

        for (BlobColour history = BLOB_HISTORY_0;
             history <= BLOB_HISTORY_MAX;
             history = static_cast<BlobColour>(history + 1))
        {
            m_colour_map[history] = history;
            m_colour_map_rgb[history] = QColor(r1, g1, b1, a1);
            if(preset_color == 0)
            {
                r1 += static_cast<int>(delta_r);
                g1 += static_cast<int>(delta_g);
                b1 += static_cast<int>(delta_b);
            }
            else if(preset_color == 1)
            {
                r1 -= static_cast<int>(delta_r);
                g1 -= static_cast<int>(delta_g);
                b1 -= static_cast<int>(delta_b);
//                a1 -= static_cast<int>(delta_a);
            }
            a1 += static_cast<int>(delta_a);
        }
    }
}

void RadarEngine::RadarEngine::ComputeTargetTrails()
{
    static TrailRevolutionsAge maxRevs[TRAIL_ARRAY_SIZE] =
    {
        SECONDS_TO_REVOLUTIONS(15),
        SECONDS_TO_REVOLUTIONS(30),
        SECONDS_TO_REVOLUTIONS(60),
        SECONDS_TO_REVOLUTIONS(180),
        SECONDS_TO_REVOLUTIONS(300),
        SECONDS_TO_REVOLUTIONS(600),
        TRAIL_MAX_REVOLUTIONS + 1
    };

    const bool is_trail_enable = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_RADAR_TRAIL_ENABLE).toBool();
    const int trail = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_RADAR_TRAIL_TIME).toInt();
    TrailRevolutionsAge maxRev = maxRevs[trail];
    if (!is_trail_enable)
        maxRev = 0;


    TrailRevolutionsAge revolution;
    double coloursPerRevolution = 0.;
    double colour = 0.;

    // Like plotter, continuous trails are all very white (non transparent)
    if (is_trail_enable && (trail < TRAIL_CONTINUOUS))
        coloursPerRevolution = BLOB_HISTORY_COLOURS / (double)maxRev;

    qDebug()<<Q_FUNC_INFO<<"Target trail value "<<maxRev<<"revolutions";

    // Disperse the BLOB_HISTORY values over 0..maxrev
    for (revolution = 0; revolution <= TRAIL_MAX_REVOLUTIONS; revolution++)
    {
        if (revolution >= 1 && revolution < maxRev)
        {
            m_trail_colour[revolution] = (BlobColour)(BLOB_HISTORY_0 + (int)colour);
            colour += coloursPerRevolution;
        }
        else
            m_trail_colour[revolution] = BLOB_NONE;

    }
}

void RadarEngine::RadarEngine::trigger_stopRadar()
{
    radarReceive->exitReq();
}

void RadarEngine::RadarEngine::trigger_ReqRangeChange(int range)
{
    radarTransmit->setRange(range);
}

void RadarEngine::RadarEngine::trigger_clearTrail()
{
    ClearTrails();
}

void RadarEngine::RadarEngine::trigger_ReqRadarSetting()
{
    ResetSpokes();
    radarReceive->exitReq();
    sleep(1);
    radarTransmit->setMulticastData(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_RADAR_NET_IP_CMD).toString(),
                                    RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_RADAR_NET_PORT_CMD).toUInt());
    radarReceive->start();

}

void RadarEngine::RadarEngine::checkRange(uint new_range)
{
    const uint cur_range = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::VOLATILE_RADAR_PARAMS_RANGE_DATA_RANGE).toUInt();
    const uint cur_scale = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::NON_VOLATILE_PPI_DISPLAY_LAST_SCALE).toUInt();
    if ((cur_range != static_cast<uint>(new_range)))
    {
        RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_RANGE_DATA_RANGE,new_range);
//        m_range_meters = new_range;
//        emit signal_range_change(new_range/10);
        ResetSpokes();
        qDebug()<<Q_FUNC_INFO<<"detected spoke range change from "<<cur_range<<" to "<<new_range;
    }
    if ((cur_scale != static_cast<uint>(new_range)))
    {
        trigger_ReqRangeChange(static_cast<int>(cur_scale));
        ResetSpokes();
        qDebug()<<Q_FUNC_INFO<<"range mismatch "<<cur_scale<<" to "<<new_range;
    }
}

void RadarEngine::RadarEngine::receiveThread_Report(quint8 report_type, quint8 report_field, quint32 value)
{
    quint64 now = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    radar_timeout = now + WATCHDOG_TIMEOUT;
//    RadarState *cur_radar_state = &state_radar;
    RadarState cur_radar_state = static_cast<RadarState>(RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::VOLATILE_RADAR_STATUS).toInt());
//    qDebug()<<Q_FUNC_INFO;
    switch (report_type)
    {
    case RADAR_STATE:
//        if(radarId == 0)
        cur_radar_state = static_cast<RadarState>(report_field);
        if(cur_radar_state == RADAR_TRANSMIT && TIMED_OUT(now,data_timeout))
        {
            cur_radar_state = RADAR_NO_SPOKE;
            ResetSpokes();
        }
        if(cur_radar_state == RADAR_WAKING_UP)
        {
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_WAKINGUP_TIME,static_cast<quint8>(value));
        }

        RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_STATUS,static_cast<quint8>(cur_radar_state));

        qDebug()<<Q_FUNC_INFO<<"report status radar"<<(RadarState)report_field;
        break;
    case RADAR_FILTER:
        switch (report_field)
        {
        case RADAR_GAIN:
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_FILTER_DATA_GAIN,static_cast<quint8>(value));
//            qDebug()<<Q_FUNC_INFO<<"report gain"<<filter.gain;
            emit signal_updateReport();
            break;
        case RADAR_RAIN:
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_FILTER_DATA_RAIN,static_cast<quint8>(value));
//            qDebug()<<Q_FUNC_INFO<<"report rain"<<filter.rain;
            emit signal_updateReport();
            break;
        case RADAR_SEA:
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_FILTER_DATA_SEA,static_cast<quint8>(value));
//            qDebug()<<Q_FUNC_INFO<<"report sea"<<filter.sea;
            emit signal_updateReport();
            break;
        case RADAR_TARGET_BOOST:
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_FILTER_DATA_TARGET_BOOST,static_cast<quint8>(value));
//            qDebug()<<Q_FUNC_INFO<<"report TargetBoost"<<filter.targetBoost;
            break;
        case RADAR_LOCAL_INTERFERENCE_REJECTION:
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_FILTER_DATA_INTERFERENCE,static_cast<quint8>(value));
//            qDebug()<<Q_FUNC_INFO<<"report local interference"<<filter.LInterference;
            break;
        case RADAR_TARGET_EXPANSION:
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_FILTER_DATA_TARGET_EXPAND,static_cast<quint8>(value));
//            qDebug()<<Q_FUNC_INFO<<"report argetExpan"<<filter.targetExpan;
            break;
        case RADAR_RANGE:
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_RANGE_DATA_RANGE,static_cast<uint>(value));
            checkRange(value);
            qDebug()<<Q_FUNC_INFO<<"report range"<<value;
            break;
        default:
            break;
        }
        break;
    case RADAR_ALIGN:
        switch (report_field)
        {
        case RADAR_BEARING:
        {
            // bearing alignment
            int bearing = (int)value / 10;
            if (bearing > 180)
                bearing = bearing - 360;
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_ALIGN_DATA_BEARING,static_cast<uint>(bearing));
            qDebug()<<Q_FUNC_INFO<<"report radar bearing alignment"<<bearing;
            break;
        }
        case RADAR_ANTENA:
            // antenna height
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_ALIGN_DATA_BEARING,static_cast<uint>(value));
            qDebug()<<Q_FUNC_INFO<<"report radar antenna_height"<<value/1000;
            break;
        default:
            break;
        }
        break;
    case RADAR_SCAN_AND_SIGNAL:
        switch (report_field)
        {
        case RADAR_SCAN_SPEED:
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_SCAN_DATA_SCAN_SPEED,value);
            qDebug()<<Q_FUNC_INFO<<"report radar scan_speed"<<value ;
            break;
        case RADAR_NOISE_REJECT:
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_SCAN_DATA_NOISE_REJECT,value);
            qDebug()<<Q_FUNC_INFO<<"report radar noise_reject"<<value ;
            break;
        case RADAR_TARGET_SEPARATION:
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_SCAN_DATA_TARGET_SEPARATION,value);
            qDebug()<<Q_FUNC_INFO<<"report radar target_sep"<<value;
            break;
        case RADAR_LOBE_SUPRESION:
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_SCAN_DATA_SIDE_LOBE_SUPPRESSION,value);
            qDebug()<<Q_FUNC_INFO<<"report radar side_lobe_suppression"<<value ; //0->auto
            break;
        case RADAR_INTERFERENT:
            RadarConfig::RadarConfig::getInstance("")->setConfig(RadarConfig::VOLATILE_RADAR_PARAMS_SCAN_DATA_LOCAL_INTERFERENCE,value);
            qDebug()<<Q_FUNC_INFO<<"report radar local_interference_rejection"<<value;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

}
