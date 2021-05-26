#include <QApplication>
#include <drogon/drogon.h>
#include "AlspotifyCtrl.h"

int main(int argc, char *argv[]) {
    QApplication qApplication(argc, argv);
    auto ctrl = std::make_shared<AlspotifyCtrl>();
    ctrl->start();
    drogon::app()
            .setLogPath(".")
            .setLogLevel(trantor::Logger::kWarn)
            .addListener("127.0.0.1", 29192) //TODO: loadConfigFile("./config.json")
            .setThreadNum(1)
            .run();
    QApplication::exec();
    return ctrl->end();
}
