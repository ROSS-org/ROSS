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

    // reset from default to 512MHz as that's the timebase for the POWER9 system.
    g_tw_clock_rate = 512000000.0;
    return clock_opts;
}

tw_clock tw_clock_read(void)
{
  unsigned int tbl, tbu0, tbu1;

  do {
    __asm__ __volatile__ ("mftbu %0" : "=r"(tbu0));
    __asm__ __volatile__ ("mftb %0" : "=r"(tbl));
    __asm__ __volatile__ ("mftbu %0" : "=r"(tbu1));
  } while (tbu0 != tbu1);

  return (((unsigned long long)tbu0) << 32) | tbl;
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
