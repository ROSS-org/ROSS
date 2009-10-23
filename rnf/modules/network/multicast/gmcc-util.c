#include "gmcc-util.h"


void RateAvgrInit (RateAvgrData * data, int sampleSize)
{
  if (data->samples_ != NULL) free ((void *) data->samples_);
  memset ((void *) data, 0, sizeof (RateAvgrData));
  data->samples_ = (PacketSizeAndTime *) calloc (
    data->maxSampleSize_ = sampleSize + 1,
    sizeof (PacketSizeAndTime));
}


void RateAvgrReset (RateAvgrData * data)
{
  free ((void *) data->samples_);
  memset ((void *) data, 0, sizeof (RateAvgrData));
}


double RateAvgrAddSample (RateAvgrData * data, double t, int pktSize)
{
  if (data->head_ == data->tail_) {
    data->samples_[data->tail_].time_ = t;
    data->samples_[data->tail_].pktSize_ = 0;
    data->totalPktSize_ = 0;
    data->tail_ = (data->tail_ + 1) % data->maxSampleSize_;
    return 0;
  }

  data->samples_[data->tail_].time_ = t;
  data->totalPktSize_ += (data->samples_[data->tail_].pktSize_ = pktSize);
  data->tail_ = (data->tail_ + 1) % data->maxSampleSize_;
  if (data->tail_ == data->head_) {
    data->head_ = (data->head_ + 1) % data->maxSampleSize_;
    data->totalPktSize_ -= data->samples_[data->head_].pktSize_;
  }

  return data->totalPktSize_ / (t - data->samples_[data->head_].time_);
}


double RateAvgrGetMean (RateAvgrData * data)
{
  int i;
  
  if (data->head_ == data->tail_) return 0;
  
  i = (data->tail_ - 1 + data->maxSampleSize_) % data->maxSampleSize_;
  if (i == data->head_) return 0;
  
  return data->totalPktSize_ 
         / (data->samples_[i].time_ - data->samples_[data->head_].time_);
}


void AvgCalcInit (AvgCalcData * data, int sampleSize)
{
  if (data->samples_ != NULL) free ((void *) data->samples_);
  memset ((void *) data, 0, sizeof (AvgCalcData));
  data->samples_ = (double *) calloc (
    data->maxSampleSize_ = sampleSize + 1,
    sizeof (double));
}


void AvgCalcReset (AvgCalcData * data)
{
  free ((void *) data->samples_);
  memset ((void *) data, 0, sizeof (AvgCalcData));
}


double AvgCalcAddSample (AvgCalcData * data, double s)
{
  if (data->head_ == data->tail_) {
    data->sum_ = data->samples_[data->tail_] = s;
    data->sampleNum_ = 1;
    data->tail_ = (data->tail_ + 1) % data->maxSampleSize_;
    return s;
  }

  data->sum_ += (data->samples_[data->tail_] = s);
  data->tail_ = (data->tail_ + 1) % data->maxSampleSize_;
  if (data->tail_ == data->head_) {
    data->sum_ -= data->samples_[data->head_];
    data->head_ = (data->head_ + 1) % data->maxSampleSize_;
  }
  else {
    ++ data->sampleNum_;
  }

  return data->sum_ / data->sampleNum_;
}


double AvgCalcTest (AvgCalcData * data, double s)
{
  return (data->sum_ + s) / (data->sampleNum_ + 1);
}


double AvgCalcGetMean (AvgCalcData * data)
{
  return data->sampleNum_ == 0 ? 0 : (data->sum_ / data->sampleNum_);
}


void StatEstimatorInit (StatEstimatorData * data, int sampleSize, double ewma)
{
  if (data->sample_ != NULL) free ((void *) data->sample_);
  memset ((void *) data, 0, sizeof (StatEstimatorData));
  data->sample_ = (double *) calloc ( 
    data->maxSampleSize_ = sampleSize + 1,
    sizeof (double));
  data->ewma_ = ewma;
}


void StatEstimatorReset (StatEstimatorData * data)
{
  free  ((void *) data->sample_);
  memset ((void *) data, 0, sizeof (StatEstimatorData));  
}


void StatEstimatorAddSample (StatEstimatorData * data, double s)
{
  int i;
  double d = 0;

  if (data->sampleHead_ == data->sampleTail_) {
    data->sampleSum_ = data->sample_[data->sampleTail_] = s;
    data->sampleNum_ = 1;
    data->sampleTail_ = (data->sampleTail_ + 1) % data->maxSampleSize_;
  }
  else {
    data->sampleSum_ += (data->sample_[data->sampleTail_] = s);
    data->sampleTail_ = (data->sampleTail_ + 1) % data->maxSampleSize_;
    if (data->sampleTail_ == data->sampleHead_) {
      data->sampleSum_ -= data->sample_[data->sampleHead_];
      data->sampleHead_ = (data->sampleHead_ + 1) % data->maxSampleSize_;
    }
    else {
      ++ data->sampleNum_;
    }
  }

  data->mean_ = data->sampleSum_ / data->sampleNum_;
  
  for (i = data->sampleHead_; i != data->sampleTail_; 
       i = (i + 1) % data->maxSampleSize_) {
    d += (data->sample_[i] - data->mean_) * (data->sample_[i] - data->mean_);    
  }
  data->dev_ = sqrt (d / data->sampleNum_);
}


double StatEstimatorTest (StatEstimatorData * data, double s)
{
  if (data->sampleNum_ == 0) return s;

  return (data->sampleSum_ + s) / (data->sampleNum_ + 1);
}
