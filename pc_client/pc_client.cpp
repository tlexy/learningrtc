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
    _audio_com_box->addItem(tr("ϵͳ¼����"));

    _video_com_box = new QComboBox(this);
    _video_com_box->addItem(tr("ϵͳ�����"));

    _join_btn = new QPushButton(tr("���뷿��"), this);
    _leave_btn = new QPushButton(tr("�뿪����"), this);

    _room_id_le = new QLineEdit(this);
    _uid_le = new QLineEdit(this);

    _listen_btn = new QPushButton(tr("����"), this);
    _pushlish_btn = new QPushButton(tr("����"), this);
    _ip_le = new QLineEdit(this);
    _ip_le->setText(QString("127.0.0.1"));
    _port_le = new QLineEdit(this);
    QLabel* ip_lbl = new QLabel(tr("�Զ�IP��ַ"), this);
    QLabel* port_lbl = new QLabel(tr("�˿�"), this);

    QLabel* video_lbl = new QLabel(tr("�����"), this);
    QLabel* audio_lbl = new QLabel(tr("¼����"), this);

    QLabel* room_lbl = new QLabel(tr("����ID"), this);
    QLabel* uid_lbl = new QLabel(tr("�û�ID"), this);

    vBodyLeftLayout->addStretch();
    vBodyLeftLayout->addLayout(create_layout({ audio_lbl, _audio_com_box }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ video_lbl, _video_com_box }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ room_lbl, _room_id_le }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ uid_lbl, _uid_le }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ _join_btn, _leave_btn }, new QHBoxLayout));
    vBodyLeftLayout->addSpacing(20);
    vBodyLeftLayout->addLayout(create_layout({ ip_lbl, _ip_le }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ port_lbl, _port_le }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ _listen_btn, _pushlish_btn }, new QHBoxLayout));
    vBodyLeftLayout->addStretch();

    hBodyLayout->addWidget(new QWidget());

    hMainLayout->addLayout(vBodyLeftLayout, 2);
    hMainLayout->addLayout(hBodyLayout, 7);

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

    //startTimer(200);
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

