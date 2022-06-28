#include "opengl_player_widget.h"
#include <webrtc_camera/video_frame/video_frame_buffer.h>
#include <webrtc_camera/video_frame/i420_buffer.h>
#include <QDebug>
#include <QTimer>

#pragma execution_character_set("utf-8")

#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTURE 1

#define GET_STR(x) #x

//顶点shader
const char* vString = GET_STR(
    attribute vec4 vertexIn;
    attribute vec2 textureIn;
    varying vec2 textureOut;
    void main(void)
    {
        gl_Position = vertexIn;
        textureOut = textureIn;
    }
);


//片元shader
const char* tString = GET_STR(
    varying vec2 textureOut;
    uniform sampler2D texY;
    uniform sampler2D texU;
    uniform sampler2D texV;
    void main(void)
    {
        vec3 yuv;
        vec3 rgb;
        yuv.x = texture2D(texY, textureOut).r;
        yuv.y = texture2D(texU, textureOut).r - 0.5;
        yuv.z = texture2D(texV, textureOut).r - 0.5;
        rgb = mat3(1.0, 1.0, 1.0,
            0.0, -0.39465, 2.03211,
            1.13983, -0.58060, 0.0) * yuv;
        gl_FragColor = vec4(rgb, 1.0);
    }

);


OpenGLPlayerWidget::OpenGLPlayerWidget()
	:QOpenGLWidget(Q_NULLPTR)
{
}

OpenGLPlayerWidget::~OpenGLPlayerWidget()
{
    makeCurrent();
    //vbo.destroy();
    /*yuv_textures[0]->destroy();
    yuv_textures[1]->destroy();
    yuv_textures[2]->destroy();*/
    doneCurrent();
}

void OpenGLPlayerWidget::OnFrame(const webrtc::VideoFrame& frame)
{
	_qu.push_back(frame);
}

void OpenGLPlayerWidget::init(int width, int height, int millis)
{
    _width = width;
    _height = height;
	startTimer(std::chrono::milliseconds(millis));
}

void OpenGLPlayerWidget::timerEvent(QTimerEvent*)
{
    webrtc::VideoFrame frame = _qu.pop(_new_frame, std::chrono::milliseconds(0));
    if (_new_frame)
    {
        _frame = frame;
        update();
    }
}

void OpenGLPlayerWidget::initializeGL()
{
    //为当前环境初始化OpenGL函数
    initializeOpenGLFunctions();
    //glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);    //设置背景色为白色
    _sp = new QGLShaderProgram(this);
    //_sp->addShaderFromSourceFile(QGLShader::Vertex, QString(":/glsl/res/vertex_showyuv.glsl"));
    //_sp->addShaderFromSourceFile(QGLShader::Fragment, QString(":/glsl/res/fragment_showyuv.glsl"));
    _sp->addShaderFromSourceCode(QGLShader::Fragment, tString);
    _sp->addShaderFromSourceCode(QGLShader::Vertex, vString);

    //设置顶点坐标的变量
    _sp->bindAttributeLocation("vertexIn", ATTRIB_VERTEX);
    //设置材质坐标
    _sp->bindAttributeLocation("textureIn", ATTRIB_TEXTURE);

    //初始化纹理对象
    //for (int i = 0; i < 3; i++)
    //{
    //    yuv_textures[i] = new QOpenGLTexture(QOpenGLTexture::Target2D);
    //    if (i == 0)
    //    {
    //        yuv_textures[i]->setSize(_width, _height);
    //    }
    //    else
    //    {
    //        yuv_textures[i]->setSize(_width / 2, _height / 2);
    //    }

    //    yuv_textures[i]->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
    //    yuv_textures[i]->create();
    //    yuv_textures[i]->setFormat(QOpenGLTexture::R8_UNorm);
    //    yuv_textures[i]->allocateStorage();        //存储配置(放大缩小过滤、格式、size)
    //    //yuv_textures[i]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, yuvArr[i]);
    //    _texids[i] = yuv_textures[i]->textureId();
    //}

    _sp->link();
    _sp->bind();

    ////初始化VBO,将顶点数据存储到buffer中,等待VAO激活后才能释放
    //float vertices[] = {
    //    //顶点坐标               //纹理坐标的Y方向需要是反的,因为opengl中的坐标系是Y原点位于下方
    //   -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,        //左下
    //   1.0f , -1.0f, 0.0f,  1.0f, 1.0f,        //右下
    //   -1.0f, 1.0f,  0.0f,  0.0f, 0.0f,        //左上
    //   1.0f,  1.0f,  0.0f,  1.0f, 0.0f         //右上
    //};

    ////顶点
    //glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, 0, 5 * sizeof(float), vertices);
    //glEnableVertexAttribArray(ATTRIB_VERTEX);

    ////材质
    //glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 5*sizeof(float), vertices + 3);
    //glEnableVertexAttribArray(ATTRIB_TEXTURE);

    //传递顶点和材质坐标
    //顶点
    static const GLfloat ver[] = {
        -1.0f,-1.0f,
        1.0f,-1.0f,
        
        1.0f,1.0f,
        -1.0f, 1.0f
    };

    //材质
    static const GLfloat tex[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        
        1.0f, 0.0f,
        0.0f, 0.0f
    };


    //顶点
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, ver);
    glEnableVertexAttribArray(ATTRIB_VERTEX);

    //材质
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, tex);
    glEnableVertexAttribArray(ATTRIB_TEXTURE);

    _unis[0] = _sp->uniformLocation("texY");
    _unis[1] = _sp->uniformLocation("texU");
    _unis[2] = _sp->uniformLocation("texV");

    //创建材质
    glGenTextures(3, _texids);

    //Y
    glBindTexture(GL_TEXTURE_2D, _texids[0]);
    //放大过滤，线性插值   GL_NEAREST(效率高，但马赛克严重)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //创建材质显卡空间
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, _width, _height, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    //U
    glBindTexture(GL_TEXTURE_2D, _texids[1]);
    //放大过滤，线性插值
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //创建材质显卡空间
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, _width / 2, _height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    //V
    glBindTexture(GL_TEXTURE_2D, _texids[2]);
    //放大过滤，线性插值
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //创建材质显卡空间
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, _width / 2, _height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

}

void OpenGLPlayerWidget::paintGL()
{
    //glClear(GL_COLOR_BUFFER_BIT);
    if (!_new_frame)
    {
        return;
    }
    /*yuv_textures[0]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, _frame.video_frame_buffer()->GetI420()->DataY());
    yuv_textures[1]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, _frame.video_frame_buffer()->GetI420()->DataU());
    yuv_textures[2]->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, _frame.video_frame_buffer()->GetI420()->DataV());*/
    //_sp->bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texids[0]); //0层绑定到Y材质
    //修改材质内容(复制内存内容)
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, GL_RED, GL_UNSIGNED_BYTE, _frame.video_frame_buffer()->GetI420()->DataY());
    //与shader uni遍历关联
    glUniform1i(_unis[0], 0);


    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, _texids[1]); //1层绑定到U材质
                                           //修改材质内容(复制内存内容)
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width / 2, _height / 2, GL_RED, GL_UNSIGNED_BYTE, _frame.video_frame_buffer()->GetI420()->DataU());
    //与shader uni遍历关联
    glUniform1i(_unis[1], 1);


    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, _texids[2]); //2层绑定到V材质
                                           //修改材质内容(复制内存内容)
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width / 2, _height / 2, GL_RED, GL_UNSIGNED_BYTE, _frame.video_frame_buffer()->GetI420()->DataV());
    //与shader uni遍历关联
    glUniform1i(_unis[2], 2);

    //使用顶点数组方式绘制图形
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	_new_frame = false;
}

void OpenGLPlayerWidget::resizeGL(int width, int height)
{
    if (height == 0)// prevents being divided by zero
        height = 1;// set the height to 1

    // Set the viewport
    glViewport(0, 0, width, height);
    update();
}