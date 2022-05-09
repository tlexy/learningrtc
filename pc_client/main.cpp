#include "pc_client.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    pc_client w;
    w.show();
    return a.exec();
}
