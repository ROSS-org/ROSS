#include <ross.h>

extern unsigned long long g_tw_clock_rate;

static const tw_optdef clock_opts [] =
{
 TWOPT_GROUP("ROSS Timing"),
 TWOPT_ULONGLONG("clock-rate", g_tw_clock_rate, "CPU Clock Rate"),
 TWOPT_END()
};

const tw_optdef *tw_clock_setup(void)
{
	return clock_opts;
}

tw_clock tw_clock_read(void)
{
#ifdef ZERO_BASED
  static volatile int inited = 0;
  static volatile tw_clock base = 0;
#else
  const tw_clock base = 0;
#endif

  const tw_clock scale = 1000000;
  struct timeval tv;
  gettimeofday(&tv,NULL);

#ifdef ZERO_BASED
  if(inited == 0) {
    base = ((tw_clock) tv.tv_sec)*scale + (tw_clock) tv.tv_usec;
    inited = 1;
  }
#endif

  return
    (((tw_clock) tv.tv_sec)*scale + (tw_clock) tv.tv_usec) - base;
}

void
tw_clock_init(tw_pe * me)
{
	me->clock_time = 0;
	me->clock_offset = tw_clock_read();
}

tw_clock
tw_clock_now(tw_pe * me)
{
	me->clock_time = tw_clock_read() - me->clock_offset;
	return me->clock_time;
}
