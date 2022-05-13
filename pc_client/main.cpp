#include "pc_client.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    PcClient w;
    w.resize(1280, 720);
    w.init();

    w.show();
    return a.exec();
}
