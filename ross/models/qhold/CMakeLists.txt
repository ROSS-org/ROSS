INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(qhold_srcs
	qhold_driver.c
	qhold.h
)

ADD_EXECUTABLE(qhold qhold_main.c ${qhold_srcs})

TARGET_LINK_LIBRARIES(qhold ROSS m)
