#include <CUnit/CUnit.h>
#include "log.h"

void test_ctoll()
{
        CU_ASSERT_EQUAL(ctoll("INFO"), INFO);
        CU_ASSERT_EQUAL(ctoll("WARN"), WARN);
        CU_ASSERT_EQUAL(ctoll("DEBUG"), DEBUG);
        CU_ASSERT_EQUAL(ctoll("FAIL"), FAIL);
}

void init_log_tests()
{
        CU_pSuite psuite;
        psuite = CU_add_suite("log_tests", 0, 0);
        CU_add_test(psuite, "sum_test", test_ctoll);
}
