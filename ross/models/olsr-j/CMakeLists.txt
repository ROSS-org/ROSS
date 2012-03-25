INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(olsr_srcs
	olsr-driver.c  
	olsr.h
)

ADD_EXECUTABLE(olsr-j olsr-main.c ${olsr_srcs})

TARGET_LINK_LIBRARIES(olsr-j ROSS m)

IF(UNIT_TESTS)
	SET(TEST_DIR ${ROSS_SOURCE_DIR}/../test)
	INCLUDE_DIRECTORIES(${TEST_DIR})

	SET(olsr-test-srcs
		${olsr_srcs}
		olsr-test.c
		olsr-test_Runner.c
		${TEST_DIR}/unity.c)

	ADD_EXECUTABLE(olsr-test ${olsr-test-srcs})
	TARGET_LINK_LIBRARIES(olsr-test ROSS m)
	ADD_TEST(olsr-test olsr-test)

ENDIF(UNIT_TESTS)
