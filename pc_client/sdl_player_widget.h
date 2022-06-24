#ifndef LXP_SDL_PLAYER_WIDGET
#define LXP_SDL_PLAYER_WIDGET

#include <memory>
#include <QtWidgets/QWidget>
#include <3rd/SDL2-2.0.14/include/SDL.h>
#include <memory>
#include <thread>
#include <common/util/threadqueue.hpp>
#include <webrtc_camera/video_frame/video_frame.h>
#include <webrtc_camera/video_frame_subscriber.h>


class SdlPlayerWidget : public QWidget, public webrtc::test::VideoFrameSubscriber
{
	Q_OBJECT
public:
	SdlPlayerWidget();
	void start(int width, int height);
	void stop();

	void init();

	virtual void OnFrame(const webrtc::VideoFrame&) override;
	~SdlPlayerWidget();
private:
	void player_thread();
	void init_sdl();

private:
	std::shared_ptr<std::thread> _th{ nullptr };
	ThreadQueue<webrtc::VideoFrame> _qu;
	bool _is_stop{ true };
	int _width;
	int _height;

	int _sdl_pix_fmt;
	SDL_Window* _win;
	SDL_Renderer* _renderer;

};

#endif