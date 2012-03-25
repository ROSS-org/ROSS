#include <ross.h>
#include "unity.h"

void tearDown(void)
{
}

void setUp(void)
{
}

void test_region(void)
{
    TEST_ASSERT_EQUAL(region(15), 0);
}