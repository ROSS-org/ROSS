INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(olsr_srcs
	olsr-driver.c  
	olsr.h
)

ADD_EXECUTABLE(olsr-j ${olsr_srcs})

TARGET_LINK_LIBRARIES(olsr-j ROSS m)

