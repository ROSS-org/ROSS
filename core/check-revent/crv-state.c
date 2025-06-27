#include "ross.h"

// Defining simple linked list because we don't care about speed at
// initialization for SEQUENTIAL_ROLLBACK_CHECK
struct llist_chptr {
    crv_checkpointer * val;
    struct llist_chptr * next;
};
struct llist_chptr * head_linked_list = NULL;
struct llist_chptr * tail_linked_list = NULL;

struct crv_checkpointer ** checkpointer_for_lps = NULL;

void crv_add_custom_state_checkpoint(crv_checkpointer * state_checkpoint) {
    struct llist_chptr * next = malloc(sizeof(struct llist_chptr));
    if (next == NULL) {
        tw_error(TW_LOC, "Could not allocate memory");
    }
    next->val = state_checkpoint;
    next->next = NULL;

    if (head_linked_list == NULL) {
        tail_linked_list = head_linked_list = next;
    } else {
        tail_linked_list->next = next;
        tail_linked_list = next;
    }
}

static void crv_clean_linkedlist(void) {
    if (head_linked_list == NULL) {
        return;
    }

    struct llist_chptr * prev = head_linked_list;
    struct llist_chptr * iter = prev->next;
    while (iter != NULL) {
        free(prev);
        prev = iter;
        iter = iter->next;
    }
    free(tail_linked_list);
    head_linked_list = NULL;
    tail_linked_list = NULL;
}

size_t crv_init_checkpoints(void) {
    // No need to construct array with crv_checkpointer's if there isn't any
    if (head_linked_list == NULL) {
        return 0;
    }

    checkpointer_for_lps = malloc(g_tw_nlp * sizeof(struct crv_checkpointer *));

    size_t largest_lp_checkpoint = 0;
    for (size_t i = 0; i < g_tw_nlp; i++) {
        tw_lp * lp = g_tw_lp[i];
        struct crv_checkpointer * state_checkpointer = NULL;

        // Finding crv_checkpointer, if it exists, for current lp
        for (struct llist_chptr * iter = head_linked_list; iter != NULL; iter = iter->next) {
            if (lp->type == iter->val->lptype) {
                state_checkpointer = iter->val;
                break;
            }
        }

        checkpointer_for_lps[i] = state_checkpointer;

        if (state_checkpointer != NULL && state_checkpointer->sz_storage > largest_lp_checkpoint) {
            largest_lp_checkpoint = state_checkpointer->sz_storage;
        }

    }

    crv_clean_linkedlist();

    return largest_lp_checkpoint;
}

static crv_checkpointer * get_chkpntr(tw_lpid id) {
    if (checkpointer_for_lps == NULL) {
        return NULL;
    }
    return checkpointer_for_lps[id];
}

static void print_event(crv_checkpointer const * chkptr, tw_lp * clp, tw_event * cev) {
    fprintf(stderr, "\n  Event:\n  ---------\n");
    fprintf(stderr, "  Bit field contents\n");
    tw_bf b = cev->cv;
    fprintf(stderr, "  c0  c1  c2  c3  c4  c5  c6  c7  c8  c9  c10 c11 c12 c13 c14 c15\n");
    fprintf(stderr, "  %d   %d   %d   %d   %d   %d   %d   %d   %d   %d   %d   %d   %d   %d   %d   %d \n", b.c0, b.c1, b.c2, b.c3, b.c4, b.c5, b.c6, b.c7, b.c8, b.c9, b.c10, b.c11, b.c12, b.c13, b.c14, b.c15);
    fprintf(stderr, "  c16 c17 c18 c19 c20 c21 c22 c23 c24 c25 c26 c27 c28 c29 c30 c31\n");
    fprintf(stderr, "  %d   %d   %d   %d   %d   %d   %d   %d   %d   %d   %d   %d   %d   %d   %d   %d \n", b.c16, b.c17, b.c18, b.c19, b.c20, b.c21, b.c22, b.c23, b.c24, b.c25, b.c26, b.c27, b.c28, b.c29, b.c30, b.c31);
    fprintf(stderr, "  ---------\n  Event contents\n");
    tw_fprint_binary_array(stderr, "", tw_event_data(cev), g_tw_msg_sz);
    if (chkptr && chkptr->print_event) {
        fprintf(stderr, "---------------------------------\n");
        chkptr->print_event(stderr, "", clp->cur_state, tw_event_data(cev));
    }
    fprintf(stderr, "---------------------------------\n");
}

void crv_check_lpstates(
         tw_lp * clp,
         tw_event * cev,
         crv_lpstate_checkpoint_internal const * before_state,
         char const * before_msg,
         char const * after_msg
) {
    crv_checkpointer const * chkptr = get_chkpntr(clp->id);

    bool state_equal;
    if (chkptr && chkptr->check_lps) {
        state_equal = chkptr->check_lps(before_state->state, clp->cur_state);
    } else {
        state_equal = !memcmp(before_state->state, clp->cur_state, clp->type->state_sz);
    }

    if (!state_equal) {
        fprintf(stderr, "Error found by a rollback of an event\n");
        fprintf(stderr, "  The state of the LP is not consistent when rollbacking!\n");
        fprintf(stderr, "  LPID = %lu\n", clp->gid);
        fprintf(stderr, "\n  LP contents (%s):\n", before_msg);
        tw_fprint_binary_array(stderr, "", before_state->state, clp->type->state_sz);
        if (chkptr && chkptr->print_checkpoint) {
            fprintf(stderr, "---------------------------------\n");
            chkptr->print_checkpoint(stderr, "", before_state->state);
            fprintf(stderr, "---------------------------------\n");
        }
        fprintf(stderr, "\n  LP contents (%s):\n", after_msg);
        tw_fprint_binary_array(stderr, "", clp->cur_state, clp->type->state_sz);
        if (chkptr && chkptr->print_lp) {
            fprintf(stderr, "---------------------------------\n");
            chkptr->print_lp(stderr, "", clp->cur_state);
            fprintf(stderr, "---------------------------------\n");
        }
        print_event(chkptr, clp, cev);
	    tw_net_abort();
    }
    if (memcmp(&before_state->rng, clp->rng, sizeof(struct tw_rng_stream))) {
        fprintf(stderr, "Error found by rollback of an event\n");
        fprintf(stderr, "  Random number generation `rng` did not rollback properly!\n");
        fprintf(stderr, "  This often happens if the random number generation was not properly rollbacked by the reverse handler!\n");
        fprintf(stderr, "  LPID = %lu\n", clp->gid);
        fprintf(stderr, "\n  rng contents (%s):\n", before_msg);
        tw_fprint_binary_array(stderr, "", &before_state->rng, sizeof(struct tw_rng_stream));
        fprintf(stderr, "\n  rng contents (%s):\n", after_msg);
        tw_fprint_binary_array(stderr, "", clp->rng, sizeof(struct tw_rng_stream));
        print_event(chkptr, clp, cev);
	    tw_net_abort();
    }
    if (memcmp(&before_state->core_rng, clp->core_rng, sizeof(struct tw_rng_stream))) {
        fprintf(stderr, "Error found by rollback of an event\n");
        fprintf(stderr, "  Random number generation `core_rng` did not rollback properly!\n");
        fprintf(stderr, "  LPID = %lu\n", clp->gid);
        fprintf(stderr, "\n  core_rng contents (%s):\n", before_msg);
        tw_fprint_binary_array(stderr, "", &before_state->core_rng, sizeof(struct tw_rng_stream));
        fprintf(stderr, "\n  core_rng contents (%s):\n", after_msg);
        tw_fprint_binary_array(stderr, "", clp->core_rng, sizeof(struct tw_rng_stream));
        print_event(chkptr, clp, cev);
	    tw_net_abort();
    }
}

void crv_copy_lpstate(crv_lpstate_checkpoint_internal * into, tw_lp const * clp) {
    crv_checkpointer const * chkptr = get_chkpntr(clp->id);

    if (chkptr && chkptr->save_lp) {
        chkptr->save_lp(into->state, clp->cur_state);
    } else {
        memcpy(into->state, clp->cur_state, clp->type->state_sz);
    }
    memcpy(&into->rng, clp->rng, sizeof(struct tw_rng_stream));
    memcpy(&into->core_rng, clp->core_rng, sizeof(struct tw_rng_stream));
}

void crv_clean_lpstate(crv_lpstate_checkpoint_internal * state, tw_lp const * clp) {
    crv_checkpointer const * chkptr = get_chkpntr(clp->id);

    if (chkptr && chkptr->clean_lp) {
        chkptr->clean_lp(state->state);
    }
}
