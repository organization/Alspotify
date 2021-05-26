#include "AlspotifyCtrl.h"

#include <QApplication>
#include <QEvent>
#include <QMainWindow>
#include <QWidget>
#include <QBoxLayout>
#include <QLabel>

#include <chrono>
#include <vector>
#include <map>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include <alsongpp/alsong.h>

struct Info {
    bool playing = false;
    int64_t progress = 0;
    std::string title;
    std::string artist;
    int64_t duration = 0;
    std::map<int64_t, std::vector<std::string>> lyrics;
} now_info;

std::string last_uri;

struct {
    struct {
        struct {
            std::string color = "#FFFFFF";
            std::string background = "rgba(29, 29, 29, .70)";
            int32_t font_size = 12;
            std::string align = "right";
            int32_t width = 500;
            int32_t height = 150;
        } lyric;
        std::string font = "IBM Plex Sans KR SemiBold";
    } style;
    struct {
        std::string overflow = "none";
        int32_t count = 3;
    } lyric;
    struct {
        int32_t x = 1320;
        int32_t y = 830;
        int32_t w = 500;
        int32_t h = 150;
    } window_position;
} config;

void updateProgress(int64_t progress);

class LyricItem : public QLabel {
public:
    int64_t lyric_id;
    QFont font = QFont(config.style.font.c_str(), config.style.lyric.font_size);
    QFontMetrics font_metrics = QFontMetrics(font);
    explicit LyricItem(int64_t lyric_id) {
        this->lyric_id = lyric_id;
        setObjectName("LyricItem");
        hide();

        setStyleSheet(
                std::string("QLabel {color: ")
                        .append(config.style.lyric.color)
                        .append(";background: ")
                        .append(config.style.lyric.background)
                        .append(";padding: 2px 5px;margin: 1px 0;}")
                        .c_str());
        setFont(this->font);
        if (config.lyric.overflow == "wrap") {
            setWordWrap(true);
        }
    }
    bool event(QEvent *event) override {
        if (event->type() == QEvent::User) {
            show();
            return true;
        }
        return false;
    }
};

std::vector<std::function<void (Info, std::string, LyricItem *)>> handler;

class LyricsView : public QWidget {
public:
    std::vector<LyricItem *> lyric_items;
    LyricsView() {
        setObjectName("LyricView");
        auto layout = new QBoxLayout(QBoxLayout::Direction::TopToBottom);

        setLayout(layout);

        for (int32_t i = 0; i < config.lyric.count; i++) {
            auto lyric_item = new LyricItem(i);
            auto label_layout = new QBoxLayout(QBoxLayout::Direction::RightToLeft);
            this->lyric_items.emplace_back(lyric_item);
            layout->addLayout(label_layout);

            if (config.style.lyric.align == "left") {
                label_layout->addStretch(1);
                label_layout->addWidget(lyric_item);
            } else if (config.style.lyric.align == "right") {
                label_layout->addWidget(lyric_item);
                label_layout->addStretch(1);
            } else {
                label_layout->addWidget(lyric_item);
            }
            lyric_item->setText("테스트입니다~");
            lyric_item->show();

            handler.emplace_back([] (const Info& info, const std::string& current, LyricItem *lyric_item) {
                if (!info.lyrics.empty()) {
                    if (!lyric_item->isVisible()) {
                        lyric_item->moveToThread(QApplication::instance()->thread());
                        QApplication::postEvent(lyric_item, new QEvent(QEvent::User));
                    }
                    std::cout << current << std::endl;
                    auto bounding_rect = lyric_item->font_metrics.boundingRect(current.c_str());
                    auto elide_width = config.style.lyric.width - 20;

                    if (bounding_rect.width() > elide_width) {
                        if (config.lyric.overflow == "elide") {
                            lyric_item->setText(lyric_item->font_metrics.elidedText(current.c_str(), Qt::TextElideMode::ElideRight, elide_width));
                            goto end;
                        }
                    }
                    lyric_item->setText(current.c_str());
                end:
                    lyric_item->setFixedSize(bounding_rect.width() + 20, bounding_rect.height() + 10);
                } else {
                    if (lyric_item->isVisible()) {
                        //lyric_item->setText(" ");
                        //lyric_item->hide();
                    }
                }
            });
        }

        setStyleSheet(
                std::string("QWidget {width: ")
                        .append(std::to_string(config.style.lyric.width))
                        .append("px;height: ")
                        .append(std::to_string(config.style.lyric.height))
                        .append("px;margin-bottom: 20px;}").c_str());
    }
    ~LyricsView() override {
        lyric_items.clear();
    }
};

class MainWindow : public QMainWindow {
public:
    LyricsView *lyrics_view;
    MainWindow() {
        setWindowTitle("Alspotify");
        setWindowFlag(Qt::WindowType::FramelessWindowHint, true);
        setWindowFlag(Qt::WindowType::WindowStaysOnTopHint, true);
        setWindowFlag(Qt::WindowType::WindowTransparentForInput, true);
        setWindowFlag(Qt::WindowType::SubWindow, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        //setWindowOpacity(0);

        auto layout = new QHBoxLayout();
        auto widget = new QWidget();
        widget->setObjectName("Root");
        widget->setLayout(layout);

        lyrics_view = new LyricsView();
        layout->addWidget(lyrics_view);
        setCentralWidget(widget);
        setGeometry(config.window_position.x, config.window_position.y, config.window_position.w, config.window_position.h);
        widget->setStyleSheet(
                std::string("QMainWindow {width: ")
                        .append(std::to_string(config.window_position.w))
                        .append("px;")
                        .append("align-items: flex-end;flex-direction: column;}").c_str()
                );
    };
    ~MainWindow() override {
        delete lyrics_view;
    }
};

MainWindow *win;

void updateProgress(int64_t progress) {
    auto last = now_info.lyrics.lower_bound(progress);
    if (last != now_info.lyrics.begin()) {
        --last;
    }
    auto vec = last->second;
    int32_t count = 0;
    for (const auto& current : vec) {
        if (count < config.lyric.count) {
            auto item = win->lyrics_view->lyric_items[count];
            handler[count](now_info, current, item);
            ++count;
        }
    }
}

inline int64_t now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

int64_t last_update = -1;
bool is_running = true;

void AlspotifyCtrl::start() {
    win = new MainWindow();
    win->show();
    std::thread([] {
        while (is_running) {
            last_update = now_ms();
            now_info.progress += 50;
            updateProgress(now_info.progress);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }).detach();
}

int32_t AlspotifyCtrl::end() {
    is_running = false;
    delete win;
    return 0;
}

void update_lyric(const std::map<int64_t, std::vector<std::string>>& spotify_lyrics) {
    alsongpp::Alsong alsong;
    try {
        auto lyric_list = alsong.get_resemble_lyric_list(now_info.artist, now_info.title, now_info.duration);
        now_info.lyrics = alsong.get_lyric_by_id(lyric_list[0].lyric_id).lyrics;
    } catch (const std::exception& e) {
        if (!spotify_lyrics.empty()) {
            now_info.lyrics = spotify_lyrics;
        } else {
            std::cerr << "Error while retrieving lyric: " << e.what() << std::endl;
            return;
        }
    }
    std::cout << "Retrieved lyric: " << now_info.artist << " - " << now_info.title << std::endl;
}

void AlspotifyCtrl::asyncHandleHttpRequest(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    const auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::HttpStatusCode::k200OK);
    callback(resp);
    std::cout << "test" << req->body() << std::endl;
    auto json_object = req->getJsonObject();

    if (!json_object) {
        return;
    }

    auto timestamp_json = json_object->get("timestamp", 0L);
    if (!timestamp_json.isNumeric() || timestamp_json.asInt64() == 0) {
        return;
    }

    int64_t timestamp = timestamp_json.asInt64();
    int64_t unixtime_in_ms = now_ms();
    if (std::abs(timestamp - unixtime_in_ms) > 5000) {
        return;
    }

    auto playing = json_object->get("playing", false).asBool();
    if (!playing) {
        now_info.playing = false;
        return;
    } else {
        now_info.playing = true;
    }

    auto progress = json_object->get("progress", 0).asInt64();
    auto duration = json_object->get("duration", 0).asInt64();
    if (progress < 0 || progress > duration) {
        return;
    }

    duration += now_ms() - timestamp;
    progress = std::max(0L, std::min(duration, progress));

    auto title = json_object->get("title", std::string()).asString();
    auto artist = json_object->get("artist", std::string()).asString();

    now_info.progress = progress;
    if (title.empty() || artist.empty()) {
        auto now_t = now_ms();
        if (last_update != now_t) {
            last_update = now_t;
            updateProgress(progress);
        }
        return;
    }

    if (duration < 0) {
        return;
    }

    now_info.playing = true;
    now_info.title = title;
    now_info.artist = artist;
    now_info.duration = duration;

    auto uri = json_object->get("uri", std::string()).asString();
    if (!uri.empty() && uri != last_uri) {
        last_uri = uri;
        std::map<int64_t, std::vector<std::string>> spotify_lyrics;
        auto lyrics = json_object->get("lyrics", 0);
        if (lyrics.isNumeric()) {
            return;
        }
        for (const auto& key : lyrics.getMemberNames()) {
            std::vector<std::string> lyric_list;
            auto detail = lyrics.get(key, 0);

            if (detail.isNumeric()) {
                continue;
            }

            for (const auto &i : detail) {
                lyric_list.emplace_back(i.asString());
            }

            spotify_lyrics[std::stol(key)] = lyric_list;
        }
        update_lyric(spotify_lyrics);
    }
}
