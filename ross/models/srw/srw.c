#include <ross.h>
#include "srw.h"

/**
 * @file srw.c
 * @brief SRW module
 *
 * SRW module.  Contains functions relevant / necessary for the functional
 * requirements pertaining to the SRW waveform.
 * NOTE: None of the netdmf_* event handlers below generate artificial
 * data -- we assume that the NetDMF file was read in and that no other
 * data should be generated that was not in that file.
 */

int total_radios    = 0;
int total_movements = 0;
int total_fail      = 0;
int total_comm      = 0;


/**
 * Initializer function for SRW.  Here we set all the necessary
 * initial values we assume for the beginning of the simulation.
 * We also bootstrap the event queue with some random data.
 */
void srw_init(srw_state *s, tw_lp *lp)
{
  int i;
  int num_radios;
  tw_event *e;
  /* global_id is required to make sure all nodes get unique IDs */
  static long global_id = 0;
  /* current_id is just the starting ID for this particular iteration */
  long current_id = global_id;

  /* Initialize the state of this LP (or master) */
  num_radios    = tw_rand_integer(lp->rng, 1, SRW_MAX_GROUP_SIZE);
  s->num_radios = num_radios;
  s->movements  = 0;
  s->comm_fail  = 0;
  s->comm_try   = 0;

  /* Initialize nodes */
  for (i = 0; i < num_radios; i++) {
    (s->nodes[i]).node_id   = global_id++;
    (s->nodes[i]).lng       = 0.0;
    (s->nodes[i]).lat       = 0.0;
    (s->nodes[i]).movements =   0;
    (s->nodes[i]).comm_fail =   0;
    (s->nodes[i]).comm_try  =   0;
  }

  /* Priming events */
  for (i = 0; i < num_radios; i++) {
    tw_stime ts;
    srw_msg_data *msg;

    // GPS event first, as it's the simplest
    ts = tw_rand_unif(lp->rng);
    ts = ts * SRW_GPS_RATE;

    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->node_id = current_id + i;
    msg->type = GPS;
    tw_event_send(e);

    // Now schedule some movements
    ts = tw_rand_exponential(lp->rng, SRW_MOVE_MEAN);
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->node_id = current_id + i;
    msg->type = MOVEMENT;
    // We have to figure out a good way to set the initial lat/long
    tw_event_send(e);

    // Now some communications
    ts = tw_rand_exponential(lp->rng, SRW_COMM_MEAN);
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->node_id = current_id + i;
    msg->type = COMMUNICATION;
    // Something should probably go here as well...
    tw_event_send(e);
  }
}

/**
 * Initializer function for SRW.  Here we set all the necessary
 * initial values we assume for the beginning of the simulation.
 */
void netdmf_srw_init(srw_state *s, tw_lp *lp)
{
  /* Initialize the state of this LP (or master) */
  s->num_radios = 0;
  s->movements  = 0;
  s->comm_fail  = 0;
  s->comm_try   = 0;
}

/**
 * Forward event handler for SRW.  Supported operations are currently:
 * - GPS
 * - MOVEMENT
 * - COMMUNICATION
 */
void srw_event(srw_state *s, tw_bf *bf, srw_msg_data *m, tw_lp *lp)
{
  tw_stime ts;
  tw_event *e;
  srw_msg_data *msg;

  switch(m->type) {
  case GPS:
    
    // Schedule next event
    e = tw_event_new(lp->gid, SRW_GPS_RATE, lp);
    msg = tw_event_data(e);
    msg->node_id = m->node_id;
    msg->type = GPS;
    tw_event_send(e);
    break;

  case MOVEMENT:
    s->movements++;
    (s->nodes[m->node_id]).movements++;

    // Schedule next event
    ts = tw_rand_exponential(lp->rng, SRW_MOVE_MEAN);
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->node_id = m->node_id;
    msg->type = MOVEMENT;
    tw_event_send(e);
    break;

  case COMMUNICATION:
    s->comm_try++;
    (s->nodes[m->node_id]).comm_try++;

    // Schedule next event
    ts = tw_rand_exponential(lp->rng, SRW_COMM_MEAN);
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->node_id = m->node_id;
    msg->type = COMMUNICATION;
    tw_event_send(e);
    break;
  }
}

/**
 * Forward event handler for SRW.  Supported operations are currently:
 * - GPS
 * - MOVEMENT
 * - COMMUNICATION
 */
void netdmf_srw_event(srw_state *s, tw_bf *bf, srw_msg_data *m, tw_lp *lp)
{
  switch(m->type) {
  case GPS:
    break;

  case MOVEMENT:
    s->movements++;
    (s->nodes[m->node_id]).movements++;
    break;

  case COMMUNICATION:
    s->comm_try++;
    (s->nodes[m->node_id]).comm_try++;
    break;
  }
}

/**
 * Reverse event handler for SRW.
 */
void srw_revent(srw_state *s, tw_bf *bf, srw_msg_data *m, tw_lp *lp)
{
  abort();
}

/**
 * Reverse event handler for SRW.
 */
void netdmf_srw_revent(srw_state *s, tw_bf *bf, srw_msg_data *m, tw_lp *lp)
{
  abort();
}

static int top_node = 0;
static int top_move = 0;

/**
 * Final function for SRW.  
 */
void srw_final(srw_state *s, tw_lp *lp)
{
  int i;
  int moves = 0;

  for (i = 0; i < s->num_radios; i++) {
    moves = (s->nodes[i]).movements;
    if (moves > top_move) {
      top_move = moves;
      top_node = (s->nodes[i]).node_id;
    }
  }

  total_radios    += s->num_radios;
  total_movements += s->movements;
  total_fail      += s->comm_fail;
  total_comm      += s->comm_try;
}

/**
 * Final function for SRW.  
 */
void netdmf_srw_final(srw_state *s, tw_lp *lp)
{
  int i;
  int moves = 0;

  for (i = 0; i < s->num_radios; i++) {
    moves = (s->nodes[i]).movements;
    if (moves > top_move) {
      top_move = moves;
      top_node = (s->nodes[i]).node_id;
    }
  }

  total_radios    += s->num_radios;
  total_movements += s->movements;
  total_fail      += s->comm_fail;
  total_comm      += s->comm_try;
}

tw_peid srw_map(tw_lpid gid)
{
  return (tw_peid)gid / g_tw_nlp;
}

tw_peid netdmf_srw_map(tw_lpid gid)
{
  return (tw_peid)gid / g_tw_nlp;
}

tw_lptype srw_lps[] = {
  {
    (init_f)   srw_init,
    (pre_run_f) NULL,
    (event_f)  srw_event,
    (revent_f) srw_revent,
    (final_f)  srw_final,
    (map_f)    srw_map,
    sizeof    (srw_state)
  },
  {0},
};

tw_lptype netdmf_srw_lps[] = {
  {
    (init_f)   netdmf_srw_init,
    (pre_run_f) NULL,
    (event_f)  netdmf_srw_event,
    (revent_f) netdmf_srw_revent,
    (final_f)  netdmf_srw_final,
    (map_f)    netdmf_srw_map,
    sizeof    (srw_state)
  },
  {0},
};

/** Filename of [optional] NetDMF configuration */
char netdmf_config[1024] = "";

/** Various additional options for SRW */
const tw_optdef srw_opts[] = {
  TWOPT_GROUP("SRW Model"),
#ifdef WITH_NETDMF
  TWOPT_CHAR("netdmf-config", netdmf_config, "NetDMF Configuration file"),
#endif /* WITH_NETDMF */
  TWOPT_END()
};

#ifdef WITH_NETDMF
void rn_netdmf_init();

tw_petype srw_pes[] = {
  {
    (pe_init_f)  0,
    (pre_run_f) NULL,
    (pe_init_f)  rn_netdmf_init,
    (pe_gvt_f)   0,
    (pe_final_f) 0,
    (pe_periodic_f) 0
  },
  {0},
};
#endif /* WITH_NETDMF */


// Done mainly so doxygen will catch and differentiate this main
// from other mains while allowing smooth compilation.
#define srw_main main

int srw_main(int argc, char *argv[])
{
  int i;
  int num_lps_per_pe = 1;

  tw_opt_add(srw_opts);

  /* This configures g_tw_npe */
  tw_init(&argc, &argv);

  /* Must call this to properly set g_tw_nlp */
  tw_define_lps(num_lps_per_pe, sizeof(srw_msg_data), 0);

#ifdef WITH_NETDMF
  /* IF we ARE using NETDMF... */

  if (!strcmp("", netdmf_config)) {
    /* AND we DO NOT have a config file */

    /* There is no NetDMF configuration file.  Create a scenario with fake
     * data, i.e. no changes are required: fake data is created by default */
    for (i = 0; i < g_tw_nlp; i++) {
      tw_lp_settype(i, &srw_lps[0]);
    }
    printf("No NetDMF configuration specified.\n");
  }
  else {
    /* OR we DO have a config file */

    /* Must repeatedly call this to copy the function pointers appropriately */
    for (i = 0; i < g_tw_nlp; i++) {
      tw_lp_settype(i, &netdmf_srw_lps[0]);
    }
    /* Read in the netdmf_config file.  This must be done after
     * we set up the LPs (via tw_lp_settype) so we have data
     * to configure. */
    for (i = 0; i < g_tw_npe; i++) {
      tw_pe_settype(g_tw_pe[i], &srw_pes[0]);
    }
  }
#else /* WITH_NETDMF */
  /* WE are NOT using NETDMF */

  /* Must repeatedly call this to copy the function pointers appropriately */
  for (i = 0; i < g_tw_nlp; i++) {
    tw_lp_settype(i, &srw_lps[0]);
  }
#endif /* WITH_NETDMF */

  tw_run();

  if (tw_ismaster()) {
    printf("Total radios in simulation:    %d\n", total_radios);
    printf("Total movements:               %d\n", total_movements);
    printf("Total communcation failures:   %d\n", total_fail);
    printf("Total communcation attempts:   %d\n", total_comm);
    printf("Total communication successes: %d\n", total_comm - total_fail);
    printf("Node %d moved the most:        %d\n", top_node, top_move);
  }

  tw_end();

  return 0;
}
