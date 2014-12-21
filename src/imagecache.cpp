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

ImageCache::ImageCache(const std::string &directory):
    directory_(directory)
{
    if (!QDir(directory_.c_str()).exists())
        QDir().mkpath(directory_.c_str());
}

bool ImageCache::Exists(const std::string &url) {
    std::string path = GetPath(url);
    QFile file(QString(path.c_str()));
    return file.exists();
}

QImage ImageCache::Get(const std::string &url) {
    return QImage(QString(GetPath(url).c_str()));
}

void ImageCache::Set(const std::string &url, const QImage &image) {
    image.save(QString(GetPath(url).c_str()));
}

std::string ImageCache::GetPath(const std::string &url) {
    return directory_ + "/" + Util::Md5(url) + ".png";
}
