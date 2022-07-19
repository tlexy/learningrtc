#include <stdio.h>
#include <3rd/SDL2-2.0.14/include/SDL.h>
#include <memory>
#include <thread>
#include <common/util/threadqueue.hpp>
#include <webrtc_camera/video_frame/video_frame.h>
#include <webrtc_camera/video_frame_subscriber.h>

class SdlPlayer : public webrtc::test::VideoFrameSubscriber
{
public:
	void start(int width, int height);
	void stop();

	virtual void OnFrame(const webrtc::VideoFrame&) override;

private:
	void player_thread();

private:
	std::shared_ptr<std::thread> _th{nullptr};
	ThreadQueue<webrtc::VideoFrame> _qu;
	bool _is_stop{ true };
	int _width;
	int _height;
};
