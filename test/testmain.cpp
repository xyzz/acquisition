#include <QTest>
#include <memory>

#include "porting.h"
#include "test/testitem.h"

#define TEST(Class) result |= QTest::qExec(std::make_unique<Class>().get())

int test_main() {
    int result = 0;

    TEST(TestItem);

    return result != 0 ? -1 : 0;
}
