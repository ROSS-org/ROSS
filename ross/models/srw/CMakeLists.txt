INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(srw_srcs
    srw.c
    srw.h
)

ADD_EXECUTABLE(srw ${srw_srcs})

TARGET_LINK_LIBRARIES(srw ROSS m)

ADD_TEST(srwtest mpirun -np 2 ${CMAKE_CURRENT_BINARY_DIR}/srw --synch=1)
