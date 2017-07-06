#include "ross.h"
#include "io-config.h"

size_t io_lp_serialize (tw_lp *lp, void *buffer) {
    int i, j;

    io_lp_store tmp;

    tmp.gid = lp->gid;
    for (i = 0; i < g_tw_nRNG_per_lp; i++) {
        for (j = 0; j < 4; j++) {
            tmp.rng[j] = lp->rng->Ig[j];
            tmp.rng[j+4] = lp->rng->Lg[j];
            tmp.rng[j+8] = lp->rng->Cg[j];
        }
#ifdef RAND_NORMAL
        tmp.tw_normal_u1 = lp->rng->tw_normal_u1;
        tmp.tw_normal_u2 = lp->rng->tw_normal_u2;
        tmp.tw_normal_flipflop = lp->rng->tw_normal_flipflop;
#endif
    }

    memcpy(buffer, &tmp, sizeof(io_lp_store));
    return sizeof(io_lp_store);
}

size_t io_lp_deserialize (tw_lp *lp, void *buffer) {
    int i, j;

    io_lp_store tmp;
    memcpy(&tmp, buffer, sizeof(io_lp_store));

    lp->gid = tmp.gid;

    for (i = 0; i < g_tw_nRNG_per_lp; i++) {
        for (j = 0; j < 4; j++) {
            lp->rng->Ig[j] = tmp.rng[j];
            lp->rng->Lg[j] = tmp.rng[j+4];
            lp->rng->Cg[j] = tmp.rng[j+8];
        }
#ifdef RAND_NORMAL
        lp->rng->tw_normal_u1 = tmp.tw_normal_u1;
        lp->rng->tw_normal_u2 = tmp.tw_normal_u2;
        lp->rng->tw_normal_flipflop = tmp.tw_normal_flipflop;
#endif
    }
    return sizeof(io_lp_store);
}

size_t io_event_serialize (tw_event *e, void *buffer) {
    int i;

    io_event_store tmp;

    memcpy(&(tmp.cv), &(e->cv), sizeof(tw_bf));
    tmp.dest_lp = (tw_lpid)e->dest_lp; // ROSS HACK: dest_lp is gid
    tmp.src_lp = e->src_lp->gid;
    tmp.recv_ts = e->recv_ts - g_tw_ts_end;

    memcpy(buffer, &tmp, sizeof(io_event_store));
    // printf("Storing event going to %lu at %f\n", tmp.dest_lp, tmp.recv_ts);
    return sizeof(io_event_store);
}

size_t io_event_deserialize (tw_event *e, void *buffer) {
    int i;

    io_event_store tmp;
    memcpy(&tmp, buffer, sizeof(io_event_store));

    memcpy(&(e->cv), &(tmp.cv), sizeof(tw_bf));
    e->dest_lp = (tw_lp *) tmp.dest_lp; // ROSS HACK: e->dest_lp is GID for a bit
    //undo pointer to GID conversion
    if (g_tw_mapping == LINEAR) {
        e->src_lp = g_tw_lp[((tw_lpid)tmp.src_lp) - g_tw_lp_offset];
    } else if (g_tw_mapping == CUSTOM) {
        e->src_lp = g_tw_custom_lp_global_to_local_map((tw_lpid)tmp.src_lp);
    } else {
        tw_error(TW_LOC, "RIO ERROR: Unsupported mapping");
    }
    e->recv_ts = tmp.recv_ts;
    // printf("Loading event going to %lu at %f\n", tmp.dest_lp, tmp.recv_ts);
    return sizeof(io_event_store);
}
