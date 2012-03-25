#include <ross.h>
#include "unity.h"

void tearDown(void)
{
}

void setUp(void)
{
}

void test_pizza(void)
{
    TEST_ASSERT_EQUAL_STRING("pizza", "pizz");
}

void test_pq(void)
{
    tw_event e1, e2, e3;
    
    e1.recv_ts = 1.0;
    e2.recv_ts = 2.0;
    e3.recv_ts = 3.0;
    
    tw_pq *pq = tw_pq_create();
    
    tw_pq_enqueue(pq, &e3);
    tw_pq_enqueue(pq, &e2);
    tw_pq_enqueue(pq, &e1);
    
    TEST_ASSERT_EQUAL_FLOAT(1.0, tw_pq_minimum(pq));
}
