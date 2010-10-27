INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(wifi_srcs
	coding-error.h
	coding-error.c
	prop-loss.h
	prop-loss.c
	wifi.c  
	wifi.h
)

ADD_EXECUTABLE(wifi ${wifi_srcs})

TARGET_LINK_LIBRARIES(wifi ROSS m)

ADD_TEST(wifitest mpirun -np 1 ${CMAKE_CURRENT_BINARY_DIR}/wifi --synch=1)
