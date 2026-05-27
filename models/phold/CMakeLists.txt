set(phold_srcs
    phold.h
)

set(phold_targets
    phold
    phold_comm_test
    phold_gvt_hook_test
    phold_gvt_hook_model_test
    phold_gvt_hook_timestamp_test
    phold_gvt_hook_comm_test
)

add_executable(phold phold.main.c ${phold_srcs})
add_executable(phold_comm_test phold.main.c ${phold_srcs})
add_executable(phold_gvt_hook_test phold-gvt-hook.main.c ${phold_srcs})
add_executable(phold_gvt_hook_model_test phold-gvt-hook.main.c ${phold_srcs})
add_executable(phold_gvt_hook_timestamp_test phold-gvt-hook.main.c ${phold_srcs})
add_executable(phold_gvt_hook_comm_test phold-gvt-hook.main.c ${phold_srcs})

# Include dirs ride along ROSS's INTERFACE_INCLUDE_DIRECTORIES, so this
# directory no longer needs its own INCLUDE_DIRECTORIES block. The legacy
# BGPM branch keyed on a variable that was never set (the actual option is
# USE_BGPM, gated to bgq builds) and is dropped.
foreach(t ${phold_targets})
    target_link_libraries(${t} PRIVATE ROSS m)
endforeach()

ROSS_TEST_SCHEDULERS(phold)
ROSS_TEST_INSTRUMENTATION(phold)

SET_TARGET_PROPERTIES(phold_comm_test PROPERTIES COMPILE_DEFINITIONS TEST_COMM_ROSS)
ROSS_TEST_SCHEDULERS(phold_comm_test)
ROSS_TEST_INSTRUMENTATION(phold_comm_test)

ROSS_TEST_SCHEDULERS(phold_gvt_hook_test)

SET_TARGET_PROPERTIES(phold_gvt_hook_model_test PROPERTIES COMPILE_DEFINITIONS TRIGGER_BY_MODEL)
ROSS_TEST_SCHEDULERS(phold_gvt_hook_model_test)

SET_TARGET_PROPERTIES(phold_gvt_hook_timestamp_test PROPERTIES COMPILE_DEFINITIONS TRIGGER_AT_TIMESTAMP)
ROSS_TEST_SCHEDULERS(phold_gvt_hook_timestamp_test)

SET_TARGET_PROPERTIES(phold_gvt_hook_comm_test PROPERTIES COMPILE_DEFINITIONS TEST_COMM_ROSS)
ROSS_TEST_SCHEDULERS(phold_gvt_hook_comm_test)

install(TARGETS ${phold_targets} RUNTIME DESTINATION bin)
