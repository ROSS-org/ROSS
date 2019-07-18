#include "ross.h"

MPI_Datatype MPI_TYPE_TW_STIME;

int tw_stime_cmp(tw_stime x, tw_stime y) {
    if(x.t < y.t) return -1;
    if(x.t > y.t) return  1;
    for(int i = 0; i < TW_STIME_BITS_SIZE; i++) {
        if(x.bits[i] < y.bits[i]) return -1;
        if(x.bits[i] > y.bits[i]) return  1;
    }
    return 0;
}

tw_stime tw_stime_add(tw_stime x, tw_stime y) {
    tw_stime rv;
    rv.t = x.t + y.t;
    for (int i = 0; i < TW_STIME_BITS_SIZE; i++) {
        rv.bits[i] = x.bits[i] + y.bits[i];
    }
    return rv;
}

tw_stime tw_stime_max(void) {
    tw_stime rv;
    rv.t = DBL_MAX;
    return rv;
}

tw_stime tw_stime_create(double x){
    tw_stime rv;
    rv.t = x;
    for (int i = 0; i < TW_STIME_BITS_SIZE; i++) {
        rv.bits[i] = 0;
    }
    return rv;
}

void tw_stime_mpi_datatype(void) {
    int structlen = 2;

    int blocklens[structlen];
    MPI_Datatype types[structlen];
    MPI_Aint displacements[structlen];

    blocklens[0] = 1;
    types[0] = MPI_DOUBLE;
    displacements[0] = 0;

    blocklens[1] = TW_STIME_BITS_SIZE;
    types[1] = MPI_LONG_LONG_INT;
    displacements[1] = sizeof(double);

    MPI_Type_create_struct(structlen, blocklens, displacements, types, &MPI_TYPE_TW_STIME);
    MPI_Type_commit(&MPI_TYPE_TW_STIME);
}
