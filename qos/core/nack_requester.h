#ifndef NACK_REQUESTER_H
#define NACK_REQUESTER_H

#include <memory>
#include <map>
#include <set>
#include <stdint.h>
#include <rtc_base/numerics/sequence_number_util.h>
#include <atomic>
#include <vector>

namespace uvcore
{
	class Timer;
}

class UdpChannel;

using namespace webrtc;

class NackRequesterBase {
public:
	virtual ~NackRequesterBase() = default;
	virtual void ProcessNacks() = 0;
};

class NackRequester final : public NackRequesterBase {
public:
	NackRequester(std::shared_ptr<UdpChannel> sender);
	~NackRequester();

	void ProcessNacks() override;

	int OnReceivedPacket(uint16_t seq_num, bool is_keyframe);
	int OnReceivedPacket(uint16_t seq_num, bool is_keyframe, bool is_recovered);

	void ClearUpTo(uint16_t seq_num);
	void UpdateRtt(int64_t rtt_ms);

	//to do
	//销毁对象，关闭定时器

private:
	enum NackFilterOptions { kSeqNumOnly, kTimeOnly, kSeqNumAndTime };
	struct NackInfo {
		NackInfo();
		NackInfo(uint16_t seq_num,
			uint16_t send_at_seq_num,
			int64_t created_at_time);

		uint16_t seq_num;
		uint16_t send_at_seq_num;
		int64_t created_at_time;
		int64_t sent_at_time;
		int retries;
	};
	std::atomic<int> _rtt = 100;
	std::shared_ptr<UdpChannel> _sender;
	std::shared_ptr<uvcore::Timer> _timer = nullptr;

	const int64_t _send_nack_delay_ms;
	uint16_t _newest_seq_num = 0;
	bool _initialized = false;

	std::map<uint16_t, NackInfo, DescendingSeqNumComp<uint16_t>> _nack_list;
	std::set<uint16_t, DescendingSeqNumComp<uint16_t>> _keyframe_list;
	//std::set<uint16_t, DescendingSeqNumComp<uint16_t>> _recovered_list;

private:
	void AddPacketsToNack(uint16_t seq_num_start, uint16_t seq_num_end);
	void OnTimerCb(uvcore::Timer*);

	void GetNackBatch(NackFilterOptions options, std::vector<uint16_t>&);
	bool RemovePacketsUntilKeyFrame();
};

#endif