SET(syncIO_srcs
    regress_test.cc)

ADD_EXECUTABLE(regress_test ${syncIO_srcs})

TARGET_LINK_LIBRARIES(regress_test syncIO)

FILE(COPY ${CMAKE_SOURCE_DIR}/ross/IO/test/16-procs_case DESTINATION
	  ${CMAKE_BINARY_DIR}/ross/IO/test)

FILE(COPY ${CMAKE_SOURCE_DIR}/ross/IO/test/8-procs_case DESTINATION
	  ${CMAKE_BINARY_DIR}/ross/IO/test)

FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/ross/IO/test/writeOutData)

ADD_TEST(syncIO_test mpiexec -np 8 ${CMAKE_CURRENT_BINARY_DIR}/regress_test 0 8 4)
ADD_TEST(syncIO_test2 mpiexec -np 16 ${CMAKE_CURRENT_BINARY_DIR}/regress_test 50400 16 4)
