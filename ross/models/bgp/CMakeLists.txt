INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET( bgp_srcs
     bgp.h
     bgp-extern.h
     bgp-types.h
     bgp.c 
     bgp-globals.c
     bgp-cn.c
     bgp-ion.c 
     bgp-fs.c
     bgp-controller.c
     bgp-ddn.c )

ADD_EXECUTABLE(bgp ${bgp_srcs})

TARGET_LINK_LIBRARIES(bgp ROSS m)
