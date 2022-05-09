#pragma once

#include <QtWidgets/QWidget>
#include "ui_pc_client.h"

class pc_client : public QWidget
{
    Q_OBJECT

public:
    pc_client(QWidget *parent = Q_NULLPTR);

private:
    Ui::pc_clientClass ui;
};
