#include "pc_client.h"
#include <QtWidgets/QApplication>
#include <audio/common/audio_common.h>
#include "common/pc_global.h"
#include <endec/core/audio_io.h>

#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "Iphlpapi.lib")
#pragma comment (lib, "Psapi.lib")
#pragma comment (lib, "Userenv.lib")

void aac_cb(const uint8_t*, int len)
{
    std::cout << "aac callback, len=" << len << std::endl;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    PcGlobal::get_instance()->init();
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
    w.destroy();
    PcGlobal::get_instance()->destroy();
    return ret;
}
