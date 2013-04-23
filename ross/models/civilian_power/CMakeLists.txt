INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(civilian_srcs
dissim-main.c
dissim-main.h
civilian.h
powerline.h
substation.h
generator.h
building.h)

ADD_EXECUTABLE(civilian ${civilian_srcs})

TARGET_LINK_LIBRARIES(civilian ROSS m)
