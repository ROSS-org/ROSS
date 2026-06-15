# Subsystem driver tests. Each driver is a complete ROSS program (calls
# tw_init / tw_run / tw_end itself) that exercises one piece of the
# engine in isolation. See improve-testing.md for design rationale.
#
# This file is reached via add_subdirectory(tests) at top-level, itself
# gated on ROSS_BUILD_TESTING -- so by the time we get here, the gate
# is on. Same pattern as ROSS_TEST_SCHEDULERS.

# ROSS_DRIVER_TEST(<name> [SCHEDULERS <Sequential|OptDebug|RollbackCheck>...])
#
# Default scheduler set is all three single-rank schedulers. Override
# with SCHEDULERS when a driver's event-handler side effects don't
# survive replay under --synch=4/6 (e.g. drivers with assertions in
# the handler whose state they mutate).
function(ROSS_DRIVER_TEST name)
    if(NOT ROSS_BUILD_TESTING)
        return()
    endif()

    cmake_parse_arguments(ARG "" "" "SCHEDULERS" ${ARGN})
    if(NOT ARG_SCHEDULERS)
        set(ARG_SCHEDULERS Sequential OptDebug RollbackCheck)
    endif()

    add_executable(${name}
        drivers/${name}.c
        helpers/test_lp_minimal.c)
    target_include_directories(${name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/helpers)
    target_link_libraries(${name} PRIVATE ROSS::ROSS)

    foreach(sched IN LISTS ARG_SCHEDULERS)
        if(sched STREQUAL "Sequential")
            add_test(NAME ${name}_SCHED_Sequential
                     COMMAND $<TARGET_FILE:${name}> --synch=1)
        elseif(sched STREQUAL "OptDebug")
            add_test(NAME ${name}_SCHED_OptDebug
                     COMMAND $<TARGET_FILE:${name}> --synch=4 --nkp=1)
        elseif(sched STREQUAL "RollbackCheck")
            add_test(NAME ${name}_SCHED_RollbackCheck
                     COMMAND $<TARGET_FILE:${name}> --synch=6 --nkp=1)
        else()
            message(FATAL_ERROR
                "ROSS_DRIVER_TEST(${name}): unknown scheduler '${sched}'")
        endif()
        set_tests_properties(${name}_SCHED_${sched} PROPERTIES TIMEOUT 30)
    endforeach()
endfunction()

ROSS_DRIVER_TEST(lifecycle_test)
ROSS_DRIVER_TEST(rand_reverse_test)

# Splay's event handler asserts and mutates LP state (last_ts,
# delivery_count) without a paired reverse handler. Sequential only.
ROSS_DRIVER_TEST(splay_order_test SCHEDULERS Sequential)
