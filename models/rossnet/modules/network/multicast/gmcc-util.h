#ifndef ROSS_MCAST_GMCC_UTIL_H
#define ROSS_MCAST_GMCC_UTIL_H

#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef struct PacketSizeAndTimeStruct {
  double time_;
  int pktSize_;
} PacketSizeAndTime;


typedef struct RateAvgrDataStruct {
  int maxSampleSize_, head_, tail_, totalPktSize_;
  PacketSizeAndTime * samples_;  
} RateAvgrData;


typedef struct AvgCalcDataStruct {
  int maxSampleSize_, head_, tail_, sampleNum_;
  double * samples_, sum_;
} AvgCalcData;


typedef struct StatEstimatorDataStruct {
  int maxSampleSize_, 
      sampleHead_, sampleTail_, sampleNum_; 
  double * sample_, mean_, dev_, ewma_, sampleSum_;
} StatEstimatorData;


inline void RateAvgrInit (RateAvgrData * data, int sampleSize);
inline void RateAvgrReset (RateAvgrData * data);
double RateAvgrAddSample (RateAvgrData * data, double t, int pktSize);
inline double RateAvgrGetMean (RateAvgrData * data);

inline void AvgCalcInit (AvgCalcData * data, int sampleSize);
inline void AvgCalcReset (AvgCalcData * data);
double AvgCalcAddSample (AvgCalcData * data, double s);
inline double AvgCalcTest (AvgCalcData * data, double s);
inline double AvgCalcGetMean (AvgCalcData * data);

inline void StatEstimatorInit
  (StatEstimatorData * data, int sampleSize, double ewma);
inline void StatEstimatorReset (StatEstimatorData * data);
void StatEstimatorAddSample (StatEstimatorData * data, double s);
inline double StatEstimatorTest (StatEstimatorData * data, double s);

#endif
