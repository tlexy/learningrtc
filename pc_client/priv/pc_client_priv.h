#pragma once

#include <QtWidgets/QWidget>
#include <string>
#include <stdint.h>
#include <vector>
#include <memory>

#pragma execution_character_set("utf-8")

class NetTcpClient;
namespace uvcore {
    class ThreadTimerEventLoop;
    class UdpServer;
}

namespace tests {
    class PeerConnection;
}

class PcClientPrivate : public QObject
{
    Q_OBJECT

public:
    PcClientPrivate(QObject*parent = Q_NULLPTR);

    void init(const std::string& appid, const std::string& ip, int port);
    void destroy();
    
    void join_room(const std::string& roomid, int64_t uid);

    void listen(int port);
    /// <summary>
    /// 连接到对端并发送数据
    /// </summary>
    /// <param name="ip"></param>
    /// <param name="port"></param>
    /// <param name="audio_device_idx"></param>
    void connect_to_peer(const std::string& ip, int port, int audio_device_idx);

public slots:
    

public:
    std::shared_ptr<uvcore::UdpServer> udp_server;
    std::shared_ptr<tests::PeerConnection> pc{nullptr};
private:
    std::shared_ptr<uvcore::ThreadTimerEventLoop> _loop;
    std::shared_ptr<NetTcpClient> _client{nullptr};
    std::string _appid;
    std::string _roomid;
    int64_t _uid;

};
