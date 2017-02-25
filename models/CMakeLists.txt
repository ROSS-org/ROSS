## BASIC SCHEDULERS
FUNCTION(ROSS_TEST_SCHEDULERS target_name)
	ADD_TEST(${target_name}_SCHED_Sequential ${target_name} --synch=1)
	ADD_TEST(${target_name}_SCHED_Conservative mpirun -np 2 ./${target_name} --synch=2)
	ADD_TEST(${target_name}_SCHED_Optimistic mpirun -np 2 ./${target_name} --synch=3)
	ADD_TEST(${target_name}_SCHED_Realtime mpirun -np 2 ./${target_name} --synch=5 --gvt-interval=1 --clock-rate=$ENV{CLOCK_SPEED})
	ADD_TEST(${target_name}_SCHED_OptDebug ${target_name} --synch=4 --nkp=1 --extramem=1000000)

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
	ADD_TEST(${target_name}_INST_OptPE mpirun -np 2 ./${target_name} --synch=3 --real-time-samp=100 --enable-gvt-stats=1 --event-trace=1)
	ADD_TEST(${target_name}_INST_OptLP mpirun -np 2 ./${target_name} --synch=3 --real-time-samp=100 --enable-gvt-stats=1 --event-trace=1 --granularity=1)

	ADD_TEST(${target_name}_INST_RTOptPE mpirun -np 2 ./${target_name} --synch=5 --real-time-samp=100 --enable-gvt-stats=1 --event-trace=1)
	ADD_TEST(${target_name}_INST_RTOptLP mpirun -np 2 ./${target_name} --synch=5 --real-time-samp=100 --enable-gvt-stats=1 --event-trace=1 --granularity=1)

	ADD_TEST(${target_name}_INST_ConsPE mpirun -np 2 ./${target_name} --synch=2 --real-time-samp=100 --enable-gvt-stats=1 --event-trace=1)
	ADD_TEST(${target_name}_INST_ConsLP mpirun -np 2 ./${target_name} --synch=2 --real-time-samp=100 --enable-gvt-stats=1 --event-trace=1 --granularity=1)

	ADD_TEST(${target_name}_INST_DisableOutput mpirun -np 2 ./${target_name} --synch=3 --real-time-samp=100 --enable-gvt-stats=1 --event-trace=1 --disable-output=1)
	ADD_TEST(${target_name}_INST_ForceOverflow mpirun -np 2 ./${target_name} --synch=3 --event-trace=1 --buffer-free=0)
	ADD_TEST(${target_name}_INST_RBEventTrace mpirun -np 2 ./${target_name} --synch=3 --event-trace=2)

	SET(inst_tests
		${target_name}_INST_OptPE
		${target_name}_INST_OptLP
		${target_name}_INST_RTOptPE
		${target_name}_INST_RTOptLP
		${target_name}_INST_ConsPE
		${target_name}_INST_ConsLP
		${target_name}_INST_DisableOutput
		${target_name}_INST_ForceOverflow
		${target_name}_INST_RBEventTrace)
	SET_TESTS_PROPERTIES(${inst_tests} PROPERTIES TIMEOUT 60)
ENDFUNCTION(ROSS_TEST_INSTRUMENTATION)


# Automatically search and add model subdirectories
CMAKE_POLICY(SET CMP0009 NEW) # follow sym-links
FILE(GLOB_RECURSE my_list . FOLLOW_SYMLINKS */CMakeLists.txt)
FOREACH(cmf ${my_list})
    GET_FILENAME_COMPONENT(dir_path ${cmf} PATH)
    ADD_SUBDIRECTORY(${dir_path})
#    MESSAGE(${dir_path})
ENDFOREACH(cmf)
