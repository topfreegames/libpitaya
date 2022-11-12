/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.6 */

#ifndef PB_PROTOS_BIG_MESSAGE_PB_H_INCLUDED
#define PB_PROTOS_BIG_MESSAGE_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Struct definitions */
typedef struct _protos_NPC { 
    pb_callback_t name;
    double health;
    pb_callback_t publicId;
} protos_NPC;

typedef struct _protos_PlayerResponse { 
    pb_callback_t publicId;
    pb_callback_t accessToken;
    pb_callback_t name;
    pb_callback_t items;
    double health;
} protos_PlayerResponse;

typedef struct _protos_BigMessageResponse { 
    bool has_player;
    protos_PlayerResponse player;
    pb_callback_t npcs;
    pb_callback_t chests;
} protos_BigMessageResponse;

typedef struct _protos_BigMessageResponse_NpcsEntry { 
    pb_callback_t key;
    bool has_value;
    protos_NPC value;
} protos_BigMessageResponse_NpcsEntry;

typedef struct _protos_BigMessage { 
    pb_callback_t code;
    bool has_response;
    protos_BigMessageResponse response;
} protos_BigMessage;


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define protos_PlayerResponse_init_default       {{{NULL}, NULL}, {{NULL}, NULL}, {{NULL}, NULL}, {{NULL}, NULL}, 0}
#define protos_NPC_init_default                  {{{NULL}, NULL}, 0, {{NULL}, NULL}}
#define protos_BigMessageResponse_init_default   {false, protos_PlayerResponse_init_default, {{NULL}, NULL}, {{NULL}, NULL}}
#define protos_BigMessageResponse_NpcsEntry_init_default {{{NULL}, NULL}, false, protos_NPC_init_default}
#define protos_BigMessage_init_default           {{{NULL}, NULL}, false, protos_BigMessageResponse_init_default}
#define protos_PlayerResponse_init_zero          {{{NULL}, NULL}, {{NULL}, NULL}, {{NULL}, NULL}, {{NULL}, NULL}, 0}
#define protos_NPC_init_zero                     {{{NULL}, NULL}, 0, {{NULL}, NULL}}
#define protos_BigMessageResponse_init_zero      {false, protos_PlayerResponse_init_zero, {{NULL}, NULL}, {{NULL}, NULL}}
#define protos_BigMessageResponse_NpcsEntry_init_zero {{{NULL}, NULL}, false, protos_NPC_init_zero}
#define protos_BigMessage_init_zero              {{{NULL}, NULL}, false, protos_BigMessageResponse_init_zero}

/* Field tags (for use in manual encoding/decoding) */
#define protos_NPC_name_tag                      1
#define protos_NPC_health_tag                    2
#define protos_NPC_publicId_tag                  3
#define protos_PlayerResponse_publicId_tag       1
#define protos_PlayerResponse_accessToken_tag    2
#define protos_PlayerResponse_name_tag           3
#define protos_PlayerResponse_items_tag          4
#define protos_PlayerResponse_health_tag         5
#define protos_BigMessageResponse_player_tag     1
#define protos_BigMessageResponse_npcs_tag       2
#define protos_BigMessageResponse_chests_tag     3
#define protos_BigMessageResponse_NpcsEntry_key_tag 1
#define protos_BigMessageResponse_NpcsEntry_value_tag 2
#define protos_BigMessage_code_tag               1
#define protos_BigMessage_response_tag           2

/* Struct field encoding specification for nanopb */
#define protos_PlayerResponse_FIELDLIST(X, a) \
X(a, CALLBACK, SINGULAR, STRING,   publicId,          1) \
X(a, CALLBACK, SINGULAR, STRING,   accessToken,       2) \
X(a, CALLBACK, SINGULAR, STRING,   name,              3) \
X(a, CALLBACK, REPEATED, STRING,   items,             4) \
X(a, STATIC,   SINGULAR, DOUBLE,   health,            5)
#define protos_PlayerResponse_CALLBACK pb_default_field_callback
#define protos_PlayerResponse_DEFAULT NULL

#define protos_NPC_FIELDLIST(X, a) \
X(a, CALLBACK, SINGULAR, STRING,   name,              1) \
X(a, STATIC,   SINGULAR, DOUBLE,   health,            2) \
X(a, CALLBACK, SINGULAR, STRING,   publicId,          3)
#define protos_NPC_CALLBACK pb_default_field_callback
#define protos_NPC_DEFAULT NULL

#define protos_BigMessageResponse_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, MESSAGE,  player,            1) \
X(a, CALLBACK, REPEATED, MESSAGE,  npcs,              2) \
X(a, CALLBACK, REPEATED, STRING,   chests,            3)
#define protos_BigMessageResponse_CALLBACK pb_default_field_callback
#define protos_BigMessageResponse_DEFAULT NULL
#define protos_BigMessageResponse_player_MSGTYPE protos_PlayerResponse
#define protos_BigMessageResponse_npcs_MSGTYPE protos_BigMessageResponse_NpcsEntry

#define protos_BigMessageResponse_NpcsEntry_FIELDLIST(X, a) \
X(a, CALLBACK, SINGULAR, STRING,   key,               1) \
X(a, STATIC,   OPTIONAL, MESSAGE,  value,             2)
#define protos_BigMessageResponse_NpcsEntry_CALLBACK pb_default_field_callback
#define protos_BigMessageResponse_NpcsEntry_DEFAULT NULL
#define protos_BigMessageResponse_NpcsEntry_value_MSGTYPE protos_NPC

#define protos_BigMessage_FIELDLIST(X, a) \
X(a, CALLBACK, SINGULAR, STRING,   code,              1) \
X(a, STATIC,   OPTIONAL, MESSAGE,  response,          2)
#define protos_BigMessage_CALLBACK pb_default_field_callback
#define protos_BigMessage_DEFAULT NULL
#define protos_BigMessage_response_MSGTYPE protos_BigMessageResponse

extern const pb_msgdesc_t protos_PlayerResponse_msg;
extern const pb_msgdesc_t protos_NPC_msg;
extern const pb_msgdesc_t protos_BigMessageResponse_msg;
extern const pb_msgdesc_t protos_BigMessageResponse_NpcsEntry_msg;
extern const pb_msgdesc_t protos_BigMessage_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define protos_PlayerResponse_fields &protos_PlayerResponse_msg
#define protos_NPC_fields &protos_NPC_msg
#define protos_BigMessageResponse_fields &protos_BigMessageResponse_msg
#define protos_BigMessageResponse_NpcsEntry_fields &protos_BigMessageResponse_NpcsEntry_msg
#define protos_BigMessage_fields &protos_BigMessage_msg

/* Maximum encoded size of messages (where known) */
/* protos_PlayerResponse_size depends on runtime parameters */
/* protos_NPC_size depends on runtime parameters */
/* protos_BigMessageResponse_size depends on runtime parameters */
/* protos_BigMessageResponse_NpcsEntry_size depends on runtime parameters */
/* protos_BigMessage_size depends on runtime parameters */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
