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
#
#	ADD_EXECUTABLE(olsr-test ${olsr-test-srcs})
#	TARGET_LINK_LIBRARIES(olsr-test ROSS m)
#	ADD_TEST(olsr-test olsr-test)

	EXEC_PROGRAM("/usr/bin/ruby ${CMAKE_SOURCE_DIR}/test/auto/generate_test_runner.rb ${CMAKE_CURRENT_SOURCE_DIR}/olsr-test.c")

	ADD_EXECUTABLE(olsr-test ${olsr-test-srcs})
	TARGET_LINK_LIBRARIES(olsr-test ROSS m)
	GET_TARGET_PROPERTY(test_binary olsr-test LOCATION)

	# Make sure you run the test_myprog executable after building the myprog application
	ADD_CUSTOM_TARGET(tester ALL DEPENDS olsr-test)
	ADD_CUSTOM_COMMAND(
		TARGET tester
		COMMAND ${test_binary}
		DEPENDS olsr-test
	)

ENDIF(UNIT_TESTS)
