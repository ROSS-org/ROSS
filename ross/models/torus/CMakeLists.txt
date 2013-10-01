INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(torus_srcs
torus.c		torus.h)

ADD_EXECUTABLE(torus ${torus_srcs})

TARGET_LINK_LIBRARIES(torus ROSS m)
