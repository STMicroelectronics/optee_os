/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 */

#ifndef __RSE_COMMS_PROTOCOL_H__
#define __RSE_COMMS_PROTOCOL_H__

#include <compiler.h>
#include <psa/client.h>
#include "rse_comms_protocol_embed.h"
#include "rse_comms_protocol_pointer_access.h"
#include <stdint.h>

enum rse_comms_protocol_version_t {
	RSE_COMMS_PROTOCOL_EMBED = 0,
	RSE_COMMS_PROTOCOL_POINTER_ACCESS = 1,
};

struct __packed serialized_rse_comms_header_t {
	uint8_t protocol_ver;
	uint8_t seq_num;
	uint16_t client_id;
};

/* message passed from Host to RSE to deliver a PSA client call */
struct __packed serialized_rse_comms_msg_t {
	struct serialized_rse_comms_header_t header;
	union __packed {
		struct rse_embed_msg_t embed;
		struct rse_pointer_access_msg_t pointer_access;
	} msg;
};

/* reply message to hold the PSA client reply result returned by RSE */
struct __packed serialized_rse_comms_reply_t {
	struct serialized_rse_comms_header_t header;
	union __packed {
		struct rse_embed_reply_t embed;
		struct rse_pointer_access_reply_t pointer_access;
	} reply;
};

psa_status_t rse_protocol_serialize_msg(psa_handle_t handle,
					int16_t type,
					const psa_invec *in_vec,
					uint8_t in_len,
					const psa_outvec *out_vec,
					uint8_t out_len,
					struct serialized_rse_comms_msg_t *msg,
					size_t *msg_len);

psa_status_t rse_protocol_deserialize_reply(psa_outvec *out_vec,
					    uint8_t out_len,
					    psa_status_t *return_val,
					    const struct
					    serialized_rse_comms_reply_t *reply,
					    size_t reply_size);

#endif /* __RSE_COMMS_PROTOCOL_H__ */
