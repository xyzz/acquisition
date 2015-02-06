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

#include "testutil.h"

#include "util.h"

const double kDelta = 1e-6;

#define QCOMPAREDOUBLE(a, b) QVERIFY(a - kDelta <= b && b <= a + kDelta)

void TestUtil::TestModMatcher() {
    double result = 0.0;
    QVERIFY(Util::MatchMod("+# to maximum Life", "+4.2 to maximum Life", &result));
    QCOMPAREDOUBLE(result, 4.2);
    QVERIFY(!Util::MatchMod("+# to maximum Life", "+4.2 to maximum Lif", &result));
    QVERIFY(Util::MatchMod("Adds #-# Physical Damage", "Adds 1.5-3.2 Physical Damage", &result));
    QCOMPAREDOUBLE(result, (1.5 + 3.2) / 2);
}
