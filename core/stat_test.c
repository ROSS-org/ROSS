#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "stat_tree.h"

#define N (1024)
#define MULTIPLIER (97)


int
main(int argc, char **argv)
{
    stat_node *t;
    //t = AVL_EMPTY;

    if(argc != 2) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        return 1;
    }

    t = init_stat_tree(0);
    printf("checking that tree init works...\n");
    statPrintKeys(t);

    printf("\nchecking height is correct...\n");
    printf("height %d\n", statGetHeight(t));

    stat_increment(t, 10, 0);
    printf("\nchecking that node 10, stat 0 is incremented...\n");
    statPrintKeys(t);

    t = add_nodes(t);
    printf("\nchecking that adding nodes/bins works...\n");
    statPrintKeys(t);

    statSanityCheck(t);

    printf("\nchecking if writing bins at GVT works...\n");
    char filename[9];
    sprintf(filename, "test.out");
    FILE *foo_log=fopen(filename, "a");
    if(foo_log)
        t = gvt_write_bins(foo_log, t, atoi(argv[1]));

    statSanityCheck(t);
    statPrintKeys(t);
        
    statDestroy(t);

    return 0;
}
