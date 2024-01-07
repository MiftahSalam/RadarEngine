#include "radarconfig.h"
#include "radarengine.h"
#include "constants.h"

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

RadarEngine::RadarEngine* RadarEngine::RadarEngine::m_instance{nullptr};

RadarEngine::RadarEngine* RadarEngine::RadarEngine::GetInstance(QObject* parent)
{
    if(m_instance == nullptr) m_instance = new RadarEngine(parent);
    return  m_instance;
}

RadarEngine::RadarEngine::RadarEngine(QObject *parent):
    QObject(parent)
{
    qDebug()<<Q_FUNC_INFO;

    radar_timeout = 0;

    m_cur_radar_state = static_cast<RadarState>(RadarConfig::getInstance("")->getConfig(VOLATILE_RADAR_STATUS).toInt());
    m_old_draw_trails = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_RADAR_TRAIL_ENABLE).toBool();
    m_old_trail = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_RADAR_TRAIL_TIME).toInt();
    m_old_preset = RadarConfig::getInstance("")->getConfig(VOLATILE_DISPLAY_PRESET_COLOR).toInt();

    computeColourMap();
    computeTargetTrails();

    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(timerTimeout()));

    m_radar_capture = new RadarImageCapture(this, this);
    m_radar_receive = RadarReceive::getInstance(this,this);
    m_radar_transmit = RadarTransmit::getInstance(this,this);
    radarDraw = RadarDraw::make_Draw(this,0);
    radarArpa = RadarArpa::getInstance(this,this);

    RadarConfig *instance = RadarConfig::getInstance("");

    setupGZ();

    connect(m_radar_receive,&RadarReceive::UpdateReport,
            this,&RadarEngine::receiveThreadReport);
    connect(m_radar_receive,&RadarReceive::ProcessRadarSpoke,
            this,&RadarEngine::radarReceiveProcessRadarSpoke);

    connect(this,SIGNAL(SignalStayAlive()),m_radar_transmit,SLOT(RadarStayAlive()));
    connect(instance,&RadarConfig::configValueChange,this,&RadarEngine::onRadarConfigChange);

    TriggerReqRadarSetting();
    timer->start(1000);
}

void RadarEngine::RadarEngine::setupGZ()
{
    RadarConfig *instance = RadarConfig::getInstance("");

    GuardZone *guardZone = new GuardZone(this,this);
    guardZone->SetType(static_cast<GZType>(instance->getConfig(NON_VOLATILE_GZ_MODE).toInt()));
    guardZone->SetShown(instance->getConfig(NON_VOLATILE_PPI_DISPLAY_SHOW_GZ).toBool());
    guardZone->SetInnerRange(instance->getConfig(NON_VOLATILE_GZ_START_RANGE).toInt());
    guardZone->SetOutterRange(instance->getConfig(NON_VOLATILE_GZ_END_RANGE).toInt());
    guardZone->SetStartBearing(instance->getConfig(NON_VOLATILE_GZ_START_BEARING).toDouble());
    guardZone->SetEndBearing(instance->getConfig(NON_VOLATILE_GZ_END_BEARING).toDouble());

    guardZones.insert("GZ 1",guardZone);

    guardZone = new GuardZone(this,this);
    guardZone->SetType(static_cast<GZType>(instance->getConfig(NON_VOLATILE_GZ_MODE1).toInt()));
    guardZone->SetShown(instance->getConfig(NON_VOLATILE_PPI_DISPLAY_SHOW_GZ1).toBool());
    guardZone->SetInnerRange(instance->getConfig(NON_VOLATILE_GZ_START_RANGE1).toInt());
    guardZone->SetOutterRange(instance->getConfig(NON_VOLATILE_GZ_END_RANGE1).toInt());
    guardZone->SetStartBearing(instance->getConfig(NON_VOLATILE_GZ_START_BEARING1).toDouble());
    guardZone->SetEndBearing(instance->getConfig(NON_VOLATILE_GZ_END_BEARING1).toDouble());

    guardZones.insert("GZ 2",guardZone);
}

void RadarEngine::RadarEngine::onRadarConfigChange(QString key, QVariant val)
{
//    qDebug()<<Q_FUNC_INFO<<"key"<<key<<"val"<<val;
    if(key == NON_VOLATILE_PPI_DISPLAY_LAST_SCALE) TriggerReqRangeChange(val.toInt());
    else if(key == NON_VOLATILE_RADAR_TRAIL_TIME || key == NON_VOLATILE_RADAR_TRAIL_ENABLE) TriggerClearTrail();
    else if(key == NON_VOLATILE_RADAR_NET_IP_DATA) TriggerReqRadarSetting();
    else if(key == NON_VOLATILE_GZ_START_RANGE) guardZones["GZ 1"]->SetInnerRange(val.toInt());
    else if(key == NON_VOLATILE_GZ_START_RANGE1) guardZones["GZ 2"]->SetInnerRange(val.toInt());
    else if(key == NON_VOLATILE_GZ_START_BEARING) guardZones["GZ 1"]->SetStartBearing(val.toDouble());
    else if(key == NON_VOLATILE_GZ_START_BEARING1) guardZones["GZ 2"]->SetStartBearing(val.toDouble());
    else if(key == NON_VOLATILE_GZ_END_RANGE) guardZones["GZ 1"]->SetOutterRange(val.toInt());
    else if(key == NON_VOLATILE_GZ_END_RANGE1) guardZones["GZ 2"]->SetOutterRange(val.toInt());
    else if(key == NON_VOLATILE_GZ_END_BEARING) guardZones["GZ 1"]->SetEndBearing(val.toDouble());
    else if(key == NON_VOLATILE_GZ_END_BEARING1) guardZones["GZ 2"]->SetEndBearing(val.toDouble());
    else if(key == NON_VOLATILE_GZ_MODE) guardZones["GZ 1"]->SetType(static_cast<GZType>(val.toInt()));
    else if(key == NON_VOLATILE_GZ_MODE1) guardZones["GZ 2"]->SetType(static_cast<GZType>(val.toInt()));
    else if(key == NON_VOLATILE_PPI_DISPLAY_SHOW_GZ) guardZones["GZ 1"]->SetShown(val.toBool());
    else if(key == NON_VOLATILE_PPI_DISPLAY_SHOW_GZ1) guardZones["GZ 2"]->SetShown(val.toBool());
}

void RadarEngine::RadarEngine::TriggerReqTx()
{
    m_radar_transmit->RadarTx();
}

void RadarEngine::RadarEngine::TriggerReqStby()
{
    m_radar_transmit->RadarStby();
    RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_STATUS,RADAR_STANDBY);
}

RadarEngine::RadarEngine::~RadarEngine()
{
    m_radar_receive->ExitReq();
}

void RadarEngine::RadarEngine::timerTimeout()
{
    quint64 now = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());

    const RadarState state_radar = static_cast<RadarState>(RadarConfig::getInstance("")->getConfig(VOLATILE_RADAR_STATUS).toInt());
    const bool is_trail_enable = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_RADAR_TRAIL_ENABLE).toBool();
    const int trail = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_RADAR_TRAIL_TIME).toInt();

    if(state_radar == RADAR_TRANSMIT && TIMED_OUT(now,stay_alive_timeout))
    {
        emit SignalStayAlive();
        stay_alive_timeout = now + STAYALIVE_TIMEOUT;
    }
    if(((state_radar == RADAR_STANDBY) | (state_radar == RADAR_TRANSMIT)) && TIMED_OUT(now,radar_timeout))
    {
        RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_STATUS,RADAR_OFF);
        resetSpokes();
    }

//    state_radar = RADAR_STANDBY; //temporary
//    state_radar = RADAR_TRANSMIT; //temporary

    if(m_cur_radar_state != state_radar)
    {
        m_cur_radar_state = state_radar;
        if(state_radar == RADAR_STANDBY)
        {
            resetSpokes();
        }
        else if(state_radar != RADAR_TRANSMIT)
        {
            radarArpa->DeleteAllTargets();
            foreach(const QString &key, guardZones.keys())
            {
                guardZones.value(key)->ResetBogeys();
            }
        }
    }

    const int preset_color = RadarConfig::getInstance("")->getConfig(VOLATILE_DISPLAY_PRESET_COLOR).toInt();
    if(m_old_preset != preset_color)
    {
        clearTrails();
        computeColourMap();
        computeTargetTrails();
        m_old_preset = preset_color;
    }

    if(m_old_draw_trails != is_trail_enable)
    {
        if(!is_trail_enable)
            clearTrails();

        computeColourMap();
        computeTargetTrails();
        m_old_draw_trails = is_trail_enable;
    }

    if(m_old_trail != trail)
    {
        clearTrails();
        computeColourMap();
        computeTargetTrails();
        m_old_trail = trail;
    }
}

void RadarEngine::RadarEngine::TriggerReqControlChange(int ct, int val)
{
    m_radar_transmit->SetControlValue(static_cast<ControlType>(ct),val);
}

void RadarEngine::RadarEngine::radarReceiveProcessRadarSpoke(int angle_raw, QByteArray data, int dataSize)

{
//    qDebug()<<Q_FUNC_INFO<<"radarId"<<m_range_meters<<m_old_range<<angle_raw;
    quint64 now = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    radar_timeout = now + WATCHDOG_TIMEOUT;
    data_timeout = now + DATA_TIMEOUT;
//    state_radar = RADAR_TRANSMIT; //need for offline mode

    RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_STATUS,RADAR_TRANSMIT);

    const bool heading_up = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_HEADING_UP).toBool();
    const bool mti_enable = RadarConfig::getInstance("")->getConfig(VOLATILE_RADAR_PARAMS_FILTER_CONTROL_MTI).toBool();
    const int mti = RadarConfig::getInstance("")->getConfig(VOLATILE_RADAR_PARAMS_FILTER_DATA_MTI).toInt();
    const bool is_trail_enable = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_RADAR_TRAIL_ENABLE).toBool();
    const double currentHeading = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_NAV_DATA_LAST_HEADING).toDouble();
    const double currentOwnShipLat = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_NAV_DATA_LAST_LATITUDE).toDouble();
    const double currentOwnShipLon = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_NAV_DATA_LAST_LONGITUDE).toDouble();

    short int hdt_raw = heading_up ? 0 : static_cast<short>(SCALE_DEGREES_TO_RAW(currentHeading));
    int bearing_raw = angle_raw + hdt_raw;

    int angle = MOD_ROTATION2048(angle_raw / 2);
    int bearing = MOD_ROTATION2048(bearing_raw / 2);

    quint8 weakest_normal_blob = 50; //next load from configuration file
    quint8 *hist_data = history[bearing].line;
    UINT8 *raw_data = reinterpret_cast<UINT8*>(data.data());

    history[bearing].time = now;
    history[bearing].lat = currentOwnShipLat;
    history[bearing].lon = currentOwnShipLon;

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
//    raw_data[RETURNS_PER_LINE-1] = 255; //range rings

    /*check Guardzone*/
    //    if(gz_settings.show && gz_settings.enable_alarm)
    //        m_gz->ProcessSpokePoly(angle, raw_data, rng_gz);
    //    m_gz->ProcessSpoke(angle, raw_data, m_history[bearing].line, rng_gz);


    /*Trail handler*/
    if(is_trail_enable)
    {
        uint cur_range = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_LAST_SCALE).toUInt();
//        const quint8 unit = static_cast<quint8>(RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_UNIT).toUInt());

//        switch (unit) {
//        case 1:
//            cur_range *= KM_TO_NM;
//            break;
//        default:
//            break;
//        }

        if (m_old_range != cur_range && m_old_range != 0 && cur_range != 0)
        {
            // zoom trails
            float zoom_factor = static_cast<float>(m_old_range) / static_cast<float>(cur_range);
            zoomTrails(zoom_factor);
        }
        if (m_old_range == 0 || cur_range == 0)
        {
            clearTrails();
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

    foreach(const QString &key, guardZones.keys())
    {
        guardZones.value(key)->ProcessSpoke(bearing,raw_data);
    }

    emit SignalPlotRadarSpoke(bearing,raw_data,static_cast<size_t>(dataSize));
}

void RadarEngine::RadarEngine::resetSpokes()
{
    quint8 zap[RETURNS_PER_LINE];

    qDebug()<<Q_FUNC_INFO<<"reset spokes, history and trails";

    memset(zap, 0, sizeof(zap));
    memset(history, 0, sizeof(history));

    for (size_t r = 0; r < LINES_PER_ROTATION; r++)
        emit SignalPlotRadarSpoke(static_cast<int>(r),zap,sizeof(zap));

}

void RadarEngine::RadarEngine::zoomTrails(float zoom_factor)
{
    // zoom relative trails
    memset(&m_trails.copy_of_relative_trails, 0, sizeof(m_trails.copy_of_relative_trails));
    for (int i = 0; i < LINES_PER_ROTATION; i++)
    {
        for (int j = 0; j < RETURNS_PER_LINE; j++)
        {
            int index_j = (int(static_cast<float>(j) * zoom_factor));
            if (index_j >= RETURNS_PER_LINE) break;
            if (m_trails.relative_trails[i][j] != 0)
            {
                m_trails.copy_of_relative_trails[i][index_j] = m_trails.relative_trails[i][j];
            }
        }
    }
    memcpy(&m_trails.relative_trails[0][0], &m_trails.copy_of_relative_trails[0][0], sizeof(m_trails.copy_of_relative_trails));
}

void RadarEngine::RadarEngine::clearTrails()
{
    memset(&m_trails, 0, sizeof(m_trails));
}

void RadarEngine::RadarEngine::computeColourMap()
{
    for (int i = 0; i <= UINT8_MAX; i++)
    {
        colourMap[i] = (i >= 200 /*red strong threshold*/) ? BLOB_STRONG
                                                              : (i >= 100 /*green strong threshold*/)
                                                                ? BLOB_INTERMEDIATE
                                                                : (i >= 50 /*blue strong threshold*/) ? BLOB_WEAK : BLOB_NONE; //next implementation load from conf file
    }

    for (int i = 0; i < BLOB_COLOURS; i++)
        colourMapRGB[i] = QColor(0, 0, 0);

    const int preset_color = RadarConfig::getInstance("")->getConfig(VOLATILE_DISPLAY_PRESET_COLOR).toInt();
    QColor color;

    if(preset_color == 0)
        color = QColor(255, 255, 0);
    else if(preset_color == 1)
        color = QColor(0, 255, 0);

    colourMapRGB[BLOB_STRONG] = color;
    colourMapRGB[BLOB_INTERMEDIATE] = color;
    colourMapRGB[BLOB_WEAK] = color;

    const bool is_trail_enable = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_RADAR_TRAIL_ENABLE).toBool();
    if (is_trail_enable)
    {
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
            colourMap[history] = history;
            colourMapRGB[history] = QColor(r1, g1, b1, a1);
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

void RadarEngine::RadarEngine::computeTargetTrails()
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

    const bool is_trail_enable = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_RADAR_TRAIL_ENABLE).toBool();
    const int trail = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_RADAR_TRAIL_TIME).toInt();
    TrailRevolutionsAge maxRev = maxRevs[trail];
    if (!is_trail_enable)
        maxRev = 0;


    TrailRevolutionsAge revolution;
    double coloursPerRevolution = 0.;
    double colour = 0.;

    // Like plotter, continuous trails are all very white (non transparent)
    if (is_trail_enable && (trail < TRAIL_CONTINUOUS))
        coloursPerRevolution = BLOB_HISTORY_COLOURS / static_cast<double>(maxRev);

    qDebug()<<Q_FUNC_INFO<<"Target trail value "<<maxRev<<"revolutions";

    // Disperse the BLOB_HISTORY values over 0..maxrev
    for (revolution = 0; revolution <= TRAIL_MAX_REVOLUTIONS; revolution++)
    {
        if (revolution >= 1 && revolution < maxRev)
        {
            m_trail_colour[revolution] = static_cast<BlobColour>(BLOB_HISTORY_0 + static_cast<int>(colour));
            colour += coloursPerRevolution;
        }
        else
            m_trail_colour[revolution] = BLOB_NONE;
    }
}

void RadarEngine::RadarEngine::TriggerStopRadar()
{
    m_radar_receive->ExitReq();
}

void RadarEngine::RadarEngine::TriggerReqRangeChange(int range)
{
    m_radar_transmit->SetRange(range/2);
}

void RadarEngine::RadarEngine::TriggerClearTrail()
{
    clearTrails();
}

void RadarEngine::RadarEngine::TriggerReqRadarSetting()
{
    resetSpokes();
    m_radar_receive->ExitReq();
    sleep(1);
    m_radar_transmit->SetMulticastData(RadarConfig::getInstance("")->getConfig(NON_VOLATILE_RADAR_NET_IP_CMD).toString(),
                                    RadarConfig::getInstance("")->getConfig(NON_VOLATILE_RADAR_NET_PORT_CMD).toUInt());
    m_radar_receive->start();
}

void RadarEngine::RadarEngine::checkRange(uint new_range)
{
    uint cur_range = RadarConfig::getInstance("")->getConfig(VOLATILE_RADAR_PARAMS_RANGE_DATA_RANGE).toUInt();
    const uint cur_scale = RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_LAST_SCALE).toUInt();
//    const quint8 unit = static_cast<quint8>(RadarConfig::getInstance("")->getConfig(NON_VOLATILE_PPI_DISPLAY_UNIT).toUInt());

//    switch (unit) {
//    case 1:
//        cur_range /= KM_TO_NM;
//        break;
//    default:
//        break;
//    }
    if ((cur_range != static_cast<uint>(new_range)))
    {
        RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_RANGE_DATA_RANGE,new_range*2/10);
//        m_range_meters = new_range;
//        emit signal_range_change(new_range/10);
        resetSpokes();
        qDebug()<<Q_FUNC_INFO<<"detected spoke range change from "<<cur_range<<" to "<<new_range;
    }
    if ((cur_scale != static_cast<uint>(new_range*2/10)))
    {
        TriggerReqRangeChange(static_cast<int>(cur_scale));
        resetSpokes();
        qDebug()<<Q_FUNC_INFO<<"range mismatch "<<cur_scale<<" to "<<new_range;
    }
}

void RadarEngine::RadarEngine::receiveThreadReport(quint8 report_type, quint8 report_field, quint32 value)
{
    quint64 now = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    radar_timeout = now + WATCHDOG_TIMEOUT;
    RadarState prev_cur_radar_state = static_cast<RadarState>(RadarConfig::getInstance("")->getConfig(VOLATILE_RADAR_STATUS).toInt());
    RadarState cur_radar_state;

    switch (report_type)
    {
    case RADAR_STATE:
        cur_radar_state = static_cast<RadarState>(report_field);
        if(cur_radar_state == RADAR_TRANSMIT && TIMED_OUT(now,data_timeout))
        {
            cur_radar_state = RADAR_NO_SPOKE;
            resetSpokes();
        }
        if(cur_radar_state == RADAR_WAKING_UP)
        {
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_WAKINGUP_TIME,static_cast<quint8>(value));
        }

        if(prev_cur_radar_state != RADAR_TRANSMIT)
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_STATUS,static_cast<quint8>(cur_radar_state));

        qDebug()<<Q_FUNC_INFO<<"report status radar now"<<static_cast<RadarState>(report_field);
        qDebug()<<Q_FUNC_INFO<<"report status radar prev"<<prev_cur_radar_state;
        break;
    case RADAR_FILTER:
        switch (report_field)
        {
        case RADAR_GAIN:
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_FILTER_DATA_GAIN,static_cast<quint8>(value));
//            qDebug()<<Q_FUNC_INFO<<"report gain"<<filter.gain;
            emit SignalUpdateReport();
            break;
        case RADAR_RAIN:
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_FILTER_DATA_RAIN,static_cast<quint8>(value));
//            qDebug()<<Q_FUNC_INFO<<"report rain"<<filter.rain;
            emit SignalUpdateReport();
            break;
        case RADAR_SEA:
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_FILTER_DATA_SEA,static_cast<quint8>(value));
//            qDebug()<<Q_FUNC_INFO<<"report sea"<<filter.sea;
            emit SignalUpdateReport();
            break;
        case RADAR_TARGET_BOOST:
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_FILTER_DATA_TARGET_BOOST,static_cast<quint8>(value));
//            qDebug()<<Q_FUNC_INFO<<"report TargetBoost"<<filter.targetBoost;
            break;
        case RADAR_LOCAL_INTERFERENCE_REJECTION:
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_FILTER_DATA_INTERFERENCE,static_cast<quint8>(value));
//            qDebug()<<Q_FUNC_INFO<<"report local interference"<<filter.LInterference;
            break;
        case RADAR_TARGET_EXPANSION:
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_FILTER_DATA_TARGET_EXPAND,static_cast<quint8>(value));
//            qDebug()<<Q_FUNC_INFO<<"report argetExpan"<<filter.targetExpan;
            break;
        case RADAR_RANGE:
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_RANGE_DATA_RANGE,static_cast<uint>(value));
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
            int bearing = static_cast<int>(value) / 10;
            if (bearing > 180)
                bearing = bearing - 360;
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_ALIGN_DATA_BEARING,static_cast<uint>(bearing));
            qDebug()<<Q_FUNC_INFO<<"report radar bearing alignment"<<bearing;
            break;
        }
        case RADAR_ANTENA:
            // antenna height
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_ALIGN_DATA_BEARING,static_cast<uint>(value));
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
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_SCAN_DATA_SCAN_SPEED,value);
            qDebug()<<Q_FUNC_INFO<<"report radar scan_speed"<<value ;
            break;
        case RADAR_NOISE_REJECT:
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_SCAN_DATA_NOISE_REJECT,value);
            qDebug()<<Q_FUNC_INFO<<"report radar noise_reject"<<value ;
            break;
        case RADAR_TARGET_SEPARATION:
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_SCAN_DATA_TARGET_SEPARATION,value);
            qDebug()<<Q_FUNC_INFO<<"report radar target_sep"<<value;
            break;
        case RADAR_LOBE_SUPRESION:
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_SCAN_DATA_SIDE_LOBE_SUPPRESSION,value);
            qDebug()<<Q_FUNC_INFO<<"report radar side_lobe_suppression"<<value ; //0->auto
            break;
        case RADAR_INTERFERENT:
            RadarConfig::getInstance("")->setConfig(VOLATILE_RADAR_PARAMS_SCAN_DATA_LOCAL_INTERFERENCE,value);
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
