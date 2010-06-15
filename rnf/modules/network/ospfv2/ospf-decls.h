#ifndef INC_ospf_decls
#define INC_ospf_decls

/*
 * ospf-messages.h
 */
FWD(struct, ospf_message);
FWD(struct, ospf_hello);
FWD(struct, ospf_dd_pkt);

/*
 * ospf-data-types.h
 */
FWD(struct, ospf_lsa);
FWD(struct, ospf_db_entry);

FWD(struct, ospf_lsa_link);
FWD(enum, ospf_message_type);
FWD(enum, ospf_lsa_type);

/*
 * ospf-types.h
 */
FWD(struct, ospf_global_state);
FWD(struct, ospf_state);
FWD(struct, ospf_statistics);
FWD(enum, ospf_link_type);

/*
 * ospf-neighbor-types.h
 */
FWD(struct, ospf_nbr);

/*
 * ospf-data-structure.h
 */
#endif
