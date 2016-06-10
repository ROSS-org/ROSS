#include <ross.h>


void get_time_ahead_GVT(tw_pe *me, tw_stime current_rt)
{
    tw_kp *kp;
    int i;
    st_stats stats;
    stats.kp_stats.time = current_rt;

    printf("%lf,", tw_clock_read() / g_tw_clock_rate);    
    for(i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);
        stats.kp_stats.id = kp->id;
        stats.kp_stats.pe_id = kp->pe->id;
        stats.kp_stats.vt_ahead_gvt = kp->last_time - me->GVT;
        st_buffer_push(ST_KP, &stats);
        // TODO remove print after debug
        printf("%lf", kp->last_time - me->GVT);
        if(i < g_tw_nkp - 1)
           printf(",");
        else
           printf("\n"); 
    }
}
