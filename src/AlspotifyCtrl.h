#ifndef ALSPOTIFY_ALSPOTIFYCTRL_H
#define ALSPOTIFY_ALSPOTIFYCTRL_H

#include <drogon/HttpSimpleController.h>

class AlspotifyCtrl : public drogon::HttpSimpleController<AlspotifyCtrl>
{
public:
    void start();
    int32_t end();
    void asyncHandleHttpRequest(const drogon::HttpRequestPtr &req,
                                        std::function<void (const drogon::HttpResponsePtr &)> &&callback) override;
    PATH_LIST_BEGIN
        PATH_ADD("/", drogon::Post, drogon::Options);
    PATH_LIST_END
};
#endif//ALSPOTIFY_ALSPOTIFYCTRL_H
