#ifndef INC_check_revent_state_check
#define INC_check_revent_state_check

typedef struct crv_lpstate_checkpoint {
    void * state;
    tw_rng_stream rng;
    tw_rng_stream core_rng;
} crv_lpstate_checkpoint;

void crv_copy_lpstate(crv_lpstate_checkpoint * into, const tw_lp * clp);

void crv_check_lpstates(
         tw_lp * clp,
         tw_event * cev,
         const crv_lpstate_checkpoint * before_state,
         const char * before_msg,
         const char * after_msg
);

#endif
