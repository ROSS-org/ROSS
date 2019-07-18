#ifndef INC_time_bits_h
#define INC_time_bits_h

#define TW_STIME_BITS_SIZE 2

typedef struct tw_stime {
    double t;
    long long int bits[TW_STIME_BITS_SIZE];
} tw_stime;

extern MPI_Datatype MPI_TYPE_TW_STIME;
void tw_stime_mpi_datatype(void);

int tw_stime_cmp(tw_stime x, tw_stime y);
tw_stime tw_stime_add(tw_stime x, tw_stime y);
tw_stime tw_stime_max(void);
tw_stime tw_stime_create(double x);

#define TW_STIME_CRT(x)     (tw_stime_create(x))
#define TW_STIME_DBL(x)     (x.t)
#define TW_STIME_CMP(x, y)  (tw_stime_cmp((x), (y)))
#define TW_STIME_ADD(x, y)  (tw_stime_add((x), (y)))
#define TW_STIME_MAX        (tw_stime_max())

#endif
