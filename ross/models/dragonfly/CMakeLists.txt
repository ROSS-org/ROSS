INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(dragonfly_srcs
dragonfly.c		dragonfly.h)

ADD_EXECUTABLE(dragonfly ${dragonfly_srcs})

TARGET_LINK_LIBRARIES(dragonfly ROSS m)
