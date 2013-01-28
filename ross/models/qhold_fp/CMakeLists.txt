INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})

SET(qhold_fp_srcs
	qhold_fp_driver.c
	qhold_fp.h
)

ADD_EXECUTABLE(qhold_fp qhold_fp_main.c ${qhold_fp_srcs})

TARGET_LINK_LIBRARIES(qhold_fp ROSS m)
