INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(raid_srcs
        raid.h
        raid-extern.h
        raid-types.h
        raid.c
        raid-globals.c
        raid-controller.c
        raid-server.c
        raid-disk.c
        raid-io.c
)

ADD_EXECUTABLE(raid ${raid_srcs})

TARGET_LINK_LIBRARIES(raid ROSS m)
