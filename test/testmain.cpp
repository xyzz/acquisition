#include <QTest>
#include <memory>

#include "porting.h"
#include "test/testitem.h"
#include "test/testshop.h"

#define TEST(Class) result |= QTest::qExec(std::make_unique<Class>().get())

int test_main() {
    int result = 0;

    TEST(TestItem);
    TEST(TestShop);

    return result != 0 ? -1 : 0;
}
