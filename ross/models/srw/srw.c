#include <ross.h>
#include "srw.h"

/**
 * @file srw.c
 * @brief SRW module
 *
 * SRW module.  Contains functions relevant / necessary for the functional
 * requirements pertaining to the SRW waveform.
 */

int total_radios    = 0;
int total_movements = 0;
int total_fail      = 0;
int total_comm      = 0;


/**
 * Initializer function for SRW.  Here we set all the necessary
 * initial values we assume for the beginning of the simulation.
 */
void srw_init(srw_state *s, tw_lp *lp)
{
  int i;
  int num_radios;
  tw_event *e;

  /* Initialize the state of this LP (or master) */
  num_radios = tw_rand_integer(lp->rng, 1, SRW_MAX_GROUP_SIZE);
  s->num_radios = num_radios;
  s->movements = 0;
  s->comm_fail = 0;
  s->comm_try = 0;

  /* Priming events */
  for (i = 0; i < num_radios; i++) {
    tw_stime ts;
    srw_msg_data *msg;

    // GPS event first, as it's the simplest
    ts = tw_rand_unif(lp->rng);
    ts = ts * SRW_GPS_RATE;

    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->type = GPS;
    tw_event_send(e);

    // Now schedule some movements
    ts = tw_rand_exponential(lp->rng, SRW_MOVE_MEAN);
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->type = MOVEMENT;
    // We have to figure out a good way to set the initial lat/long
    tw_event_send(e);

    // Now some communications
    ts = tw_rand_exponential(lp->rng, SRW_COMM_MEAN);
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->type = COMMUNICATION;
    // Something should probably go here as well...
    tw_event_send(e);
  }
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
    msg->type = GPS;
    tw_event_send(e);
    break;

  case MOVEMENT:
    s->movements++;

    // Schedule next event
    ts = tw_rand_exponential(lp->rng, SRW_MOVE_MEAN);
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->type = MOVEMENT;
    tw_event_send(e);
    break;

  case COMMUNICATION:
    s->comm_try++;

    // Schedule next event
    ts = tw_rand_exponential(lp->rng, SRW_COMM_MEAN);
    e = tw_event_new(lp->gid, ts, lp);
    msg = tw_event_data(e);
    msg->type = COMMUNICATION;
    tw_event_send(e);
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
 * Final function for SRW.  
 */
void srw_final(srw_state *s, tw_lp *lp)
{
  total_radios += s->num_radios;
  total_movements += s->movements;
  total_fail += s->comm_fail;
  total_comm += s->comm_try;
}

tw_peid srw_map(tw_lpid gid)
{
  return (tw_peid)gid / g_tw_nlp;
}

tw_lptype srw_lps[] = {
  {
    (init_f)   srw_init,
    (event_f)  srw_event,
    (revent_f) srw_revent,
    (final_f)  srw_final,
    (map_f)    srw_map,
    sizeof    (srw_state)
  },
  {0},
};

/** Filename of [optional] NetDMF configuration */
char netdmf_config[1024] = "";

/** Various additional options for SRW */
const tw_optdef srw_opts[] = {
  TWOPT_GROUP("SRW Model"),
  TWOPT_CHAR("netdmf-config", netdmf_config,
	     "NetDMF Configuration file"),
  TWOPT_END()
};

// Done mainly so doxygen will catch and differentiate this main
// from other mains while allowing smooth compilation.
#define srw_main main

int srw_main(int argc, char *argv[])
{
  int i;
  int TWnlp;
  int TWnkp;
  int TWnpe;
  int num_lps_per_pe = 1;

  tw_opt_add(srw_opts);

  /* This configures g_tw_npe */
  tw_init(&argc, &argv);

  if (!strcmp("", netdmf_config)) {
    printf("No NetDMF configuration specified.\n");
  }
  else {
    /* Read in the netdmf_config file. */
  }

  /* Must call this to properly set g_tw_nlp */
  tw_define_lps(num_lps_per_pe, sizeof(srw_msg_data), 0);

  /* Must repeatedly call this to copy the function pointers appropriately */
  for (i = 0; i < g_tw_nlp; i++) {
    tw_lp_settype(i, &srw_lps[0]);
  }

  tw_run();

  if (tw_ismaster()) {
    printf("Total radios in simulation: %d\n", total_radios);
    printf("Total movements: %d\n", total_movements);
    printf("Total communcation failures: %d\n", total_fail);
    printf("Total communcation attempts: %d\n", total_comm);
    printf("Total communication successes: %d\n", total_comm - total_fail);
  }

  tw_end();

  return 0;
}
