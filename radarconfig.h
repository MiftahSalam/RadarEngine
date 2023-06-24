#ifndef RADARCONFIG_H
#define RADARCONFIG_H

#include "radarconfig_global.h"
#include <QVariant>
#include <QObject>

namespace RadarConfig {

const QString NON_VOLATILE_PPI_DISPLAY_SHOW_RING("PPI/Display/show_ring");
const QString NON_VOLATILE_PPI_DISPLAY_SHOW_COMPASS = "PPI/Display/show_compass";
const QString NON_VOLATILE_PPI_DISPLAY_SHOW_HEADING_MARKER = "PPI/Display/show_heading_marker";
const QString NON_VOLATILE_PPI_DISPLAY_HEADING_UP = "PPI/Display/heading_up";
const QString NON_VOLATILE_PPI_DISPLAY_LAST_SCALE = "PPI/Display/last_scale";
const QString NON_VOLATILE_PPI_DISPLAY_SHOW_GZ = "PPI/Display/show_gz";
const QString NON_VOLATILE_PPI_DISPLAY_SHOW_GZ1 = "PPI/Display/show_gz1";
const QString NON_VOLATILE_PPI_DISPLAY_SHOW_SWEEP = "PPI/Display/show_sweep";
const QString NON_VOLATILE_PPI_DISPLAY_SHOW_ARPA = "PPI/Display/show_arpa";
const QString NON_VOLATILE_PPI_DISPLAY_USE_OPENGL_SOFTWARE = "PPI/Display/use_opengl_software";

const QString NON_VOLATILE_RADAR_NET_IP_DATA = "Radar/net/ip_data";
const QString NON_VOLATILE_RADAR_NET_IP_REPORT = "Radar/net/ip_report";
const QString NON_VOLATILE_RADAR_NET_IP_CMD = "Radar/net/ip_command";
const QString NON_VOLATILE_RADAR_NET_PORT_DATA = "Radar/net/port_data";
const QString NON_VOLATILE_RADAR_NET_PORT_REPORT = "Radar/net/port_report";
const QString NON_VOLATILE_RADAR_NET_PORT_CMD = "Radar/net/port_command";
const QString NON_VOLATILE_RADAR_TRAIL_ENABLE = "Radar/trail/enable";
const QString NON_VOLATILE_RADAR_TRAIL_TIME = "Radar/trail/time";

const QString NON_VOLATILE_ARPA_PARAMS_MIN_CONTOUR_LEN = "Arpa/params/min_contour_len";
const QString NON_VOLATILE_ARPA_PARAMS_SEARCH_RADIUS1 = "Arpa/params/search_radius1";
const QString NON_VOLATILE_ARPA_PARAMS_SEARCH_RADIUS2 = "Arpa/params/search_radius2";
const QString NON_VOLATILE_ARPA_PARAMS_MAX_TARGET_SIZE = "Arpa/params/max_target_size";
const QString NON_VOLATILE_ARPA_CONTROL_CREATE_ARPA_BY_CLICK = "Arpa/control/create_arpa_by_click";
const QString NON_VOLATILE_ARPA_NET_CONFIG = "Arpa/net/config";

const QString NON_VOLATILE_GZ_ENABLE_ALARM = "GZ/enable_alarm";
const QString NON_VOLATILE_GZ_MODE = "GZ/mode";
const QString NON_VOLATILE_GZ_NOTIF_THRESHOLD = "GZ/notif_threshold";
const QString NON_VOLATILE_GZ_TIMEOUT = "GZ/timeout";
const QString NON_VOLATILE_GZ_START_BEARING = "GZ/start_bearing";
const QString NON_VOLATILE_GZ_END_BEARING = "GZ/end_bearing";
const QString NON_VOLATILE_GZ_START_RANGE = "GZ/start_range";
const QString NON_VOLATILE_GZ_END_RANGE = "GZ/end_range";

const QString NON_VOLATILE_GZ_ENABLE_ALARM1 = "GZ/enable_alarm1";
const QString NON_VOLATILE_GZ_MODE1 = "GZ/mode1";
const QString NON_VOLATILE_GZ_NOTIF_THRESHOLD1 = "GZ/notif_threshold1";
const QString NON_VOLATILE_GZ_TIMEOUT1 = "GZ/timeout1";
const QString NON_VOLATILE_GZ_START_BEARING1 = "GZ/start_bearing1";
const QString NON_VOLATILE_GZ_END_BEARING1 = "GZ/end_bearing1";
const QString NON_VOLATILE_GZ_START_RANGE1 = "GZ/start_range1";
const QString NON_VOLATILE_GZ_END_RANGE1 = "GZ/end_range1";

const QString NON_VOLATILE_NAV_DATA_LAST_HEADING = "Nav/data/last_heading";
const QString NON_VOLATILE_NAV_DATA_LAST_LATITUDE = "Nav/data/last_lat";
const QString NON_VOLATILE_NAV_DATA_LAST_LONGITUDE = "Nav/data/last_lon";
const QString NON_VOLATILE_NAV_CONTROL_GPS_AUTO = "Nav/control/gps_auto";
const QString NON_VOLATILE_NAV_CONTROL_HEADING_AUTO = "Nav/control/heading_auto";
const QString NON_VOLATILE_NAV_NET_CONFIG = "Nav/net/config";

const QString VOLATILE_NAV_STATUS_HEADING = "Nav/status/heading";
const QString VOLATILE_NAV_STATUS_GPS = "Nav/status/gps";

const QString VOLATILE_GZ_CONFIRMED = "GZ/confirmed";
const QString VOLATILE_GZ_TIME = "GZ/time";

const QString VOLATILE_GZ_CONFIRMED1 = "GZ/confirmed1";
const QString VOLATILE_GZ_TIME1 = "GZ/time1";

const QString VOLATILE_DISPLAY_PRESET_COLOR = "Radar/display/preset_color";

const QString VOLATILE_RADAR_PARAMS_FILTER_DATA_GAIN = "Radar/params/filter/data/gain";
const QString VOLATILE_RADAR_PARAMS_FILTER_DATA_RAIN = "Radar/params/filter/data/rain";
const QString VOLATILE_RADAR_PARAMS_FILTER_DATA_SEA = "Radar/params/filter/data/sea";
const QString VOLATILE_RADAR_PARAMS_FILTER_DATA_MTI = "Radar/params/filter/data/mti";
const QString VOLATILE_RADAR_PARAMS_FILTER_CONTROL_GAIN = "Radar/params/filter/control/gain";
const QString VOLATILE_RADAR_PARAMS_FILTER_CONTROL_RAIN = "Radar/params/filter/control/rain";
const QString VOLATILE_RADAR_PARAMS_FILTER_CONTROL_SEA = "Radar/params/filter/control/sea";
const QString VOLATILE_RADAR_PARAMS_FILTER_CONTROL_MTI = "Radar/params/filter/control/mti";
const QString VOLATILE_RADAR_PARAMS_FILTER_DATA_TARGET_BOOST = "Radar/params/filter/data/targetBoost";
const QString VOLATILE_RADAR_PARAMS_FILTER_DATA_INTERFERENCE = "Radar/params/filter/data/LInterference";
const QString VOLATILE_RADAR_PARAMS_FILTER_DATA_TARGET_EXPAND = "Radar/params/filter/data/targetExpan";
const QString VOLATILE_RADAR_PARAMS_ALIGN_DATA_BEARING = "Radar/params/align/data/bearing";
const QString VOLATILE_RADAR_PARAMS_ALIGN_DATA_ANTENA_HEIGHT = "Radar/params/align/data/antena_height";
const QString VOLATILE_RADAR_PARAMS_SCAN_DATA_SCAN_SPEED = "Radar/params/scan/data/scan_speed";
const QString VOLATILE_RADAR_PARAMS_SCAN_DATA_NOISE_REJECT = "Radar/params/scan/data/noise_reject";
const QString VOLATILE_RADAR_PARAMS_SCAN_DATA_TARGET_SEPARATION = "Radar/params/scan/data/target_sep";
const QString VOLATILE_RADAR_PARAMS_SCAN_DATA_SIDE_LOBE_SUPPRESSION = "Radar/params/scan/data/side_lobe_suppression";
const QString VOLATILE_RADAR_PARAMS_SCAN_DATA_LOCAL_INTERFERENCE = "Radar/params/scan/data/local_interference_rejection";
const QString VOLATILE_RADAR_PARAMS_RANGE_DATA_RANGE = "Radar/params/range/data/range";
const QString VOLATILE_RADAR_STATUS = "Radar/STATUS";
const QString VOLATILE_RADAR_WAKINGUP_TIME = "Radar/WAKINGUP_TIME";

class RadarConfig: public QObject
{
    Q_OBJECT
public:
    RadarConfig(RadarConfig& other) = delete;
    void operator=(const RadarConfig&) = delete;

    static RadarConfig* getInstance(const QString &path);
    bool setConfig(const QString& key, const QVariant& value);
    QVariant getConfig(const QString& key) const;
    void saveConfig() const;

signals:
    void configValueChange(QString key, QVariant value);

protected:
    RadarConfig(QObject *parent=nullptr, QString path="");
    ~RadarConfig() override;

private:
    void initConfig();
    void loadConfig();

    static RadarConfig* instance;
    static QStringList nonVolatileKeys;

    QString filePath;
    QMap<QString,QVariant> nonVolatileVar, volatileVar;
};
}

#endif // RADARCONFIG_H
