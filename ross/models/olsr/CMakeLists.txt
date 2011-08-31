SET(network_dir "../network/coding/802.11b")
SET(prop_dir "../propagation/free-space")
SET(mobility_dir "../mobility")

INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR} ${network_dir} ${prop_dir} ${mobility_dir})

SET(wireless_common
	${network_dir}/coding-error.h
	${network_dir}/coding-error.c
	${prop_dir}/prop-loss.h
	${prop_dir}/prop-loss.c
	${mobility_dir}/mobility.h
	${mobility_dir}/mobility.c
)

SET(olsr_common
	olsr-constants.h
	olsr-types.h
	olsr-types.c
	olsr-hello.h
	olsr-hello.c
	olsr-node.h
	olsr-node.c
)

SET(olsr_model
    ${wireless_common}
	${olsr_common}
	olsr.c  
	olsr.h
)

ADD_EXECUTABLE(olsr ${olsr_model})

IF(HAVE_LIBMASS)
	TARGET_LINK_LIBRARIES(olsr ROSS mass)
ELSEIF(HAVE_LIBM)
	TARGET_LINK_LIBRARIES(olsr ROSS m)
ENDIF()




