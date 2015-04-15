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

add_test(NAME sequential
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND phold --synch=1)

add_test(NAME conservative
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	    COMMAND mpirun -np 2 ./phold --synch=2)

add_test(NAME optimistic
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	    COMMAND mpirun -np 2 ./phold --synch=3)
