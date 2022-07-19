#ifndef H264_PLAYER_WIDGET
#define H264_PLAYER_WIDGET

#include <memory>
#include <QtWidgets/QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QGLShaderProgram>
#include <QOpenGLTexture> 
#include <QOpenGLBuffer>
#include <stdio.h>
#include <memory>
#include <thread>
#include <common/util/threadqueue.hpp>
#include <video/test/h264_helper.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#pragma execution_character_set("utf-8")

class H264FFmpegDecoder;

class H264Player : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT
public:
	H264Player();

	bool init(int width, int height, const char* h264_file);
	void start(int interval);

	~H264Player();

protected:
	void timerEvent(QTimerEvent*) override;
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int width, int height) override;

public slots:
	//void tick_frame();

private:
	std::shared_ptr<std::thread> _th{ nullptr };
	char* _filename;
	FILE* _fp;
	H264FFmpegDecoder* _decoder = nullptr;
	NALU* _test_nalu;
	AVFrame* _frame = nullptr;
	bool _is_stop{ true };
	int _width;
	int _height;

	GLuint _unis[3] = { 0 };
	GLuint _texids[3] = {0};
	GLuint _ebo;

	QGLShaderProgram* _sp;
	QOpenGLTexture* yuv_textures[3];

};

#endif