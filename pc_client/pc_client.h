#pragma once

#include <QtWidgets/QWidget>
#include <string>
#include <stdint.h>
#include <vector>

#pragma execution_character_set("utf-8")

class QComboBox;
class QPushButton;
class QLineEdit;
class QBoxLayout;

class PcClient : public QWidget
{
    Q_OBJECT

public:
    PcClient(QWidget *parent = Q_NULLPTR);

    void init();

public slots:
    void slot_audio_device_change(int index);

private:
    QBoxLayout* create_layout(const std::vector<QWidget*> widgets, QBoxLayout*);

private:
    QComboBox* _audio_com_box;
    QComboBox* _video_com_box;
    QPushButton* _join_btn;
    QPushButton* _leave_btn;
    QLineEdit* _room_id_le;
    QLineEdit* _uid_le;

    std::string _appid;
    std::string _room_id;
    int64_t _uid;

};
