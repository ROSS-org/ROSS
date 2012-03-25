INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(TEST_DIR ${ROSS_SOURCE_DIR}/../test)
INCLUDE_DIRECTORIES(${TEST_DIR})

SET(ross_pq_test_srcs
	test_ross_pq_example_Runner.c
	test_ross_pq_example.c
	${TEST_DIR}/unity.c
)

ADD_EXECUTABLE(ross_test_pq ${ross_pq_test_srcs})

TARGET_LINK_LIBRARIES(ross_test_pq ROSS m)

ADD_TEST(pq_test ross_test_pq)
