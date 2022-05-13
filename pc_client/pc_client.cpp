#include "pc_client.h"
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <audio/common/audio_common.h>

#pragma execution_character_set("utf-8")

PcClient::PcClient(QWidget *parent)
    : QWidget(parent)
{
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
    vBodyLeftLayout->addStretch();

    hBodyLayout->addWidget(new QWidget());

    hMainLayout->addLayout(vBodyLeftLayout, 2);
    hMainLayout->addLayout(hBodyLayout, 7);

    hMainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(hMainLayout);

    for (auto it = AudioCommon::mic_device_list.begin(); it != AudioCommon::mic_device_list.end(); ++it)
    {
        _audio_com_box->addItem(tr(it->second->name));
    }
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

