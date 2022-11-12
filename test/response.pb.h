/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.6 */

#ifndef PB_PROTOS_RESPONSE_PB_H_INCLUDED
#define PB_PROTOS_RESPONSE_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Struct definitions */
typedef struct _protos_Response { 
    int32_t code;
    pb_callback_t msg;
} protos_Response;


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define protos_Response_init_default             {0, {{NULL}, NULL}}
#define protos_Response_init_zero                {0, {{NULL}, NULL}}

/* Field tags (for use in manual encoding/decoding) */
#define protos_Response_code_tag                 1
#define protos_Response_msg_tag                  2

/* Struct field encoding specification for nanopb */
#define protos_Response_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, INT32,    code,              1) \
X(a, CALLBACK, SINGULAR, STRING,   msg,               2)
#define protos_Response_CALLBACK pb_default_field_callback
#define protos_Response_DEFAULT NULL

extern const pb_msgdesc_t protos_Response_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define protos_Response_fields &protos_Response_msg

/* Maximum encoded size of messages (where known) */
/* protos_Response_size depends on runtime parameters */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
