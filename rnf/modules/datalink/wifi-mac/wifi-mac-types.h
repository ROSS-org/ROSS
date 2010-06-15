#ifndef WIFI_TYPES_H
#define WIFI_TYPES_H

/*********************************************************************/
/* Code take from NS-3, version 3.7 **********************************/
/* Order of files:                  *********************************/

/*

   112 aarfcd-wifi-manager.h                                                             
    68 aarf-wifi-manager.h                                                               
   107 adhoc-wifi-mac.h                                                                  
   102 amrr-wifi-manager.h                                                               
    57 amsdu-subframe-header.h                                                           
   114 arf-wifi-manager.h                                                                
    51 capability-information.h                                                          
    91 cara-wifi-manager.h                                                               
    78 constant-rate-wifi-manager.h
   179 dca-txop.h
    43 dcf.h
   334 dcf-manager.h
   177 edca-txop-n.h
    47 error-rate-model.h
    95 ideal-wifi-manager.h
   132 interference-helper.h
   137 jakes-propagation-loss-model.h
   454 mac-low.h
    66 mac-rx-middle.h
    48 mac-tx-middle.h
   218 mgt-headers.h
   255 minstrel-wifi-manager.h
    55 msdu-aggregator.h
    57 msdu-standard-aggregator.h
   139 nqap-wifi-mac.h
   172 nqsta-wifi-mac.h
    90 onoe-wifi-manager.h
    96 propagation-delay-model.h
   375 propagation-loss-model.h
   120 qadhoc-wifi-mac.h
   140 qap-wifi-mac.h
   108 qos-tag.h
    50 qos-utils.h
   161 qsta-wifi-mac.h
    60 random-stream.h
   128 rraa-wifi-manager.h
    68 ssid.h
    48 status-code.h
    54 supported-rates.h
    51 wifi-channel.h
   172 wifi.h
   293 wifi-mac.h
   216 wifi-mac-header.h
   128 wifi-mac-queue.h
    44 wifi-mac-trailer.h
   277 wifi-mode.h
   134 wifi-net-device.h
   464 wifi-phy.h
    38 wifi-phy-standard.h
    94 wifi-phy-state-helper.h
    32 wifi-preamble.h
   371 wifi-remote-station-manager.h
   113 yans-error-rate-model.h
   100 yans-wifi-channel.h
   193 yans-wifi-phy.h
  7606 total

*/

/*****************************************************************/
/* FWD struct DECs ***********************************************/
/*****************************************************************/
struct AarfcdWifiManager_tag;
typedef struct AarfcdWifiManager_tag AarfcdWifiManager;
struct AarfcdWifiRemoteStation_tag;
typedef struct AarfcdWifiRemoteStation_tag AarfcdWifiRemoteStation;
struct AarfWifiManager_tag;
typedef struct AarfWifiManager_tag AarfWifiManager;
enum AccessClass_tag;
typedef enum AccessClass_tag AccessClass;
union ActionValue_tag;
typedef union ActionValue_tag ActionValue;
struct AdhocWifiMac_tag;
typedef struct AdhocWifiMac_tag AdhocWifiMac;
enum AddressType_tag;
typedef enum AddressType_tag AddressType;
struct AmrrWifiManager_tag;
typedef struct AmrrWifiManager_tag AmrrWifiManager;
struct AmrrWifiRemoteStation_tag;
typedef struct AmrrWifiRemoteStation_tag AmrrWifiRemoteStation;
struct AmsduSubframeHeader_tag;
typedef struct AmsduSubframeHeader_tag AmsduSubframeHeader;
struct ArfWifiManager_tag;
typedef struct ArfWifiManager_tag ArfWifiManager;
struct ArfWifiRemoteStation_tag;
typedef struct ArfWifiRemoteStation_tag ArfWifiRemoteStation;
struct CapabilityInformation_tag;
typedef struct CapabilityInformation_tag CapabilityInformation;
struct CaraWifiManager_tag;
typedef struct CaraWifiManager_tag CaraWifiManager;
struct CaraWifiRemoteStation_tag;
typedef struct CaraWifiRemoteStation_tag CaraWifiRemoteStation;
enum CategoryValue_tag;
typedef enum CategoryValue_tag CategoryValue;
struct ComplexNumber_tag;
typedef struct ComplexNumber_tag ComplexNumber;
struct ConstantSpeedPropagationDelayModel_tag;
typedef struct ConstantSpeedPropagationDelayModel_tag ConstantSpeedPropagationDelayModel;
struct ConstantRateWifiManager_tag;
typedef struct ConstantRateWifiManager_tag ConstantRateWifiManager;
struct ConstantRateWifiRemoteStation_tag;
typedef struct ConstantRateWifiRemoteStation_tag ConstantRateWifiRemoteStation;
struct DcaTxop_tag;
typedef struct DcaTxop_tag DcaTxop;
struct Dcf_tag;
typedef struct Dcf_tag Dcf;
struct DcfManager_tag;
typedef struct DcfManager_tag DcfManager;
struct DcfState_tag;
typedef struct DcfState_tag DcfState;
struct DestinationList_tag;
typedef struct DestinationList_tag DestinationList;
struct EdcaTxopN_tag;
typedef struct EdcaTxopN_tag EdcaTxopN;
struct ErrorRateModel_tag;
typedef struct ErrorRateModel_tag ErrorRateModel;
struct FixedRssLossModel_tag;
typedef struct FixedRssLossModel_tag FixedRssLossModel;
struct FriisPropagationLossModel_tag;
typedef struct FriisPropagationLossModel_tag FriisPropagationLossModel;
struct JakesPropagationLossModel_tag;
typedef struct JakesPropagationLossModel_tag JakesPropagationLossModel;
struct JakesPropagationLossModel_PathCoefficients_tag;
typedef struct JakesPropagationLossModel_PathCoefficients_tag JakesPropagationLossModel_PathCoefficients;
struct IdealWifiManager_tag;
typedef struct IdealWifiManager_tag IdealWifiManager;
struct IdealWifiManagerThresholds_tag;
typedef struct IdealWifiManagerThresholds_tag IdealWifiManagerThresholds;
enum InterworkActionValue_tag;
typedef enum InterworkActionValue_tag InterworkActionValue;
enum LinkMetricActionValue_tag;
typedef enum LinkMetricActionValue_tag LinkMetricActionValue;
struct LogDistancePropagationLossModel_tag;
typedef struct LogDistancePropagationLossModel_tag LogDistancePropagationLossModel;
struct LowDcfListener_tag;
typedef struct LowDcfListener_tag LowDcfListener;
FWD(struct, Mac48Address);
struct MacLow_tag;
typedef struct MacLow_tag MacLow;
struct MacLowTransmissionListener_tag;
typedef struct MacLowTransmissionListener_tag MacLowTransmissionListener;
struct MacLowDcfListener_tag;
typedef struct MacLowDcfListener_tag MacLowDcfListener;
struct MacLowTransmissionParameters_tag;
typedef struct MacLowTransmissionParameters_tag MacLowTransmissionParameters;
struct MacRxMiddle_tag;
typedef struct MacRxMiddle_tag MacRxMiddle;
enum MacState_tag;
typedef enum MacState_tag MacState;
struct MacTxMiddle_tag;
typedef struct MacTxMiddle_tag MacTxMiddle;
struct MgtBeaconHeader_tag;
typedef struct MgtBeaconHeader_tag MgtBeaconHeader;
struct MgtProbeRequestHeader_tag;
typedef struct MgtProbeRequestHeader_tag MgtProbeRequestHeader;
struct MgtProbeResponseHeader_tag;
typedef struct MgtProbeResponseHeader_tag MgtProbeResponseHeader;
struct MinstrelWifiManager_tag;
typedef struct MinstrelWifiManager_tag MinstrelWifiManager;
struct MinstrelWifiRemoteStation_tag;
typedef struct MinstrelWifiRemoteStation_tag MinstrelWifiRemoteStation;
struct MobilityModel_tag;
typedef struct MobilityModel_tag MobilityModel;
struct MsduAggregator_tag;
typedef struct MsduAggregator_tag MsduAggregator;
struct MsduStandardAggregator_tag;
typedef struct MsduStandardAggregator_tag MsduStandardAggregator;
struct NakagamiPropagationLossModel_tag;
typedef struct NakagamiPropagationLossModel_tag NakagamiPropagationLossModel;
struct NqapWifiMac_tag;
typedef struct NqapWifiMac_tag NqapWifiMac;
struct OnoeWifiManager_tag;
typedef struct OnoeWifiManager_tag OnoeWifiManager;
struct OriginatorRxStatus_tag;
typedef struct OriginatorRxStatus_tag OriginatorRxStatus;
struct Originators_tag;
typedef struct Originators_tag Originators;
struct Packet_tag;
typedef struct Packet_tag Packet;
enum PathSelectionActionValue_tag;
typedef enum PathSelectionActionValue_tag PathSelectionActionValue;
struct PathsSet_tag;
typedef struct PathsSet_tag PathsSet;
enum PeerLinkMgtActionValue_tag;
typedef enum PeerLinkMgtActionValue_tag PeerLinkMgtActionValue;
struct PhyListener_tag;
typedef struct PhyListener_tag PhyListener;
struct PhyMacLowListener_tag;
typedef struct PhyMacLowListener_tag PhyMacLowListener;
struct QadhocWifiMac_tag;
typedef struct QadhocWifiMac_tag QadhocWifiMac;
struct QadhocWifiMacQueues_tag;
typedef struct QadhocWifiMacQueues_tag QadhocWifiMacQueues;
struct QapWifiMacQueues_tag;
typedef struct QapWifiMacQueues_tag QapWifiMacQueues;
struct QapWifiMac_tag;
typedef struct QapWifiMac_tag QapWifiMac;
enum QosAckPolicy_tag;
typedef enum QosAckPolicy_tag QosAckPolicy;
struct QosOriginators_tag;
typedef struct QosOriginators_tag QosOriginators;
struct QosTag_tag;
typedef struct QosTag_tag QosTag;
struct QstaWifiMac_tag;
typedef struct QstaWifiMac_tag QstaWifiMac;
struct QstaWifiMacQueues_tag;
typedef struct QstaWifiMacQueues_tag QstaWifiMacQueues;
struct RandomPropagationDelayModel_tag;
typedef struct RandomPropagationDelayModel_tag RandomPropagationDelayModel;
struct RandomPropagationLossModel_tag;
typedef struct RandomPropagationLossModel_tag RandomPropagationLossModel;
struct RateInfo_tag;
typedef struct RateInfo_tag RateInfo;
enum ResourceCoordinationActionValue_tag;
typedef enum ResourceCoordinationActionValue_tag ResourceCoordinationActionValue;
struct RraaWifiManager_tag;
typedef struct RraaWifiManager_tag RraaWifiManager;
struct RraaWifiRemoteStation_tag;
typedef struct RraaWifiRemoteStation_tag RraaWifiRemoteStation;
struct SnrPer_tag;
typedef struct SnrPer_tag SnrPer;
struct Ssid_tag;
typedef struct Ssid_tag Ssid;
struct StatusCode_tag;
typedef struct StatusCode_tag StatusCode;
struct SupportedRates_tag;
typedef struct SupportedRates_tag SupportedRates;
enum TypeOfStation_tag;
typedef enum TypeOfStation_tag TypeOfStation;
struct ThreeLogDistancePropagationLossModel_tag;
typedef struct ThreeLogDistancePropagationLossModel_tag ThreeLogDistancePropagationLossModel;
struct Thresholds_tag;
typedef struct Thresholds_tag Thresholds;
struct TransmissionListener_tag;
typedef struct TransmissionListener_tag TransmissionListener;
struct TxFailed_tag;
typedef struct TxFailed_tag TxFailed;
struct TxOk_tag;
typedef struct TxOk_tag TxOk;
struct TxTime_tag;
typedef struct TxTime_tag TxTime;
struct WifiActionHeader_tag;
typedef struct WifiActionHeader_tag WifiActionHeader;
struct WifiMac_tag;
typedef struct WifiMac_tag WifiMac;
struct WifiMacHeader_tag;
typedef struct WifiMacHeader_tag WifiMacHeader;
struct WifiMacParameters_tag;
typedef struct WifiMacParameters_tag WifiMacParameters;
struct WifiMacQueue_tag;
typedef struct WifiMacQueue_tag WifiMacQueue;
struct WifiMacQueueItem_tag;
typedef struct WifiMacQueueItem_tag WifiMacQueueItem;
enum WifiMacType_tag;
typedef enum WifiMacType_tag WifiMacType;
struct WifiMode_tag;
typedef struct WifiMode_tag WifiMode;
struct WifiPhy_tag;
typedef struct WifiPhy_tag WifiPhy;
enum WifiPreamble_tag;
typedef enum WifiPreamble_tag WifiPreamble;
struct WifiRemoteStationManager_tag;
typedef struct WifiRemoteStationManager_tag WifiRemoteStationManager;


/*****************************************************************/
/* Type Defs *****************************************************/
/*****************************************************************/

typedef unsigned int bool; // C does not have a bool type
typedef void (*callback_adhocwifimac)(WifiMacHeader *h);
typedef void (*callback_nqstawifimac)(Mac48Address *addr);
typedef void (*callback_wifimac)(Packet *p);

/*****************************************************************/
/* Struct DEFS ***************************************************/
/*****************************************************************/

/*
 * defined from NS-3 src/nodes/mac48-address.h
 */
DEF(struct, Mac48Address)
{
  uint8_t m_address[6];
};

/**
 * \brief an implementation of the AARF-CD algorithm
 *
 * This algorithm was first described in "Efficient Collision Detection for Auto Rate Fallback Algorithm".
 * The implementation available here was done by Federico Maguolo for a very early development
 * version of ns-3. Federico died before merging this work in ns-3 itself so his code was ported
 * to ns-3 later without his supervision.
 */

struct AarfcdWifiManager_tag
{
  uint32_t m_minTimerThreshold;
  uint32_t m_minSuccessThreshold;
  double m_successK;
  uint32_t m_maxSuccessThreshold;
  double m_timerK;
  uint32_t m_minRtsWnd;
  uint32_t m_maxRtsWnd;
  bool m_rtsFailsAsDataFails;
  bool m_turnOffRtsAfterRateDecrease;
  bool m_turnOnRtsAfterRateIncrease;
};

struct AarfcdWifiRemoteStation_tag
{
  uint32_t m_timer;
  uint32_t m_success;
  uint32_t m_failed;
  bool 	   m_recovery;
  bool     m_justModifyRate;
  uint32_t m_retry;
  uint32_t m_successThreshold;
  uint32_t m_timerTimeout;
  uint32_t m_rate;
  bool     m_rtsOn;
  uint32_t m_rtsWnd;
  uint32_t m_rtsCounter;
  bool     m_haveASuccess;
  
  AarfcdWifiManager *m_manager;
};

struct AarfWifiManager_tag
{
  uint32_t m_minTimerThreshold;
  uint32_t m_minSuccessThreshold;
  double m_successK;
  uint32_t m_maxSuccessThreshold;
  double m_timerK;
};

// AarfWifiRemoteStation -> AarfWifiManager *m_manager;

// #include "ns3/mac48-address.h"
// #include "ns3/callback.h"
// #include "ns3/packet.h"


/**
 * \brief the Adhoc state machine
 *
 * For now, this class is really empty but it should contain
 * the code for the distributed generation of beacons in an adhoc 
 * network.
 */
struct AdhocWifiMac_tag
{
  DcaTxop *m_dca;
  // Callback<void,Ptr<Packet>, Mac48Address, Mac48Address> m_upCallback;
  WifiRemoteStationManager *m_stationManager;
  WifiPhy *m_phy;
  DcfManager *m_dcfManager;
  MacRxMiddle *m_rxMiddle;
  MacLow *m_low;
  Ssid *m_ssid;
  // TracedCallback<const WifiMacHeader &> m_txOkCallback;
  callback_adhocwifimac m_txOkCallback;
  // TracedCallback<const WifiMacHeader &> m_txErrCallback;
  callback_adhocwifimac m_txErrCallback;
};

// #include "ns3/nstime.h"

/**
 * \brief AMRR Rate control algorithm
 *
 * This class implements the AMRR rate control algorithm which
 * was initially described in <i>IEEE 802.11 Rate Adaptation:
 * A Practical Approach</i>, by M. Lacage, M.H. Manshaei, and 
 * T. Turletti.
 */

struct AmrrWifiManager_tag
{
  tw_stime m_updatePeriod;
  double m_failureRatio;
  double m_successRatio;
  uint32_t m_maxSuccessThreshold;
  uint32_t m_minSuccessThreshold;
};

struct AmrrWifiRemoteStation_tag
{
  AmrrWifiManager *m_stations;
  tw_stime m_nextModeUpdate;
  uint32_t m_tx_ok;
  uint32_t m_tx_err;
  uint32_t m_tx_retr;
  uint32_t m_retry;
  uint32_t m_txrate;
  uint32_t m_successThreshold;
  uint32_t m_success;
  bool m_recovery;
};

// #include "ns3/header.h"
// #include "ns3/mac48-address.h"

struct AmsduSubframeHeader_tag
{
  Mac48Address m_da;
  Mac48Address m_sa;
  uint16_t m_length;
};

/**
 * \brief ARF Rate control algorithm
 *
 * This class implements the so-called ARF algorithm which was
 * initially described in <i>WaveLAN-II: A High-performance wireless 
 * LAN for the unlicensed band</i>, by A. Kamerman and L. Monteban. in
 * Bell Lab Technical Journal, pages 118-133, Summer 1997.
 *
 * This implementation differs from the initial description in that it
 * uses a packet-based timer rather than a time-based timer as described 
 * in XXX (I cannot find back the original paper which described how
 * the time-based timer could be easily replaced with a packet-based 
 * timer.)
 */

struct ArfWifiManager_tag
{
  uint32_t m_timerThreshold;
  uint32_t m_successThreshold;
};

struct ArfWifiRemoteStation_tag
{
  uint32_t m_timer;
  uint32_t m_success;
  uint32_t m_failed;
  bool m_recovery;
  uint32_t m_retry;
  
  uint32_t m_timerTimeout;
  uint32_t m_successThreshold;

  uint32_t m_rate;
  
  ArfWifiManager *m_manager;
};

// #include "ns3/buffer.h"

struct CapabilityInformation_tag
{
  uint16_t m_capability;
};

struct CaraWifiManager_tag
{
  uint32_t m_timerTimeout;
  uint32_t m_successThreshold;
  uint32_t m_failureThreshold;
  uint32_t m_probeThreshold;
};

struct CaraWifiRemoteStation_tag
{
  uint32_t m_timer;
  uint32_t m_success;
  uint32_t m_failed;
  
  uint32_t m_rate;
  
  CaraWifiManager *m_manager;
};

struct ConstantRateWifiManager_tag
{
  WifiMode *m_dataMode;
  WifiMode *m_ctlMode;
};

// ConstantRateWifiRemoteStation -> ConstantRateWifiManager *m_manager;

/**
 * \brief handle packet fragmentation and retransmissions.
 *
 * This class implements the packet fragmentation and 
 * retransmission policy. It uses the ns3::MacLow and ns3::DcfManager
 * helper classes to respectively send packets and decide when 
 * to send them. Packets are stored in a ns3::WifiMacQueue until
 * they can be sent.
 *
 * The policy currently implemented uses a simple fragmentation
 * threshold: any packet bigger than this threshold is fragmented
 * in fragments whose size is smaller than the threshold.
 *
 * The retransmission policy is also very simple: every packet is
 * retransmitted until it is either successfully transmitted or
 * it has been retransmitted up until the ssrc or slrc thresholds.
 *
 * The rts/cts policy is similar to the fragmentation policy: when
 * a packet is bigger than a threshold, the rts/cts protocol is used.
 */

struct DcaTxop_tag
{
  /**
   * \param packet packet to send
   * \param hdr header of packet to send.
   *
   * Store the packet in the internal queue until it
   * can be sent safely.
   */

  Dcf *m_dcf;
  DcfManager *m_manager;
  TxOk *m_txOkCallback;
  TxFailed *m_txFailedCallback;
  WifiMacQueue *m_queue;
  MacTxMiddle *m_txMiddle;
  MacLow *m_low;
  WifiRemoteStationManager *m_stationManager;
  TransmissionListener *m_transmissionListener;
  // RandomStream *m_rng; replaced by ROSS internal RNG
  bool m_accessOngoing;
  Packet *m_currentPacket;
  WifiMacHeader *m_currentHdr;
  uint8_t m_fragmentNumber;
};

struct Dcf_tag

/**
 * \brief keep track of the state needed for a single DCF 
 * function.
 *
 * Multiple instances of a DcfState can be registered in a single
 * DcfManager to implement 802.11e-style relative QoS.
 * DcfState::SetAifsn and DcfState::SetCwBounds allow the user to 
 * control the relative QoS differentiation.
 */

struct DcfState_tag
{

  uint32_t m_aifsn;
  uint32_t m_backoffSlots;
  // the backoffStart variable is used to keep track of the
  // time at which a backoff was started or the time at which
  // the backoff counter was last updated.
  tw_stime m_backoffStart;
  uint32_t m_cwMin;
  uint32_t m_cwMax;
  uint32_t m_cw;
  bool m_accessRequested;

  // added to vectoring in C
  DcfState *next;
};

/**
 * \brief Manage a set of ns3::DcfState
 *
 * Handle a set of independent ns3::DcfState, each of which represents
 * a single DCF within a MAC stack. Each ns3::DcfState has a priority
 * implicitely associated with it (the priority is determined when the 
 * ns3::DcfState is added to the DcfManager: the first DcfState to be
 * added gets the highest priority, the second, the second highest
 * priority, and so on.) which is used to handle "internal" collisions.
 * i.e., when two local DcfState are expected to get access to the 
 * medium at the same time, the highest priority local DcfState wins
 * access to the medium and the other DcfState suffers a "internal" 
 * collision.
 */

struct DcfManager_tag
{
  DcfState **States;
  DcfState *m_states;
  tw_stime m_lastAckTimeoutEnd;
  tw_stime m_lastCtsTimeoutEnd;
  tw_stime m_lastNavStart;
  tw_stime m_lastNavDuration;
  tw_stime m_lastRxStart;
  tw_stime m_lastRxDuration;
  bool m_lastRxReceivedOk;
  tw_stime m_lastRxEnd;
  tw_stime m_lastTxStart;
  tw_stime m_lastTxDuration;
  tw_stime m_lastBusyStart;
  tw_stime m_lastBusyDuration;
  tw_stime m_lastSwitchingStart; 
  tw_stime m_lastSwitchingDuration; 
  bool m_rxing;
  bool m_sleeping;
  tw_stime m_eifsNoDifs;
  tw_event *m_accessTimeout; // is a timer event in ROSS
  uint32_t m_slotTimeUs;
  tw_stime m_sifs;
  PhyListener *m_phyListener;
  LowDcfListener *m_lowListener;
};

/* This queue contains packets for a particular access class.
 * possibles access classes are:
 *   
 *   -AC_VO : voice, tid = 6,7         ^
 *   -AC_VI : video, tid = 4,5         |
 *   -AC_BE : best-effort, tid = 0,3   |  priority  
 *   -AC_BK : background, tid = 1,2    |
 * 
 * For more details see section 9.1.3.1 in 802.11 standard.
 */

enum TypeOfStation_tag
{
  STA,
  AP,
  ADHOC_STA
};

struct EdcaTxopN_tag
{
  Dcf *m_dcf;
  DcfManager *m_manager;
  WifiMacQueue *m_queue;
  TxOk *m_txOkCallback;
  TxFailed *m_txFailedCallback;
  MacLow *m_low;
  MacTxMiddle *m_txMiddle;
  TransmissionListener *m_transmissionListener;
  // RandomStream *m_rng; replace with ROSS rng
  WifiRemoteStationManager *m_stationManager;
  uint8_t m_fragmentNumber;
  
  /* current packet could be a simple MSDU or, if an aggregator for this queue is
     present, could be an A-MSDU.
   */
  Packet *m_currentPacket;
  WifiMacHeader *m_currentHdr;
  MsduAggregator *m_aggregator;
  TypeOfStation m_typeOfStation;
};

struct ErrorRateModel_tag
{
  // dummy struct as placeholder for nearly empty class
  // man C++ creates such a WASTE!!
};

struct IdealWifiManagerThresholds_tag
{
  double value;
  WifiMode *mode;
  Thresholds *next;
};

struct IdealWifiManager_tag
{
  // typedef std::vector<std::pair<double,WifiMode> > Thresholds;

  double m_ber;
  IdealWifiManagerThresholds *m_thresholds;
  double m_minSnr;
  double m_maxSnr;
};

struct IdealWifiRemoteStation_tag
{
  IdealWifiManager *m_manager;
  double m_lastSnr;
};

struct SnrPer_tag
{
  double snr;
  double per;
};

struct NiChange_tag
{
  tw_stime m_time;
  double m_delta;
};

struct InterferenceHelper_tag
{
  uint32_t m_size;
  WifiMode *m_payloadMode;
  WifiPreamble *m_preamble;
  tw_stime m_startTime;
  tw_stime m_endTime;
  double m_rxPowerW;
  tw_stime m_maxPacketDuration;
  double m_noiseFigure; /**< noise figure (linear) */
  tw_event *m_events;
  ErrorRateModel *m_errorRateModel;
};

/**
 * \brief a Jakes propagation loss model
 *
 * The Jakes propagation loss model implemented here is 
 * described in [1].
 * 
 *
 * We call path the set of rays that depart from a given 
 * transmitter and arrive to a given receiver. For each ray
 * The complex coefficient is compute as follow:
 * \f[ u(t)=u_c(t) + j u_s(t)\f]
 * \f[ u_c(t) = \frac{2}{\sqrt{N}}\sum_{n=0}^{M}a_n\cos(\omega_n t+\phi_n)\f]
 * \f[ u_s(t) = \frac{2}{\sqrt{N}}\sum_{n=0}^{M}b_n\cos(\omega_n t+\phi_n)\f]
 * where
 * \f[ a_n=\left \{ \begin{array}{ll}
 * \sqrt{2}\cos\beta_0 & n=0 \\
 * 2\cos\beta_n & n=1,2,\ldots,M
 * \end{array}
 * \right .\f]
 * \f[ b_n=\left \{ \begin{array}{ll}
 * \sqrt{2}\sin\beta_0 & n=0 \\
 * 2\sin\beta_n & n=1,2,\ldots,M
 * \end{array}
 * \right .\f]
 * \f[ \beta_n=\left \{ \begin{array}{ll}
 * \frac{\pi}{4} & n=0 \\
 * \frac{\pi n}{M} & n=1,2,\ldots,M
 * \end{array}
 * \right .\f]
 * \f[ \omega_n=\left \{ \begin{array}{ll}
 * 2\pi f_d & n=0 \\
 * 2\pi f_d \cos\frac{2\pi n}{N} & n=1,2,\ldots,M
 * \end{array}
 * \right .\f]
 *
 * The parameter \f$f_d\f$ is the doppler frequency and \f$N=4M+2\f$ where
 * \f$M\f$ is the number of oscillators per ray.
 *
 * The attenuation coefficent of the path is the magnitude of the sum of 
 * all the ray coefficients. This attenuation coefficient could be greater than
 * \f$1\f$, hence it is divide by \f$ \frac{2N_r}{\sqrt{N}} \sum_{n+0}^{M}\sqrt{a_n^2 +b_n^2}\f$
 * where \f$N_r\f$ is the number of rays.
 *
 * The initail phases \f$\phi_i\f$ are random and they are choosen according 
 * to a given distribution.
 * 
 * [1] Y. R. Zheng and C. Xiao, "Simulation Models With Correct 
 * Statistical Properties for Rayleigh Fading Channel", IEEE
 * Trans. on Communications, Vol. 51, pp 920-928, June 2003
 */

struct ComplexNumber_tag
{
  double real;
  double imag;
};

struct JakesPropagationLossModel_PathCoefficients_tag
{
  MobilityModel *m_receiver;
  uint8_t m_nOscillators;
  uint8_t m_nRays;
  double **m_phases;
  tw_stime m_lastUpdate;
  JakesPropagationLossModel *m_jakes;

  // need to allow for "vectors" of these
  JakesPropagationLossModel_PathCoefficients *next;
};

struct PathsSet_tag
{
  MobilityModel *sender;
  JakesPropagationLossModel_PathCoefficients *receivers;
  PathsSet *next; // for vectors of these
};

struct JakesPropagationLossModel_tag
{
  double PI; // set this to the real value of PI
  ComplexNumber* m_amp;
  // RandomVariable m_variable; replace with ROSS rng
  double m_fd;  
  PathsSet * m_paths;
  uint8_t m_nRays;
  uint8_t m_nOscillators;
};


struct MacLowTransmissionListener_tag
{
  //dummy struct for class container
};

/**
 * \brief listen to NAV events
 *
 * This class is typically connected to an instance of ns3::Dcf
 * and calls to its methods are forwards to the corresponding
 * ns3::Dcf methods.
 */

struct MacLowDcfListener_tag
{
  //dummy struct for class container
};

/**
 * \brief control how a packet is transmitted.
 *
 * The ns3::MacLow::StartTransmission method expects
 * an instance of this class to describe how the packet
 * should be transmitted.
 */

struct MacLowTransmissionParameters_tag
{
  uint32_t m_nextSize;
  enum {
    ACK_NONE,
    ACK_NORMAL,
    ACK_FAST,
    ACK_SUPER_FAST
  } m_waitAck;
  bool m_sendRts;
  tw_stime m_overrideDurationId;
};

/**
 * \brief handle RTS/CTS/DATA/ACK transactions.
 */

struct MacLow_tag
{
// typedef Callback<void, Ptr<Packet>, const WifiMacHeader*> MacLowRxCallback;

  WifiPhy *m_phy;
  WifiRemoteStationManager *m_stationManager;
// MacLowRxCallback m_rxCallback; Need to CONVER TO Function Pointer
//  typedef std::vector<MacLowDcfListener *>::const_iterator DcfListenersCI;
//  typedef std::vector<MacLowDcfListener *> DcfListeners;
//  DcfListeners m_dcfListeners;
  MacLowDcfListener *m_dcfListeners;

  tw_event *m_normalAckTimeoutEvent;
  tw_event *m_fastAckTimeoutEvent;
  tw_event *m_superFastAckTimeoutEvent;
  tw_event *m_fastAckFailedTimeoutEvent;
  tw_event *m_ctsTimeoutEvent;
  tw_event *m_sendCtsEvent;
  tw_event *m_sendAckEvent;
  tw_event *m_sendDataEvent;
  tw_event *m_waitSifsEvent;
  tw_event *m_navCounterResetCtsMissed;

  Packet  *m_currentPacket;
  WifiMacHeader *m_currentHdr;
  MacLowTransmissionParameters *m_txParams;
  MacLowTransmissionListener *m_listener;
  Mac48Address m_self;
  Mac48Address m_bssid;
  tw_stime m_ackTimeout;
  tw_stime m_ctsTimeout;
  tw_stime m_sifs;
  tw_stime m_slotTime;
  tw_stime m_pifs;
  tw_stime m_lastNavStart;
  tw_stime m_lastNavDuration;

  // Listerner needed to monitor when a channel switching occurs. 
  PhyMacLowListener *m_phyMacLowListener; 
};

struct OriginatorRxStatus_tag
{
  // typedef std::list<Ptr<const Packet> > Fragments;
  // typedef std::list<Ptr<const Packet> >::const_iterator FragmentsCI;

  bool m_defragmenting;
  uint16_t m_lastSequenceControl;
  Packet * m_fragments;
};

struct Originators_tag
{
  Mac48Address addr;
  OriginatorRxStatus *status;
};

struct QosOriginators_tag
{
  Mac48Address addr;
  OriginatorRxStatus *status;
};


struct MacRxMiddle_tag
{
  // typedef Callback<void, Ptr<Packet>, const WifiMacHeader*> ForwardUpCallback;
  // typedef std::map <Mac48Address, OriginatorRxStatus *, std::less<Mac48Address> > Originators;
  // typedef std::map <std::pair<Mac48Address, uint8_t>, OriginatorRxStatus *, std::less<std::pair<Mac48Address,uint8_t> > > QosOriginators;
  // typedef std::map <Mac48Address, OriginatorRxStatus *, std::less<Mac48Address> >::iterator OriginatorsI;
  // typedef std::map <std::pair<Mac48Address, uint8_t>, OriginatorRxStatus *, std::less<std::pair<Mac48Address,uint8_t> > >::iterator QosOriginatorsI;
  Originators *m_originatorStatus;
  QosOriginators *m_qosOriginatorStatus;
  // ForwardUpCallback m_callback; CONVERT TO FUNCTION POINTER
};

struct MacTxMiddle_tag
{
  Mac48Address *m_qosSequences; // this was a C++ map
  uint16_t m_sequence;
};

struct MgtAssocRequestHeader_tag
{
  Ssid *m_ssid;
  SupportedRates *m_rates;
  CapabilityInformation *m_capability;
  uint16_t m_listenInterval;
};

struct MgtAssocResponseHeader_tag
{
  SupportedRates *m_rates;
  CapabilityInformation *m_capability;
  StatusCode *m_code;
  uint16_t m_aid;
};

struct MgtProbeRequestHeader_tag
{
  Ssid *m_ssid;
  SupportedRates *m_rates;
};

struct MgtProbeResponseHeader_tag
{
  uint64_t m_timestamp;
  Ssid *m_ssid;
  uint64_t m_beaconInterval;
  SupportedRates *m_rates;
  CapabilityInformation *m_capability;
};

struct MgtBeaconHeader_tag
{
  // empty struct in NS-3
};

/**
 * \brief See IEEE 802.11 chapter 7.3.1.11
 *
 * Header format: | category: 1 | action value: 1 |
 */
  /* Compatible with open80211s implementation */
enum CategoryValue_tag
{
  MESH_PEERING_MGT = 30,
  MESH_LINK_METRIC = 31,
  MESH_PATH_SELECTION = 32,
  MESH_INTERWORKING = 33,
  MESH_RESOURCE_COORDINATION = 34,
  MESH_PROXY_FORWARDING = 35
};

  /* Compatible with open80211s implementation */
enum PeerLinkMgtActionValue_tag
{
  PEER_LINK_OPEN = 0,
  PEER_LINK_CONFIRM = 1,
  PEER_LINK_CLOSE = 2
};

enum LinkMetricActionValue_tag
  {
    LINK_METRIC_REQUEST = 0,
    LINK_METRIC_REPORT
  };
  /* Compatible with open80211s implementation */
enum PathSelectionActionValue_tag
  {
    PATH_SELECTION = 0
  };

enum InterworkActionValue_tag
  {
    PORTAL_ANNOUNCEMENT = 0
  };

enum ResourceCoordinationActionValue_tag
  {
    CONGESTION_CONTROL_NOTIFICATION = 0,
    MDA_SETUP_REQUEST,
    MDA_SETUP_REPLY,
    MDAOP_ADVERTISMENT_REQUEST,
    MDAOP_ADVERTISMENTS,
    MDAOP_SET_TEARDOWN,
    BEACON_TIMING_REQUEST,
    BEACON_TIMING_RESPONSE,
    TBTT_ADJUSTMENT_REQUEST,
    MESH_CHANNEL_SWITCH_ANNOUNCEMENT
  };

union ActionValue_tag
{
  PeerLinkMgtActionValue peerLink;
  LinkMetricActionValue linkMetrtic;
  PathSelectionActionValue pathSelection;
  InterworkActionValue interwork;
  ResourceCoordinationActionValue resourceCoordination;
}; 

struct WifiActionHeader_tag
{
  uint8_t m_category;
  uint8_t m_actionValue;
  ActionValue m_av;
};

/**
 * \author Duy Nguyen
 * \brief Implementation of Minstrel Rate Control Algorithm 
 *
 * Porting Minstrel from Madwifi and Linux Kernel 
 * http://linuxwireless.org/en/developers/Documentation/mac80211/RateControl/minstrel
 */

/**
 * A struct to contain all information related to a data rate 
 */
struct RateInfo_tag
{
  /**
   * Perfect transmission time calculation, or frame calculation
   * Given a bit rate and a packet length n bytes 
   */
  tw_stime perfectTxTime;		
  uint32_t retryCount;  ///< retry limit
  uint32_t adjustedRetryCount;  ///< adjust the retry limit for this rate
  uint32_t numRateAttempt;  ///< how many number of attempts so far
  uint32_t numRateSuccess;  ///< number of successful pkts 
  uint32_t prob;  ///< (# pkts success )/(# total pkts)

  /**
   * EWMA calculation
   * ewma_prob =[prob *(100 - ewma_level) + (ewma_prob_old * ewma_level)]/100 
   */
  uint32_t ewmaProb;

  uint32_t prevNumRateAttempt;  ///< from last rate
  uint32_t prevNumRateSuccess;  ///< from last rate
  uint64_t successHist;  ///< aggregate of all successes
  uint64_t attemptHist;  ///< aggregate of all attempts
  uint32_t throughput;  ///< throughput of a rate
};

/**
 * Data structure for a Minstrel Rate table 
 * A vector of a struct RateInfo 
 */
// typedef std::vector<struct RateInfo> MinstrelRate;

/**
 * Data structure for a Sample Rate table
 * A vector of a vector uint32_t 
 */
// typedef std::vector<std::vector<uint32_t> > SampleRate;

struct TxTime_tag
{
  tw_stime time;
  WifiMode *mode;
};

struct MinstrelWifiManager_tag
{
  // typedef std::vector<std::pair<Time,WifiMode> > TxTime;

  TxTime m_calcTxTime;  ///< to hold all the calculated TxTime for all modes
  tw_stime m_updateStats;  ///< how frequent do we calculate the stats(1/10 seconds)
  double m_lookAroundRate;  ///< the % to try other rates than our current rate 
  double m_ewmaLevel;  ///< exponential weighted moving average
  uint32_t m_segmentSize;  ///< largest allowable segment size
  uint32_t m_sampleCol;  ///< number of sample columns
  uint32_t m_pktLen;  ///< packet length used  for calculate mode TxTime
  
};

struct MinstrelWifiRemoteStation_tag
{
  /**
   *
   * Retry Chain table is implemented here
   *
   * Try |         LOOKAROUND RATE              | NORMAL RATE
   *     | random < best    | random > best     |
   * --------------------------------------------------------------
   *  1  | Best throughput  | Random rate       | Best throughput
   *  2  | Random rate      | Best throughput   | Next best throughput
   *  3  | Best probability | Best probability  | Best probability
   *  4  | Lowest Baserate  | Lowest baserate   | Lowest baserate
   *
   * Note: For clarity, multiple blocks of if's and else's are used
   * After a failing 7 times, DoReportFinalDataFailed will be called
   */
  /**
   * \param packet lenghth 
   * \param current WifiMode
   * \returns calcuated transmit duration 
   */
  MinstrelWifiManager *m_stations;
  tw_stime m_nextStatsUpdate;  ///< 10 times every second
  // MinstrelRate m_minstrelTable;  ///< minstrel table	
  RateInfo *m_minstrealTable;

  // SampleRate m_sampleTable;  ///< sample table
  uint32_t *m_sampleTable;

  /**
   * To keep track of the current position in the our random sample table
   * going row by row from 1st column until the 10th column(Minstrel defines 10)
   * then we wrap back to the row 1 col 1.
   * note: there are many other ways to do this.
   */
  uint32_t m_col, m_index;							

  uint32_t m_maxTpRate;  ///< the current throughput rate 
  uint32_t m_maxTpRate2;  ///< second highest throughput rate
  uint32_t m_maxProbRate;  ///< rate with highest prob of success

  int m_packetCount;  ///< total number of packets as of now
  int m_sampleCount;  ///< how many packets we have sample so far

  bool m_isSampling;  ///< a flag to indicate we are currently sampling
  uint32_t m_sampleRate;  ///< current sample rate
  bool 	m_sampleRateSlower;  ///< a flag to indicate sample rate is slower
  uint32_t m_currentRate;  ///< current rate we are using

  uint32_t m_shortRetry;  ///< short retries such as control packts
  uint32_t m_longRetry;  ///< long retries such as data packets
  uint32_t m_retry;  ///< total retries short + long
  uint32_t m_err;  ///< retry errors
  uint32_t m_txrate;  ///< current transmit rate

  bool m_initialized;  ///< for initializing tables
}; 

/**
 * \brief Abstract class that concrete msdu aggregators have to implement
 */
struct MsduAggregator_tag
//{
  // typedef std::list<std::pair<Ptr<Packet>, AmsduSubframeHeader> > DeaggregatedMsdus;
  // dummy struct
//};

struct MsduStandardAggregator_tag
{
  uint32_t m_maxAmsduLength;
};

/**
 * \brief non-QoS AP state machine
 *
 * Handle association, dis-association and authentication,
 * of STAs within an IBSS.
 * This class uses two output queues, each of which is server by
 * a single DCF
 *   - the highest priority DCF serves the queue which contains
 *     only beacons.
 *   - the lowest priority DCF serves the queue which contains all
 *     other frames, including user data frames.
 */

struct NqapWifiMac_tag
{
  DcaTxop *m_dca;
  DcaTxop *m_beaconDca;
  WifiRemoteStationManager *m_stationManager;
  WifiPhy *m_phy;
  // Callback<void, Ptr<Packet>,Mac48Address, Mac48Address> m_upCallback; Convert to function pointer
  tw_stime m_beaconInterval;
  bool m_enableBeaconGeneration;
  DcfManager *m_dcfManager;
  MacRxMiddle *m_rxMiddle;
  MacLow *m_low;
  Ssid *m_ssid;
  tw_event *m_beaconEvent;
};

/**
 * \brief a non-QoS STA state machine
 *
 * This state machine handles association, disassociation,
 * authentication and beacon monitoring. It does not perform
 * channel scanning.
 * If the station detects a certain number of missed beacons
 * while associated, it automatically attempts a new association
 * sequence.
 */

enum MacState_tag
  {
    ASSOCIATED,
    WAIT_PROBE_RESP,
    WAIT_ASSOC_RESP,
    BEACON_MISSED,
    REFUSED
  };

struct NqstaWifiMac_tag
{
  MacState m_state;
  tw_stime m_probeRequestTimeout;
  tw_stime m_assocRequestTimeout;
  tw_event *m_probeRequestEvent;
  tw_event *m_assocRequestEvent;
  // Callback<void, Ptr<Packet>, Mac48Address, Mac48Address> m_forwardUp; CONVERT to Function Pointer
  // Callback<void> m_linkUp; CONVERT to Function Pointer
  // Callback<void> m_linkDown; CONVERT to Function Pointer
  DcaTxop *m_dca;
  tw_event *m_beaconWatchdog;
  tw_stime m_beaconWatchdogEnd;
  uint32_t m_maxMissedBeacons;

  WifiPhy *m_phy;
  WifiRemoteStationManager *m_stationManager;
  DcfManager *m_dcfManager;
  MacRxMiddle *m_rxMiddle;
  MacLow *m_low;
  Ssid *m_ssid;
  // TracedCallback<Mac48Address> m_assocLogger; CONVERT to Function Pointer
  callback_nqstawifimac m_assocLogger;
  // TracedCallback<Mac48Address> m_deAssocLogger; CONVERT to Function Pointer
  callback_nqstawifimac m_deAssocLogger;
};

/**
 * \brief an implementation of rate control algorithm developed 
 *        by Atsushi Onoe
 *
 * This algorithm is well known because it has been used as the default
 * rate control algorithm for the madwifi driver. I am not aware of
 * any publication or reference about this algorithm beyond the madwifi
 * source code.
 */

struct OnoeWifiManager_tag
{
  tw_stime m_updatePeriod;
  uint32_t m_addCreditThreshold;
  uint32_t m_raiseThreshold;
};

struct OnoeWifiRemoteStation_tag
{
  OnoeWifiManager *m_stations;
  tw_stime m_nextModeUpdate;
  uint32_t m_shortRetry;
  uint32_t m_longRetry;
  uint32_t m_tx_ok;
  uint32_t m_tx_err;
  uint32_t m_tx_retr;
  uint32_t m_tx_upper;
  uint32_t m_txrate;
};

/**
 * \brief calculate a propagation delay.
 */

/**
 * \brief the propagation delay is random
 */
struct RandomPropagationDelayModel_tag
{
  // RandomVariable m_variable; -- replace with ROSS rng
};

/**
 * \brief the propagation speed is constant
 */
struct ConstantSpeedPropagationDelayModel_tag
{
  double m_speed;
};

/**
 * \brief Modelize the propagation loss through a transmission medium
 *
 * Calculate the receive power (dbm) from a transmit power (dbm)
 * and a mobility model for the source and destination positions.
 */
struct PropagationLossModel_tag
// We don't need this struct other than to understand the PLM
// is the base class for all these others below.

/**
 * \brief The propagation loss follows a random distribution.
 */ 
struct RandomPropagationLossModel_tag
{
  // RandomVariable m_variable; replace with ROSS rng
  RandomPropagationLossModel *m_next;
};

/**
 * \brief a Friis propagation loss model
 *
 * The Friis propagation loss model was first described in
 * "A Note on a Simple Transmission Formula", by 
 * "Harald T. Friis".
 * 
 * The original equation was described as:
 *  \f$ \frac{P_r}{P_t} = \frac{A_r A_t}{d^2\lambda^2} \f$
 *  with the following equation for the case of an
 *  isotropic antenna with no heat loss:
 *  \f$ A_{isotr.} = \frac{\lambda^2}{4\pi} \f$
 *
 * The final equation becomes:
 * \f$ \frac{P_r}{P_t} = \frac{\lambda^2}{(4 \pi d)^2} \f$
 *
 * Modern extensions to this original equation are:
 * \f$ P_r = \frac{P_t G_t G_r \lambda^2}{(4 \pi d)^2 L}\f$
 *
 * With:
 *  - \f$ P_r \f$ : reception power (W)
 *  - \f$ P_t \f$ : transmission power (W)
 *  - \f$ G_t \f$ : transmission gain (unit-less)
 *  - \f$ G_r \f$ : reception gain (unit-less)
 *  - \f$ \lambda \f$ : wavelength (m)
 *  - \f$ d \f$ : distance (m)
 *  - \f$ L \f$ : system loss (unit-less)
 *
 *
 * This model is invalid for small distance values.
 * The current implementation returns the txpower as the rxpower
 * for any distance smaller than MinDistance.
 */

struct FriisPropagationLossModel_tag
{
  double PI;
  double m_lambda;
  double m_systemLoss;
  double m_minDistance;
};

/**
 * \brief a log distance propagation model.
 *
 * This model calculates the reception power with a so-called
 * log-distance propagation model:
 * \f$ L = L_0 + 10 n log_{10}(\frac{d}{d_0})\f$
 *
 * where:
 *  - \f$ n \f$ : the path loss distance exponent
 *  - \f$ d_0 \f$ : reference distance (m)
 *  - \f$ L_0 \f$ : path loss at reference distance (dB)
 *  - \f$ d \f$ : distance (m)
 *  - \f$ L \f$ : path loss (dB)
 *
 * When the path loss is requested at a distance smaller than
 * the reference distance, the tx power is returned.
 *
 */

struct LogDistancePropagationLossModel_tag
{
  double m_exponent;
  double m_referenceDistance;
  double m_referenceLoss;
  LogDistancePropagationLossModel *m_next;
};

/**
 * \brief A log distance path loss propagation model with three distance
 * fields. This model is the same as ns3::LogDistancePropagationLossModel
 * except that it has three distance fields: near, middle and far with
 * different exponents.
 *
 * Within each field the reception power is calculated using the log-distance
 * propagation equation:
 * \f[ L = L_0 + 10 \cdot n_0 log_{10}(\frac{d}{d_0})\f]
 * Each field begins where the previous ends and all together form a continuous function.
 *
 * There are three valid distance fields: near, middle, far. Actually four: the
 * first from 0 to the reference distance is invalid and returns txPowerDbm.
 *
 * \f[ \underbrace{0 \cdots\cdots}_{=0} \underbrace{d_0 \cdots\cdots}_{n_0} \underbrace{d_1 \cdots\cdots}_{n_1} \underbrace{d_2 \cdots\cdots}_{n_2} \infty \f]
 *
 * Complete formula for the path loss in dB:
 *
 * \f[\displaystyle L =
\begin{cases}
0 & d < d_0 \\
L_0 + 10 \cdot n_0 \log_{10}(\frac{d}{d_0}) & d_0 \leq d < d_1 \\
L_0 + 10 \cdot n_0 \log_{10}(\frac{d_1}{d_0}) + 10 \cdot n_1 \log_{10}(\frac{d}{d_1}) & d_1 \leq d < d_2 \\
L_0 + 10 \cdot n_0 \log_{10}(\frac{d_1}{d_0}) + 10 \cdot n_1 \log_{10}(\frac{d_2}{d_1}) + 10 \cdot n_2 \log_{10}(\frac{d}{d_2})& d_2 \leq d
\end{cases}\f]
 *
 * where:
 *  - \f$ L \f$ : resulting path loss (dB)
 *  - \f$ d \f$ : distance (m)
 *  - \f$ d_0, d_1, d_2 \f$ : three distance fields (m)
 *  - \f$ n_0, n_1, n_2 \f$ : path loss distance exponent for each field (unitless)
 *  - \f$ L_0 \f$ : path loss at reference distance (dB)
 *
 * When the path loss is requested at a distance smaller than the reference
 * distance \f$ d_0 \f$, the tx power (with no path loss) is returned. The
 * reference distance defaults to 1m and reference loss defaults to
 * ns3::FriisPropagationLossModel with 5.15 GHz and is thus \f$ L_0 \f$ = 46.67 dB.
 */

struct ThreeLogDistancePropagationLossModel_tag
{
  double m_distance0;
  double m_distance1;
  double m_distance2;

  double m_exponent0;
  double m_exponent1;
  double m_exponent2;

  double m_referenceLoss;
  ThreeLogDistancePropagationLossModel *m_next;
};

/**
 * \brief Nakagami-m fast fading propagation loss model.
 *
 * The Nakagami-m distribution is applied to the power level. The probability
 * density function is defined as
 * \f[ p(x; m, \omega) = \frac{2 m^m}{\Gamma(m) \omega^m} x^{2m - 1} e^{-\frac{m}{\omega} x^2} = 2 x \cdot p_{\text{Gamma}}(x^2, m, \frac{m}{\omega}) \f]
 * with \f$ m \f$ the fading depth parameter and \f$ \omega \f$ the average received power.
 *
 * It is implemented by either a ns3::GammaVariable or a ns3::ErlangVariable
 * random variable.
 *
 * Like in ns3::ThreeLogDistancePropagationLossModel, the m parameter is varied
 * over three distance fields:
 * \f[ \underbrace{0 \cdots\cdots}_{m_0} \underbrace{d_1 \cdots\cdots}_{m_1} \underbrace{d_2 \cdots\cdots}_{m_2} \infty \f]
 *
 * For m = 1 the Nakagami-m distribution equals the Rayleigh distribution. Thus
 * this model also implements Rayleigh distribution based fast fading.
 */

struct NakagamiPropagationLossModel_tag
{
  double m_distance1;
  double m_distance2;

  double m_m0;
  double m_m1;
  double m_m2;

  // ErlangVariable        m_erlangRandomVariable; Convert to ROSS rngs
  // GammaVariable         m_gammaRandomVariable; Convert to ROSS rngs
};

/**
 * \brief The propagation loss is fixed. The user can set received power level.
 */ 
struct FixedRssLossModel_tag
{
  double m_rss;
};

struct QadhocWifiMacQueues_tag
{
  AccessClass *ac;
  EdcaTxopN *edcatxopn;
};

struct QadhocWifiMac_tag
{
  /**
  * When an A-MSDU is received, is deaggregated by this method and all extracted packets are
  * forwarded up.
  */

  // typedef std::map<AccessClass, Ptr<EdcaTxopN> > Queues;
  // typedef std::list<std::pair<Ptr<Packet>, AmsduSubframeHeader> > DeaggregatedMsdus;
  // typedef std::list<std::pair<Ptr<Packet>, AmsduSubframeHeader> >::const_iterator DeaggregatedMsdusCI;

  QadhocWifiMacQueues *m_queues;
  EdcaTxopN *m_voEdca;
  EdcaTxopN *m_viEdca;
  EdcaTxopN *m_beEdca;
  EdcaTxopN *m_bkEdca;
  MacLow *m_low;
  WifiPhy *m_phy;
  WifiRemoteStationManager *m_stationManager;
  MacRxMiddle *m_rxMiddle;
  MacTxMiddle *m_txMiddle;
  DcfManager *m_dcfManager;
  Ssid *m_ssid;
  tw_stime m_eifsNoDifs;
};

struct QapWifiMacQueues_tag
{
  AccessClass *ac;
  EdcaTxopN *edcatxopn;
};

struct QapWifiMac_tag
{
  // typedef std::map<AccessClass, Ptr<EdcaTxopN> > Queues;
  // typedef std::list<std::pair<Ptr<Packet>, AmsduSubframeHeader> > DeaggregatedMsdus;
  // typedef std::list<std::pair<Ptr<Packet>, AmsduSubframeHeader> >::const_iterator DeaggregatedMsdusCI;
  
  QapWifiMacQueues *m_queues;
  DcaTxop *m_beaconDca;
  MacLow *m_low;
  WifiPhy *m_phy;
  WifiRemoteStationManager *m_stationManager;
  MacRxMiddle *m_rxMiddle;
  MacTxMiddle *m_txMiddle;
  DcfManager *m_dcfManager;
  Ssid *m_ssid;
  tw_event *m_beaconEvent;
  tw_stime m_beaconInterval;
  bool m_enableBeaconGeneration;
  // Callback<void,Ptr<Packet>, Mac48Address, Mac48Address> m_forwardUp; Convert to function pointer
};

/**
 * As per IEEE Std. 802.11-2007, Section 6.1.1.1.1, when EDCA is used the
 * the Traffic ID (TID) value corresponds to one of the User Priority (UP)
 * values defined by the IEEE Std. 802.1D-2004, Annex G, table G-2.
 *
 * Note that this correspondence does not hold for HCCA, since in that
 * case the mapping between UPs and TIDs should be determined by a
 * TSPEC element as per IEEE Std. 802.11-2007, Section 7.3.2.30
 */
enum UserPriority {
  UP_BK = 1, /**< background  */
  UP_BE = 0, /**< best effort (default) */
  UP_EE = 3, /**< excellent effort  */
  UP_CL = 4, /**< controlled load */
  UP_VI = 5, /**< video, < 100ms latency and jitter */
  UP_VO = 6, /**< voice, < 10ms latency and jitter */
  UP_NC = 7  /**< network control */
};
  
  /**
   * The aim of the QosTag is to provide means for an Application to
   * specify the TID which will be used by a QoS-aware WifiMac for a
   * given traffic flow. Note that the current QosTag class was
   * designed to be completely mac/wifi specific without any attempt
   * at being generic. 
   */
struct QosTag_tag
{
  uint8_t m_tid;
};

enum AccessClass_tag
{
  AC_VO = 0,
  AC_VI = 1,
  AC_BE = 2,
  AC_BK = 3,
  AC_BE_NQOS = 4,
  AC_UNDEF
};

struct QstaWifiMacQueues_tag
{
  AccessClass *ac;
  EdcaTxopN *edcatxopn;
};

struct QstaWifiMac_tag
{
  // typedef std::map<AccessClass, Ptr<EdcaTxopN> > Queues;
  // typedef std::list<std::pair<Ptr<Packet>, AmsduSubframeHeader> > DeaggregatedMsdus;
  // typedef std::list<std::pair<Ptr<Packet>, AmsduSubframeHeader> >::const_iterator DeaggregatedMsdusCI;
    
  QstaWifiMacQueues *m_queues;
  MacLow *m_low;
  WifiPhy *m_phy;
  WifiRemoteStationManager *m_stationManager;
  DcfManager *m_dcfManager;
  MacRxMiddle *m_rxMiddle;
  MacTxMiddle *m_txMiddle;
  Ssid *m_ssid;
  
  // CONVERT THESE TO Function Pointers !!
  // Callback<void, Ptr<Packet>, Mac48Address, Mac48Address> m_forwardUp;
  // Callback<void> m_linkUp;
  // Callback<void> m_linkDown;
  
  tw_stime m_probeRequestTimeout;
  tw_stime m_assocRequestTimeout;
  tw_event *m_probeRequestEvent;
  tw_event *m_assocRequestEvent;
  
  tw_stime m_beaconWatchdogEnd;
  tw_event *m_beaconWatchdog;
  
  uint32_t m_maxMissedBeacons;
};

struct Thresholds_tag
{
  uint32_t datarate;
  double pori;
  double pmtl;
  uint32_t ewnd;
};

/**
 * \brief Robust Rate Adaptation Algorithm
 *
 * This is an implementation of RRAA as described in
 * "Robust rate adaptation for 802.11 wireless networks"
 * by "Starsky H. Y. Wong", "Hao Yang", "Songwu Lu", and,
 * "Vaduvur Bharghavan" published in Mobicom 06.
 */

struct RraaWifiManager_tag
{
  bool m_basic;
  tw_stime m_timeout;
  uint32_t m_ewndfor54;
  uint32_t m_ewndfor48;
  uint32_t m_ewndfor36;
  uint32_t m_ewndfor24;
  uint32_t m_ewndfor18;
  uint32_t m_ewndfor12;
  uint32_t m_ewndfor9;
  uint32_t m_ewndfor6;
  double m_porifor48;
  double m_porifor36;
  double m_porifor24;
  double m_porifor18;
  double m_porifor12;
  double m_porifor9;
  double m_porifor6;
  double m_pmtlfor54;
  double m_pmtlfor48;
  double m_pmtlfor36;
  double m_pmtlfor24;
  double m_pmtlfor18;
  double m_pmtlfor12;
  double m_pmtlfor9;
};

struct RraaWifiRemoteStation_tag
{
  uint32_t m_counter;
  uint32_t m_failed;
  uint32_t m_rtsWnd;
  uint32_t m_rtsCounter;
  tw_stime m_lastReset;
  bool m_rtsOn;
  bool m_lastFrameFail;
  bool m_initialized;
  uint32_t m_rate;
  RraaWifiManager *m_stations;
};

/**
 * \brief a IEEE 802.11 SSID
 *
 */
struct Ssid_tag
{
  uint8_t m_ssid[33];
  uint8_t m_length;
};

struct StatusCode_tag
{
  uint16_t m_code;
};

struct SupportedRates_tag
{
  uint8_t m_nRates;
  uint8_t m_rates[8];
};

/**
 * \brief A 802.11 Channel
 *
 * This class works in tandem with the ns3::WifiPhy class. If you want to
 * provide a new Wifi PHY layer, you have to subclass both ns3::WifiChannel 
 * and ns3::WifiPhy.
 *
 * Typically, MyWifiChannel will define a Send method whose job is to distribute
 * packets from a MyWifiPhy source to a set of MyWifiPhy destinations. MyWifiPhy
 * also typically defines a Receive method which is invoked by MyWifiPhy.
 */

struct WifiChannel_tag
// Don't need since just a method container class 


/**
 * \brief base class for all MAC-level wifi objects.
 *
 * This class encapsulates all the low-level MAC functionality
 * DCA, EDCA, etc) and all the high-level MAC functionality
 * (association/disassociation state machines).
 *
 */


struct WifiMac_tag
{
  tw_stime m_maxPropagationDelay;
  uint32_t m_maxMsduSize;

  // NOTE these configure functions will need to be pulled
//   void Configure80211a (void);
//   void Configure80211b (void);
//   void Configure80211_10Mhz (void);
//   void Configure80211_5Mhz ();
//   void Configure80211p_CCH (void);
//   void Configure80211p_SCH (void);

  /**
   * The trace source fired when packets come into the "top" of the device
   * at the L3/L2 transition, before being queued for transmission.
   *
   * \see class CallBackTraceSource
   */
  // TracedCallback<Ptr<const Packet> > m_macTxTrace; CONVERT TO Function Pointer
  callback_wifimac m_macTxTrace;

  /**
   * The trace source fired when packets coming into the "top" of the device
   * are dropped at the MAC layer during transmission.
   *
   * \see class CallBackTraceSource
   */
  // TracedCallback<Ptr<const Packet> > m_macTxDropTrace;
  callback_wifimac m_macTxDropTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3 
   * transition).  This is a promiscuous trace.
   *
   * \see class CallBackTraceSource
   */
  // TracedCallback<Ptr<const Packet> > m_macPromiscRxTrace;
  callback_wifimac m_macPromiscRxTrace;

  /**
   * The trace source fired for packets successfully received by the device
   * immediately before being forwarded up to higher layers (at the L2/L3 
   * transition).  This is a non- promiscuous trace.
   *
   * \see class CallBackTraceSource
   */
  // TracedCallback<Ptr<const Packet> > m_macRxTrace;
  callback_wifimac m_macRxTrace;

  /**
   * The trace source fired when packets coming into the "top" of the device
   * are dropped at the MAC layer during reception.
   *
   * \see class CallBackTraceSource
   */
  // TracedCallback<Ptr<const Packet> > m_macRxDropTrace;
  callback_wifimac m_macRxDropTrace;
};

enum WifiMacType_tag
{
  WIFI_MAC_CTL_RTS = 0,
  WIFI_MAC_CTL_CTS,
  WIFI_MAC_CTL_ACK,
  WIFI_MAC_CTL_BACKREQ,
  WIFI_MAC_CTL_BACKRESP,

  WIFI_MAC_MGT_BEACON,
  WIFI_MAC_MGT_ASSOCIATION_REQUEST,
  WIFI_MAC_MGT_ASSOCIATION_RESPONSE,
  WIFI_MAC_MGT_DISASSOCIATION,
  WIFI_MAC_MGT_REASSOCIATION_REQUEST,
  WIFI_MAC_MGT_REASSOCIATION_RESPONSE,
  WIFI_MAC_MGT_PROBE_REQUEST,
  WIFI_MAC_MGT_PROBE_RESPONSE,
  WIFI_MAC_MGT_AUTHENTICATION,
  WIFI_MAC_MGT_DEAUTHENTICATION,
  WIFI_MAC_MGT_ACTION,
  WIFI_MAC_MGT_ACTION_NO_ACK,
  WIFI_MAC_MGT_MULTIHOP_ACTION,

  WIFI_MAC_DATA,
  WIFI_MAC_DATA_CFACK,
  WIFI_MAC_DATA_CFPOLL,
  WIFI_MAC_DATA_CFACK_CFPOLL,
  WIFI_MAC_DATA_NULL,
  WIFI_MAC_DATA_NULL_CFACK,
  WIFI_MAC_DATA_NULL_CFPOLL,
  WIFI_MAC_DATA_NULL_CFACK_CFPOLL,

  WIFI_MAC_QOSDATA,
  WIFI_MAC_QOSDATA_CFACK,
  WIFI_MAC_QOSDATA_CFPOLL,
  WIFI_MAC_QOSDATA_CFACK_CFPOLL,
  WIFI_MAC_QOSDATA_NULL,
  WIFI_MAC_QOSDATA_NULL_CFPOLL,
  WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL
};

enum QosAckPolicy_tag
 {
    NORMAL_ACK = 0,
    NO_ACK = 1,
    NO_EXPLICIT_ACK = 2,
    BLOCK_ACK = 3
  };
  
enum AddressType_tag
 {
    ADDR1,	
    ADDR2,
    ADDR3,
    ADDR4
  };

struct WifiMacHeader_tag
{
  uint8_t m_ctrlType;
  uint8_t m_ctrlSubtype;
  uint8_t m_ctrlToDs;
  uint8_t m_ctrlFromDs;
  uint8_t m_ctrlMoreFrag;
  uint8_t m_ctrlRetry;
  uint8_t m_ctrlPwrMgt;
  uint8_t m_ctrlMoreData;
  uint8_t m_ctrlWep;
  uint8_t m_ctrlOrder;
  uint16_t m_duration;
  Mac48Address *m_addr1;
  Mac48Address *m_addr2;
  Mac48Address *m_addr3;
  uint8_t m_seqFrag;
  uint16_t m_seqSeq;
  Mac48Address *m_addr4;
  uint8_t m_qosTid;
  uint8_t m_qosEosp;
  uint8_t m_qosAckPolicy;
  uint8_t m_amsduPresent;
  uint16_t m_qosStuff;
};

/**
 * \brief a 802.11e-specific queue.
 *
 * This queue implements what is needed for the 802.11e standard
 * Specifically, it refers to 802.11e/D9, section 9.9.1.6, paragraph 6.
 *
 * When a packet is received by the MAC, to be sent to the PHY, 
 * it is queued in the internal queue after being tagged by the 
 * current time.
 *
 * When a packet is dequeued, the queue checks its timestamp 
 * to verify whether or not it should be dropped. If 
 * dot11EDCATableMSDULifetime has elapsed, it is dropped.
 * Otherwise, it is returned to the caller.
 */

struct WifiMacQueueItem_tag
 {
    Packet *packet;
    WifiMacHeader *hdr;
    tw_stime tstamp;
    WifiMacQueueItem *next;
 };

struct WifiMacQueue_tag
{
  // struct Item;
  // typedef std::list<struct Item> PacketQueue;
  // typedef std::list<struct Item>::reverse_iterator PacketQueueRI;
  // typedef std::list<struct Item>::iterator PacketQueueI;
  
 
  WifiMacQueueItem *m_queue;
  WifiMacParameters *m_parameters;
  uint32_t m_size;
  uint32_t m_maxSize;
  tw_stime m_maxDelay;
};

/**
 * \brief represent a single transmission mode
 *
 * A WifiMode is implemented by a single integer which is used
 * to lookup in a global array the characteristics of the
 * associated transmission mode. It is thus extremely cheap to
 * keep a WifiMode variable around.
 */
enum WifiModeModulationType_tag
  BPSK,
  QPSK,
  DBPSK,
  DQPSK,
  QAM,
  UNKNOWN
};


struct WifiMode_tag
{
  //WifiMode (uint32_t uid);
  uint32_t m_uid;
};


/**
 * \class ns3::WifiModeValue
 * \brief hold objects of type ns3::WifiMode
 */

//ATTRIBUTE_HELPER_HEADER (WifiMode);

/**
 * \brief create WifiMode class instances and keep track of them.
 *
 * This factory ensures that each WifiMode created has a unique name
 * and assigns to each of them a unique integer.
 */
struct WifiModeFactory_tag
  bool Search (std::string name, WifiMode *mode);
  uint32_t AllocateUid (std::string uniqueName);
  WifiModeFactoryWifiModeItem item;

  typedef std::vector<struct WifiModeItem> WifiModeItemList;
  WifiModeItemList m_itemList;	
}

struct WifiModeFactoryWifiModeItem_tag
  std::string uniqueUid;
  uint32_t bandwidth;
  uint32_t dataRate;
  uint32_t phyRate;
  enum WifiMode::ModulationType modulation;
  uint8_t constellationSize;
  bool isMandatory;
  enum WifiPhyStandard standard;
};

/**
 * \brief Hold together all Wifi-related objects.
 *
 * This class holds together ns3::WifiChannel, ns3::WifiPhy,
 * ns3::WifiMac, and, ns3::WifiRemoteStationManager.
 */
struct WifiNetDevice_tag
{
  Node* m_node;
  WifiPhy* m_phy;
  WifiMac* m_mac;
  WifiRemoteStationManager* m_stationManager;
  NetDevice_ReceiveCallback m_forwardUp;
  NetDevice_PromiscReceiveCallback m_promiscRx;

  //TracedCallback<Ptr<const Packet>, Mac48Address> m_rxLogger;
  //TracedCallback<Ptr<const Packet>, Mac48Address> m_txLogger;

  uint32_t m_ifIndex;
  bool m_linkUp;
  //TracedCallback<> m_linkChanges;
  uint16_t m_mtu;
  bool m_configComplete;
};

enum WifiPhyState_tag
    /**
     * The PHY layer is IDLE.
     */
    IDLE,
    /**
     * The PHY layer has sense the medium busy through the CCA mechanism
     */
    CCA_BUSY,
    /**
     * The PHY layer is sending a packet.
     */
    TX,
    /**
     * The PHY layer is receiving a packet.
     */
    RX,
    /**
     * The PHY layer is switching to other channel.
     */
    SWITCHING
  };
}

struct WifiPhy_tag
//{
  /**
   * arg1: packet received successfully
   * arg2: snr of packet
   * arg3: mode of packet
   * arg4: type of preamble used for packet.
   */
  //typedef Callback<void,Ptr<Packet>, double, WifiMode, enum WifiPreamble> RxOkCallback;
  /**
   * arg1: packet received unsuccessfully
   * arg2: snr of packet
   */
  //typedef Callback<void,Ptr<const Packet>, double> RxErrorCallback;

  //tw_event* GetTypeId (void);

 // TracedCallback<Ptr<const Packet> > m_phyTxBeginTrace;

  /**
   * The trace source fired when a packet ends the transmission process on
   * the medium.
   *
   * \see class CallBackTraceSource
   */
 // TracedCallback<Ptr<const Packet> > m_phyTxEndTrace;

  /**
   * The trace source fired when the phy layer drops a packet as it tries
   * to transmit it.
   *
   * \see class CallBackTraceSource
   */
 // TracedCallback<Ptr<const Packet> > m_phyTxDropTrace;

  /**
   * The trace source fired when a packet begins the reception process from
   * the medium.
   *
   * \see class CallBackTraceSource
   */
  //TracedCallback<Ptr<const Packet> > m_phyRxBeginTrace;

  /**
   * The trace source fired when a packet ends the reception process from
   * the medium.
   *
   * \see class CallBackTraceSource
   */
 // TracedCallback<Ptr<const Packet> > m_phyRxEndTrace;

  /**
   * The trace source fired when the phy layer drops a packet it has received.
   *
   * \see class CallBackTraceSource
   */
  //TracedCallback<Ptr<const Packet> > m_phyRxDropTrace;

  /**
   * A trace source that emulates a wifi device in monitor mode
   * sniffing a packet being received. 
   * 
   * As a reference with the real world, firing this trace
   * corresponds in the madwifi driver to calling the function
   * ieee80211_input_monitor()
   *
   * \see class CallBackTraceSource
   */
 // TracedCallback<Ptr<const Packet>, uint16_t, uint16_t, uint32_t, bool, double, double> m_phyPromiscSniffRxTrace;

  /**
   * A trace source that emulates a wifi device in monitor mode
   * sniffing a packet being transmitted. 
   * 
   * As a reference with the real world, firing this trace
   * corresponds in the madwifi driver to calling the function
   * ieee80211_input_monitor()
   *
   * \see class CallBackTraceSource
   */
 // TracedCallback<Ptr<const Packet>, uint16_t, uint16_t, uint32_t, bool> m_phyPromiscSniffTxTrace;

//};

enum WifiPhyStandard_tag
  WIFI_PHY_STANDARD_80211a,
  WIFI_PHY_STANDARD_80211b,
  WIFI_PHY_STANDARD_80211_10Mhz,
  WIFI_PHY_STANDARD_80211_5Mhz,
  WIFI_PHY_STANDARD_holland,
  WIFI_PHY_STANDARD_80211p_CCH,
  WIFI_PHY_STANDARD_80211p_SCH,
  WIFI_PHY_UNKNOWN
};

struct WifiPhyStateHelper_tag
{
  bool m_rxing;
  tw_stime m_endTx;
  tw_stime m_endRx;
  tw_stime m_endCcaBusy;
  tw_stime m_endSwitching; 
  tw_stime m_startTx;
  tw_stime m_startRx;
  tw_stime m_startCcaBusy;
  tw_stime m_startSwitching; 
  tw_stime m_previousStateChangeTime;

  Listeners* m_listeners;
//  TracedCallback<Ptr<const Packet>, double, WifiMode, enum WifiPreamble> m_rxOkTrace;
//  TracedCallback<Ptr<const Packet>, double> m_rxErrorTrace;
//  TracedCallback<Ptr<const Packet>,WifiMode,WifiPreamble,uint8_t> m_txTrace;
 // WifiPhy_RxOkCallback m_rxOkCallback;
  //WifiPhy_RxErrorCallback m_rxErrorCallback;
};

enum WifiPreamble_tag
  WIFI_PREAMBLE_LONG,
  WIFI_PREAMBLE_SHORT
};

/**
 * \brief hold a list of per-remote-station state.
 *
 * \sa ns3::WifiRemoteStation.
 */
struct WifiRemoteStationManager_tag
{
  WifiRemoteStation* Stations;
  Stations m_stations;
  WifiMode m_defaultTxMode;
  NonUnicastWifiRemoteStation *m_nonUnicast;
  BasicModes m_basicModes;
  bool m_isLowLatency;
  uint32_t m_maxSsrc;
  uint32_t m_maxSlrc;
  uint32_t m_rtsCtsThreshold;
  uint32_t m_fragmentationThreshold;
  WifiMode m_nonUnicastMode;

  /**
   * The trace source fired when the transmission of a RTS has failed
   *
   * \see class CallBackTraceSource
   */
  //TracedCallback<Mac48Address> m_macTxRtsFailed;

  /**
   * The trace source fired when the transmission of a data packet has failed 
   *
   * \see class CallBackTraceSource
   */
  //TracedCallback<Mac48Address> m_macTxDataFailed;

  /**
   * The trace source fired when the transmission of a RTS has
   * exceeded the maximum number of attempts
   *
   * \see class CallBackTraceSource
   */
  //TracedCallback<Mac48Address> m_macTxFinalRtsFailed;

  /**
   * The trace source fired when the transmission of a data packet has
   * exceeded the maximum number of attempts
   *
   * \see class CallBackTraceSource
   */
  //TracedCallback<Mac48Address> m_macTxFinalDataFailed;

};

/**
 * \brief hold per-remote-station state.
 *
 * The state in this class is used to keep track
 * of association status if we are in an infrastructure
 * network and to perform the selection of tx parameters
 * on a per-packet basis.
 */
enum WifiRemoteStationState_tag
  BRAND_NEW,
  DISASSOC,
  WAIT_ASSOC_TX_OK,
  GOT_ASSOC_TX_OK
}

struct WifiRemoteStation_tag
  WifiMode* SupportedModes;
  WifiRemoteStationState m_state;
  SupportedModes m_modes;
 // TracedValue<uint32_t> m_ssrc;
  //TracedValue<uint32_t> m_slrc;
  uint32_t m_ssrc;
  uint32_t m_slrc;
  double m_avgSlrcCoefficient;
  double m_avgSlrc;
  Mac48Address m_address;
};

#ifdef ENABLE_GSL
typedef struct FunctionParameterType
{
 double beta;
 double n;
} FunctionParameters;

double IntegralFunction (double x, void *params);
#endif

/**
 * \brief Model the error rate for different modulations.
 *
 * A packet of interest (e.g., a packet can potentially be received by the MAC) 
 * is divided into chunks. Each chunk is related to an start/end receiving event. 
 * For each chunk, it calculates the ratio (SINR) between received power of packet 
 * of interest and summation of noise and interfering power of all the other incoming 
 * packets. Then, it will calculate the success rate of the chunk based on 
 * BER of the modulation. The success reception rate of the packet is derived from 
 * the success rate of all chunks.
 *
 * The 802.11b modulations:
 *    - 1 Mbps mode is based on DBPSK. BER is from equation 5.2-69 from John G. Proakis
 *      Digitial Communications, 2001 edition
 *    - 2 Mbps model is based on DQPSK. Equation 8 from "Tight bounds and accurate 
 *      approximations for dqpsk transmission bit error rate", G. Ferrari and G.E. Corazza 
 *      ELECTRONICS LETTERS, 40(20):1284-1285, September 2004
 *    - 5.5 Mbps and 11 Mbps are based on equations (18) and (17) from "Properties and 
 *      performance of the ieee 802.11b complementarycode-key signal sets", 
 *      Michael B. Pursley and Thomas C. Royster. IEEE TRANSACTIONS ON COMMUNICATIONS, 
 *      57(2):440-449, February 2009.
 *    - More detailed description and validation can be found in 
 *      http://www.nsnam.org/~pei/80211b.pdf
 */
struct YansErrorRateModel_tag
{
  static const double WLAN_SIR_PERFECT;
  static const double WLAN_SIR_IMPOSSIBLE;
};

/**
 * \brief A Yans wifi channel
 *
 * This wifi channel implements the propagation model described in
 * "Yet Another Network Simulator", (http://cutebugs.net/files/wns2-yans.pdf).
 *
 * This class is expected to be used in tandem with the ns3::YansWifiPhy 
 * class and contains a ns3::PropagationLossModel and a ns3::PropagationDelayModel.
 * By default, no propagation models are set so, it is the caller's responsability
 * to set them before using the channel.
 */
struct YansWifiChannel_tag
{
  YansWifiPhy* PhyList;
  PhyList m_phyList;
  PropagationLossModel* m_loss;
  PropagationDelayModel* m_delay;
};

/**
 * \brief 802.11 PHY layer model
 *
 * This PHY implements a model of 802.11a. The model
 * implemented here is based on the model described
 * in "Yet Another Network Simulator", 
 * (http://cutebugs.net/files/wns2-yans.pdf).
 *
 *
 * This PHY model depends on a channel loss and delay
 * model as provided by the ns3::PropagationLossModel
 * and ns3::PropagationDelayModel classes, both of which are
 * members of the ns3::YansWifiChannel class.
 */
struct YansWifiPhy_tag
  double   m_edThresholdW;
  double   m_ccaMode1ThresholdW;
  double   m_txGainDb;
  double   m_rxGainDb;
  double   m_txPowerBaseDbm;
  double   m_txPowerEndDbm;
  uint32_t m_nTxPower;

  YansWifiChannel* m_channel;
  uint16_t m_channelNumber;
  //Ptr<Object> m_device;
  //Ptr<Object> m_mobility;
  Modes* m_modes;
  tw_event* m_endRxEvent;
  UniformVariable m_random;
  /// Standard-dependent center frequency of 0-th channel, MHz 
  double m_channelStartingFrequency;
  WifiPhyStateHelper* m_state;
  InterferenceHelper m_interference;
  tw_stime m_channelSwitchDelay;

};
