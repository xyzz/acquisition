/*
    Copyright 2015 Ilya Zhuravlev

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

#pragma once

#include <QNetworkReply>

// Automatically calls deleteLater on its QNetworkReply object when we go out of scope
class SelfDestructingReply {
public:
    explicit SelfDestructingReply(QNetworkReply *reply) :
        reply_(reply)
    {}
    ~SelfDestructingReply() {
        reply_->deleteLater();
    }
    SelfDestructingReply(const SelfDestructingReply &o) = delete;
    SelfDestructingReply &operator=(const SelfDestructingReply &o) = delete;
    QNetworkReply *operator->() {
        return reply_;
    }
private:
    QNetworkReply *reply_;
};
