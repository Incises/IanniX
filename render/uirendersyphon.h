#ifdef USE_GLWIDGET
#include <QGLWidget>
#else
#include <QOpenGLWidget>
#endif
class UiRenderSyphon {
public:
    explicit UiRenderSyphon();
    ~UiRenderSyphon();

public:
    void openPort();
    void publishTexture(GLuint textureId, int textureTarget, int width, int height);

protected:
    void *mSyphon;
};
