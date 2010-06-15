#ifndef INC_ospf_decls
#define INC_ospf_decls

/*
 * ospf-messages.h
 */
struct ospf_message_tag;
typedef struct ospf_message_tag ospf_message;
struct ospf_hello_tag;
typedef struct ospf_hello_tag ospf_hello;
struct ospf_dd_pkt_tag;
typedef struct ospf_dd_pkt_tag ospf_dd_pkt;

/*
 * ospf-data-types.h
 */
struct ospf_lsa_tag;
typedef struct ospf_lsa_tag ospf_lsa;
struct ospf_db_entry_tag;
typedef struct ospf_db_entry_tag ospf_db_entry;

struct ospf_lsa_link_tag;
typedef struct ospf_lsa_link_tag ospf_lsa_link;
enum ospf_message_type_tag;
typedef enum ospf_message_type_tag ospf_message_type;
enum ospf_lsa_type_tag;
typedef enum ospf_lsa_type_tag ospf_lsa_type;

/*
 * ospf-types.h
 */
struct ospf_global_state_tag;
typedef struct ospf_global_state_tag ospf_global_state;
struct ospf_state_tag;
typedef struct ospf_state_tag ospf_state;
struct ospf_statistics_tag;
typedef struct ospf_statistics_tag ospf_statistics;
enum ospf_link_type_tag;
typedef enum ospf_link_type_tag ospf_link_type;

/*
 * ospf-neighbor-types.h
 */
struct ospf_nbr_tag;
typedef struct ospf_nbr_tag ospf_nbr;

/*
 * ospf-data-structure.h
 */
#endif
