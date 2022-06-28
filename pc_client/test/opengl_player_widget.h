﻿#ifndef OPENGL_PLAYER_WIDGET
#define OPENGL_PLAYER_WIDGET

#include <memory>
#include <QtWidgets/QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QGLShaderProgram>
#include <QOpenGLTexture> 
#include <QOpenGLBuffer>

#include <3rd/SDL2-2.0.14/include/SDL.h>
#include <memory>
#include <thread>
#include <common/util/threadqueue.hpp>
#include <webrtc_camera/video_frame/video_frame.h>
#include <webrtc_camera/video_frame_subscriber.h>



class OpenGLPlayerWidget : public QOpenGLWidget, protected QOpenGLFunctions, public webrtc::test::VideoFrameSubscriber
{
	Q_OBJECT
public:
	OpenGLPlayerWidget();
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int width, int height) override;

	void init(int width, int height, int millis);

	virtual void OnFrame(const webrtc::VideoFrame&) override;
	~OpenGLPlayerWidget();

protected:
	void timerEvent(QTimerEvent*);

public slots:
	//void tick_frame();

private:
	std::shared_ptr<std::thread> _th{ nullptr };
	ThreadQueue<webrtc::VideoFrame> _qu;
	bool _is_stop{ true };
	int _width;
	int _height;

	webrtc::VideoFrame _frame;
	bool _new_frame{ false };

	GLuint _unis[3] = { 0 };
	GLuint _texids[3] = {0};
	GLuint _ebo;

	QGLShaderProgram* _sp;
	QOpenGLTexture* yuv_textures[3];

};

#endif