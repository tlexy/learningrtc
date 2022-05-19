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
}

class PcClientPrivate : public QObject
{
    Q_OBJECT

public:
    PcClientPrivate(QObject*parent = Q_NULLPTR);

    void init(const std::string& appid, const std::string& ip, int port);
    void destroy();
    
    void join_room(const std::string& roomid, int64_t uid);

public slots:
    

private:
    

private:
    std::shared_ptr<uvcore::ThreadTimerEventLoop> _loop;
    std::shared_ptr<NetTcpClient> _client{nullptr};
    std::string _appid;
    std::string _roomid;
    int64_t _uid;

};
