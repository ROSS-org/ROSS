/**
 * RAID File Server Farm Simulation
 *
 * FILE: raid-globals.c
 * AUTHOR: Matt Perry
 * DATE: Apr 27 2011
 */
#include "raid.h"

int g_disk_distro = 0;
int g_ttl_fs = 4;
int g_nfs_per_pe = 0;
tw_lpid g_nlp_per_pe = 0;
int g_raid_start_events = 0;
int g_add_mem = 100;
Stats g_stats;
