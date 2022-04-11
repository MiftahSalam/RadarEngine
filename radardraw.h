#ifndef RADARDRAW_H
#define RADARDRAW_H

#include <radarconfig_global.h>
#include "radarengine_global.h"

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

    virtual void init(QObject *parent = nullptr) = 0;
    virtual void DrawRadarImage() = 0;
    virtual void ProcessRadarSpoke(int angle, quint8* data, size_t len) = 0;

    virtual ~RadarDraw() = 0;

    static QString methods;
    static QString GetDrawingMethods();
};

class RDVert : public RadarDraw
{
public:
    RDVert(RadarEngine* re);
    void init(QObject *parent = nullptr) override;
    void DrawRadarImage() override;
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
