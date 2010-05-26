INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(wifi_srcs
	essl-prob-wifi.h  
	gsl-prob-wifi.h  
	none-prob-wifi.h  
	wifi.c  
	wifi.h
)

ADD_EXECUTABLE(wifi ${wifi_srcs})

TARGET_LINK_LIBRARIES(wifi ROSS m)
