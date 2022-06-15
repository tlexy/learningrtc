#ifndef LEARNING_RTC_JB_ENTITY_H
#define LEARNING_RTC_JB_ENTITY_H

#include <memory>
#include <stdint.h>
#include <uvnet/core/ip_address.h>
#include <uvnet/core/udp_server.h>
#include <rtp_base/core/rtp.h>
#include <list>
#include <mutex>
#include <common/audio/mid_buf.h>

class RtpCacher;
//class RtpReceiver;
class AacHelper;
class FileSaver;

//两个问题：（1）jetterbuffer大小；（2）什么时候强制拉取cache层的数据？
class JetterBufferEntity
{
public:
	JetterBufferEntity();
	void init();
	/*void start_recv(const uvcore::IpAddress& addr,
		std::shared_ptr<uvcore::UdpServer> server);*/
	void push(rtp_packet_t*);
	//设置最小的输出时长，只有jetterbuffer达到这个的最小值时，才开始向外输出
	void set_output_buffer(int64_t ms);

	void update();
	void decode();

protected:
	virtual bool force_cache();
	virtual void do_decode() = 0;

protected:
	//std::shared_ptr<RtpReceiver> _rtp_receiver{nullptr};
	std::shared_ptr<RtpCacher> _rtp_cacher;

	std::mutex _mutex;
	std::list<rtp_packet_t*> _rtp_list;
	bool _is_init{ false };
	int64_t _output_ms{0};

private:
	void on_rtp_packet(rtp_packet_t*);
};

//专用于aac音频的
class AacJetterBufferEntity : public JetterBufferEntity
{
public:
	AacJetterBufferEntity();

	/// <summary>
	/// 获取pcm buffer，并将获取到实际数据长度返回
	/// </summary>
	/// <param name="data">存放输出数据的内存地址</param>
	/// <param name="len">希望获得的长度</param>
	/// <returns>实际获得的长度</returns>
	int get_pcm_buffer(int8_t* data, int len);

protected:
	virtual bool force_cache();
	virtual void do_decode();

	void aac_init();

private:
	std::shared_ptr<AacHelper> _aac_helper{nullptr};
	int _bit_dep{0};
	int _channel{0};
	int _sample_rate{0};
	bool _output_init{false};
	int _frame_size;
	int64_t _output_len{0};//
	uint8_t* _decode_buf;
	int _decode_buf_len;
	std::shared_ptr<mid_buf> _pcm_buffer;
	std::mutex _pcm_buffer_mutex;

	//FileSaver* _file_saver{nullptr};
};

#endif