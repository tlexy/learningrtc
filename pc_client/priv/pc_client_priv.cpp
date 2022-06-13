#include "pc_client_priv.h"
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QVariant>
#include <QDebug>
#include "../net/net_client.h"
#include <uvnet/core/ip_address.h>
#include <uvnet/server/thread_timer_eventloop.h>
#include <log4u/core/common_log.h>
#include "../common/pc_global.h"
#include "../component/comm_thread.h"
#include <uvnet/core/udp_server.h>
#include "../test/peer_connection.h"
#include <uvnet/utils/sock_utils.h>
#include <QDebug>
#include <audio/common/audio_common.h>

#pragma execution_character_set("utf-8")

#define CONN_COMM(SIG, SLOT) connect(PcGlobal::get_instance()->comm_thread().get(), &CommThread::SIG, this, &PcClientPrivate::SLOT)

PcClientPrivate::PcClientPrivate(QObject*parent)
    : QObject(parent)
{
}

void PcClientPrivate::init(const std::string& appid, const std::string& ip, int port)
{
    _appid = appid;

    _loop = std::make_shared<uvcore::ThreadTimerEventLoop>();
    _loop->init(3000, nullptr);
    _loop->start_thread();

    udp_server = std::make_shared<uvcore::UdpServer>(_loop->get_loop());

    uvcore::IpAddress addr(port);
    addr.setIp(ip);
    _client = std::make_shared<NetTcpClient>(_loop->get_loop(), addr);

    //connect(PcGlobal::get_instance()->comm_thread().get(), &CommThread::sig_join_resp, this, &PcClientPrivate::slot_joinresp);
//    CONN_COMM(sig_join_resp, slot_joinresp);

}

void PcClientPrivate::destroy()
{
    _loop->stop_thread();
    if (pc)
    {
        pc->destory();
    }
}

void PcClientPrivate::join_room(const std::string& roomid, int64_t uid)
{
    _roomid = roomid;
    _uid = uid;
    _client->join_room(_appid, roomid, uid);
}

void PcClientPrivate::listen(int port)
{
    pc = std::make_shared<tests::PeerConnection>(udp_server);
    pc->listen(port);
    pc->start_play();
}

void PcClientPrivate::connect_to_peer(const std::string& ip, int port, int audio_device_idx)
{
    if (!sockets::isIp(ip))
    {
        qDebug() << "Not a Ip: " << QString::fromStdString(ip) << " FILE: " <<__FILE__;
        return;
    }

    //AudioCommon::init_device();
    uvcore::IpAddress addr(port);
    addr.setIp(ip);
    pc = std::make_shared<tests::PeerConnection>(udp_server);
    pc->connect(addr);
    pc->set_recorder_device(audio_device_idx);
    pc->start_stream();
}

