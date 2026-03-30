/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SCMI Telemetry management protocol support.
 */

#include <internal/scmi_telemetry.h>

#include <mod_scmi.h>
#include <mod_scmi_telemetry.h>
#include <mod_telemetry.h>

#include <fwk_assert.h>
#include <fwk_attributes.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>
#include <fwk_string.h>

#include <string.h>

#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
#    include <mod_resource_perms.h>
#endif

/*!
 * \brief SCMI Telemetry context structure.
 *
 * This structure holds the context for SCMI telemetry, including configuration,
 * APIs, and various operational parameters.
 */
struct scmi_telemetry_context {
    /*! SCMI Telemetry Configuration */
    const struct mod_scmi_telemetry_config *config;

    /*! Pointer to the bound SCMI protocol API */
    const struct mod_scmi_from_protocol_api *scmi_api;

    /*! Pointer to the telemetry protocol support API */
    const struct mod_telemetry_protocol_support_api *telemetry_api;

    /*! Flag to indicate if telemetry is enabled */
    bool telemetry_enable;

    /*! Current telemetry sampling rate */
    uint32_t current_sampling_rate;

    /*! Pointer to the table storing telemetry agents */
    struct mod_scmi_telemetry_agent *agent_table;

    /*! Total enabled Data Events */
    uint32_t total_de_enabled_count;

    /*! List of data events enabled. */
    telemetry_de_handle_st *de_enabled_list;

#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
    /*! Pointer to the SCMI telemetry permissions API */
    const struct mod_scmi_telemetry_permissions_api *perms_api;
#endif
};

/*!
 * Function prototypes for handling various SCMI telemetry protocol commands.
 * Each function processes a specific SCMI message type and responds
 * accordingly.
 */
static int protocol_version_handler(
    fwk_id_t service_id,
    const uint32_t *payload); /*!< Handles SCMI Protocol Version request */

static int protocol_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload); /*!< Handles SCMI Protocol Attributes request */

static int protocol_message_attributes_handler(
    fwk_id_t service_id,
    const uint32_t
        *payload); /*!< Handles SCMI Protocol Message Attributes request */

static int scmi_telemetry_list_shmti_handler(
    fwk_id_t service_id,
    const uint32_t *payload); /*!< Handles retrieval of SHMTI list */

static int scmi_telemetry_de_description_handler(
    fwk_id_t service_id,
    const uint32_t
        *payload); /*!< Handles retrieval of Data Event (DE) description */

static int scmi_telemetry_list_update_intervals_handler(
    fwk_id_t service_id,
    const uint32_t
        *payload); /*!< Handles retrieval of available update intervals */

static int scmi_telemetry_de_configure_handler(
    fwk_id_t service_id,
    const uint32_t
        *payload); /*!< Handles configuration of a specific Data Event (DE) */

static int scmi_telemetry_de_enabled_list_handler(
    fwk_id_t service_id,
    const uint32_t *payload); /*!< Retrieves the list of enabled Data Events */

static int scmi_telemetry_config_set_handler(
    fwk_id_t service_id,
    const uint32_t *payload); /*!< Handles setting telemetry configuration */

static int scmi_telemetry_config_get_handler(
    fwk_id_t service_id,
    const uint32_t
        *payload); /*!< Handles retrieval of current telemetry configuration */

static int scmi_telemetry_reset_handler(
    fwk_id_t service_id,
    const uint32_t *payload); /*!< Resets the telemetry infrastructure. */

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
static int scmi_telemetry_notify_handler(
    fwk_id_t service_id,
    const uint32_t *payload); /*!< Handles sending telemetry notifications */
#endif

/*!
 * Internal variables
 * These store context and lookup tables for handling telemetry-related
 * messages.
 */

static struct scmi_telemetry_context
    scmi_telemetry_ctx; /*!< Global context for SCMI telemetry */

/*!
 * Lookup table mapping SCMI message types to their corresponding handlers.
 * This allows efficient dispatch of incoming SCMI messages.
 */
static handler_table_t msg_handler_table[] = {
    [MOD_SCMI_PROTOCOL_VERSION] = protocol_version_handler,
    [MOD_SCMI_PROTOCOL_ATTRIBUTES] = protocol_attributes_handler,
    [MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] =
        protocol_message_attributes_handler,
    [MOD_SCMI_TELEMETRY_LIST_SHMTI] = scmi_telemetry_list_shmti_handler,
    [MOD_SCMI_TELEMETRY_DE_DESCRIPTION] = scmi_telemetry_de_description_handler,
    [MOD_SCMI_TELEMETRY_LIST_UPDATE_INTERVALS] =
        scmi_telemetry_list_update_intervals_handler,
    [MOD_SCMI_TELEMETRY_DE_CONFIGURE] = scmi_telemetry_de_configure_handler,
    [MOD_SCMI_TELEMETRY_DE_ENABLED_LIST] =
        scmi_telemetry_de_enabled_list_handler,
    [MOD_SCMI_TELEMETRY_CONFIG_SET] = scmi_telemetry_config_set_handler,
    [MOD_SCMI_TELEMETRY_CONFIG_GET] = scmi_telemetry_config_get_handler,
    [MOD_SCMI_TELEMETRY_RESET] = scmi_telemetry_reset_handler,
};

/*!
 * Lookup table defining expected payload sizes for each SCMI message type.
 * Used to validate incoming SCMI messages.
 */
static size_t payload_size_table[] = {
    [MOD_SCMI_PROTOCOL_VERSION] = 0, /*!< No payload for version request */
    [MOD_SCMI_PROTOCOL_ATTRIBUTES] =
        0, /*!< No payload for attributes request */
    [MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] =
        (unsigned int)sizeof(struct scmi_protocol_message_attributes_a2p),
    [MOD_SCMI_TELEMETRY_LIST_SHMTI] = (unsigned int)sizeof(uint32_t),
    [MOD_SCMI_TELEMETRY_DE_DESCRIPTION] = (unsigned int)sizeof(uint32_t),
    [MOD_SCMI_TELEMETRY_LIST_UPDATE_INTERVALS] =
        (unsigned int)sizeof(struct scmi_telemetry_list_update_intervals_a2p),
    [MOD_SCMI_TELEMETRY_DE_CONFIGURE] =
        (unsigned int)sizeof(struct scmi_telemetry_de_configure_a2p),
    [MOD_SCMI_TELEMETRY_DE_ENABLED_LIST] =
        (unsigned int)sizeof(struct scmi_telemetry_de_enabled_list_a2p),
    [MOD_SCMI_TELEMETRY_CONFIG_SET] =
        (unsigned int)sizeof(struct scmi_telemetry_config_set_a2p),
    [MOD_SCMI_TELEMETRY_CONFIG_GET] =
        (unsigned int)sizeof(struct scmi_telemetry_config_get_a2p),
    [MOD_SCMI_TELEMETRY_RESET] =
        (unsigned int)sizeof(struct scmi_telemetry_reset_a2p),
};

/* Ensure the message and payload size tables are consistent */
static_assert(
    FWK_ARRAY_SIZE(msg_handler_table) == FWK_ARRAY_SIZE(payload_size_table),
    "[SCMI] telemetry protocol table sizes are not consistent");

/*!
 * SCMI Telemetry protocol implementation
 */

static int protocol_version_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    struct scmi_protocol_version_p2a outmsg = {
        .status = (int32_t)SCMI_SUCCESS,
        .version = SCMI_PROTOCOL_VERSION_TELEMETRY,
    };

    return scmi_telemetry_ctx.scmi_api->respond(
        service_id, &outmsg, sizeof(outmsg));
}

static int protocol_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    struct scmi_telemetry_protocol_attributes_p2a outmsg = {
        .status = (int32_t)SCMI_SUCCESS,
        .de_impl_rev_dword = { 0 },
    };

    int status;
    unsigned int agent_id;

    /* Retrieve agent ID associated with the service */
    status = scmi_telemetry_ctx.scmi_api->get_agent_id(service_id, &agent_id);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Validate agent ID */
    if (agent_id >= scmi_telemetry_ctx.config->agent_count) {
        return FWK_E_PARAM;
    }

    /* Retrieve the total number of Data Events (DEs) */
    status = scmi_telemetry_ctx.telemetry_api->get_num_de(&outmsg.num_de);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Assign number of groups. */
    outmsg.num_groups = 0; // Groups are not supported yet.

    /* Assign protocol attributes */
    outmsg.attributes = SCMI_TELEMETRY_PROTOCOL_ATTRIBUTES(
        scmi_telemetry_ctx.config->single_sample_async_read_support,
        scmi_telemetry_ctx.config->continuous_update_notification_support,
        scmi_telemetry_ctx.config->event_group_sampling_config_support,
        scmi_telemetry_ctx.config->telemetry_reset_support,
        scmi_telemetry_ctx.config->fastchannel_support,
        scmi_telemetry_ctx.config->shmti_count);

    /* Respond to the request */
    return scmi_telemetry_ctx.scmi_api->respond(
        service_id, &outmsg, sizeof(outmsg));
}

static int protocol_message_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    struct scmi_protocol_message_attributes_p2a outmsg = {
        .status = (int32_t)SCMI_NOT_FOUND,
    };

    if (payload == NULL) {
        return FWK_E_PARAM;
    }

    size_t outmsg_size = sizeof(outmsg.status);
    struct scmi_protocol_message_attributes_a2p params;

    /* Copy payload data */
    params = *(const struct scmi_protocol_message_attributes_a2p *)payload;

    /* Validate message ID and check if a handler exists */
    if (msg_handler_table[params.message_id] != NULL) {
        outmsg.status = (int32_t)SCMI_SUCCESS;
        outmsg_size = sizeof(outmsg);
    }

    /* Respond with the attributes */
    return scmi_telemetry_ctx.scmi_api->respond(
        service_id, &outmsg, outmsg_size);
}

#ifdef RESOURCE_PERMISSION
/*!
 * \brief Retrieves the agent identifier associated with a given service.
 *
 * \param[in] service_id The SCMI service identifier.
 * \param[out] agent_id Pointer to store the retrieved agent ID.
 *
 * \retval ::FWK_SUCCESS The operation was successful.
 * \retval ::FWK_E_PARAM Invalid agent ID or service ID.
 * \return One of the standard framework error codes.
 */
static int get_agent_id(fwk_id_t service_id, unsigned int *agent_id)
{
    int status;

    if (agent_id == NULL) {
        return FWK_E_PARAM;
    }

    status = scmi_telemetry_ctx.scmi_api->get_agent_id(service_id, agent_id);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (*agent_id >= scmi_telemetry_ctx.config->agent_count) {
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

/*
 * \brief Retrieves the agent entry from the agent table based on the service
 *   ID.
 *
 * \param[in] service_id The SCMI service identifier.
 * \param[out] agent Pointer to store the retrieved agent entry.
 *
 * \retval ::FWK_SUCCESS The operation was successful.
 * \retval ::FWK_E_PARAM Invalid service ID or agent entry.
 * \return One of the standard framework error codes.
 */
static int get_agent_entry(
    fwk_id_t service_id,
    const struct mod_scmi_telemetry_agent **agent)
{
    int status;
    unsigned int agent_id;

    if (agent == NULL) {
        return FWK_E_PARAM;
    }

    status = get_agent_id(service_id, &agent_id);
    if (status != FWK_SUCCESS) {
        return status;
    }

    *agent = &scmi_telemetry_ctx.config->agent_table[agent_id];

    return FWK_SUCCESS;
}

/*!
 * \brief Handles the request policy for SCMI telemetry.
 *
 * This function defines the default policy behavior for handling telemetry
 * requests. It sets the policy status to execute the message handler.
 *
 * \param[out] policy_status The policy status to be executed.
 * \param[out] mode Pointer to store the telemetry mode (optional).
 * \param[out] other_params Pointer to store any other param (optional).
 * \param[in] agent_id The agent ID requesting the reset.
 * \param[in] domain_id The domain ID for which the reset is requested.
 *
 * \retval ::FWK_SUCCESS The operation was successful.
 * \retval ::FWK_E_PARAM Invalid policy_status param.
 */
FWK_WEAK int scmi_telemetry_reset_request_policy(
    enum mod_scmi_telemetry_policy_status *policy_status,
    enum mod_telemetry_mode *mode,
    uint32_t *other_params,
    uint32_t agent_id,
    uint32_t domain_id)
{
    if (policy_status == NULL) {
        return FWK_E_PARAM;
    }

    *policy_status = MOD_SCMI_TELEMETRY_EXECUTE_MESSAGE_HANDLER;

    return FWK_SUCCESS;
}
#endif

/*!
 * Data event configuration handlers.
 *
 */

/*!
 * \brief Determines the maximum number of objects that can fit in a payload.
 *
 * This function calculates how many objects can be included in a single SCMI
 * telemetry response payload, given the maximum payload size.
 *
 * \param[in] service_id SCMI service identifier.
 * \param[in] header_size Size of the response header.
 * \param[in] object_size Size of each individual object.
 * \param[in, out] remain_count Pointer to the remaining number of objects.
 *      Updated to reflect the number of objects left after processing.
 * \param[out] total_count Pointer to store the computed number of objects
 *      that can be included in the response payload.
 *
 * \retval ::FWK_SUCCESS If the operation is successful.
 * \retval ::FWK_E_RANGE If there are no remaining objects to process.
 * \retval ::FWK_E_SIZE If the payload size is too small to fit any objects.
 * \retval ::FWK_E_PARAM If an invalid parameter is provided.
 */
static int max_objects_in_payload(
    fwk_id_t service_id,
    size_t header_size,
    size_t object_size,
    uint32_t *remain_count,
    uint32_t *total_count)
{
    int status;
    size_t max_payload_size;
    uint32_t total_remaining = 0;
    uint32_t count = 0;

    /* Ensure valid output parameters */
    if (remain_count == NULL || total_count == NULL || object_size == 0) {
        return FWK_E_PARAM;
    }

    *total_count = 0;
    total_remaining = *remain_count;

    /* Check if there are objects left to process */
    if (total_remaining == 0) {
        return FWK_E_RANGE;
    }

    /*
     * Get the maximum payload size to determine how many entries can
     * be returned in one response.
     */
    status = scmi_telemetry_ctx.scmi_api->get_max_payload_size(
        service_id, &max_payload_size);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Ensure payload can accommodate at least the header */
    if (max_payload_size < header_size) {
        return FWK_E_SIZE;
    }

    max_payload_size -= header_size;
    count = max_payload_size / object_size;

    /* Ensure count does not exceed remaining objects */
    count = (uint32_t)FWK_MIN(count, total_remaining);

    /* If count is zero, the payload size is too small */
    if (count == 0) {
        return FWK_E_SIZE;
    }

    /* Update the remaining count and total count */
    *remain_count = total_remaining - count;
    *total_count = count;

    return FWK_SUCCESS;
}

/*!
 * \brief Handles retrieval of SHMTI list for SCMI Telemetry.
 *
 * This function processes the request to retrieve SHMTI descriptors available
 * in the telemetry system. It ensures correct indexing, payload handling, and
 * response formatting.
 *
 * \param[in] service_id The SCMI service identifier.
 * \param[in] payload Pointer to the request payload containing the index.
 *
 * \retval ::FWK_SUCCESS The operation was successful.
 * \retval ::FWK_E_PARAM If an invalid parameter is provided.
 * \return One of the standard framework error codes.
 */
static int scmi_telemetry_list_shmti_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    int status;
    unsigned int i;
    unsigned int agent_idx;
    uint32_t index;
    uint32_t count, remain_count;
    struct scmi_telemetry_shmti_desc shmti_desc;
    const struct mod_telemetry_shmti_info *shmti_info;
    const struct mod_scmi_telemetry_config *config = scmi_telemetry_ctx.config;
    const struct mod_scmi_telemetry_agent_shmti_config *agent_shmtis_cfg;
    uint64_t agent_start_addr;
    int respond_status;
    uint32_t payload_size;

    struct scmi_telemetry_list_shmti_p2a return_values = {
        .status = SCMI_GENERIC_ERROR
    };
    payload_size = sizeof(return_values);

    /* Retrieve agent ID associated with the service */
    status = scmi_telemetry_ctx.scmi_api->get_agent_id(service_id, &agent_idx);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Validate payload pointer */
    if (payload == NULL) {
        return FWK_E_PARAM;
    }

    index = *payload;

    /* Validate the requested index */
    if (index >= config->agent_table[agent_idx].shmti_count) {
        return_values.status = SCMI_NOT_FOUND;
        goto exit;
    }

    agent_shmtis_cfg = config->agent_table[agent_idx].shmtis;
    remain_count = config->agent_table[agent_idx].shmti_count - index;

    /* Determine the maximum number of entries that can fit in the response
     * payload */
    status = max_objects_in_payload(
        service_id,
        sizeof(struct scmi_telemetry_list_shmti_p2a),
        sizeof(struct scmi_telemetry_shmti_desc),
        &remain_count,
        &count);

    if (status != FWK_SUCCESS) {
        goto exit;
    }

    /* Iterate through the SHMTI entries and add them to the response */
    for (i = 0; i < count; i++, index++) {
        /* SHMTI ID = index. */
        shmti_info = &config->shmti_list[agent_shmtis_cfg[index].shmti_id];
        agent_start_addr = agent_shmtis_cfg[index].start_addr;
        shmti_desc.shmti_id = shmti_info->shmti_id;
        shmti_desc.addr_lo = (uint32_t)agent_start_addr;
        shmti_desc.addr_hi = (uint32_t)(((uint64_t)agent_start_addr) >> 32);
        shmti_desc.length = shmti_info->length;

        status = scmi_telemetry_ctx.scmi_api->write_payload(
            service_id, payload_size, &shmti_desc, sizeof(shmti_desc));
        if (status != FWK_SUCCESS) {
            goto exit;
        }

        payload_size += sizeof(shmti_desc);
    }

    return_values.status = SCMI_SUCCESS;
    return_values.num_shmti =
        SCMI_TELEMETRY_LIST_SHMTI_TOTAL_SHMTI(remain_count, count);

    /* Write the final response data */
    status = scmi_telemetry_ctx.scmi_api->write_payload(
        service_id, 0, &return_values, sizeof(return_values));

exit:
    /* Send response, handling both success and error cases */
    respond_status = scmi_telemetry_ctx.scmi_api->respond(
        service_id,
        (return_values.status == SCMI_SUCCESS) ? NULL : &return_values.status,
        (return_values.status == SCMI_SUCCESS) ? payload_size :
                                                 sizeof(return_values.status));

    if (respond_status != FWK_SUCCESS) {
        FWK_LOG_DEBUG(
            "[SCMI-TELEMETRY] Failed response in %s @%d", __func__, __LINE__);
        return respond_status;
    }

    return status;
}

/*!
 * \brief Handles retrieval of Data Event (DE) descriptions for SCMI Telemetry.
 *
 * This function processes a request to retrieve descriptions of telemetry
 * Data Events (DEs). It ensures proper indexing, payload handling, and response
 * formatting.
 *
 * \param[in] service_id The SCMI service identifier.
 * \param[in] payload Pointer to the request payload containing the index.
 *
 * \retval ::FWK_SUCCESS The operation was successful.
 * \retval ::FWK_E_PARAM If an invalid parameter is provided.
 * \return One of the standard framework error codes.
 */
static int scmi_telemetry_de_description_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    size_t i;
    int status, respond_status;
    uint32_t count, remain_count;
    uint32_t num_de, de_desc_index;
    struct mod_telemetry_de_desc de_desc;
    struct mod_telemetry_de_fch_attr de_fch_attr;
    char de_name[SCMI_TELEMETRY_DE_NAME_SIZE];
    struct scmi_telemetry_de_desc_p2a return_values = {
        .status = SCMI_GENERIC_ERROR
    };

    uint32_t payload_size = sizeof(return_values);
    /* Validate payload pointer */
    if (payload == NULL) {
        return FWK_E_PARAM;
    }

    /* Extract Data Event description index */
    de_desc_index = *(const uint32_t *)payload;

    /* Retrieve total number of Data Events */
    status = scmi_telemetry_ctx.telemetry_api->get_num_de(&num_de);
    if (status != FWK_SUCCESS) {
        goto exit;
    }

    /* Validate the requested index */
    if (de_desc_index >= num_de) {
        return_values.status = SCMI_NOT_FOUND;
        goto exit;
    }

    remain_count = num_de - de_desc_index;

    /* Determine the maximum number of entries that can fit in the response
     * payload based on the largest possible data event description.
     */
    status = max_objects_in_payload(
        service_id,
        sizeof(struct scmi_telemetry_de_desc_p2a),
        SCMI_TELEMETRY_DE_DESC_MAX_SIZE,
        &remain_count,
        &count);

    if (status != FWK_SUCCESS) {
        goto exit;
    }

    /* Iterate through the DE descriptors and add them to the response */
    for (i = 0; i < count; i++, de_desc_index++) {
        /* Write Data Event description in payload. */
        status = scmi_telemetry_ctx.telemetry_api->get_de_desc(
            de_desc_index, &de_desc);
        if (status != FWK_SUCCESS) {
            goto exit;
        }

        /* Write mandatory part of the DE Description.*/
        status = scmi_telemetry_ctx.scmi_api->write_payload(
            service_id,
            payload_size,
            &de_desc,
            sizeof(struct mod_telemetry_de_desc));
        if (status != FWK_SUCCESS) {
            goto exit;
        }
        payload_size += sizeof(struct mod_telemetry_de_desc);

        /* Write Data Event FastChannel description if applicable. */
        if (de_desc.attributes[0] & SCMI_TELEMETRY_DE_FCH_MASK) {
            status = scmi_telemetry_ctx.telemetry_api->get_de_fch_desc(
                de_desc_index, &de_fch_attr);
            if (status != FWK_SUCCESS) {
                goto exit;
            }

            status = scmi_telemetry_ctx.scmi_api->write_payload(
                service_id,
                payload_size,
                &de_fch_attr,
                sizeof(struct mod_telemetry_de_fch_attr));
            if (status != FWK_SUCCESS) {
                goto exit;
            }
            payload_size += sizeof(struct mod_telemetry_de_fch_attr);
        }

        /* Write Data Event name if applicable. */
        if (de_desc.attributes[0] & SCMI_TELEMETRY_DE_NAME_MASK) {
            status = scmi_telemetry_ctx.telemetry_api->get_de_name(
                de_desc_index, de_name);
            if (status != FWK_SUCCESS) {
                goto exit;
            }

            status = scmi_telemetry_ctx.scmi_api->write_payload(
                service_id, payload_size, &de_name, sizeof(de_name));
            if (status != FWK_SUCCESS) {
                goto exit;
            }
            payload_size += sizeof(de_name);
        }
    }

    /* Populate response values */
    return_values.status = SCMI_SUCCESS;
    return_values.num_desc = SCMI_TELEMETRY_DE_DESC_TOTAL(remain_count, count);

    /* Write the final response data */
    status = scmi_telemetry_ctx.scmi_api->write_payload(
        service_id, 0, &return_values, sizeof(return_values));

exit:
    /* Send response, handling both success and error cases */
    respond_status = scmi_telemetry_ctx.scmi_api->respond(
        service_id,
        (return_values.status == SCMI_SUCCESS) ? NULL : &return_values.status,
        (return_values.status == SCMI_SUCCESS) ? payload_size :
                                                 sizeof(return_values.status));

    if (respond_status != FWK_SUCCESS) {
        FWK_LOG_DEBUG(
            "[SCMI-TELEMETRY] Response failed in %s @%d", __func__, __LINE__);
        return respond_status;
    }

    return status;
}

/*!
 * \brief Encodes an update interval in SCMI telemetry format.
 *
 * This function encodes a telemetry sampling interval (e.g. in milliseconds)
 * into the SCMI-defined format for update intervals. The encoded interval
 * consists of:
 *   - A number of seconds (left-shifted into its field position),
 *   - An exponent value (bit-masked into its field).
 *
 * The format complies with the SCMI specification for telemetry update
 * intervals.
 *
 * \param[in] seconds_base Sampling interval e.g. in milliseconds.
 * \param[in] exponent Exponent value (base-10), typically negative (e.g., -3).
 *     This determines the resolution of the interval.
 *
 * \return Encoded interval value combining seconds and exponent.
 * \return 0 If the seconds value exceeds the allowed bit field range.
 */
static uint32_t encode_interval(uint32_t seconds_base, int8_t exponent)
{
    uint32_t seconds_field;
    uint32_t exponent_field;

    /* Shift seconds to proper bit position */
    seconds_field = seconds_base << SCMI_TELEMETRY_SAMPLING_RATE_SECONDS_POS;

    /* Validate seconds field fits within defined mask */
    if (seconds_field & ~(SCMI_TELEMETRY_SAMPLING_RATE_SEC_MASK)) {
        return 0; /* Indicate error */
    }

    /*
     * Cast exponent to unsigned and apply bitmask.
     * Assumes two's complement representation.
     */
    exponent_field =
        ((uint32_t)exponent) & SCMI_TELEMETRY_SAMPLING_RATE_EXP_MASK;

    /* Combine the shifted seconds and masked exponent */
    return seconds_field | exponent_field;
}

/*!
 * \brief Handles retrieval of available update intervals for SCMI Telemetry.
 *
 * This function processes a request to retrieve a list of available update
 * intervals for telemetry data refresh. It ensures correct indexing, payload
 * handling, and response formatting.
 *
 * \param[in] service_id The SCMI service identifier.
 * \param[in] payload Pointer to the request payload containing the index.
 *
 * \retval ::FWK_SUCCESS The operation was successful.
 * \retval ::FWK_E_PARAM If an invalid parameter is provided.
 * \return One of the standard framework error codes.
 */
static int scmi_telemetry_list_update_intervals_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    size_t i;
    uint32_t num_intervals;
    /* Index received from the agent as part of the payload */
    uint32_t index, count, remain_count;
    uint8_t selector;

    uint32_t sampling_rate;
    uint32_t interval;
    enum mod_telemetry_update_interval_formats interval_format;

    struct scmi_telemetry_list_update_intervals_a2p *params = NULL;
    struct scmi_telemetry_list_update_intervals_p2a return_values = {
        .status = SCMI_GENERIC_ERROR
    };

    uint32_t payload_size = sizeof(return_values);
    int status = FWK_SUCCESS;
    int respond_status;

    /* Validate payload pointer */
    if (payload == NULL) {
        return FWK_E_PARAM;
    }

    params = (struct scmi_telemetry_list_update_intervals_a2p *)payload;
    index = params->index;
    selector =
        SCMI_TELEMETRY_UPDATE_INTERVALS_GET_GROUP_ID_SELECTOR(params->flags);

    /* Group ID not supported. */
    if (selector ==
        SCMI_TELEMETRY_UPDATE_INTERVALS_GROUP_ID_SELECTOR_EVENT_GROUP) {
        status = FWK_E_PARAM;
        return_values.status = SCMI_NOT_SUPPORTED;
        goto exit;
    }

    /* Retrieve available update interval information */
    status = scmi_telemetry_ctx.telemetry_api->get_update_intervals_info(
        &num_intervals, &interval_format);
    if (status != FWK_SUCCESS) {
        goto exit;
    }

    /* Validate the requested index */
    if (index >= num_intervals) {
        return_values.status = SCMI_NOT_FOUND;
        goto exit;
    }

    remain_count = num_intervals - index;

    /*!
     * Determine the maximum number of entries that can fit in the response
     * payload
     */
    status = max_objects_in_payload(
        service_id,
        sizeof(struct scmi_telemetry_list_update_intervals_p2a),
        sizeof(uint32_t),
        &remain_count,
        &count);
    if (status != FWK_SUCCESS) {
        goto exit;
    }

    /*!
     * Iterate through the available update intervals and add them to the
     * response
     */
    for (i = 0; i < count; i++, index++) {
        status = scmi_telemetry_ctx.telemetry_api->get_update_interval(
            index, &sampling_rate);

        if (status != FWK_SUCCESS) {
            goto exit;
        };

        interval = encode_interval(sampling_rate, -3);

        if (interval == 0) {
            return_values.status = SCMI_OUT_OF_RANGE;
            goto exit;
        }

        status = scmi_telemetry_ctx.scmi_api->write_payload(
            service_id, payload_size, &interval, sizeof(interval));
        if (status != FWK_SUCCESS) {
            goto exit;
        }
        payload_size += sizeof(interval);
    }

    /* Populate response values */
    return_values.status = (int32_t)SCMI_SUCCESS;
    return_values.flags = SCMI_TELEMETRY_UPDATE_INTERVALS_FLAG(
        remain_count, interval_format, count);

    /* Write the final response data */
    status = scmi_telemetry_ctx.scmi_api->write_payload(
        service_id, 0, &return_values, sizeof(return_values));

exit:
    /* Send response, handling both success and error cases */
    respond_status = scmi_telemetry_ctx.scmi_api->respond(
        service_id,
        (return_values.status == SCMI_SUCCESS) ? NULL : &return_values.status,
        (return_values.status == SCMI_SUCCESS) ? payload_size :
                                                 sizeof(return_values.status));

    if (respond_status != FWK_SUCCESS) {
        FWK_LOG_DEBUG(
            "[SCMI-TELEMETRY] Response failed in %s @%d", __func__, __LINE__);
        return respond_status;
    }

    return status;
}

/*!
 * \brief Finds the DE handle from the enabled list.
 *
 * \param[in] scmi_de_id SCMI DE_ID for the requested data event.
 * \param[out] de_handle Data Event handle found in the enabled de list.
 * \param[out] enabled_de_index Index of the Data Event in the enabled de list.
 */
static int find_enabled_de(
    uint32_t scmi_de_id,
    telemetry_de_handle_st *de_handle,
    size_t *enabled_de_index)
{
    int status;
    size_t i;
    struct mod_telemetry_de_status de_status;
    telemetry_de_handle_st curr_de;

    for (i = 0; i < scmi_telemetry_ctx.total_de_enabled_count; ++i) {
        curr_de = scmi_telemetry_ctx.de_enabled_list[i];
        status = scmi_telemetry_ctx.telemetry_api->get_enabled_de_status(
            curr_de, &de_status);
        if (status == FWK_SUCCESS && scmi_de_id == de_status.de_id) {
            *de_handle = curr_de;
            *enabled_de_index = i;
            return FWK_SUCCESS;
        }
    }

    return FWK_E_PARAM;
}

/*!
 * \brief Enables new DE and add it to the enabled DE list.
 *
 * \param[in] scmi_de_id SCMI DE_ID for the requested data event.
 * \param[in] enable_ts Flag to indicate if DE needs to be enabled with
 *            timestamp.
 * \param[out] shmti_id SHMTI ID containing the given Data Event.
 * \param[out] shmti_de_offset Byte offset from the start of the given shmti.
 */
static int de_configure_enable_data_event(
    unsigned int agent_id,
    uint32_t scmi_de_id,
    bool enable_ts,
    uint32_t *shmti_id,
    uint32_t *shmti_de_offset)
{
    int status;
    unsigned int shmti_idx;
    size_t de_index;
    telemetry_de_handle_st de_handle;
    uint32_t curr_shmti_id = 0;
    uint32_t available_shmti_count;
    const struct mod_scmi_telemetry_agent_shmti_config *agent_shmtis;

    if (scmi_telemetry_ctx.total_de_enabled_count ==
        scmi_telemetry_ctx.config->max_enabled_de_count) {
        return FWK_E_PARAM;
    }

    status = find_enabled_de(scmi_de_id, &de_handle, &de_index);
    if (status == FWK_SUCCESS) {
        /* DE is already enabled. */
        return FWK_E_PARAM;
    }

    available_shmti_count =
        scmi_telemetry_ctx.config->agent_table[agent_id].shmti_count;
    agent_shmtis = scmi_telemetry_ctx.config->agent_table[agent_id].shmtis;
    for (shmti_idx = 0; shmti_idx < available_shmti_count; ++shmti_idx) {
        curr_shmti_id = agent_shmtis[shmti_idx].shmti_id;
        if (enable_ts) {
            status = scmi_telemetry_ctx.telemetry_api->enable_de_ts(
                scmi_de_id, curr_shmti_id, &de_handle, shmti_de_offset);
        } else {
            status = scmi_telemetry_ctx.telemetry_api->enable_de_non_ts(
                scmi_de_id, curr_shmti_id, &de_handle, shmti_de_offset);
        }

        if (status != FWK_E_NOMEM) {
            break;
        }
    }

    if (status != FWK_SUCCESS) {
        return status;
    }

    *shmti_id = curr_shmti_id;
    /* Add the enabled Data event to enabled DE list */
    scmi_telemetry_ctx
        .de_enabled_list[scmi_telemetry_ctx.total_de_enabled_count++] =
        de_handle;

    return FWK_SUCCESS;
}

/*!
 * \brief Disables data event and remove the it from the enabled DE list.
 *
 * \param[in] scmi_de_id SCMI DE_ID for the requested data event.
 *
 */
static int de_configure_disable_data_event(uint32_t scmi_de_id)
{
    int status;
    size_t de_index;
    telemetry_de_handle_st de_handle;

    if (scmi_telemetry_ctx.total_de_enabled_count == 0) {
        return FWK_E_PARAM;
    }

    status = find_enabled_de(scmi_de_id, &de_handle, &de_index);
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = scmi_telemetry_ctx.telemetry_api->disable_de(de_handle);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /*!
     * Shift elements left to fill the removed entry.
     * This maintains list continuity while removing the DE index.
     * The loop runs until the second-last element since the last one
     * is already shifted properly.
     */
    for (; de_index < scmi_telemetry_ctx.total_de_enabled_count - 1;
         de_index++) {
        scmi_telemetry_ctx.de_enabled_list[de_index] =
            scmi_telemetry_ctx.de_enabled_list[de_index + 1];
    }
    /* Decrement the enabled DE count */
    scmi_telemetry_ctx.total_de_enabled_count--;

    return FWK_SUCCESS;
}

/*!
 * \brief Disables all enabled data events.
 */
static int de_configure_disable_all_data_events(void)
{
    int status = FWK_SUCCESS;
    unsigned int de_enabled_itr;

    if (scmi_telemetry_ctx.total_de_enabled_count == 0) {
        return FWK_E_PARAM;
    }

    for (de_enabled_itr = scmi_telemetry_ctx.total_de_enabled_count;
         de_enabled_itr > 0u;
         --de_enabled_itr) {
        status = scmi_telemetry_ctx.telemetry_api->disable_de(
            scmi_telemetry_ctx.de_enabled_list[de_enabled_itr - 1]);
        if (status != FWK_SUCCESS) {
            break;
        }
    }
    scmi_telemetry_ctx.total_de_enabled_count = de_enabled_itr;

    return status;
}

/*!
 * \brief Handles the configuration of a specific Data Event (DE) for SCMI
 * Telemetry.
 *
 * This function processes a request to enable or disable a telemetry Data Event
 * (DE). It ensures proper validation of the DE ID and flags, applies the
 * required configuration, and formats the response accordingly.
 *
 * \param[in] service_id The SCMI service identifier.
 * \param[in] payload Pointer to the request payload containing DE
 *     configuration.
 *
 * \retval ::FWK_SUCCESS The operation was successful.
 * \retval ::FWK_E_PARAM If an invalid parameter is detected.
 * \return One of the standard framework error codes.
 */
static int scmi_telemetry_de_configure_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    int status;
    int respond_status;
    unsigned int agent_id;
    uint32_t de_id;
    uint32_t flags;
    uint32_t response_size;
    uint8_t selector;
    uint32_t shmti_id = SCMI_TELEMETRY_DE_SHMTI_ID_NOT_SUPPORTED;
    uint32_t shmti_de_offset = SCMI_TELEMETRY_DE_SHMTI_ID_OFFSET_NOT_SUPPORTED;
    struct scmi_telemetry_de_configure_a2p *params = NULL;
    struct scmi_telemetry_de_configure_p2a return_values = {
        .status = SCMI_GENERIC_ERROR,
    };

    /* Retrieve agent ID associated with the service */
    status = scmi_telemetry_ctx.scmi_api->get_agent_id(service_id, &agent_id);
    if (status != FWK_SUCCESS) {
        return status;
    }
    /* Validate agent ID */
    if (agent_id >= scmi_telemetry_ctx.config->agent_count) {
        return FWK_E_PARAM;
    }

    /* Validate payload pointer */
    if (payload == NULL) {
        return FWK_E_PARAM;
    }
    /* Parse request parameters */
    params = (struct scmi_telemetry_de_configure_a2p *)payload;
    de_id = params->id;
    flags = params->flags;
    selector = SCMI_TELEMETRY_DE_CONFIGURE_ID_SELECTOR(flags);
    /* Event Group Selector is not supported.*/
    if (selector == SCMI_TELEMETRY_DE_CONFIGURE_ID_SELECTOR_EVENT_GROUP) {
        status = FWK_E_PARAM;
        return_values.status = SCMI_NOT_SUPPORTED;
        goto exit;
    }

    /* Check if all DEs should be disabled */
    if (SCMI_TELEMETRY_ALL_DE_DISABLED(flags)) {
        if (SCMI_TELEMETRY_DE_CONFIGURE_DE_MODE(flags) != 0) {
            status = FWK_E_PARAM;
            return_values.status = SCMI_INVALID_PARAMETERS;
            goto exit;
        }

        status = de_configure_disable_all_data_events();
        return_values.status =
            (status == FWK_SUCCESS) ? SCMI_SUCCESS : SCMI_GENERIC_ERROR;
        goto exit;
    }

    /* Process DE configuration mode */
    switch (SCMI_TELEMETRY_DE_CONFIGURE_DE_MODE(flags)) {
    case SCMI_TELEMETRY_DE_DISABLE:
        status = de_configure_disable_data_event(de_id);
        break;
    case SCMI_TELEMETRY_DE_ENABLE_NON_TS:
        status = de_configure_enable_data_event(
            agent_id, de_id, false, &shmti_id, &shmti_de_offset);
        break;
    case SCMI_TELEMETRY_DE_ENABLE_TS:
        status = de_configure_enable_data_event(
            agent_id, de_id, true, &shmti_id, &shmti_de_offset);
        break;
    default:
        status = FWK_E_PARAM;
        break;
    }
    return_values.shmti_id = shmti_id;
    return_values.shmti_de_offset = shmti_de_offset;

    if (status == FWK_SUCCESS) {
        return_values.status = SCMI_SUCCESS;
    } else if (status == FWK_E_SUPPORT) {
        return_values.status = SCMI_NOT_SUPPORTED;
    } else if (status == FWK_E_PARAM) {
        return_values.status = SCMI_INVALID_PARAMETERS;
    }
exit:
    /* Determine response size */
    response_size = (return_values.status == SCMI_SUCCESS) ?
        sizeof(return_values) :
        sizeof(return_values.status);

    /* Send response */
    respond_status = scmi_telemetry_ctx.scmi_api->respond(
        service_id, &return_values, response_size);

    /* Log response failure, if any */
    if (respond_status != FWK_SUCCESS) {
        FWK_LOG_DEBUG(
            "[SCMI-TELEMETRY] Response failed in %s @%d", __func__, __LINE__);
        return respond_status;
    }

    return status;
}

/*!
 * \brief Retrieves the list of enabled Data Events (DEs) for SCMI Telemetry.
 *
 * This function processes a request to fetch the list of currently enabled DEs.
 * It ensures proper indexing, payload handling, and response formatting.
 *
 * \param[in] service_id The SCMI service identifier.
 * \param[in] payload Pointer to the request payload containing the index.
 *
 * \retval ::FWK_SUCCESS The operation was successful.
 * \retval ::FWK_E_PARAM If an invalid parameter is provided.
 * \return One of the standard framework error codes.
 */
static int scmi_telemetry_de_enabled_list_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    int status = FWK_SUCCESS;
    size_t i;
    telemetry_de_handle_st de_handle;
    uint32_t start_index, count, remain_count;
    uint8_t selector;
    struct mod_telemetry_de_status de_status;
    struct scmi_telemetry_de_enabled_list_a2p *params = NULL;
    struct scmi_telemetry_de_enabled_list_p2a return_values = {
        .status = SCMI_GENERIC_ERROR
    };

    uint32_t de_enabled_value[2];

    uint32_t payload_size = sizeof(return_values);
    int respond_status;

    /* Validate payload pointer */
    if (payload == NULL) {
        return FWK_E_PARAM;
    }

    /* Parse request parameters */
    params = (struct scmi_telemetry_de_enabled_list_a2p *)payload;

    start_index = params->index;
    selector = SCMI_TELEMETRY_DE_ENABLED_LIST_ID_SELECTOR(params->flags);

    /* Event Group Selector is not supported.*/
    if (selector == SCMI_TELEMETRY_DE_ENABLED_LIST_ID_SELECTOR_EVENT_GROUP) {
        status = FWK_E_PARAM;
        return_values.status = SCMI_NOT_SUPPORTED;
        goto exit;
    }

    /* Validate the requested first enabled event index */
    if (start_index >= scmi_telemetry_ctx.total_de_enabled_count) {
        return_values.status = SCMI_NOT_FOUND;
        goto exit;
    }

    remain_count = scmi_telemetry_ctx.total_de_enabled_count - start_index;

    /*!
     * Determine the maximum number of entries that can fit in the response
     * payload
     */
    status = max_objects_in_payload(
        service_id,
        sizeof(struct scmi_telemetry_de_enabled_list_p2a),
        sizeof(de_enabled_value),
        &remain_count,
        &count);
    if (status != FWK_SUCCESS) {
        goto exit;
    }

    /* Retrieve the list of enabled DEs */
    for (i = 0; i < count; ++i) {
        de_handle = scmi_telemetry_ctx.de_enabled_list[start_index + i];
        status = scmi_telemetry_ctx.telemetry_api->get_enabled_de_status(
            de_handle, &de_status);
        if (status != FWK_SUCCESS) {
            goto exit;
        }
        de_enabled_value[0] = de_status.de_id;
        de_enabled_value[1] = de_status.ts_enabled ?
            SCMI_TELEMETRY_DE_TS_ENABLED :
            SCMI_TELEMETRY_DE_TS_DISABLED;

        status = scmi_telemetry_ctx.scmi_api->write_payload(
            service_id,
            payload_size,
            &de_enabled_value[0],
            sizeof(de_enabled_value));
        if (status != FWK_SUCCESS) {
            goto exit;
        }
        payload_size += sizeof(de_enabled_value);
    }

    /* Populate response values */
    return_values.status = (int32_t)SCMI_SUCCESS;
    return_values.flags =
        SCMI_TELEMETRY_DE_ENABLED_LIST_FLAG(remain_count, count);

    /* Write the final response data */
    status = scmi_telemetry_ctx.scmi_api->write_payload(
        service_id, 0, &return_values, sizeof(return_values));

exit:
    /* Send response, handling both success and error cases */
    respond_status = scmi_telemetry_ctx.scmi_api->respond(
        service_id,
        (return_values.status == SCMI_SUCCESS) ? NULL : &return_values.status,
        (return_values.status == SCMI_SUCCESS) ? payload_size :
                                                 sizeof(return_values.status));

    if (respond_status != FWK_SUCCESS) {
        FWK_LOG_DEBUG(
            "[SCMI-TELEMETRY] Response failed in %s @%d", __func__, __LINE__);
        return respond_status;
    }

    return status;
}

/*!
 * \brief Retrieves the  multiplier for a given sampling rate.
 *
 * This function extracts the exponent from the sampling rate and adjusts it
 * for conversion. The exponent is incremented by 7 to account for the
 * `sec * 10^7` conversion.
 *
 * \param[in] sampling_rate The raw sampling rate value from SCMI.
 *
 * \return The computed multiplier for converting seconds.
 */
static inline uint64_t decode_sampling_rate_sec_multiplier(
    uint32_t sampling_rate)
{
    int32_t exp_raw =
        (int32_t)((sampling_rate & SCMI_TELEMETRY_SAMPLING_RATE_EXP_MASK) >> SCMI_TELEMETRY_SAMPLING_RATE_EXP_POS);

    if (exp_raw & SCMI_TELEMETRY_SAMPLING_RATE_EXPONENT_SIGN_BIT) {
        exp_raw |=
            ~(SCMI_TELEMETRY_SAMPLING_RATE_EXP_MASK >>
              SCMI_TELEMETRY_SAMPLING_RATE_EXP_POS);
    }

    /* Compute the multiplier using power-of-10 adjusted by 7. */
    return (uint64_t)__builtin_powi(10, exp_raw + 7);
}

/*!
 * \brief Converts a raw SCMI sampling rate to milliseconds.
 *
 * This function extracts the base seconds component and computes the
 * milliseconds equivalent using a multiplier derived from the exponent.
 *
 * \param[in] sampling_rate The raw sampling rate value from SCMI.
 *
 * \return The sampling rate in milliseconds.
 */
uint64_t convert_sampling_rate_to_msecs(uint32_t sampling_rate)
{
    uint32_t seconds;
    uint64_t sec_multiplier;

    /* Get the multiplier */
    sec_multiplier = decode_sampling_rate_sec_multiplier(sampling_rate);

    /* Extract the seconds component from the sampling rate */
    seconds = SCMI_TELEMETRY_SAMPLING_RATE_SECONDS(sampling_rate);

    /* Compute and return the final value in milliseconds
     * Since sec_multiplier is adjusted by 10^7 for msec
     * conversion divide it by 10^4 effectivly multiply
     * by 10^3.
     */
    return (seconds * sec_multiplier) / 10000;
}

/*!
 * \brief Handles setting telemetry configuration for SCMI Telemetry.
 *
 * This function processes a request to enable or disable telemetry,
 * sets the sampling rate, and validates the configuration mode.
 *
 * \param[in] service_id The SCMI service identifier.
 * \param[in] payload Pointer to the request payload containing configuration
 * parameters.
 *
 * \retval ::FWK_SUCCESS The operation was successful.
 * \return One of the standard framework error codes.
 */
static int scmi_telemetry_config_set_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    int status;
    int respond_status;
    uint32_t response_size;
    uint64_t sampling_rate_msec;
    uint8_t selector;

    struct scmi_telemetry_config_set_a2p *params = NULL;
    struct scmi_telemetry_config_set_p2a return_values = {
        .status = SCMI_GENERIC_ERROR,
    };

    /* Validate payload pointer */
    if (payload == NULL) {
        return FWK_E_PARAM;
    }

    /* Parse request parameters */
    params = (struct scmi_telemetry_config_set_a2p *)payload;

    /* Validate ID selector */
    selector = SCMI_TELEMETRY_CONFIG_SET_ID_SELECTOR(params->control);
    if (selector == SCMI_TELEMETRY_CONFIG_CONTROL_ID_SELECTOR_EVENT_GROUP) {
        status = FWK_E_PARAM;
        return_values.status = SCMI_NOT_SUPPORTED;
        goto exit;
    }

    /* Check if telemetry should be disabled */
    if ((SCMI_TELEMETRY_CONFIG_CONTROL_EN_MASK & params->control) == 0) {
        status = scmi_telemetry_ctx.telemetry_api->telemetry_disable();
        if (status != FWK_SUCCESS) {
            return_values.status = SCMI_PROTOCOL_ERROR;
        } else {
            scmi_telemetry_ctx.telemetry_enable = false;
            return_values.status = SCMI_SUCCESS;
        }
        goto exit;
    }

    if (SCMI_TELEMETRY_CONFIG_SET_CONTROL_MODE(params->control) !=
        SCMI_TELEMETRY_CONFIG_CONTROL_MODE_SHMTI) {
        return_values.status = SCMI_NOT_SUPPORTED;
        status = FWK_E_PARAM;
        goto exit;
    }

    /* Enable with requested samping rate. */
    sampling_rate_msec = convert_sampling_rate_to_msecs(params->sampling_rate);
    if (sampling_rate_msec != 0) {
        status = scmi_telemetry_ctx.telemetry_api->set_sampling_rate(
            (uint32_t)sampling_rate_msec);
        if (status != FWK_SUCCESS) {
            return_values.status = SCMI_PROTOCOL_ERROR;
            goto exit;
        }
        scmi_telemetry_ctx.current_sampling_rate = params->sampling_rate;
    }

    /* Enable telemetry */
    status = scmi_telemetry_ctx.telemetry_api->telemetry_enable();
    if (status != FWK_SUCCESS) {
        return_values.status = SCMI_PROTOCOL_ERROR;
    } else {
        scmi_telemetry_ctx.telemetry_enable = true;
        return_values.status = SCMI_SUCCESS;
    }
    return_values.status = SCMI_SUCCESS;

exit:
    /* Determine response size */
    response_size = (return_values.status == SCMI_SUCCESS) ?
        sizeof(return_values) :
        sizeof(return_values.status);

    /* Send response */
    respond_status = scmi_telemetry_ctx.scmi_api->respond(
        service_id, &return_values, response_size);

    /* Log response failure */
    if (respond_status != FWK_SUCCESS) {
        FWK_LOG_DEBUG(
            "[SCMI-TELEMETRY] Response failed in %s @%d", __func__, __LINE__);
        return respond_status;
    }

    return status;
}

/*!
 * \brief Handles retrieval of current telemetry configuration.
 *
 * This function processes a request to fetch the current telemetry
 * configuration. It returns the current telemetry enable state and the active
 * sampling rate.
 *
 * \param[in] service_id The SCMI service identifier.
 * \param[in] payload Pointer to the request payload (unused in this function).
 *
 * \retval ::FWK_SUCCESS The operation was successful.
 * \return One of the standard framework error codes.
 */
static int scmi_telemetry_config_get_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    int status = FWK_SUCCESS;
    int respond_status;
    uint32_t response_size;
    uint8_t selector;
    struct scmi_telemetry_config_get_a2p *params = NULL;
    struct scmi_telemetry_config_get_p2a return_values = {
        .status = SCMI_GENERIC_ERROR,
    };

    /* Validate payload pointer */
    if (payload == NULL) {
        return FWK_E_PARAM;
    }

    /* Parse request parameters */
    params = (struct scmi_telemetry_config_get_a2p *)payload;

    /* Validate ID selector */
    selector = SCMI_TELEMETRY_CONFIG_GET_ID_SELECTOR(params->flags);
    if (selector == SCMI_TELEMETRY_CONFIG_GET_FLAGS_ID_SELECTOR_EVENT_GROUP) {
        status = FWK_E_PARAM;
        return_values.status = SCMI_NOT_SUPPORTED;
        goto exit;
    }

    /* Check if telemetry is enabled and set the response accordingly */
    if (scmi_telemetry_ctx.telemetry_enable) {
        return_values.control = (1U << SCMI_TELEMETRY_CONFIG_CONTROL_EN_POS);
        return_values.sampling_rate = scmi_telemetry_ctx.current_sampling_rate;
    } else {
        return_values.control = 0;
        return_values.sampling_rate = 0;
    }

    return_values.status = SCMI_SUCCESS;
exit:
    /* Determine response size */
    response_size = (return_values.status == SCMI_SUCCESS) ?
        sizeof(return_values) :
        sizeof(return_values.status);

    /* Send response */
    respond_status = scmi_telemetry_ctx.scmi_api->respond(
        service_id, &return_values, response_size);

    /* Log response failure */
    if (respond_status != FWK_SUCCESS) {
        FWK_LOG_DEBUG(
            "[SCMI-TELEMETRY] Response failed in %s @%d", __func__, __LINE__);
        return respond_status;
    }

    return status;
}

/*!
 * \brief Handles resetting of current telemetry infrastructure.
 *
 * This function processes a request to reset all the telemetry.
 *
 * \param[in] service_id The SCMI service identifier.
 * \param[in] payload Pointer to the request payload (unused in this function).
 *
 * \retval ::FWK_SUCCESS The operation was successful.
 * \return One of the standard framework error codes.
 */
static int scmi_telemetry_reset_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    int status;
    int respond_status;
    uint32_t response_size;
    struct scmi_telemetry_reset_a2p *params = NULL;
    struct scmi_telemetry_reset_p2a return_values = {
        .status = SCMI_GENERIC_ERROR,
    };

    /* Validate payload pointer */
    if (payload == NULL) {
        return FWK_E_PARAM;
    }

    if (!scmi_telemetry_ctx.config->telemetry_reset_support) {
        status = FWK_E_SUPPORT;
        return_values.status = SCMI_NOT_SUPPORTED;
        goto exit;
    }

    /* Parse request parameters */
    params = (struct scmi_telemetry_reset_a2p *)payload;
    if (params->flags != 0) {
        status = FWK_E_PARAM;
        return_values.status = SCMI_INVALID_PARAMETERS;
        goto exit;
    }

    status = scmi_telemetry_ctx.telemetry_api->telemetry_reset();
    if (status != FWK_SUCCESS) {
        return_values.status = SCMI_PROTOCOL_ERROR;
        goto exit;
    }

    /* Resets the sampling rate. */
    scmi_telemetry_ctx.current_sampling_rate = 0;
    scmi_telemetry_ctx.telemetry_enable = false;

    return_values.status = SCMI_SUCCESS;
exit:
    /* Determine response size */
    response_size = (return_values.status == SCMI_SUCCESS) ?
        sizeof(return_values) :
        sizeof(return_values.status);

    /* Send response */
    respond_status = scmi_telemetry_ctx.scmi_api->respond(
        service_id, &return_values, response_size);

    /* Log response failure */
    if (respond_status != FWK_SUCCESS) {
        FWK_LOG_DEBUG(
            "[SCMI-TELEMETRY] Response failed in %s @%d", __func__, __LINE__);
        return respond_status;
    }

    return status;
}

#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
/*!
 * \brief Handles SCMI resource permission checks for telemetry.
 *
 * This function validates whether a given SCMI message has the required
 * permissions before it is processed. It currently returns `FWK_E_SUPPORT`
 * indicating that the permission check is not implemented.
 *
 * \param[in] service_id The SCMI service identifier.
 * \param[in] payload Pointer to the request payload.
 * \param[in] payload_size Size of the request payload.
 * \param[in] message_id The SCMI message identifier.
 *
 * \retval ::FWK_E_SUPPORT Resource permission checks are not supported.
 */
static int scmi_telemetry_permissions_handler(
    fwk_id_t service_id,
    const uint32_t *payload,
    size_t payload_size,
    unsigned int message_id)
{
    return FWK_E_SUPPORT;
}
#endif

/*!
 * \brief Retrieves the SCMI protocol ID for telemetry.
 *
 * This function returns the protocol ID associated with SCMI telemetry.
 *
 * \param[in] protocol_id The framework identifier for the protocol.
 * \param[out] scmi_protocol_id Pointer to store the SCMI protocol ID.
 *
 * \retval ::FWK_SUCCESS Operation completed successfully.
 */
static int scmi_telemetry_get_scmi_protocol_id(
    fwk_id_t protocol_id,
    uint8_t *scmi_protocol_id)
{
    *scmi_protocol_id = MOD_SCMI_PROTOCOL_ID_TELEMETRY;

    return FWK_SUCCESS;
}

/*!
 * \brief Handles incoming SCMI messages for telemetry.
 *
 * This function validates incoming SCMI telemetry messages and dispatches them
 * to their respective handlers based on the message ID. It also ensures that
 * any required resource permissions are checked.
 *
 * \param[in] protocol_id The framework identifier for the SCMI protocol.
 * \param[in] service_id The SCMI service identifier.
 * \param[in] payload Pointer to the request payload.
 * \param[in] payload_size Size of the request payload.
 * \param[in] message_id The SCMI message identifier.
 *
 * \retval ::FWK_SUCCESS Operation completed successfully.
 * \return One of the standard framework error codes.
 */
static int scmi_telemetry_message_handler(
    fwk_id_t protocol_id,
    fwk_id_t service_id,
    const uint32_t *payload,
    size_t payload_size,
    unsigned int message_id)
{
    int validation_result;

    /* Validate the SCMI message */
    validation_result = scmi_telemetry_ctx.scmi_api->scmi_message_validation(
        MOD_SCMI_PROTOCOL_ID_TELEMETRY,
        service_id,
        payload,
        payload_size,
        message_id,
        payload_size_table,
        (unsigned int)MOD_SCMI_TELEMETRY_COMMAND_COUNT,
        msg_handler_table);

#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
    /* Check if the message requires resource permission validation */
    if (message_id >= MOD_SCMI_MESSAGE_ID_ATTRIBUTE) {
        if (scmi_telemetry_permissions_handler(
                service_id, payload, payload_size, message_id) != FWK_SUCCESS) {
            validation_result = SCMI_DENIED;
        }
    }
#endif

    if (validation_result != SCMI_SUCCESS) {
        return scmi_telemetry_ctx.scmi_api->respond(
            service_id, &validation_result, sizeof(validation_result));
    }

    /* Dispatch the message to the appropriate handler */
    if (msg_handler_table[message_id] == NULL) {
        return FWK_E_PARAM;
    }

    return msg_handler_table[message_id](service_id, payload);
}

/*!
 * \brief SCMI telemetry module API structure.
 *
 * This structure defines the interface for handling SCMI telemetry messages.
 */
static struct mod_scmi_to_protocol_api
    scmi_telemetry_mod_scmi_to_protocol_api = {
        .get_scmi_protocol_id = scmi_telemetry_get_scmi_protocol_id,
        .message_handler = scmi_telemetry_message_handler
    };

/*
 * Framework handlers
 */

/*!
 * \brief Initializes the SCMI telemetry module.
 *
 * This function initializes the SCMI telemetry module by storing the provided
 * configuration data. It ensures that the configuration and agent table are
 * valid.
 *
 * \param[in] module_id The module identifier.
 * \param[in] element_count Number of elements in the module (unused).
 * \param[in] data Pointer to the module configuration.
 *
 * \retval ::FWK_SUCCESS Initialization was successful.
 * \retval ::FWK_E_PARAM If the configuration data is NULL or the agent table is
 *     NULL.
 * \return One of the standard framework error codes.
 */
static int scmi_telemetry_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    const struct mod_scmi_telemetry_config *config;

    if (data == NULL) {
        return FWK_E_PARAM;
    }

    config = (const struct mod_scmi_telemetry_config *)data;

    if (config->agent_table == NULL) {
        return FWK_E_PARAM;
    }

    scmi_telemetry_ctx.de_enabled_list = fwk_mm_calloc(
        config->max_enabled_de_count, sizeof(telemetry_de_handle_st));
    if (scmi_telemetry_ctx.de_enabled_list == NULL) {
        return FWK_E_NOMEM;
    }
    scmi_telemetry_ctx.config = config;

    return FWK_SUCCESS;
}

/*!
 * \brief Binds required module dependencies for SCMI telemetry.
 *
 * This function binds the SCMI protocol API and, if enabled, binds
 * the SCMI notification API and telemetry API.
 *
 * \param[in] id The module identifier.
 * \param[in] round The binding round (ensures dependencies are bound in order).
 *
 * \retval ::FWK_SUCCESS The binding was successful.
 * \retval ::FWK_E_PANIC If any of the required bindings fail.
 * \return One of the standard framework error codes.
 */
static int scmi_telemetry_bind(fwk_id_t id, unsigned int round)
{
    int status;

    /* Perform binding only in the first round */
    if (round == 1) {
        return FWK_SUCCESS;
    }

    /* Bind the SCMI protocol API */
    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
        FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL),
        &scmi_telemetry_ctx.scmi_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

#ifdef BUILD_HAS_MOD_TELEMETRY
    /* Bind the telemetry protocol API */
    return fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_TELEMETRY),
        FWK_ID_API(FWK_MODULE_IDX_TELEMETRY, 0),
        &scmi_telemetry_ctx.telemetry_api);
#else
    return FWK_SUCCESS;
#endif
}

static int scmi_telemetry_start(fwk_id_t id)
{
    const struct mod_scmi_telemetry_config *config = scmi_telemetry_ctx.config;

    return scmi_telemetry_ctx.telemetry_api->shmti_init(
        config->shmti_list, config->shmti_count);
}

/*!
 * \brief Processes incoming API bind requests.
 *
 * This function handles bind requests for the SCMI telemetry protocol API.
 *
 * \param[in] source_id The identifier of the module requesting the API.
 * \param[in] target_id The identifier of the module providing the API.
 * \param[in] api_id The API identifier.
 * \param[out] api Pointer to the requested API.
 *
 * \retval ::FWK_SUCCESS If the requested API is provided successfully.
 * \retval ::FWK_E_ACCESS If the requested API is not available.
 * \return One of the standard framework error codes.
 */
static int scmi_telemetry_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    switch ((enum scmi_telemetry_api_idx)fwk_id_get_api_idx(api_id)) {
    case MOD_SCMI_TELEMETRY_PROTOCOL_API:
        if (api == NULL) {
            return FWK_E_PARAM;
        }
        *api = &scmi_telemetry_mod_scmi_to_protocol_api;
        break;

    default:
        return FWK_E_ACCESS;
    }

    return FWK_SUCCESS;
}

#if defined(BUILD_HAS_SCMI_NOTIFICATIONS) && defined(BUILD_HAS_NOTIFICATION)
/*!
 * \brief Processes received SCMI telemetry notifications.
 *
 * This function is a stub for handling telemetry notifications.
 *
 * \param[in] event The received event.
 * \param[out] resp_event The response event (if needed).
 *
 * \retval ::FWK_SUCCESS The function always returns success.
 */
static int scmi_telemetry_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    return FWK_SUCCESS;
}
#endif

/*!
 * \brief SCMI Telemetry Management Protocol Definition.
 *
 * This structure defines the module type and function handlers
 * required for SCMI telemetry management.
 */
const struct fwk_module module_scmi_telemetry = {
    .api_count = (unsigned int)MOD_SCMI_TELEMETRY_API_COUNT,
    .type = FWK_MODULE_TYPE_PROTOCOL,
    .init = scmi_telemetry_init,
    .bind = scmi_telemetry_bind,
    .start = scmi_telemetry_start,
    .process_bind_request = scmi_telemetry_process_bind_request,
#if defined(BUILD_HAS_SCMI_NOTIFICATIONS) && defined(BUILD_HAS_NOTIFICATION)
    .process_notification = scmi_telemetry_process_notification,
#endif
};
