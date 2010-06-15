#ifndef INC_ospf_decls
#define INC_ospf_decls

/*
 * ospf-messages.h
 */
struct ospf_message_t;
typedef ospf_message_t ospf_message;
struct ospf_hello_t;
typedef ospf_hello_t ospf_hello;
struct ospf_dd_pkt_t;
typedef ospf_dd_pkt_t ospf_dd_pkt;

/*
 * ospf-data-types.h
 */
struct ospf_lsa_t;
typedef ospf_lsa_t ospf_lsa;
struct ospf_db_entry_t;
typedef ospf_db_entry_t ospf_db_entry;

struct ospf_lsa_link_t;
typedef ospf_lsa_link_t ospf_lsa_link;
enum ospf_message_type_t;
typedef ospf_message_type_t ospf_message_type;
enum ospf_lsa_type_t;
typedef ospf_lsa_type_t ospf_lsa_type;

/*
 * ospf-types.h
 */
struct ospf_global_state_t;
typedef ospf_global_state_t ospf_global_state;
struct ospf_state_t;
typedef ospf_state_t ospf_state;
struct ospf_statistics_t;
typedef ospf_statistics_t ospf_statistics;
enum ospf_link_type_t;
typedef ospf_link_type_t ospf_link_type;

/*
 * ospf-neighbor-types.h
 */
struct ospf_nbr_t;
typedef ospf_nbr_t ospf_nbr;

/*
 * ospf-data-structure.h
 */
#endif
