#include <ross.h>


void get_time_ahead_GVT(tw_pe *me, tw_stime current_rt)
{
    tw_kp *kp;
    int i, index;
    int element_size = sizeof(tw_stime);
    int num_bytes = (g_tw_nkp + 1) * element_size;
    char data[num_bytes];
    memcpy(&data, &current_rt, element_size);
    index = element_size;

    printf("%lf,", tw_clock_read() / g_tw_clock_rate);    
    for(i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);
        data[index] = kp->last_time - me->GVT;
        index += element_size;

        // TODO remove print after debug
        printf("%lf", kp->last_time - me->GVT);
        if(i < g_tw_nkp - 1)
           printf(",");
        else
           printf("\n"); 
    }
    st_buffer_push(g_st_buffer, &data, num_bytes);
}
