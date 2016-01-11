INCLUDE_DIRECTORIES(${ROSS_BINARY_DIR})
IF(BGPM)
	INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR} ${BGPM_LIB})
ELSE(NOT(BGPM))
	INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR})
ENDIF(BGPM)

SET(phold_srcs
phold.c		phold.h)

ADD_EXECUTABLE(phold ${phold_srcs})

IF(BGPM)
	TARGET_LINK_LIBRARIES(phold ROSS imp_bgpm m)
ELSE(NOT(BGPM))
	TARGET_LINK_LIBRARIES(phold ROSS m)
ENDIF(BGPM)

add_test(PholdSequential phold --synch=1)

add_test(PholdConservative mpirun -np 2 ./phold --synch=2)

add_test(PholdOptimistic mpirun -np 2 ./phold --synch=3)

add_test(PholdRealtime mpirun -np 2 ./phold --synch=5 --gvt-interval=1 --clock-rate=$ENV{CLOCK_SPEED})

# For our optimistic debug bug :)
add_test(PholdOptDebug phold --synch=4 --nkp=1 --extramem=1000000)
