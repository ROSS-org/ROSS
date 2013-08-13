IF(MEMORY)

INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(memtest_srcs
memory.c memory.h)

ADD_EXECUTABLE(memory ${memtest_srcs})

TARGET_LINK_LIBRARIES(memory ROSS m)
ENDIF(MEMORY)
