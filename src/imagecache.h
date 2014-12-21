#pragma once

#include <QImage>
#include <string>

class MainWindow;

class ImageCache {
public:
    explicit ImageCache(const std::string &directory);
    bool Exists(const std::string &url);
    QImage Get(const std::string &url);
    void Set(const std::string &url, const QImage &image);
private:
    std::string GetPath(const std::string &url);
    std::string directory_;
};
