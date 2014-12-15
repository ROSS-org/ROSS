#ifndef BUDDY_H
#define BUDDY_H

typedef enum purpose { FREE, SPLIT, USED } purpose_t;

typedef struct buddy_list
{
    purpose_t use;
    struct buddy_list *next_free;
    unsigned size;
} buddy_list_t;

typedef struct buddy_list_head
{
    buddy_list_t *ptr;
    unsigned int count;
    unsigned int order;
} buddy_list_head_t;

buddy_list_head_t * create_buddy_table(unsigned int power_of_two);
unsigned int next_power2(unsigned int v);

#endif /* BUDDY_H */
