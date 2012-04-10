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

        # Generate our test_Runner.c file from our C file containing the tests
        ADD_CUSTOM_COMMAND(
           OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/olsr-test_Runner.c
           COMMAND /usr/bin/ruby ${CMAKE_SOURCE_DIR}/test/auto/generate_test_runner.rb ${CMAKE_CURRENT_SOURCE_DIR}/olsr-test.c
           DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/olsr-test.c
        )
        ADD_EXECUTABLE(olsr-test ${TEST_DIR}/unity.c olsr-test.c olsr-test_Runner.c ${olsr_srcs})

	TARGET_LINK_LIBRARIES(olsr-test ROSS m)
	GET_TARGET_PROPERTY(test_binary olsr-test LOCATION)

	# Make sure you run the test executable after building the binary
	ADD_CUSTOM_TARGET(tester ALL DEPENDS olsr-test)
	ADD_CUSTOM_COMMAND(
		TARGET tester
		COMMAND ${test_binary}
		DEPENDS olsr-test
	)

ENDIF(UNIT_TESTS)
