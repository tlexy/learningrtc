#include "sdl_player_widget.h"
#include <webrtc_camera/video_frame/video_frame_buffer.h>
#include <webrtc_camera/video_frame/i420_buffer.h>
#include <QDebug>

#pragma execution_character_set("utf-8")

SdlPlayerWidget::SdlPlayerWidget()
	:QWidget(Q_NULLPTR)
{
}

SdlPlayerWidget::~SdlPlayerWidget()
{}

void SdlPlayerWidget::start(int width, int height)
{
	_width = width;
	_height = height;
	_is_stop = false;
	if (!_th)
	{
		_th = std::make_shared<std::thread>(&SdlPlayerWidget::player_thread, this);
	}
}

void SdlPlayerWidget::stop()
{
	_is_stop = true;
	if (_th)
	{
		_th->join();
		_th = nullptr;
	}
	int a = 1;
}

void SdlPlayerWidget::init_sdl()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
		//fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL - %s\n", SDL_GetError());
		return;
	}

	SDL_Window* win = NULL;
	SDL_Renderer* renderer = NULL;

	win = SDL_CreateWindowFrom((void*)this->winId());
	SDL_SetWindowTitle(win, "LXP Media Player");
	renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	_win = win;
	_renderer = renderer;

	//_timer = new QTimer(this);
	//connect(_timer, SIGNAL(timeout()), this, SLOT(slot_render()));
	//_timer->start(1000 / 20);
	setUpdatesEnabled(false);
}

void SdlPlayerWidget::init()
{
	init_sdl();
	_sdl_pix_fmt = SDL_PIXELFORMAT_IYUV;
}

void SdlPlayerWidget::OnFrame(const webrtc::VideoFrame& frame)
{
	_qu.push_back(frame);
}

void SdlPlayerWidget::player_thread()
{
	SDL_Rect        rect;
	Uint32 	  pixformat;

	//for render
	/*SDL_Window* win = NULL;
	SDL_Renderer* renderer = NULL;*/
	SDL_Texture* texture = NULL;
	bool flag = false;

	//if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
	//	//fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
	//	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL - %s\n", SDL_GetError());
	//	return;
	//}

	//win = SDL_CreateWindow("Media Player",
	//	SDL_WINDOWPOS_UNDEFINED,
	//	SDL_WINDOWPOS_UNDEFINED,
	//	_width, _height,
	//	SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	//if (!win) {
	//	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window by SDL");
	//	goto __QUIT;
	//}

	//renderer = SDL_CreateRenderer(win, -1, 0);
	//if (!renderer) {
	//	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Renderer by SDL");
	//	goto __QUIT;
	//}

	pixformat = SDL_PIXELFORMAT_IYUV;
	texture = SDL_CreateTexture(_renderer,
		pixformat,
		SDL_TEXTUREACCESS_STREAMING,
		_width,
		_height);


	while (!_is_stop)
	{
		
		webrtc::VideoFrame frame = _qu.pop(flag, std::chrono::milliseconds(100));
		if (flag)
		{
			SDL_UpdateYUVTexture(texture, NULL,
				frame.video_frame_buffer()->GetI420()->DataY(), frame.video_frame_buffer()->GetI420()->StrideY(),
				frame.video_frame_buffer()->GetI420()->DataU(), frame.video_frame_buffer()->GetI420()->StrideU(),
				frame.video_frame_buffer()->GetI420()->DataV(), frame.video_frame_buffer()->GetI420()->StrideV());

			// Set Size of Window
			rect.x = 0;
			rect.y = 0;
			rect.w = _width;
			rect.h = _height;

			SDL_RenderClear(_renderer);
			SDL_RenderCopy(_renderer, texture, NULL, &rect);
			SDL_RenderFillRect(_renderer, &rect);
			SDL_RenderPresent(_renderer);
			//SDL_Delay(41);

		}
	}


	/*SDL_Event event;
	SDL_PollEvent(&event);
	switch (event.type) {
	case SDL_QUIT:
		goto __QUIT;
		break;
	default:
		break;
	}*/

__QUIT:
	/*if (win) {
		SDL_DestroyWindow(win);
	}
	if (renderer) {
		SDL_DestroyRenderer(renderer);
	}*/
	if (texture) {
		SDL_DestroyTexture(texture);
	}
	//SDL_Quit();
	return;
}