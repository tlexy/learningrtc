#include "pc_client.h"
#include <QtWidgets/QApplication>
#include <audio/common/audio_common.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    AudioCommon::init_device();

    PcClient w;
    w.resize(1280, 720);
    w.init();

    w.show();
    return a.exec();
}
