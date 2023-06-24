#include "radarconfig.h"

#include <QFile>
#include <QDir>
#include <QSettings>
#include <QDateTime>

RadarConfig::RadarConfig* RadarConfig::RadarConfig::instance{nullptr};
QStringList RadarConfig::RadarConfig::nonVolatileKeys =
        QStringList()<<NON_VOLATILE_PPI_DISPLAY_SHOW_RING
                    <<NON_VOLATILE_PPI_DISPLAY_SHOW_COMPASS
                   <<NON_VOLATILE_PPI_DISPLAY_SHOW_HEADING_MARKER
                  <<NON_VOLATILE_PPI_DISPLAY_HEADING_UP
                  <<NON_VOLATILE_PPI_DISPLAY_LAST_SCALE
                 <<NON_VOLATILE_PPI_DISPLAY_SHOW_GZ
                <<NON_VOLATILE_PPI_DISPLAY_SHOW_GZ1
                <<NON_VOLATILE_PPI_DISPLAY_SHOW_ARPA
               <<NON_VOLATILE_PPI_DISPLAY_USE_OPENGL_SOFTWARE
               <<NON_VOLATILE_PPI_DISPLAY_SHOW_SWEEP
               <<NON_VOLATILE_RADAR_NET_IP_DATA
              <<NON_VOLATILE_RADAR_NET_IP_REPORT
             <<NON_VOLATILE_RADAR_NET_IP_CMD
            <<NON_VOLATILE_RADAR_NET_PORT_DATA
           <<NON_VOLATILE_RADAR_NET_PORT_REPORT
          <<NON_VOLATILE_RADAR_NET_PORT_CMD
         <<NON_VOLATILE_RADAR_TRAIL_ENABLE
        <<NON_VOLATILE_RADAR_TRAIL_TIME
       <<NON_VOLATILE_ARPA_PARAMS_MIN_CONTOUR_LEN
      <<NON_VOLATILE_ARPA_PARAMS_SEARCH_RADIUS1
     <<NON_VOLATILE_ARPA_PARAMS_SEARCH_RADIUS2
    <<NON_VOLATILE_ARPA_PARAMS_MAX_TARGET_SIZE
   <<NON_VOLATILE_ARPA_CONTROL_CREATE_ARPA_BY_CLICK
  <<NON_VOLATILE_ARPA_NET_CONFIG
<<NON_VOLATILE_GZ_ENABLE_ALARM
<<NON_VOLATILE_GZ_MODE
<<NON_VOLATILE_GZ_TIMEOUT
<<NON_VOLATILE_GZ_NOTIF_THRESHOLD
<<NON_VOLATILE_GZ_START_BEARING
<<NON_VOLATILE_GZ_END_BEARING
<<NON_VOLATILE_GZ_START_RANGE
<<NON_VOLATILE_GZ_END_RANGE
<<NON_VOLATILE_GZ_ENABLE_ALARM1
<<NON_VOLATILE_GZ_MODE1
<<NON_VOLATILE_GZ_TIMEOUT1
<<NON_VOLATILE_GZ_NOTIF_THRESHOLD1
<<NON_VOLATILE_GZ_START_BEARING1
<<NON_VOLATILE_GZ_END_BEARING1
<<NON_VOLATILE_GZ_START_RANGE1
<<NON_VOLATILE_GZ_END_RANGE1
<<NON_VOLATILE_NAV_DATA_LAST_HEADING
<<NON_VOLATILE_NAV_DATA_LAST_LATITUDE
<<NON_VOLATILE_NAV_DATA_LAST_LONGITUDE
<<NON_VOLATILE_NAV_CONTROL_GPS_AUTO
<<NON_VOLATILE_NAV_CONTROL_HEADING_AUTO
<<NON_VOLATILE_NAV_NET_CONFIG
                      ;

RadarConfig::RadarConfig::RadarConfig(QObject *parent, QString path): QObject (parent), filePath(path)
{
    QFile file(filePath);
    if(filePath.isEmpty() || !file.exists())
    {
        filePath = QDir::homePath()+QDir::separator()+".radar.conf";
        file.setFileName(filePath);
    }

    if(!file.exists()) initConfig();
    else loadConfig();
}

bool RadarConfig::RadarConfig::setConfig(const QString &key, const QVariant &value)
{
    bool valid = false;
    if(volatileVar.contains(key))
    {
        valid = true;
        volatileVar.insert(key,value);
    }
    else if(nonVolatileVar.contains(key))
    {
        valid = true;
        nonVolatileVar.insert(key,value);
    }

    emit configValueChange(key,value);
    return valid;
}

QVariant RadarConfig::RadarConfig::getConfig(const QString &key) const
{
    QVariant val;
    if(volatileVar.contains(key)) val = volatileVar.value(key);
    else if(nonVolatileVar.contains(key)) val = nonVolatileVar.value(key);

    return val;
}

void RadarConfig::RadarConfig::loadConfig()
{
    qDebug()<<Q_FUNC_INFO;
    //volatile
    volatileVar.insert(VOLATILE_NAV_STATUS_GPS,0); //(0->offline, 1->no data, 2->data not valid, 3->data valid)
    volatileVar.insert(VOLATILE_NAV_STATUS_HEADING,0); //(0->offline, 1->no data, 2->data not valid, 3->data valid)

    volatileVar.insert(VOLATILE_GZ_CONFIRMED,false);
    volatileVar.insert(VOLATILE_GZ_TIME,QDateTime::currentSecsSinceEpoch());

    volatileVar.insert(VOLATILE_GZ_CONFIRMED1,false);
    volatileVar.insert(VOLATILE_GZ_TIME1,QDateTime::currentSecsSinceEpoch());

    volatileVar.insert(VOLATILE_DISPLAY_PRESET_COLOR,0); //display mode index (0 -> day, 1 -> night)

    volatileVar.insert(VOLATILE_RADAR_PARAMS_FILTER_DATA_GAIN,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_FILTER_DATA_RAIN,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_FILTER_DATA_SEA,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_FILTER_DATA_MTI,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_FILTER_CONTROL_GAIN,true); //auto
    volatileVar.insert(VOLATILE_RADAR_PARAMS_FILTER_CONTROL_RAIN,true); //auto
    volatileVar.insert(VOLATILE_RADAR_PARAMS_FILTER_CONTROL_SEA,true); //auto
    volatileVar.insert(VOLATILE_RADAR_PARAMS_FILTER_CONTROL_MTI,true); //enable
    volatileVar.insert(VOLATILE_RADAR_PARAMS_FILTER_DATA_TARGET_BOOST,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_FILTER_DATA_INTERFERENCE,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_FILTER_DATA_TARGET_EXPAND,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_ALIGN_DATA_BEARING,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_ALIGN_DATA_ANTENA_HEIGHT,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_SCAN_DATA_SCAN_SPEED,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_SCAN_DATA_NOISE_REJECT,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_SCAN_DATA_TARGET_SEPARATION,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_SCAN_DATA_SIDE_LOBE_SUPPRESSION,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_SCAN_DATA_LOCAL_INTERFERENCE,0);
    volatileVar.insert(VOLATILE_RADAR_PARAMS_RANGE_DATA_RANGE,0);
    volatileVar.insert(VOLATILE_RADAR_STATUS,0);
    volatileVar.insert(VOLATILE_RADAR_WAKINGUP_TIME,0);

    /*non volatile*/
    QSettings config(filePath,QSettings::IniFormat);
    foreach (QString key, config.allKeys())
    {
        nonVolatileVar.insert(key,config.value(key));
    }

    /*match desired keys*/
    QStringList nonVolatileVar_keys = nonVolatileVar.keys();
    QStringList buf_nonVolatileKeys = nonVolatileKeys;
    int i = 0;

    while (i < nonVolatileVar_keys.size())
    {
//        qDebug()<<Q_FUNC_INFO<<"nonVolatileVar keys"<<nonVolatileVar_keys.at(i);
        if(!buf_nonVolatileKeys.contains(nonVolatileVar_keys.at(i))) qWarning()<<Q_FUNC_INFO<<"undesired key"<<nonVolatileVar_keys.at(i);
        else buf_nonVolatileKeys.removeOne(nonVolatileVar_keys.at(i));
        i++;
    }
    foreach (QString key, buf_nonVolatileKeys) {
        qWarning()<<Q_FUNC_INFO<<"key not found"<<key;
    }
}
void RadarConfig::RadarConfig::initConfig()
{
    qDebug()<<Q_FUNC_INFO;
    //non volatile
    nonVolatileVar.insert(NON_VOLATILE_PPI_DISPLAY_SHOW_RING,false);
    nonVolatileVar.insert(NON_VOLATILE_PPI_DISPLAY_HEADING_UP,false);
    nonVolatileVar.insert(NON_VOLATILE_PPI_DISPLAY_SHOW_COMPASS,true);
    nonVolatileVar.insert(NON_VOLATILE_PPI_DISPLAY_SHOW_HEADING_MARKER,true);
    nonVolatileVar.insert(NON_VOLATILE_PPI_DISPLAY_LAST_SCALE,2000);
    nonVolatileVar.insert(NON_VOLATILE_PPI_DISPLAY_SHOW_GZ,true);
    nonVolatileVar.insert(NON_VOLATILE_PPI_DISPLAY_SHOW_GZ1,true);
    nonVolatileVar.insert(NON_VOLATILE_PPI_DISPLAY_SHOW_SWEEP,false);
    nonVolatileVar.insert(NON_VOLATILE_PPI_DISPLAY_SHOW_SWEEP,true);
    nonVolatileVar.insert(NON_VOLATILE_PPI_DISPLAY_SHOW_ARPA,true);

    nonVolatileVar.insert(NON_VOLATILE_RADAR_NET_IP_DATA,"127.0.0.1");
    nonVolatileVar.insert(NON_VOLATILE_RADAR_NET_IP_REPORT,"127.0.0.1");
    nonVolatileVar.insert(NON_VOLATILE_RADAR_NET_IP_CMD,"127.0.0.1");
    nonVolatileVar.insert(NON_VOLATILE_RADAR_NET_PORT_DATA,6132);
    nonVolatileVar.insert(NON_VOLATILE_RADAR_NET_PORT_REPORT,6133);
    nonVolatileVar.insert(NON_VOLATILE_RADAR_NET_PORT_CMD,6131);
    nonVolatileVar.insert(NON_VOLATILE_RADAR_TRAIL_ENABLE,true);
    nonVolatileVar.insert(NON_VOLATILE_RADAR_TRAIL_TIME,0); //index

    nonVolatileVar.insert(NON_VOLATILE_ARPA_PARAMS_MIN_CONTOUR_LEN,90);
    nonVolatileVar.insert(NON_VOLATILE_ARPA_PARAMS_SEARCH_RADIUS1,10);
    nonVolatileVar.insert(NON_VOLATILE_ARPA_PARAMS_SEARCH_RADIUS2,50);
    nonVolatileVar.insert(NON_VOLATILE_ARPA_PARAMS_MAX_TARGET_SIZE,50);
    nonVolatileVar.insert(NON_VOLATILE_ARPA_CONTROL_CREATE_ARPA_BY_CLICK,true);
    nonVolatileVar.insert(NON_VOLATILE_ARPA_NET_CONFIG,"mqtt;InOut;127.0.0.1:1883:radar");

    nonVolatileVar.insert(NON_VOLATILE_GZ_ENABLE_ALARM,true);
    nonVolatileVar.insert(NON_VOLATILE_GZ_MODE,0); //arc mode
    nonVolatileVar.insert(NON_VOLATILE_GZ_NOTIF_THRESHOLD,10);
    nonVolatileVar.insert(NON_VOLATILE_GZ_TIMEOUT,90);
    nonVolatileVar.insert(NON_VOLATILE_GZ_START_BEARING,70);
    nonVolatileVar.insert(NON_VOLATILE_GZ_END_BEARING,120);
    nonVolatileVar.insert(NON_VOLATILE_GZ_START_RANGE,2000);
    nonVolatileVar.insert(NON_VOLATILE_GZ_END_RANGE,4200);

    nonVolatileVar.insert(NON_VOLATILE_GZ_ENABLE_ALARM1,true);
    nonVolatileVar.insert(NON_VOLATILE_GZ_MODE1,0); //arc mode
    nonVolatileVar.insert(NON_VOLATILE_GZ_NOTIF_THRESHOLD1,10);
    nonVolatileVar.insert(NON_VOLATILE_GZ_TIMEOUT1,90);
    nonVolatileVar.insert(NON_VOLATILE_GZ_START_BEARING1,20);
    nonVolatileVar.insert(NON_VOLATILE_GZ_END_BEARING1,100);
    nonVolatileVar.insert(NON_VOLATILE_GZ_START_RANGE1,1000);
    nonVolatileVar.insert(NON_VOLATILE_GZ_END_RANGE1,2200);

    nonVolatileVar.insert(NON_VOLATILE_NAV_DATA_LAST_HEADING,0.0);
    nonVolatileVar.insert(NON_VOLATILE_NAV_DATA_LAST_LATITUDE,0.0);
    nonVolatileVar.insert(NON_VOLATILE_NAV_DATA_LAST_LONGITUDE,0.0);
    nonVolatileVar.insert(NON_VOLATILE_NAV_CONTROL_GPS_AUTO,true);
    nonVolatileVar.insert(NON_VOLATILE_NAV_CONTROL_HEADING_AUTO,true);
    nonVolatileVar.insert(NON_VOLATILE_NAV_NET_CONFIG,"mqtt;InOut;127.0.0.1:1883:gps");

}

void RadarConfig::RadarConfig::saveConfig() const
{
    qDebug()<<Q_FUNC_INFO<<filePath;

    QSettings config(filePath,QSettings::IniFormat);
    QMapIterator<QString, QVariant>i(nonVolatileVar);

    while (i.hasNext())
    {
        i.next();
        config.setValue(i.key(),i.value());
    }
}

RadarConfig::RadarConfig::~RadarConfig()
{
    saveConfig();
}
RadarConfig::RadarConfig* RadarConfig::RadarConfig::getInstance(const QString& path )
{
    if(instance == nullptr) instance = new RadarConfig(nullptr, path);
    else
    {
        if(instance->filePath != path && !path.isEmpty()) return nullptr;
    }

    return  instance;
}
