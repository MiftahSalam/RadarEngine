#include "radardraw.h"
#include "radarengine.h"

#include <QOpenGLFunctions>

using namespace RadarEngine;

P2CLookupTable *lookupTable = nullptr;
P2CLookupTable* GetP2CLookupTable()
{
    if (!lookupTable)
    {
        lookupTable = static_cast<P2CLookupTable*>(malloc(sizeof(P2CLookupTable)));

        if (!lookupTable)
        {
            qDebug()<<Q_FUNC_INFO<<"Out Of Memory, fatal!";
            exit(1);
        }

        for (int arc = 0; arc < LINES_PER_ROTATION + 1; arc++)
        {
            GLfloat sine = cosf(static_cast<GLfloat>(arc * M_PI * 2 / LINES_PER_ROTATION));
            GLfloat cosine = sinf(static_cast<GLfloat>(arc * M_PI * 2 / LINES_PER_ROTATION));
            for (int radius = 0; radius < RETURNS_PER_LINE + 1; radius++)
            {
                lookupTable->x[arc][radius] = static_cast<GLfloat>(radius) * cosine/RETURNS_PER_LINE;
                lookupTable->y[arc][radius] = static_cast<GLfloat>(radius) * sine/RETURNS_PER_LINE;
                lookupTable->intx[arc][radius] = static_cast<int>(lookupTable->x[arc][radius]*RETURNS_PER_LINE);
                lookupTable->inty[arc][radius] = static_cast<int>(lookupTable->y[arc][radius]*RETURNS_PER_LINE);
//                qDebug()<<Q_FUNC_INFO<<"arc"<<arc<<"rad"<<radius<<"x"<<lookupTable->intx[arc][radius]<<"y"<<lookupTable->inty[arc][radius];
            }
        }
    }
    return lookupTable;
}

QString RadarDraw::methods = "Vertex Array";

// Factory to generate a particular draw implementation
RadarDraw* RadarDraw::make_Draw(RadarEngine *ri, int draw_method)
{
    qDebug()<<Q_FUNC_INFO;
    switch (draw_method)
    {
    case 0:
        methods = "Vertex Array";
        return new RDVert(ri);
    case 1:
        methods = "Shader";
        return new RDShader(ri);
    default:
        qDebug()<<Q_FUNC_INFO<<"unsupported draw method "<<draw_method;
    }
    return nullptr;
}

RadarDraw::~RadarDraw() {}

QString RadarDraw::GetDrawingMethods() {

    return methods;
}

#define ADD_VERTEX_POINT(angle, radius, r, g, b, a)          \
{                                                          \
    line->points[count].x = m_polarLookup->x[angle][radius]; \
    line->points[count].y = m_polarLookup->y[angle][radius]; \
    line->points[count].red = r;                             \
    line->points[count].green = g;                           \
    line->points[count].blue = b;                            \
    line->points[count].alpha = a;                           \
    count++;                                                 \
}

static const char *vertexShaderSource =
      "attribute highp vec4 posAttr;\n"
      "attribute lowp vec4 colAttr;\n"
      "varying lowp vec4 col;\n"
      "void main() {\n"
      "   col = colAttr;\n"
      "   gl_Position = posAttr;\n"
      "}\n";

  static const char *fragmentShaderSource =
      "varying lowp vec4 col;\n"
      "void main() {\n"
      "   gl_FragColor = col;\n"
      "}\n";

RDVert::RDVert(RadarEngine* re):
    m_ri(re)
{
    for (size_t i = 0; i < ARRAY_SIZE(m_vertices); i++)
    {
        m_vertices[i].count = 0;
        m_vertices[i].allocated = 0;
        m_vertices[i].timeout = 0;
        m_vertices[i].points = nullptr;
    }
    m_count = 0;
    m_oom = false;

    m_polarLookup = GetP2CLookupTable();

}

void RDVert::init(QObject *parent)
{
    m_program = new QOpenGLShaderProgram(parent);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->link();
    m_posAttr = static_cast<GLuint>(m_program->attributeLocation("posAttr"));
    m_colAttr = static_cast<GLuint>(m_program->attributeLocation("colAttr"));

}
void RDVert::SetBlob(VertexLine* line, int angle_begin, int angle_end, int r1, int r2, GLubyte red, GLubyte green,
                     GLubyte blue, GLubyte alpha)
{
    //    qDebug()<<Q_FUNC_INFO<<"angle_begin "<<angle_begin<<"angle_end "<<angle_end<<"r1 "<<r1<<"r2 "<<r2
    //           <<"red "<<red<<"green "<<green<<"blue "<<blue<<"alpha "<<alpha;
    if (r2 == 0)
    {
        return;
    }
    int arc1 = MOD_ROTATION2048(angle_begin);
    int arc2 = MOD_ROTATION2048(angle_end);
    size_t count = line->count;

    if (line->count + VERTEX_PER_QUAD > line->allocated)
    {
        const size_t extra = 8 * VERTEX_PER_QUAD;
        line->points = static_cast<VertexPoint*>(realloc(line->points, (line->allocated + extra) * sizeof(VertexPoint)));
        line->allocated += extra;
        m_count += extra;
        qDebug()<<"extra loc";
    }

    if (!line->points)
    {
        if (!m_oom)
        {
            qDebug()<<"BR24radar_pi: Out of memory";
            m_oom = true;
        }
        return;
    }

    // First triangle
    //    qDebug()<<Q_FUNC_INFO<<arc1<<arc2<<r1<<r2;
    ADD_VERTEX_POINT(arc1, r1, red, green, blue, alpha)
    ADD_VERTEX_POINT(arc1, r2, red, green, blue, alpha)
    ADD_VERTEX_POINT(arc2, r1, red, green, blue, alpha)

    // Second triangle

    ADD_VERTEX_POINT(arc2, r1, red, green, blue, alpha)
    ADD_VERTEX_POINT(arc1, r2, red, green, blue, alpha)
    ADD_VERTEX_POINT(arc2, r2, red, green, blue, alpha)

    line->count = count;
    //    for(int i=0;i<line->count;i++)
    //        qDebug()<<"count" <<i<<"x "<<line->points[i].x<<"y "<<line->points[i].y
    //               <<"red"<<line->points[i].red<<"green"<<line->points[i].green<<"blue"<<line->points[i].blue<<"alpha"<<line->points[i].alpha;

}

void RDVert::ProcessRadarSpoke(int angle, quint8 *data, size_t len)
{
    QColor colour;
    BlobColour previous_colour = BLOB_NONE;
    GLubyte strength = 0;
    qint64 now = QDateTime::currentMSecsSinceEpoch();

//    qDebug()<<Q_FUNC_INFO;

    int r_begin = 0;
    int r_end = 0;

    if (angle < 0 || angle >= LINES_PER_ROTATION)
    {
        return;
    }

    VertexLine* line = &m_vertices[angle];

    if (!line->points)
    {
        static size_t INITIAL_ALLOCATION = 600;
        line->allocated = INITIAL_ALLOCATION * VERTEX_PER_QUAD;
        m_count += INITIAL_ALLOCATION * VERTEX_PER_QUAD;
        line->points = static_cast<VertexPoint*>(malloc(line->allocated * sizeof(VertexPoint)));
        if (!line->points)
        {
            if (!m_oom)
            {
                qDebug()<<"BR24radar_pi: Out of memory";
                m_oom = true;
            }
            line->allocated = 0;
            line->count = 0;
            return;
        }
        //        qDebug()<<"init loc";

    }
    line->count = 0;
    line->timeout = now + 6000;
    //    line->timeout = now + m_ri->m_pi->m_settings.max_age;

    for (size_t radius = 0; radius < len; radius++)
    {
        strength = data[radius];

        BlobColour actual_colour = m_ri->m_colour_map[strength];
        //        qDebug()<<Q_FUNC_INFO<<"strength "<<strength<<"actual color "<<actual_colour;

        if (actual_colour == previous_colour)
        {
            // continue with same color, just register it
            r_end++;
        }
        else if (previous_colour == BLOB_NONE && actual_colour != BLOB_NONE)
        {
            // blob starts, no display, just register
            r_begin = static_cast<int>(radius);
            r_end = r_begin + 1;
            previous_colour = actual_colour;  // new color
        }
        else if (previous_colour != BLOB_NONE && (previous_colour != actual_colour))
        {
            colour = m_ri->m_colour_map_rgb[previous_colour];

            SetBlob(line, angle, angle + 1, r_begin, r_end, static_cast<GLubyte>(colour.red()), static_cast<GLubyte>(colour.green()), static_cast<GLubyte>(colour.blue()), static_cast<GLubyte>(colour.alpha()));

            previous_colour = actual_colour;
            if (actual_colour != BLOB_NONE)
            {  // change of color, start new blob
                r_begin = static_cast<int>(radius);
                r_end = r_begin + 1;
            }
        }

    }

    if (previous_colour != BLOB_NONE)
    {  // Draw final blob
        colour = m_ri->m_colour_map_rgb[previous_colour];

        SetBlob(line, angle, angle + 1, r_begin, r_end, static_cast<GLubyte>(colour.red()), static_cast<GLubyte>(colour.green()), static_cast<GLubyte>(colour.blue()), static_cast<GLubyte>(colour.alpha()));
    }
}

void RDVert::DrawRadarImage()
{
//    glEnableClientState(GL_VERTEX_ARRAY);
//    glEnableClientState(GL_COLOR_ARRAY);

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    m_program->bind();

    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    f->glEnableVertexAttribArray(0);
    f->glEnableVertexAttribArray(1);

    //    qDebug()<<Q_FUNC_INFO;
//    quint64 now = QDateTime::currentMSecsSinceEpoch();

    for (size_t i = 0; i < LINES_PER_ROTATION; i++)
    {
        VertexLine* line = &m_vertices[i];
        /*
        if (!line->count || TIMED_OUT(now, line->timeout))
        {
            continue;
        }
        */
//        glVertexPointer(2, GL_FLOAT, sizeof(VertexPoint), &line->points[0].x);
//        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexPoint), &line->points[0].red);
//        glDrawArrays(GL_TRIANGLES, 0, line->count);
        f->glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPoint), &line->points[0].x);
        f->glVertexAttribPointer(m_colAttr, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(VertexPoint), &line->points[0].red);
        f->glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(line->count));
    }

    f->glDisableVertexAttribArray(1);  // disable vertex arrays
    f->glDisableVertexAttribArray(0);
    f->glDisable(GL_BLEND);
    m_program->release();
//    glDisableClientState(GL_VERTEX_ARRAY);  // disable vertex arrays
//    glDisableClientState(GL_COLOR_ARRAY);
}

void RDVert::DrawRadarSweep(double angle)
{
        GLfloat vertices[] = {
            0.0f, 0.0f,
            0.f, 1.f,
        };

        const int preset_color = RadarConfig::RadarConfig::getInstance("")->getConfig(RadarConfig::VOLATILE_DISPLAY_PRESET_COLOR).toInt();
        GLfloat colors[3][3] = {
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f}
        };
        if(preset_color == 0)
        {
            colors[0][0] = 1.0f;
            colors[1][0] = 1.0f;
            colors[2][0] = 1.0f;
            colors[0][1] = 1.0f;
            colors[1][1] = 1.0f;
            colors[2][1] = 1.0f;
        }
        else if(preset_color == 1)
        {
            colors[0][0] = .0f;
            colors[1][0] = .0f;
            colors[2][0] = .0f;
            colors[0][1] = 1.0f;
            colors[1][1] = 1.0f;
            colors[2][1] = 1.0f;
        }

        vertices[2] = sin(static_cast<float>(deg2rad(angle)));
        vertices[3] = cos(static_cast<float>(deg2rad(angle)));

        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

        m_program->bind();
        f->glEnableVertexAttribArray(0);
        f->glEnableVertexAttribArray(1);

        f->glVertexAttribPointer(m_posAttr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
        f->glVertexAttribPointer(m_colAttr, 3, GL_FLOAT, GL_FALSE, 0, colors);

        f->glDrawArrays(GL_LINES, 0, 2);

        f->glDisableVertexAttribArray(1);  // disable vertex arrays
        f->glDisableVertexAttribArray(0);
        m_program->release();
}

RDVert::~RDVert()
{
    delete m_program;
    for (size_t i = 0; i < LINES_PER_ROTATION; i++)
    {
        if (m_vertices[i].points)
        {
            free(m_vertices[i].points);
        }
    }
}


static const char *VertexShaderText =
    "void main() \n"
    "{ \n"
    "   gl_TexCoord[0] = gl_MultiTexCoord0; \n"
    "   gl_Position = ftransform(); \n"
    "} \n";

static const char *FragmentShaderColorText =
    "uniform sampler2D tex2d; \n"
    "void main() \n"
    "{ \n"
    "   float d = length(gl_TexCoord[0].xy);\n"
    "   if (d >= 1.0) \n"
    "      discard; \n"
    "   float a = atan(gl_TexCoord[0].x, gl_TexCoord[0].y) / 6.28318; \n"
    "   gl_FragColor = texture2D(tex2d, vec2(d, a)); \n"
    "} \n";

RDShader::RDShader(RadarEngine *re):
    m_re(re)
{
}

RDShader::~RDShader()
{
    delete m_program;
    delete m_fragment;
    delete m_vertex;
}

void RDShader::init(QObject *parent)
{
    m_start_line = -1;  // No spokes received since last draw
    m_lines = 0;
    m_format = GL_BGRA;
//    m_format = QOpenGLTexture::RGBA8_UNorm;
    m_channels = SHADER_COLOR_CHANNELS;

    m_vertex = new QOpenGLShader(QOpenGLShader::Vertex);
    m_vertex->compileSourceCode(VertexShaderText);

    m_fragment = new QOpenGLShader(QOpenGLShader::Fragment);
    m_fragment->compileSourceCode(FragmentShaderColorText);

    m_program = new QOpenGLShaderProgram(parent);
    m_program->addShader(m_vertex);
    m_program->addShader(m_fragment);
//    m_program->bindAttributeLocation("gl_TexCoord[0]", 0);
    //          m_environmentProgram->bindAttributeLocation("aTexCoord", 1);
    m_program->link();
    m_program->bind();
    m_program->setUniformValue("tex2d", 0);

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    f->glGenTextures(1,&m_texture);
    f->glBindTexture(GL_TEXTURE_2D, m_texture);
    f->glTexImage2D(/* target          = */ GL_TEXTURE_2D,
                  /* level           = */ 0,
                  /* internal_format = */ m_format,
                  /* width           = */ RETURNS_PER_LINE,
                  /* heigth          = */ LINES_PER_ROTATION,
                  /* border          = */ 0,
                  /* format          = */ static_cast<GLenum>(m_format),
                  /* type            = */ GL_UNSIGNED_BYTE,
                  /* data            = */ m_data);
     f->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /*
    m_texture = new QOpenGLTexture(QImage(QDir::homePath()+QDir::separator()+"side1.png").mirrored());
    m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    m_texture->create();

    m_texture->setSize(100, 100, 1);
    m_texture->setFormat(m_format);
    m_texture->allocateStorage(QOpenGLTexture::BGRA,QOpenGLTexture::UInt8);
//    QImage image = QPixmap(10,10).toImage();
    QImage image(QDir::homePath()+QDir::separator()+"side1.png");
    m_texture->setData(0,0,0,100,100,1,0,QOpenGLTexture::BGRA,QOpenGLTexture::UInt8,static_cast<const void*>(image.bits()));
    */

    memset(&m_data,0,sizeof(unsigned char)*SHADER_COLOR_CHANNELS * LINES_PER_ROTATION * RETURNS_PER_LINE);

    /*

    float vertices[4][8] = {
        {-1.f,  -1.f, 0.0f,   -1.f, -1.f},
        {1.f, -1.f, 0.0f,   1.f, 1.0f},
        {1.f, 1.f, 0.0f,  1.0f, 1.0f},
        {-1.f,  1.f, 0.0f,  -1.0f, 1.f}
    };
    for(int j = 0; j < 4; ++j)
    {
        vertData.append(0.2*vertices[j][0]);
        vertData.append(0.2*vertices[j][1]);
        vertData.append(0.2*vertices[j][2]);

        vertData.append(vertices[j][3]);
        vertData.append(vertices[j][4]);
    }
    vbo.create();
    vbo.bind();
    vbo.allocate(vertData.constData(), vertData.count() * sizeof(GLfloat));
    */
}

void RDShader::ProcessRadarSpoke(int angle, UINT8* data, size_t len)
{
    GLubyte alpha = 255;

    if (m_start_line == -1) m_start_line = angle;  // Note that this only runs once after each draw,
    if (m_lines < LINES_PER_ROTATION) m_lines++;
    if (m_channels == SHADER_COLOR_CHANNELS)
    {
        unsigned char *d = m_data + (angle * RETURNS_PER_LINE) * m_channels;
        for (size_t r = 0; r < len; r++)
        {
            GLubyte strength = data[r];
            BlobColour colour = m_re->m_colour_map[strength];
            d[0] = static_cast<unsigned char>(m_re->m_colour_map_rgb[colour].red());
            d[1] = static_cast<unsigned char>(m_re->m_colour_map_rgb[colour].green());
            d[2] = static_cast<unsigned char>(m_re->m_colour_map_rgb[colour].blue());
            d[3] = colour != BLOB_NONE ? alpha : 0;
            d += m_channels;
        }
    }
    else
    {
        unsigned char *d = m_data + (angle * RETURNS_PER_LINE);
        for (size_t r = 0; r < len; r++)
        {
            GLubyte strength = data[r];
            BlobColour colour = m_re->m_colour_map[strength];
            *d++ = (static_cast<unsigned char>(m_re->m_colour_map_rgb[colour].red()) * alpha) >> 8;
        }
    }
}

void RDShader::DrawRadarSweep(double angle)
{
    //TBD
    Q_UNUSED(angle)
}

void RDShader::DrawRadarImage()
{
      if (!m_program || !m_texture) {
        return;
      }


      m_program->bind();
//      vbo.bind();
//      m_texture->bind();

      QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

//      f->glPushAttrib(GL_TEXTURE_BIT);
      f->glBindTexture(GL_TEXTURE_2D, m_texture);

      if (m_start_line > -1) {
        // Since the last time we have received data from [m_start_line, m_end_line>
        // so we only need to update the texture for those data lines.
        if (m_start_line + m_lines > LINES_PER_ROTATION) {
          int end_line = MOD_ROTATION2048(m_start_line + m_lines);
          // if the new data partly wraps past the end of the texture
          // tell it the two parts separately
          // First remap [0, m_end_line>
//          m_texture->setData(0,0,0,RETURNS_PER_LINE,end_line,1,0,QOpenGLTexture::BGRA,QOpenGLTexture::UInt8,static_cast<const void*>(m_data));

          f->glTexSubImage2D(/* target =   */ GL_TEXTURE_2D,
                          /* level =    */ 0,
                          /* x-offset = */ 0,
                          /* y-offset = */ 0,
                          /* width =    */ RETURNS_PER_LINE,
                          /* height =   */ end_line,
                          /* format =   */ static_cast<GLenum>(m_format),
                          /* type =     */ GL_UNSIGNED_BYTE,
                          /* pixels =   */ m_data);
          // And then remap [m_start_line, LINES_PER_ROTATION>

//          qDebug()<<Q_FUNC_INFO<<"m_start_line * RETURNS_PER_LINE * m_channels"<<m_start_line * RETURNS_PER_LINE * m_channels;
//          qDebug()<<Q_FUNC_INFO<<"m_data"<<m_data[ m_start_line * RETURNS_PER_LINE * m_channels ];
//          m_texture->setData(0,m_start_line,0,RETURNS_PER_LINE,LINES_PER_ROTATION - m_start_line,1,0,QOpenGLTexture::BGRA,QOpenGLTexture::UInt8,static_cast<const void*>(m_data + m_start_line * RETURNS_PER_LINE * m_channels));


          f->glTexSubImage2D(/* target =   */ GL_TEXTURE_2D,
                          /* level =    */ 0,
                          /* x-offset = */ 0,
                          /* y-offset = */ m_start_line,
                          /* width =    */ RETURNS_PER_LINE,
                          /* height =   */ LINES_PER_ROTATION - m_start_line,
                          /* format =   */ static_cast<GLenum>(m_format),
                          /* type =     */ GL_UNSIGNED_BYTE,
                          /* pixels =   */ m_data + m_start_line * RETURNS_PER_LINE * m_channels);

        } else {
          // Map [m_start_line, m_end_line>
//            qDebug()<<Q_FUNC_INFO<<"m_start_line * RETURNS_PER_LINE * m_channels"<<m_start_line * RETURNS_PER_LINE * m_channels;
//            qDebug()<<Q_FUNC_INFO<<"m_data"<<m_data[ m_start_line * RETURNS_PER_LINE * m_channels ];
//            QImage image(100,100, QImage::Format_ARGB32);
//            QRgb value;

//             value = qRgba(189, 149, 39, 255); // 0xffbd9527
//             for(int i = 0; i < 25; i++) image.setPixel(i, i, value);

//             value = qRgba(122, 163, 39, 255); // 0xff7aa327
//             for(int i = 25; i < 50; i++)
//             {
//             image.setPixel(26, i, value);
//             image.setPixel(i, 26, value);
//             }

//             value = qRgba(237, 187, 51, 255); // 0xffedba31
//             for(int i = 50; i < 75; i++)
//             {
//             image.setPixel(50, i, value);
//             image.setPixel(i, 50, value);
//             }

//             m_texture->setData(0,0,0,RETURNS_PER_LINE,500,1,1,QOpenGLTexture::BGRA,QOpenGLTexture::UInt8,static_cast<const void*>(image.bits()));
//            m_texture->setData(0,m_start_line,0,RETURNS_PER_LINE,m_lines,1,0,QOpenGLTexture::BGRA,QOpenGLTexture::UInt8,static_cast<const void*>(m_data + m_start_line * RETURNS_PER_LINE * m_channels));


            f->glTexSubImage2D(/* target =   */ GL_TEXTURE_2D,
                          /* level =    */ 0,
                          /* x-offset = */ 0,
                          /* y-offset = */ m_start_line,
                          /* width =    */ RETURNS_PER_LINE,
                          /* height =   */ m_lines,
                          /* format =   */ static_cast<GLenum>(m_format),
                          /* type =     */ GL_UNSIGNED_BYTE,
                          /* pixels =   */ m_data + m_start_line * RETURNS_PER_LINE * m_channels);

        }
        m_start_line = -1;
        m_lines = 0;
      }

      // We tell the GPU to draw a square from (-512,-512) to (+512,+512).
      // The shader morphs this into a circle.
      f->glDrawArrays(GL_QUADS,0,4);
      /*
      float fullscale = 1;
      glBegin(GL_QUADS);
      glTexCoord2f(-1, -1);
      glVertex2f(-fullscale, -fullscale);
      glTexCoord2f(1, -1);
      glVertex2f(fullscale, -fullscale);
      glTexCoord2f(1, 1);
      glVertex2f(fullscale, fullscale);
      glTexCoord2f(-1, 1);
      glVertex2f(-fullscale, fullscale);
      glEnd();
      */

      /*
      float fullscale = 1;
      glBegin(GL_QUADS);
      glTexCoord2f(0., 1.);
      glVertex2f(-fullscale, -fullscale);
      glTexCoord2f(1., 1.);
      glVertex2f(fullscale, -fullscale);
      glTexCoord2f(1., 0.);
      glVertex2f(fullscale, fullscale);
      glTexCoord2f(0., 0.);
      glVertex2f(-fullscale, fullscale);
      glEnd();
      */

//      m_texture->release();
//      vbo.release();
      m_program->release();
//      f->glPopAttrib();
}
