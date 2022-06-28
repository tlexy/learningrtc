#pragma once

#include <QtWidgets/QWidget>
#include <string>
#include <stdint.h>
#include <vector>
#include <memory>

#pragma execution_character_set("utf-8")

class QComboBox;
class QPushButton;
class QLineEdit;
class QBoxLayout;
class PcClientPrivate;
class SdlPlayerWidget;
class OpenGLPlayerWidget;

namespace webrtc::test
{
    class  VcmCapturer;
}

class PcClient : public QWidget
{
    Q_OBJECT

public:
    PcClient(QWidget *parent = Q_NULLPTR);

    void init();
    void destroy();

public slots:
    void slot_audio_device_change(int index);
    void slot_join();
    void slot_joinresp(int status);
    void slot_listen();
    void slot_connect();

private:
    QBoxLayout* create_layout(const std::vector<QWidget*> widgets, QBoxLayout*);

protected:
    void timerEvent(QTimerEvent* tve) override;
    void closeEvent(QCloseEvent* event) override;

private:
    QComboBox* _audio_com_box;
    QComboBox* _video_com_box;
    QPushButton* _join_btn;
    QPushButton* _leave_btn;
    QLineEdit* _room_id_le;
    QLineEdit* _uid_le;

    QLineEdit* _ip_le;
    QLineEdit* _port_le;
    QPushButton* _listen_btn;
    QPushButton* _pushlish_btn;
    int _audio_device_idx{-1};

    std::shared_ptr <OpenGLPlayerWidget> _gl_player;
    //std::shared_ptr<webrtc::test::VcmCapturer> _vcm_capturer;
    webrtc::test::VcmCapturer* _vcm_capturer;

    std::shared_ptr<PcClientPrivate> _d;

    std::string _appid;
    std::string _room_id;
    int64_t _uid;
    bool _device_init{false};

};
