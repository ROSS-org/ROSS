SET(mpiIO_srcs
    regress_test.cc)

ADD_EXECUTABLE(regress_test ${mpiIO_srcs})

TARGET_LINK_LIBRARIES(regress_test syncIO)

ADD_TEST(mpiIO_test mpiexec -np 1 ${CMAKE_CURRENT_BINARY_DIR}/regress_test 0 8 4)
