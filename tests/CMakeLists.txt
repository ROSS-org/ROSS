# Subsystem driver tests. Each driver is a complete ROSS program (calls
# tw_init / tw_run / tw_end itself) that exercises one piece of the
# engine in isolation. See improve-testing.md for design rationale.
#
# This file is reached via add_subdirectory(tests) at top-level, itself
# gated on ROSS_BUILD_TESTING -- so by the time we get here, the gate
# is on. Same pattern as ROSS_TEST_SCHEDULERS.

function(ROSS_DRIVER_TEST name)
    if(NOT ROSS_BUILD_TESTING)
        return()
    endif()

    add_executable(${name}
        drivers/${name}.c
        helpers/test_lp_minimal.c)
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/helpers)
    target_link_libraries(${name} PRIVATE ROSS::ROSS)

    # Single-rank schedulers only for now.
    add_test(NAME ${name}_SCHED_Sequential
             COMMAND $<TARGET_FILE:${name}> --synch=1)
    add_test(NAME ${name}_SCHED_OptDebug
             COMMAND $<TARGET_FILE:${name}> --synch=4 --nkp=1)
    add_test(NAME ${name}_SCHED_RollbackCheck
             COMMAND $<TARGET_FILE:${name}> --synch=6 --nkp=1)

    set_tests_properties(
        ${name}_SCHED_Sequential
        ${name}_SCHED_OptDebug
        ${name}_SCHED_RollbackCheck
        PROPERTIES TIMEOUT 30)
endfunction()

ROSS_DRIVER_TEST(lifecycle_test)
ROSS_DRIVER_TEST(rand_reverse_test)
