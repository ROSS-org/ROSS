# Launcher conventions for the test helpers below:
# - MPI tests use the find_package(MPI)-provided variables:
#       ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
#       <binary> <args> ${MPIEXEC_POSTFLAGS}
#   This replaces the hardcoded "mpirun -np 2" form so tests run under
#   whatever launcher CMake detected (e.g., mpiexec, srun, jsrun).
# - Single-rank tests (Sequential, OptDebug, RollbackCheck, _INST_Seq)
#   intentionally do NOT use the launcher wrapper — --synch=1, --synch=4,
#   and --synch=6 have a single-rank contract that would break if invoked
#   under mpiexec.
# - Test binary path is the $<TARGET_FILE:${target_name}> generator
#   expression so ctest invokes the binary by absolute path rather than
#   the cwd-relative "./${target_name}" form.
# - ${MPIEXEC_PREFLAGS} / ${MPIEXEC_POSTFLAGS} are passed through as-is;
#   we do not separate_arguments() them. ${MPIEXEC_EXECUTABLE} is treated
#   as a single command (mpiexec/mpirun/srun/jsrun), not a full command
#   line with embedded flags. Users on HPC sites with non-trivial launcher
#   needs feed extra prefix args via MPIEXEC_PREFLAGS as a CMake list,
#   which is the standard CMake MPI idiom.

## BASIC SCHEDULERS
FUNCTION(ROSS_TEST_SCHEDULERS target_name)
	# if a parent superbuild has called
	# enable_testing() at its own scope, add_test() here would otherwise
	# register against that scope even with ROSS_BUILD_TESTING=OFF.
	if(NOT ROSS_BUILD_TESTING)
		return()
	endif()
	add_test(NAME ${target_name}_SCHED_Sequential
		COMMAND $<TARGET_FILE:${target_name}> --synch=1)
	add_test(NAME ${target_name}_SCHED_Conservative
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=2
			${MPIEXEC_POSTFLAGS})
	add_test(NAME ${target_name}_SCHED_Optimistic
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=3 --extramem=100000
			${MPIEXEC_POSTFLAGS})
	add_test(NAME ${target_name}_SCHED_Realtime
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=5 --gvt-interval=1 --clock-rate=$ENV{CLOCK_SPEED} --extramem=100000
			${MPIEXEC_POSTFLAGS})
	add_test(NAME ${target_name}_SCHED_OptDebug
		COMMAND $<TARGET_FILE:${target_name}> --synch=4 --nkp=1 --extramem=1000000)
	add_test(NAME ${target_name}_SCHED_RollbackCheck
		COMMAND $<TARGET_FILE:${target_name}> --synch=6 --nkp=1)

	SET(sched_tests
		${target_name}_SCHED_Sequential
		${target_name}_SCHED_Conservative
		${target_name}_SCHED_Optimistic
		${target_name}_SCHED_Realtime
		${target_name}_SCHED_OptDebug)
	SET_TESTS_PROPERTIES(${sched_tests} PROPERTIES TIMEOUT 60)
ENDFUNCTION(ROSS_TEST_SCHEDULERS)

## INSTRUMENTATION TESTS
FUNCTION(ROSS_TEST_INSTRUMENTATION target_name)
	# See ROSS_TEST_SCHEDULERS above for why this guard exists.
	if(NOT ROSS_BUILD_TESTING)
		return()
	endif()
	add_test(NAME ${target_name}_INST_OptPE
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=3 --engine-stats=4 --event-trace=2 --extramem=100000 --vt-interval=10000 --vt-samp-end=100000
			${MPIEXEC_POSTFLAGS})
	add_test(NAME ${target_name}_INST_OptLP
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=3 --engine-stats=4 --event-trace=2 --kp-data=1 --lp-data=1 --extramem=100000 --vt-interval=10000 --vt-samp-end=100000
			${MPIEXEC_POSTFLAGS})

	add_test(NAME ${target_name}_INST_RTOptPE
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=5 --engine-stats=4 --event-trace=2 --extramem=100000 --vt-interval=10000 --vt-samp-end=100000
			${MPIEXEC_POSTFLAGS})
	add_test(NAME ${target_name}_INST_RTOptLP
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=5 --engine-stats=4 --event-trace=2 --kp-data=1 --lp-data=1 --extramem=100000 --vt-interval=10000 --vt-samp-end=100000
			${MPIEXEC_POSTFLAGS})

	add_test(NAME ${target_name}_INST_ConsPE
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=2 --engine-stats=4 --event-trace=2 --extramem=100000 --vt-interval=10000 --vt-samp-end=100000
			${MPIEXEC_POSTFLAGS})
	add_test(NAME ${target_name}_INST_ConsLP
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=2 --engine-stats=4 --event-trace=2 --kp-data=1 --lp-data=1 --extramem=100000 --vt-interval=10000 --vt-samp-end=100000
			${MPIEXEC_POSTFLAGS})

	add_test(NAME ${target_name}_INST_Seq
		COMMAND $<TARGET_FILE:${target_name}> --synch=1 --event-trace=1 --extramem=100000)

	add_test(NAME ${target_name}_INST_Model
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=3 --model-stats=4 --event-trace=2 --extramem=100000 --vt-interval=10000 --vt-samp-end=100000
			${MPIEXEC_POSTFLAGS})

	add_test(NAME ${target_name}_INST_DisableOutput
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=3 --engine-stats=4 --disable-output=1 --extramem=100000 --vt-interval=10000 --vt-samp-end=100000
			${MPIEXEC_POSTFLAGS})
	add_test(NAME ${target_name}_INST_ForceOverflow
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=3 --event-trace=1 --buffer-free=0 --extramem=100000
			${MPIEXEC_POSTFLAGS})
	add_test(NAME ${target_name}_INST_FullEventTrace
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=3 --event-trace=1 --extramem=100000
			${MPIEXEC_POSTFLAGS})
	add_test(NAME ${target_name}_INST_RBEventTrace
		COMMAND ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS}
			$<TARGET_FILE:${target_name}> --synch=3 --event-trace=2 --extramem=100000
			${MPIEXEC_POSTFLAGS})

	SET(inst_tests
		${target_name}_INST_OptPE
		${target_name}_INST_OptLP
		${target_name}_INST_RTOptPE
		${target_name}_INST_RTOptLP
		${target_name}_INST_ConsPE
		${target_name}_INST_ConsLP
		${target_name}_INST_Seq
		${target_name}_INST_Model
		${target_name}_INST_DisableOutput
		${target_name}_INST_ForceOverflow
		${target_name}_INST_RBEventTrace)
	SET_TESTS_PROPERTIES(${inst_tests} PROPERTIES TIMEOUT 60)
ENDFUNCTION(ROSS_TEST_INSTRUMENTATION)


# Discover model subdirectories via a flat (non-recursive) glob: every
# top-level entry under models/ that is a directory and contains a
# CMakeLists.txt gets added. This skips build/install/test dirs (no
# CMakeLists.txt there) and avoids the GLOB_RECURSE + FOLLOW_SYMLINKS
# recursion the previous form risked. Symlinked-in models keep working
# as a side effect of IS_DIRECTORY / EXISTS following symlinks — see
# models/README.md, where that workflow is no longer the documented
# path. New models should be standalone CMake projects that consume
# ROSS via find_package(ROSS). When the symlink workflow is fully
# retired, the glob below collapses to an explicit add_subdirectory(phold).
file(GLOB model_dirs LIST_DIRECTORIES TRUE
     RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
     "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(d ${model_dirs})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${d}"
       AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${d}/CMakeLists.txt")
        # Catch users still on the deprecated symlink workflow at the
        # moment they reconfigure. The README note only reaches new
        # users discovering the path; this reaches existing users. The
        # whole block goes away with the glob when the workflow is
        # fully retired.
        if(IS_SYMLINK "${CMAKE_CURRENT_SOURCE_DIR}/${d}")
            message(DEPRECATION
                "Model '${d}' was discovered via the deprecated "
                "symlink-into-models/ workflow. This path still works "
                "but will be removed in a future ROSS release. Please "
                "migrate to a standalone CMake project consuming ROSS "
                "via find_package(ROSS); see models/README.md.")
        endif()
        add_subdirectory(${d})
    endif()
endforeach()
