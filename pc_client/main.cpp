#include "pc_client.h"
#include <QtWidgets/QApplication>
#include <audio/common/audio_common.h>

#include <endec/core/audio_io.h>

void aac_cb(const uint8_t*, int len)
{
    std::cout << "aac callback, len=" << len << std::endl;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    AudioCommon::init_device();

    PcClient w;
    w.resize(1280, 720);
    w.init();

    /*AudioIO* audio_io = new AudioIO(128000);
    audio_io->set_io_cb(std::bind(aac_cb, std::placeholders::_1, std::placeholders::_2));
    audio_io->start();*/

    w.show();
    int ret = a.exec();
    AudioCommon::destory();
    return ret;
}
