#include "pc_client.h"
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QVariant>
#include <QDebug>
#include <audio/common/audio_common.h>
#include "priv/pc_client_priv.h"
#include <log4u/core/common_log.h>
#include "common/pc_global.h"
#include "component/comm_thread.h"
#include <QTimerEvent>
#include <audio/common/audio_common.h>
#include <chrono>
#include <webrtc_camera/core/video_capture.h>
#include <webrtc_camera/core/video_capture_factory.h>
#include <webrtc_camera/vcm_capturer.h>
#include <QCloseEvent>
#include "sdl_player_widget.h"
#include "test/opengl_player_widget.h"
#include <QMessageBox>

#pragma execution_character_set("utf-8")

#define CONN_COMM(SIG, SLOT) connect(PcGlobal::get_instance()->comm_thread().get(), &CommThread::SIG, this, &PcClient::SLOT)


PcClient::PcClient(QWidget *parent)
    : QWidget(parent)
{
    _d = std::make_shared<PcClientPrivate>(parent);
    _d->init("talkischeap", "127.0.0.1", 5678);
}

void PcClient::timerEvent(QTimerEvent* tve)
{
    /*if (!_device_init)
    {
        AudioCommon::init_device();
        for (auto it = AudioCommon::mic_device_list.begin(); it != AudioCommon::mic_device_list.end(); ++it)
        {
            QString name = QString::fromStdString(it->second->name);
            if (name[0] != 65533)
            {
                _audio_com_box->addItem(name, QVariant(it->first));
            }
        }
        _device_init = true;
    }*/
}

void PcClient::destroy()
{
    _d->destroy();
}

void PcClient::init()
{
    QHBoxLayout* hMainLayout = new QHBoxLayout;

    QVBoxLayout* vBodyLeftLayout = new QVBoxLayout;
    QHBoxLayout* hBodyLayout = new QHBoxLayout;

    _audio_com_box = new QComboBox(this);
    _audio_com_box->addItem(tr("系统录音机"));

    _video_com_box = new QComboBox(this);
    _video_com_box->addItem(tr("系统摄像机"));

    _join_btn = new QPushButton(tr("加入房间"));
    _leave_btn = new QPushButton(tr("离开房间"));

    _room_id_le = new QLineEdit();
    _uid_le = new QLineEdit();

    _listen_btn = new QPushButton(tr("监听"));
    _pushlish_btn = new QPushButton(tr("推流"));
    _ip_le = new QLineEdit(this);
    _ip_le->setText(QString("127.0.0.1"));
    _port_le = new QLineEdit(this);
    QLabel* ip_lbl = new QLabel(tr("对端IP地址"));
    QLabel* port_lbl = new QLabel(tr("端口"));

    QLabel* video_lbl = new QLabel(tr("摄像机"));
    QLabel* audio_lbl = new QLabel(tr("录音机"));

    QLabel* room_lbl = new QLabel(tr("房间ID"));
    QLabel* uid_lbl = new QLabel(tr("用户ID"));

    vBodyLeftLayout->addStretch();
    vBodyLeftLayout->addLayout(create_layout({ audio_lbl, _audio_com_box }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ video_lbl, _video_com_box }, new QHBoxLayout));
    /*vBodyLeftLayout->addLayout(create_layout({ room_lbl, _room_id_le }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ uid_lbl, _uid_le }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ _join_btn, _leave_btn }, new QHBoxLayout));*/
    vBodyLeftLayout->addSpacing(20);
    vBodyLeftLayout->addLayout(create_layout({ ip_lbl, _ip_le }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ port_lbl, _port_le }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ _listen_btn, _pushlish_btn }, new QHBoxLayout));
    vBodyLeftLayout->addStretch();

    _holder_widget = new QWidget();

    _gl_player = std::make_shared<OpenGLPlayerWidget>();//new SdlPlayerWidget()
    _gl_player->setFixedSize(QSize(640, 480));
    /*QWidget* sdlWidgetContainer = new QWidget();
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->addWidget(_sdl_player.get());
    sdlWidgetContainer->setLayout(gridLayout);
    sdlWidgetContainer->setFixedSize(740, 520);*/
    hBodyLayout->addWidget(_holder_widget);//_holder_widget; sdlWidgetContainer

    hMainLayout->addLayout(vBodyLeftLayout, 2);
    hMainLayout->addLayout(hBodyLayout, 7);

    _body_layout = hBodyLayout;

    hMainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(hMainLayout);

    for (auto it = AudioCommon::mic_device_list.begin(); it != AudioCommon::mic_device_list.end(); ++it)
    {
        QString name = QString::fromStdString(it->second->name);
        if (name[0] != 65533)
        {
            _audio_com_box->addItem(name, QVariant(it->first));
        }
    }

    connect(_join_btn, &QPushButton::clicked, this, &PcClient::slot_join);

    connect(_listen_btn, &QPushButton::clicked, this, &PcClient::slot_listen);
    connect(_pushlish_btn, &QPushButton::clicked, this, &PcClient::slot_connect);

    connect(_audio_com_box, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        [&](int index) {
            slot_audio_device_change(index);
        });

    CONN_COMM(sig_join_resp, slot_joinresp);
    CONN_COMM(sig_video_ready, slot_video_ready);
    //connect(this, SIGNAL(close()), this, SLOT(slot_close()));
    
    //startTimer(200);

    int video_width = 640;
    int video_height = 480;
    //_vcm_capturer = std::make_shared<webrtc::test::VcmCapturer>(webrtc::test::VcmCapturer::Create(640, 480, 30, 0));
    _vcm_capturer = webrtc::test::VcmCapturer::Create(video_width, video_height, 30, 0);
    _vcm_capturer->AddSubscriber(_gl_player);//std::dynamic_pointer_cast<webrtc::test::VideoFrameSubscriber>(

    _d->vcm_capturer = _vcm_capturer;
    _d->video_height = video_height;
    _d->video_width = video_width;

    //_gl_player->init(video_width, video_height, 2);
    //_vcm_capturer->StartCapture();
    //_sdl_player->start(640, 480);
}

void PcClient::slot_video_ready(const VideoParameter& param)
{
    qDebug() << param.width << ":" << param.height << ":" << param.fps;
    if (param.width > 0)
    {
        _gl_player->init(param.width, param.height, 2);
        _body_layout->addWidget(_gl_player.get());
        _holder_widget->hide();
    }
}

void PcClient::closeEvent(QCloseEvent* event)
{
//_sdl_player->stop();
    delete _vcm_capturer;
    _vcm_capturer = nullptr;
    qDebug() << "close...";
    QWidget::closeEvent(event);
}

void PcClient::slot_join()
{
    log_info("join button");
    _d->join_room("", 123);
}

void PcClient::slot_listen()
{
    log_info("listen button");
    QString sport = _port_le->text();
    int port = std::atoi(sport.toStdString().c_str());
    if (port > 65535 || port <= 0)
    {
        QMessageBox::warning(this, tr("错误"), tr("端口号格式错误"));
        return;
    }
    if (port > 0)
    {
        _d->listen(port);
    }
}

void PcClient::slot_connect()
{
    log_info("connect button");
    QString sport = _port_le->text();
    int port = std::atoi(sport.toStdString().c_str());
    if (port <= 0)
    {
        log_error("connect error, port");
        return;
    }
    if (port > 65535)
    {
        QMessageBox::warning(this, tr("错误"), tr("端口号格式错误"));
        return;
    }
    QString ipstr = _ip_le->text();
    _d->connect_to_peer(ipstr.toStdString(), port, _audio_device_idx);
}

void PcClient::slot_joinresp(int status)
{
    qDebug() << "join resp " << status;
}

void PcClient::slot_audio_device_change(int index)
{
    if (index >= _audio_com_box->count())
    {
        return;
    }
    QString device_name = _audio_com_box->itemText(index);
    QVariant var = _audio_com_box->itemData(index);
    _audio_device_idx = var.value<int>();
    qDebug() << _audio_device_idx << " : " << device_name;
    //emit sig_input_device_change(device_name);
}

QBoxLayout* PcClient::create_layout(const std::vector<QWidget*> widgets, QBoxLayout* layout)
{
    layout->setContentsMargins(10, 10, 10, 10);
    for (int i = 0; i < widgets.size(); ++i)
    {
        layout->addWidget(widgets[i]);
        if (i != widgets.size() - 1)
        {
            layout->addStretch();
        }
    }
    //layout->addStretch();
    return layout;
}

