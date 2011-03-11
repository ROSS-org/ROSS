INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(test_src
	test-clock.c
)

ADD_EXECUTABLE(test-clock test-clock.c)

TARGET_LINK_LIBRARIES(test-clock ROSS)

ADD_TEST(clock test-clock)