INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(Intersection_srcs
Intersection.c		Intersection.h)

ADD_EXECUTABLE(Intersection ${Intersection_srcs})

TARGET_LINK_LIBRARIES(Intersection ROSS m)
