/*
 *  node.c
 *  ROSS_TOP
 *
 *  Created by Mark Anderson on 6/28/11.
 *  Copyright 2011 Rensselaer Polytechnic Institute. All rights reserved.
 *
 */
#include <ross.h>
#include "olsr-node.h"

#define MAX_X_DIST 5
#define MAX_Y_DIST 5
#define MAX_Z_DIST 5
#define STD_DEV 1

#define MAX_X_MOVE 1
#define MAX_Y_MOVE 1
#define MAX_Z_MOVE 1
#define STD_DEV_MOVE 0.1

#define WIFIB_BW 20

#define NODE_POWER 80 

void olsr_node_init(olsr_node_state* node, tw_bf * bf, tw_lp * lp) {
	int rngc_x, rngc_y, rngc_z;
	
	node->address = 1;
	node->location.x = tw_rand_normal_sd(lp->rng, MAX_X_DIST, STD_DEV, &rngc_x);
	node->location.x = tw_rand_normal_sd(lp->rng, MAX_Y_DIST, STD_DEV, &rngc_y);
	node->location.z = tw_rand_normal_sd(lp->rng, MAX_Z_DIST, STD_DEV, &rngc_z);
	node->tx_power = NODE_POWER;
}

void olsr_node_move(olsr_node_state* node, tw_bf * bf, tw_lp * lp) {
	int rngc_x, rngc_y, rngc_z;
	
	node->address = 1;
	node->location.x += tw_rand_normal_sd(lp->rng, MAX_X_MOVE, STD_DEV_MOVE, &rngc_x);
	node->location.x += tw_rand_normal_sd(lp->rng, MAX_Y_MOVE, STD_DEV_MOVE, &rngc_y);
	node->location.z += tw_rand_normal_sd(lp->rng, MAX_Z_MOVE, STD_DEV_MOVE, &rngc_z);
}




	
