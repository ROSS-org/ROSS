#include <ross.h>

void crv_check_lpstates(
         tw_lp * clp,
         tw_event * cev,
         const crv_lpstate_checkpoint * before_state,
         const char * before_msg,
         const char * after_msg
) {
    if (memcmp(before_state->state, clp->cur_state, clp->type->state_sz)) {
        fprintf(stderr, "Error found by a rollback of an event\n");
        fprintf(stderr, "  The state of the LP is not consistent when rollbacking!\n");
        fprintf(stderr, "  LPID = %lu\n", clp->gid);
        fprintf(stderr, "\n  LP contents (%s):\n", before_msg);
        tw_fprint_binary_array(stderr, before_state->state, clp->type->state_sz);
        fprintf(stderr, "\n  LP contents (%s):\n", after_msg);
        tw_fprint_binary_array(stderr, clp->cur_state, clp->type->state_sz);
        fprintf(stderr, "\n  Event contents:\n");
        tw_fprint_binary_array(stderr, cev, g_tw_msg_sz);
	    tw_net_abort();
    }
    if (memcmp(&before_state->rng, clp->rng, sizeof(struct tw_rng_stream))) {
        fprintf(stderr, "Error found by rollback of an event\n");
        fprintf(stderr, "  Random number generation `rng` did not rollback properly!\n");
        fprintf(stderr, "  This often happens if the random number generation was not properly rollbacked by the reverse handler!\n");
        fprintf(stderr, "  LPID = %lu\n", clp->gid);
        fprintf(stderr, "\n  rng contents (%s):\n", before_msg);
        tw_fprint_binary_array(stderr, &before_state->rng, sizeof(struct tw_rng_stream));
        fprintf(stderr, "\n  rng contents (%s):\n", after_msg);
        tw_fprint_binary_array(stderr, clp->rng, sizeof(struct tw_rng_stream));
        fprintf(stderr, "\n  Event contents:\n");
        tw_fprint_binary_array(stderr, cev, g_tw_msg_sz);
	    tw_net_abort();
    }
    if (memcmp(&before_state->core_rng, clp->core_rng, sizeof(struct tw_rng_stream))) {
        fprintf(stderr, "Error found by rollback of an event\n");
        fprintf(stderr, "  Random number generation `core_rng` did not rollback properly!\n");
        fprintf(stderr, "  LPID = %lu\n", clp->gid);
        fprintf(stderr, "\n  core_rng contents (%s):\n", before_msg);
        tw_fprint_binary_array(stderr, &before_state->core_rng, sizeof(struct tw_rng_stream));
        fprintf(stderr, "\n  core_rng contents (%s):\n", after_msg);
        tw_fprint_binary_array(stderr, clp->core_rng, sizeof(struct tw_rng_stream));
        fprintf(stderr, "\n  Event contents:\n");
        tw_fprint_binary_array(stderr, cev, g_tw_msg_sz);
	    tw_net_abort();
    }
}

void crv_copy_lpstate(crv_lpstate_checkpoint * into, const tw_lp * clp) {
    memcpy(into->state, clp->cur_state, clp->type->state_sz);
    memcpy(&into->rng, clp->rng, sizeof(struct tw_rng_stream));
    memcpy(&into->core_rng, clp->core_rng, sizeof(struct tw_rng_stream));
}
