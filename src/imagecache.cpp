/*
    Copyright 2014 Ilya Zhuravlev

    This file is part of Acquisition.

    Acquisition is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Acquisition is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Acquisition.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "imagecache.h"

#include <QDir>
#include <QFile>
#include <QString>
#include <QCryptographicHash>

#include "mainwindow.h"
#include "util.h"

ImageCache::ImageCache(const QString &directory):
    directory_(directory)
{
    if (!QDir(directory_).exists())
        QDir().mkpath(directory_);
}

bool ImageCache::Exists(const QString &url) {
    QString path = GetPath(url);
    QFile file(path);
    return file.exists();
}

QImage ImageCache::Get(const QString &url) {
    return QImage(GetPath(url));
}

void ImageCache::Set(const QString &url, const QImage &image) {
    image.save(GetPath(url));
}

QString ImageCache::GetPath(const QString &url) {
    return directory_ + "/" + Util::Md5(url) + ".png";
}
