#pragma once

#include <QImage>
#include <QString>

class MainWindow;

class ImageCache {
public:
    explicit ImageCache(const QString &directory);
    bool Exists(const QString &url);
    QImage Get(const QString &url);
    void Set(const QString &url, const QImage &image);
private:
    QString GetPath(const QString &url);
    QString directory_;
};
