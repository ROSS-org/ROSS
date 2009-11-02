INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(phold_srcs
phold.c		phold.h)

ADD_EXECUTABLE(phold ${phold_srcs})

TARGET_LINK_LIBRARIES(phold ROSS)
