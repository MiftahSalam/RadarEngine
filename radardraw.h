#ifndef RADARDRAW_H
#define RADARDRAW_H

#include "constants.h"

#include <QtOpenGL/qgl.h>
#include <QOpenGLShaderProgram>

struct P2CLookupTable {
  GLfloat x[LINES_PER_ROTATION + 1][RETURNS_PER_LINE + 1];
  GLfloat y[LINES_PER_ROTATION + 1][RETURNS_PER_LINE + 1];
  int intx[LINES_PER_ROTATION + 1][RETURNS_PER_LINE + 1];
  int inty[LINES_PER_ROTATION + 1][RETURNS_PER_LINE + 1];
};

extern P2CLookupTable *lookupTable;
P2CLookupTable* GetP2CLookupTable();

namespace RadarEngine {

class RadarEngine;

class RadarDraw
{
public:
    static RadarDraw* make_Draw(RadarEngine *re, int draw_method);

    virtual void Init(QObject *parent = nullptr) = 0;
    virtual void DrawRadarImage() = 0;
    virtual void DrawRadarSweep(double angle) = 0;
    virtual void ProcessRadarSpoke(int angle, quint8* data, size_t len) = 0;

    virtual ~RadarDraw() = 0;

    static QString methods;
    static QString GetDrawingMethods();
};

class RDShader : public RadarDraw
{
public:
    RDShader(RadarEngine* re);
    void Init(QObject *parent = nullptr) override;
    void DrawRadarImage() override;
    virtual void DrawRadarSweep(double angle) override;
    void ProcessRadarSpoke(int angle, quint8 *data, size_t len) override;

    ~RDShader() override;


private:
    static const int SHADER_COLOR_CHANNELS = 4; // RGB + Alpha

    unsigned char m_data[SHADER_COLOR_CHANNELS * LINES_PER_ROTATION * RETURNS_PER_LINE];
    int m_start_line;  // First line received since last draw, or -1
    int m_lines;       // # of lines received since last draw
    int m_format;
    //      QOpenGLTexture::TextureFormat m_format;
    int m_channels;

//    QVector<GLfloat> vertData;
//    QOpenGLBuffer vbo;
//    QOpenGLTexture *m_texture;
    GLuint m_texture;
    QOpenGLShader *m_fragment;
    QOpenGLShader *m_vertex;
    QOpenGLShaderProgram *m_program;
    RadarEngine* m_re;

};

class RDVert : public RadarDraw
{
public:
    RDVert(RadarEngine* re);
    void Init(QObject *parent = nullptr) override;
    void DrawRadarImage() override;
    virtual void DrawRadarSweep(double angle) override;
    void ProcessRadarSpoke(int angle, quint8 *data, size_t len) override;

    ~RDVert() override;

private:
    struct VertexPoint
    {
        GLfloat x;
        GLfloat y;
        GLubyte red;
        GLubyte green;
        GLubyte blue;
        GLubyte alpha;
    };

    struct VertexLine
    {
        VertexPoint* points;
        qint64 timeout;
        size_t count;
        size_t allocated;
    };
    VertexLine m_vertices[LINES_PER_ROTATION];

    RadarEngine* m_ri;

    static const int VERTEX_PER_TRIANGLE = 3;
    static const int VERTEX_PER_QUAD = 2 * VERTEX_PER_TRIANGLE;
    static const int MAX_BLOBS_PER_LINE = RETURNS_PER_LINE;

    P2CLookupTable* m_polarLookup;

    unsigned int m_count;
    bool m_oom;

    GLuint m_posAttr;
    GLuint m_colAttr;
    QOpenGLShaderProgram *m_program;

    void SetBlob(VertexLine* line, int angle_begin, int angle_end, int r1, int r2, GLubyte red, GLubyte green, GLubyte blue,
                 GLubyte alpha);
};

}
#endif // RADARDRAW_H
