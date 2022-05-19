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

#pragma execution_character_set("utf-8")

PcClient::PcClient(QWidget *parent)
    : QWidget(parent)
{
    _d = std::make_shared<PcClientPrivate>(parent);
    _d->init("talkischeap", "127.0.0.1", 5678);
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

    _join_btn = new QPushButton(tr("加入房间"), this);
    _leave_btn = new QPushButton(tr("离开房间"), this);

    _room_id_le = new QLineEdit(this);
    _uid_le = new QLineEdit(this);

    QLabel* video_lbl = new QLabel(tr("摄像机"), this);
    QLabel* audio_lbl = new QLabel(tr("录音机"), this);

    QLabel* room_lbl = new QLabel(tr("房间ID"), this);
    QLabel* uid_lbl = new QLabel(tr("用户ID"), this);

    vBodyLeftLayout->addStretch();
    vBodyLeftLayout->addLayout(create_layout({ audio_lbl, _audio_com_box }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ video_lbl, _video_com_box }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ room_lbl, _room_id_le }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ uid_lbl, _uid_le }, new QHBoxLayout));
    vBodyLeftLayout->addLayout(create_layout({ _join_btn, _leave_btn }, new QHBoxLayout));
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

    connect(_audio_com_box, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        [&](int index) {
            slot_audio_device_change(index);
        });
}

void PcClient::slot_join()
{
    log_info("join button");
    _d->join_room("", 123);
}

void PcClient::slot_audio_device_change(int index)
{
    if (index >= _audio_com_box->count())
    {
        return;
    }
    QString device_name = _audio_com_box->itemText(index);
    QVariant var = _audio_com_box->itemData(index);
    qDebug() << var.value<int>() << " : " << device_name;
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
    return layout;
}

