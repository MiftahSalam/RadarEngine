#include "radarimagecapture.h"

#include "global.h"
#include "qimage.h"
#include "qopenglfunctions.h"
#include "qpixmap.h"
#include "qapplication.h"
#include "qdir.h"
#include <QDebug>
#include <QBuffer>

#include "constants.h"
#include "radarconfig.h"

RadarEngine::RadarEngine *RadarEngine::RadarImageCapture::m_re{nullptr};

RadarEngine::RadarImageCapture::RadarImageCapture(QObject *parent, RadarEngine *re): QObject{parent}, currentAngle(0), grabStart(false), grabPending(false)
{
    if(m_re)
    {
        qWarning()<<"cannot create more than two instance";
        exit(1);
    }

    m_re = re;
}

QBuffer* RadarEngine::RadarImageCapture::readPixel(int width, int height)
{
    int size = qMin(width, height); //scale 1
    int buffer_size = size*size*4;
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    uchar *buffer = new uchar[buffer_size];

    f->glReadPixels((width - size) / 2, (height - size) / 2, size, size, GL_BGRA, GL_UNSIGNED_BYTE, buffer);

    QBuffer* buf = new QBuffer();
    buf->setData((const char*)buffer, buffer_size);

    return buf;
}

RadarEngine::CaptureResult RadarEngine::RadarImageCapture::capture(QImage data)
{
    CaptureResult result;

    if(grabStart)
    {
        result = processCapture(data);
        grabPending = false;
    }
    else qWarning()<<Q_FUNC_INFO<<"Grab not start";

    return result;
}

RadarEngine::CaptureResult RadarEngine::RadarImageCapture::capture(QBuffer *data, int width, int height)
{
    CaptureResult result;

    if(grabStart)
    {
        int size = qMin(width, height); //scale 1
        QImage image((uchar*)data->data().data(), size, size, QImage::Format_ARGB32);
        QTransform tr;
        tr.rotate(90.);

        image = image.transformed(tr);
        result = processCapture(image);

        grabPending = false;
    }
    else qWarning()<<Q_FUNC_INFO<<"Grab not start";

    return result;
}

RadarEngine::CaptureResult RadarEngine::RadarImageCapture::processCapture(QImage image)
{
    CaptureResult result;

    QByteArray ba;
    QBuffer buf(&ba);
    QString strBase64;

    buf.open(QIODevice::WriteOnly);
    image.save(&buf, "png");

    strBase64 = QString(ba.toBase64(QByteArray::Base64Encoding));

#ifdef SAVE_CAPTURE
    /* test read and save from base64 image*/
    QByteArray ba64 = QByteArray::fromBase64(strBase64.toUtf8());
    QPixmap img;
    img.loadFromData(ba64);
    img.save(qApp->applicationDirPath()+QDir::separator()+"base64_grab.png", "png");
    //        image.save(qApp->applicationDirPath()+"/grab.png");
    //        grabStart = false; //test
#endif

    //        emit signalSendEcho(strBase64, data.width(), data.height());

    result.echo = strBase64;
    result.width = image.width();
    result.height = image.height();

    return result;
}

bool RadarEngine::RadarImageCapture::pendingGrabAvailable() const
{
    return grabPending;
}

void RadarEngine::RadarImageCapture::update()
{
    if(grabStart)
    {
        currentAngle++;
        if(currentAngle >= LINES_PER_ROTATION)
        {
            currentAngle = 0;
            grabPending = true;
        }

        //        qDebug()<<Q_FUNC_INFO<<"currentAngle"<<currentAngle<<"LINES_PER_ROTATION"<<LINES_PER_ROTATION-1;
    }
    else qWarning()<<Q_FUNC_INFO<<"Grab not start";
}

void RadarEngine::RadarImageCapture::stop()
{
    grabStart = false;
    currentAngle = 0;
    grabPending = false;
}

bool RadarEngine::RadarImageCapture::isStart() const
{
    return grabStart;
}

void RadarEngine::RadarImageCapture::start()
{
    grabStart = true;
    grabPending = false;
    currentAngle = 0;
}

void RadarEngine::RadarImageCapture::trigger_radarConfigChange(QString key, QVariant val)
{
    if(key == VOLATILE_RADAR_STATUS) stateChange(val.toInt());
}

void RadarEngine::RadarImageCapture::stateChange(int state) {
    grabStart = static_cast<RadarState>(state) == RADAR_TRANSMIT;
}
