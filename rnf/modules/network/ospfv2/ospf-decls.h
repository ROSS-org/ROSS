#ifndef INC_ospf_decls
#define INC_ospf_decls

/*
 * ospf-messages.h
 */
typedef struct ospf_message ospf_message;
typedef struct ospf_hello ospf_hello;
typedef struct ospf_dd_pkt ospf_dd_pkt;

/*
 * ospf-data-types.h
 */
typedef struct ospf_lsa ospf_lsa;
typedef struct ospf_db_entry ospf_db_entry;

typedef struct ospf_lsa_link ospf_lsa_link;
typedef enum ospf_message_type ospf_message_type;
typedef enum ospf_lsa_type ospf_lsa_type;

/*
 * ospf-types.h
 */
typedef struct ospf_global_state ospf_global_state;
typedef struct ospf_state ospf_state;
typedef struct ospf_statistics ospf_statistics;
typedef enum ospf_link_type ospf_link_type;

/*
 * ospf-neighbor-types.h
 */
typedef struct ospf_nbr ospf_nbr;

/*
 * ospf-data-structure.h
 */
#endif
