INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(olsr_srcs
	coding-error.h
	coding-error.c
	prop-loss.h
	prop-loss.c
	mobility.h
	mobility.c
	olsr.c  
	olsr.h
)

SET(olsr_test
	coding-error.h
	coding-error.c
	prop-loss.h
	prop-loss.c 
	olsr-tests.c
)


ADD_EXECUTABLE(olsr ${olsr_srcs})
# ADD_EXECUTABLE(olsr-tests ${olsr_test})

TARGET_LINK_LIBRARIES(olsr ROSS m)

