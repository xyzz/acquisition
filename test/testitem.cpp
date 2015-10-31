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

#include "testitem.h"

#include "rapidjson/document.h"

#include "item.h"
#include "testdata.h"

void TestItem::Parse() {
    rapidjson::Document doc;
    doc.Parse(kItem1.c_str());

    Item item(doc);

    // no need to check everything, just some basic properties
    QCOMPARE(item.name().c_str(), "Demon Ward");
    QCOMPARE(item.sockets().b, 0);
    QCOMPARE(item.sockets().g, 1);
    QCOMPARE(item.sockets().r, 0);
    QCOMPARE(item.sockets().w, 0);

    // the hash should be the same between different versions of Acquisition and OSes
    QCOMPARE(item.hash().c_str(), "605d9f566bc4305f4fd425efbbbed6a6");
    // This needs to match so that item hash migration is successful
    QCOMPARE(item.old_hash().c_str(), "5f083f2f5ceb10ed720bd4c1771ed09d");
}
