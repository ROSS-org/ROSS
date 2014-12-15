#ifndef BUDDY_H
#define BUDDY_H

typedef struct buddy_list
{
    void *ptr;
    struct buddy_list *next;
    unsigned int order;
} buddy_list_t;

buddy_list_t * create_buddy_table(unsigned int power_of_two);
unsigned int next_power2(unsigned int v);

#endif /* BUDDY_H */
