#ifndef INC_rn_config_h
#define INC_rn_config_h

#ifdef HAVE_ROBOT_PHYSSYM_H
#include "../modules/mobility/robot-physsym/rp.h"
#endif

#ifdef HAVE_RANDOM_WALK_H
#include "../modules/mobility/random-walk/rw.h"
#endif

#ifdef HAVE_TLM_H
#include "../modules/physical/tlm/tlm.h"
#endif

#ifdef HAVE_FTP_H
#include "../modules/application/ftp/ftp.h"
#endif

#ifdef HAVE_TCP_H
#include "../modules/transport/tcp/tcp.h"
#endif

#ifdef HAVE_EPI_H
#include "../modules/application/epi/epi.h"
#endif

#ifdef HAVE_NUM_H
#include "../modules/application/num/num.h"
#endif

#ifdef HAVE_IP_H
#include "../modules/network/ip/ip.h"
#endif

#ifdef HAVE_OSPF_H
#include "../modules/network/ospfv2/ospf.h"
#endif

#ifdef HAVE_BGP_H
#include "../modules/network/bgp4/bgp.h"
#endif

#ifdef HAVE_MULTICAST_H
#include "../modules/network/multicast/Multicast.h"
#endif

#ifdef HAVE_PHOLD_H
#include "../modules/application/phold/phold.h"
#endif

#endif
