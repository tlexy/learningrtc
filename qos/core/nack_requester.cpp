#include "nack_requester.h"
#include <net_base/udp_channel.h>
#include <uvnet/core/timer.h>
#include <uvnet/server/timer_helpers.h>
#include <common/util/sutil.h>

const int kMaxPacketAge = 10000;
const int kMaxNackPackets = 1000;
const int kDefaultRttMs = 100;
const int kMaxNackRetries = 10;
//const int kMaxReorderedPackets = 128;
//const int kNumReorderingBuckets = 10;
const int kDefaultSendNackDelayMs = 0;

NackRequester::NackInfo::NackInfo()
    : seq_num(0), send_at_seq_num(0), sent_at_time(-1), retries(0) {}

NackRequester::NackInfo::NackInfo(uint16_t seq_num,
    uint16_t send_at_seq_num,
    int64_t created_at_time)
    : seq_num(seq_num),
    send_at_seq_num(send_at_seq_num),
    created_at_time(created_at_time),
    sent_at_time(-1),
    retries(0) {}

NackRequester::NackRequester(std::shared_ptr<UdpChannel> sender)
	:_sender(sender),
    _send_nack_delay_ms(kDefaultSendNackDelayMs)
{}

NackRequester::~NackRequester()
{}

void NackRequester::OnTimerCb(uvcore::Timer*)
{
    ProcessNacks();
}

void NackRequester::ProcessNacks()
{}

void NackRequester::AddPacketsToNack(uint16_t seq_num_start, uint16_t seq_num_end)
{
    auto it = _nack_list.lower_bound(seq_num_end - kMaxPacketAge);
    _nack_list.erase(_nack_list.begin(), it);

    uint16_t num_new_nacks = ForwardDiff(seq_num_start, seq_num_end);
    if (_nack_list.size() + num_new_nacks > kMaxNackPackets)
    {
        while (RemovePacketsUntilKeyFrame()
            && _nack_list.size() + num_new_nacks > kMaxNackPackets)
        {
        }
        if (_nack_list.size() + num_new_nacks > kMaxNackPackets)
        {
            _nack_list.clear();
        }
        //to do
        //请求一个关键帧
    }

    for (uint16_t seq_num = seq_num_start; seq_num != seq_num_end; ++seq_num) {
        // Do not send nack for packets that are already recovered by FEC or RTX
       /* if (recovered_list_.find(seq_num) != recovered_list_.end())
            continue;*/
        NackInfo nack_info(seq_num, seq_num/* + WaitNumberOfPackets(0.5)*/,
            SUtil::getTimeStampMilli());
        //RTC_DCHECK(nack_list_.find(seq_num) == nack_list_.end());
        _nack_list[seq_num] = nack_info;
    }
}

int NackRequester::OnReceivedPacket(uint16_t seq_num, bool is_keyframe)
{
    return OnReceivedPacket(seq_num, is_keyframe, false);
}

int NackRequester::OnReceivedPacket(uint16_t seq_num, bool is_keyframe, bool is_recovered)
{
    using namespace std::placeholders;
    if (!_timer)
    {
        _timer = uvcore::TimerHelpers::add_uv_timer(_sender->get_loop(), 20, 20,
            std::bind(&NackRequester::OnTimerCb, this, _1));
    }
    //初始化
    if (!_initialized)
    {
        _newest_seq_num = seq_num;
        if (is_keyframe)
        {
            _keyframe_list.insert(seq_num);
        }
        _initialized = true;
        return 0;
    }
    if (seq_num == _newest_seq_num)
    {
        return 0;
    }
    if (AheadOf(_newest_seq_num, seq_num))
    {
        auto it = _nack_list.find(seq_num);
        int nacks_sent_for_packet = 0;
        if (it != _nack_list.end())
        {
            nacks_sent_for_packet = it->second.retries;
            _nack_list.erase(it);
        }
        return nacks_sent_for_packet;
    }
    if (is_keyframe)
    {
        _keyframe_list.insert(seq_num);
        auto it = _keyframe_list.lower_bound(seq_num - kMaxPacketAge);
        if (it != _keyframe_list.end())
        {
            _keyframe_list.erase(_keyframe_list.begin(), it);
        }
    }
    AddPacketsToNack(_newest_seq_num + 1, seq_num);
    _newest_seq_num = seq_num;

    std::vector<uint16_t> nack_batch;
    GetNackBatch(kSeqNumOnly, nack_batch);
    //to do
    //发送nack包
    return 0;
}

void NackRequester::GetNackBatch(NackFilterOptions options, std::vector<uint16_t>& vecs)
{}

bool NackRequester::RemovePacketsUntilKeyFrame()
{
    while (!_keyframe_list.empty())
    {
        auto it = _nack_list.lower_bound(*_keyframe_list.begin());
        if (it != _nack_list.begin())
        {
            _nack_list.erase(_nack_list.begin(), it);
            return true;
        }
        _keyframe_list.erase(_keyframe_list.begin());
    }
    return false;
}

void NackRequester::ClearUpTo(uint16_t seq_num)
{}

void NackRequester::UpdateRtt(int64_t rtt_ms)
{
    _rtt = rtt_ms;
}