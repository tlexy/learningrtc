#include "h264_player.h"
#include <QDebug>
#include <QTimer>
#include <video/test/h264_ffmpeg_decoder.h>

#pragma execution_character_set("utf-8")

#define ATTRIB_VERTEX 0
#define ATTRIB_TEXTURE 1

#define GET_STR(x) #x

//顶点shader
const char* vString0 = GET_STR(
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
const char* tString0 = GET_STR(
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


H264Player::H264Player()
	:QOpenGLWidget(Q_NULLPTR)
{
}

H264Player::~H264Player()
{
    makeCurrent();
    //vbo.destroy();
    /*yuv_textures[0]->destroy();
    yuv_textures[1]->destroy();
    yuv_textures[2]->destroy();*/
    doneCurrent();
}

bool H264Player::init(int width, int height, const char* h264_file)
{
    _width = width;
    _height = height;
    _filename = const_cast<char*>(h264_file);

    _decoder = new H264FFmpegDecoder();
    _decoder->init();

    _fp = fopen(h264_file, "rb");
    if (!_fp)
    {
        qDebug() << h264_file << " file open failed!";
        return false;
    }
    return true;
}

void H264Player::start(int interval)
{
    startTimer(interval);
    _test_nalu = AllocNALU(80000);
}

void H264Player::timerEvent(QTimerEvent*)
{
   //读取一帖数据解码，直到有一帧输出
    /*if (feof(fp))
    {
        fseek(fp, 0, SEEK_SET);
    }
    fread(datas[0], 1, width * height, fp);
    fread(datas[1], 1, width * height / 4, fp);
    fread(datas[2], 1, width * height / 4, fp);*/

    while (!feof(_fp))
    {
        int size = GetAnnexbNALU(_test_nalu, _fp);
        if (size < 4)
        {
            printf("get nul error!\n");
            continue;
        }
        _decoder->decode(_test_nalu);
        AVFrame* frame = nullptr;
        int ret = _decoder->receive_frame(frame);
        if (ret == 0)
        {
            continue;
        }
        else if (ret == 1)
        {
            _frame = frame;
            update();
            break;
        }
        else
        {
            qDebug() << "decode maybe error.";
            break;
        }
    }
}

void H264Player::initializeGL()
{
    //为当前环境初始化OpenGL函数
    initializeOpenGLFunctions();
    //glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);    //设置背景色为白色
    _sp = new QGLShaderProgram(this);
    //_sp->addShaderFromSourceFile(QGLShader::Vertex, QString(":/glsl/res/vertex_showyuv.glsl"));
    //_sp->addShaderFromSourceFile(QGLShader::Fragment, QString(":/glsl/res/fragment_showyuv.glsl"));
    _sp->addShaderFromSourceCode(QGLShader::Fragment, tString0);
    _sp->addShaderFromSourceCode(QGLShader::Vertex, vString0);

    //设置顶点坐标的变量
    _sp->bindAttributeLocation("vertexIn", ATTRIB_VERTEX);
    //设置材质坐标
    _sp->bindAttributeLocation("textureIn", ATTRIB_TEXTURE);

    _sp->link();
    _sp->bind();

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

void H264Player::paintGL()
{
    if (_frame == nullptr)
    {
        return;
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texids[0]); //0层绑定到Y材质
    //修改材质内容(复制内存内容)
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, GL_RED, GL_UNSIGNED_BYTE, _frame->data[0]);
    //与shader uni遍历关联
    glUniform1i(_unis[0], 0);


    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, _texids[1]); //1层绑定到U材质
                                           //修改材质内容(复制内存内容)
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width / 2, _height / 2, GL_RED, GL_UNSIGNED_BYTE, _frame->data[1]);
    //与shader uni遍历关联
    glUniform1i(_unis[1], 1);


    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, _texids[2]); //2层绑定到V材质
                                           //修改材质内容(复制内存内容)
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width / 2, _height / 2, GL_RED, GL_UNSIGNED_BYTE, _frame->data[2]);
    //与shader uni遍历关联
    glUniform1i(_unis[2], 2);

    //使用顶点数组方式绘制图形
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void H264Player::resizeGL(int width, int height)
{
    if (height == 0)// prevents being divided by zero
        height = 1;// set the height to 1

    // Set the viewport
    glViewport(0, 0, width, height);
    update();
}