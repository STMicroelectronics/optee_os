/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      System Control and Management Interface (SCMI) support for Telemetry
 *      Management Protocol.
 */

#ifndef INTERNAL_SCMI_TELEMETRY_H
#define INTERNAL_SCMI_TELEMETRY_H

#include <mod_telemetry.h>

#include <fwk_id.h>

#include <stdint.h>

/*!
 * \addtogroup GroupModules Modules
 * \{
 */

/*!
 * \defgroup GroupSCMI_TELEMETRY SCMI Telemetry Protocol.
 * \{
 */

#define SCMI_PROTOCOL_VERSION_TELEMETRY UINT32_C(0x10000)

/*
 * NEGOTIATE_PROTOCOL_VERSION
 */
struct scmi_telemetry_negotiate_protocol_version_a2p {
    uint32_t version;
};

/*
 * PROTOCOL_ATTRIBUTES
 */

#define SCMI_TELEMETRY_PROTOCOL_ATTRIBUTES_IMPL_VERSION_MAX_DWORD 4

/*
 * SCMI Telemetry Protocol Attributes Macros
 *
 * These macros define bit positions and masks for encoding various attributes
 * of the SCMI (System Control and Management Interface) Telemetry protocol.
 * These attributes help describe the protocol capabilities and behaviors.
 */

/* Bit position indicating support for asynchronous telemetry read operations */
#define SCMI_TELEMETRY_PROTOCOL_ATTR_ASYNC_READ_POS 31

/* Bit position indicating support for continuous telemetry updates */
#define SCMI_TELEMETRY_PROTOCOL_ATTR_CONTINUOUS_UPDATE_POS 30

/* Bit position indicating whether telemetry supports different update intervals
 * for individual groups. */
#define SCMI_TELEMETRY_PROTOCOL_ATTR_GROUP_SAMPLING_RATES_POS 18

/* Bit position indicating whether telemetry reset is supported */
#define SCMI_TELEMETRY_PROTOCOL_ATTR_TELEMETRY_RESET_POS 17

/* Bit position indicating Fast Channel (FCH) support */
#define SCMI_TELEMETRY_PROTOCOL_ATTR_FCH_POS 16

/* Bit position indicating the number of available Shared Memory Telemetry
 * Interfaces (SHMTI) */
#define SCMI_TELEMETRY_PROTOCOL_ATTR_NUM_SHMTI_POS 0

/*
 * SCMI_TELEMETRY_PROTOCOL_ATTR_ASYNC_READ_MASK
 *
 * Mask for enabling or disabling asynchronous telemetry reading.
 * When set, this indicates that telemetry data can be read asynchronously.
 */
#define SCMI_TELEMETRY_PROTOCOL_ATTR_ASYNC_READ_MASK \
    (UINT32_C(0x1) << SCMI_TELEMETRY_PROTOCOL_ATTR_ASYNC_READ_POS)

/*
 * SCMI_TELEMETRY_PROTOCOL_ATTR_CONTINUOUS_UPDATE_MASK
 *
 * Mask for enabling or disabling continuous updates.
 * When set, this indicates that telemetry data is updated continuously.
 */
#define SCMI_TELEMETRY_PROTOCOL_ATTR_CONTINUOUS_UPDATE_MASK \
    (UINT32_C(0x1) << SCMI_TELEMETRY_PROTOCOL_ATTR_CONTINUOUS_UPDATE_POS)

/*
 * SCMI_TELEMETRY_PROTOCOL_ATTR_GROUP_SAMPLING_RATES_MASK
 *
 * Mask for enabling or disabling per-event-group sampling configuration.
 */
#define SCMI_TELEMETRY_PROTOCOL_ATTR_GROUP_SAMPLING_RATES_MASK \
    (UINT32_C(0x1) << SCMI_TELEMETRY_PROTOCOL_ATTR_GROUP_SAMPLING_RATES_POS)

/*
 * SCMI_TELEMETRY_PROTOCOL_ATTR_NUM_SHMTI_MASK
 *
 * Mask to extract or set the number of Shared Memory Telemetry Interfaces
 * (SHMTI). This defines how many SHMTI instances are available.
 */
#define SCMI_TELEMETRY_PROTOCOL_ATTR_NUM_SHMTI_MASK \
    (UINT32_C(0xFFFF) << SCMI_TELEMETRY_PROTOCOL_ATTR_NUM_SHMTI_POS)

/*
 * SCMI_TELEMETRY_PROTOCOL_ATTR_TELEMETRY_RESET_MASK
 *
 * Mask indicating whether telemetry reset is supported.
 * When set, the protocol supports resetting telemetry data.
 */
#define SCMI_TELEMETRY_PROTOCOL_ATTR_TELEMETRY_RESET_MASK \
    (UINT32_C(0x1) << SCMI_TELEMETRY_PROTOCOL_ATTR_TELEMETRY_RESET_POS)

/*
 * SCMI_TELEMETRY_PROTOCOL_ATTR_FCH_MASK
 *
 * Mask indicating support for Fast Channel (FCH)
 * functionality. This feature allows better frequency coordination across
 * multiple components.
 */
#define SCMI_TELEMETRY_PROTOCOL_ATTR_FCH_MASK \
    (UINT32_C(0x1) << SCMI_TELEMETRY_PROTOCOL_ATTR_FCH_POS)

/*
 * SCMI_TELEMETRY_PROTOCOL_ATTRIBUTES
 *
 * Macro to encode SCMI Telemetry Protocol attributes into a single 32-bit
 * value. This macro shifts and combines various flags and values to form the
 * protocol attributes.
 *
 * Parameters:
 * - ASYNC: Enables asynchronous telemetry data reading (1 = enabled, 0 =
 * disabled).
 * - CONTINUOUS_UPDATE: Enables continuous telemetry updates (1 = enabled, 0 =
 * disabled).
 * - TELEMETRY_RESET: Indicates whether telemetry reset is supported (1 =
 * supported, 0 = not supported).
 * - FCH: Enables Fast Channel support (1 = enabled, 0 =
 * disabled).
 * - NUM_SHMTI: Specifies the number of available SHMTI instances.
 *
 * Returns:
 * - A 32-bit encoded value representing the protocol attributes.
 */
#define SCMI_TELEMETRY_PROTOCOL_ATTRIBUTES( \
    ASYNC, CONTINUOUS_UPDATE, GROUP_SPECIFIC, TELEMETRY_RESET, FCH, NUM_SHMTI) \
    ( \
        (((ASYNC) << SCMI_TELEMETRY_PROTOCOL_ATTR_ASYNC_READ_POS) & \
         SCMI_TELEMETRY_PROTOCOL_ATTR_ASYNC_READ_MASK) | \
        (((CONTINUOUS_UPDATE) \
          << SCMI_TELEMETRY_PROTOCOL_ATTR_CONTINUOUS_UPDATE_POS) & \
         SCMI_TELEMETRY_PROTOCOL_ATTR_CONTINUOUS_UPDATE_MASK) | \
        (( \
             (GROUP_SPECIFIC) \
             << SCMI_TELEMETRY_PROTOCOL_ATTR_GROUP_SAMPLING_RATES_POS) & \
         SCMI_TELEMETRY_PROTOCOL_ATTR_GROUP_SAMPLING_RATES_MASK) | \
        (((TELEMETRY_RESET) \
          << SCMI_TELEMETRY_PROTOCOL_ATTR_TELEMETRY_RESET_POS) & \
         SCMI_TELEMETRY_PROTOCOL_ATTR_TELEMETRY_RESET_MASK) | \
        (((FCH) << SCMI_TELEMETRY_PROTOCOL_ATTR_FCH_POS) & \
         SCMI_TELEMETRY_PROTOCOL_ATTR_FCH_MASK) | \
        (((NUM_SHMTI) << SCMI_TELEMETRY_PROTOCOL_ATTR_NUM_SHMTI_POS) & \
         SCMI_TELEMETRY_PROTOCOL_ATTR_NUM_SHMTI_MASK))
/*!
 * \brief SCMI Telemetry Protocol Attributes Response Structure
 *
 * This structure is used to return protocol attributes when handling
 * an SCMI protocol attributes request.
 */
struct scmi_telemetry_protocol_attributes_p2a {
    /*! Status of the SCMI request (SCMI_SUCCESS, SCMI_NOT_FOUND, etc.) */
    int32_t status;

    /*! Total number of Data Event (DE) descriptors available */
    uint32_t num_de;

    /*! Total number of Event Groups. */
    uint32_t num_groups;

    /*! Array of Dwords for DE Implementation Revision. */
    uint32_t de_impl_rev_dword
        [SCMI_TELEMETRY_PROTOCOL_ATTRIBUTES_IMPL_VERSION_MAX_DWORD];

    /*! Encoded attribute flags representing protocol capabilities */
    uint32_t attributes;
};

/*!
 * \brief SCMI Telemetry List SHMTI (Shared Memory Telemetry Interface)
 *
 * This section defines macros and structures for handling SHMTI in the SCMI
 * Telemetry protocol. The SHMTI region provides shared memory interfaces
 * for telemetry data storage.
 */

/* Bit position for the number of remaining SHMTI instances */
#define SCMI_TELEMETRY_LIST_SHMTI_NUM_REMAIN_POS 16

/* Bit position for the number of SHMTI instances in the response */
#define SCMI_TELEMETRY_LIST_SHMTI_NUM_POS 0

/* Mask for extracting the number of remaining SHMTI instances */
#define SCMI_TELEMETRY_LIST_SHMTI_NUM_REMAIN_MASK \
    (UINT32_C(0xFFFF) << SCMI_TELEMETRY_LIST_SHMTI_NUM_REMAIN_POS)

/* Mask for extracting the number of SHMTI instances in the response */
#define SCMI_TELEMETRY_LIST_SHMTI_NUM_MASK \
    (UINT32_C(0xFFFF) << SCMI_TELEMETRY_LIST_SHMTI_NUM_POS)

/*!
 * \brief Compute the total number of SHMTI instances in the response.
 *
 * \param[in] REMAIN  The number of remaining SHMTI instances.
 * \param[in] CURRENT The number of SHMTI instances in the current response.
 *
 * \return A 32-bit value encoding the total number of SHMTI instances.
 */
#define SCMI_TELEMETRY_LIST_SHMTI_TOTAL_SHMTI(REMAIN, CURRENT) \
    ((((REMAIN) << SCMI_TELEMETRY_LIST_SHMTI_NUM_REMAIN_POS) & \
      SCMI_TELEMETRY_LIST_SHMTI_NUM_REMAIN_MASK) | \
     (((CURRENT) << SCMI_TELEMETRY_LIST_SHMTI_NUM_POS) & \
      SCMI_TELEMETRY_LIST_SHMTI_NUM_MASK))
/*!
 * \brief Telemetry SHMTI description.
 */
struct scmi_telemetry_shmti_desc {
    /*! SHMTI ID. */
    uint32_t shmti_id;
    /*! Start address of the SHMTI region (low part). */
    uint32_t addr_lo;
    /*! Start address of the SHMTI region (high part). */
    uint32_t addr_hi;
    /*! Length of the SHMTI region. */
    uint32_t length;
};

/*!
 * \brief SCMI response structure for listing SHMTI instances.
 *
 * This structure is used in the SCMI Telemetry List SHMTI response,
 * containing the number of available SHMTI instances and their descriptors.
 */
struct scmi_telemetry_list_shmti_p2a {
    int32_t status; /*!< Status of the SCMI request (SCMI_SUCCESS, etc.) */
    uint32_t num_shmti; /*!< Number of SHMTI instances in the response */
    struct scmi_telemetry_shmti_desc
        shmti_desc[]; /*!< Array of SHMTI descriptors */
};

/*!
 * \brief SCMI Telemetry Data Event (DE) Description
 *
 * This section defines macros and structures related to telemetry
 * data event descriptions in the SCMI protocol. These descriptions
 * help configure and retrieve telemetry data.
 */

/* Bit positions for the number of DE descriptors remaining and current count */
#define SCMI_TELEMETRY_DE_DESC_NUM_REMAIN_POS 16
#define SCMI_TELEMETRY_DE_DESC_NUM_POS        0

/* Bit masks for extracting DE descriptor counts */
#define SCMI_TELEMETRY_DE_DESC_NUM_REMAIN_MASK \
    (UINT32_C(0xFFFF) << SCMI_TELEMETRY_DE_DESC_NUM_REMAIN_POS)
#define SCMI_TELEMETRY_DE_DESC_NUM_MASK \
    (UINT32_C(0xFFFF) << SCMI_TELEMETRY_DE_DESC_NUM_POS)

/*!
 * \brief Compute the total number of DE descriptors in the response.
 *
 * \param[in] REMAIN  The number of remaining DE descriptors.
 * \param[in] CURRENT The number of DE descriptors in the current response.
 *
 * \return A 32-bit value encoding the total number of DE descriptors.
 */
#define SCMI_TELEMETRY_DE_DESC_TOTAL(REMAIN, CURRENT) \
    ((((CURRENT) << SCMI_TELEMETRY_DE_DESC_NUM_POS) & \
      SCMI_TELEMETRY_DE_DESC_NUM_MASK) | \
     (((REMAIN) << SCMI_TELEMETRY_DE_DESC_NUM_REMAIN_POS) & \
      SCMI_TELEMETRY_DE_DESC_NUM_REMAIN_MASK))

/*!
 * \brief Bit positions for DE attribute fields in `de_attributes_1`
 */
#define SCMI_TELEMETRY_DE_NAME_POS              31
#define SCMI_TELEMETRY_DE_FCH_POS               30
#define SCMI_TELEMETRY_DE_TYPE_POS              22
#define SCMI_TELEMETRY_DE_PERSISTENT_POS        21
#define SCMI_TELEMETRY_DE_UNIT_EXP_POS          13
#define SCMI_TELEMETRY_DE_UNIT_POS              5
#define SCMI_TELEMETRY_DE_TIMESTAMP_EXP_POS     1
#define SCMI_TELEMETRY_DE_TIMESTAMP_SUPPORT_POS 0

/* Bit masks for `de_attributes_1` */
#define SCMI_TELEMETRY_DE_NAME_MASK \
    (UINT32_C(0x1) << SCMI_TELEMETRY_DE_NAME_POS)
#define SCMI_TELEMETRY_DE_FCH_MASK (UINT32_C(0x1) << SCMI_TELEMETRY_DE_FCH_POS)
#define SCMI_TELEMETRY_DE_TYPE_MASK \
    (UINT32_C(0xFF) << SCMI_TELEMETRY_DE_TYPE_POS)
#define SCMI_TELEMETRY_DE_PERSISTENT_MASK \
    (UINT32_C(0x1) << SCMI_TELEMETRY_DE_PERSISTENT_POS)
#define SCMI_TELEMETRY_DE_UNIT_EXP_MASK \
    (UINT32_C(0xFF) << SCMI_TELEMETRY_DE_UNIT_EXP_POS)
#define SCMI_TELEMETRY_DE_UNIT_MASK \
    (UINT32_C(0xFF) << SCMI_TELEMETRY_DE_UNIT_POS)
#define SCMI_TELEMETRY_DE_TIMESTAMP_EXP_MASK \
    (UINT32_C(0xF) << SCMI_TELEMETRY_DE_TIMESTAMP_EXP_POS)
#define SCMI_TELEMETRY_DE_TIMESTAMP_SUPPORT_MASK \
    (UINT32_C(0x1) << SCMI_TELEMETRY_DE_TIMESTAMP_SUPPORT_POS)

/*!
 * \brief Macro to construct `de_attributes_1` field.
 *
 * \param[in] DE_FCH             Fast Channel (1/0)
 * \param[in] DE_TYPE            Type of Data Event
 * \param[in] DE_PERSISTENT      Indicates persistent storage (1/0)
 * \param[in] DE_UNIT_EXP        Exponent value for unit
 * \param[in] DE_UNIT            Unit identifier
 * \param[in] TS_EXP             Exponent for timestamp granularity
 * \param[in] TS_SUPPORT         Timestamp support flag (1/0)
 *
 * \return Encoded `de_attributes_1` field.
 */
#define SCMI_TELEMETRY_DE_ATTR_1( \
    DE_FCH, DE_TYPE, DE_PERSISTENT, DE_UNIT_EXP, DE_UNIT, TS_EXP, TS_SUPPORT) \
    ((((DE_FCH) << SCMI_TELEMETRY_DE_FCH_POS) & SCMI_TELEMETRY_DE_FCH_MASK) | \
     (((DE_TYPE) << SCMI_TELEMETRY_DE_TYPE_POS) & \
      SCMI_TELEMETRY_DE_TYPE_MASK) | \
     (((DE_PERSISTENT) << SCMI_TELEMETRY_DE_PERSISTENT_POS) & \
      SCMI_TELEMETRY_DE_PERSISTENT_MASK) | \
     (((DE_UNIT_EXP) << SCMI_TELEMETRY_DE_UNIT_EXP_POS) & \
      SCMI_TELEMETRY_DE_UNIT_EXP_MASK) | \
     (((DE_UNIT) << SCMI_TELEMETRY_DE_UNIT_POS) & \
      SCMI_TELEMETRY_DE_UNIT_MASK) | \
     (((TS_EXP) << SCMI_TELEMETRY_DE_TIMESTAMP_EXP_POS) & \
      SCMI_TELEMETRY_DE_TIMESTAMP_EXP_MASK) | \
     (((TS_SUPPORT) << SCMI_TELEMETRY_DE_TIMESTAMP_SUPPORT_POS) & \
      SCMI_TELEMETRY_DE_TIMESTAMP_SUPPORT_MASK))

/*!
 * \brief Bit positions for DE attribute fields in `de_attributes_2`
 */
#define SCMI_TELEMETRY_DE_INSTANCE_ID_POS      24
#define SCMI_TELEMETRY_DE_COMP_INSTANCE_ID_POS 8
#define SCMI_TELEMETRY_DE_COMP_TYPE_POS        0

/* Bit masks for `de_attributes_2` */
#define SCMI_TELEMETRY_DE_INSTANCE_ID_MASK \
    (UINT32_C(0xFF) << SCMI_TELEMETRY_DE_INSTANCE_ID_POS)
#define SCMI_TELEMETRY_DE_COMP_INSTANCE_ID_MASK \
    (UINT32_C(0xFFFF) << SCMI_TELEMETRY_DE_COMP_INSTANCE_ID_POS)
#define SCMI_TELEMETRY_DE_COMP_TYPE_MASK \
    (UINT32_C(0xFF) << SCMI_TELEMETRY_DE_COMP_TYPE_POS)

/*!
 * \brief Macro to construct `de_attributes_2` field.
 *
 * \param[in] DE_INSTANCE_ID       Unique instance ID
 * \param[in] DE_COMP_INSTANCE_ID  Component instance ID
 * \param[in] DE_COMP_TYPE         Component type identifier
 *
 * \return Encoded `de_attributes_2` field.
 */
#define SCMI_TELEMETRY_DE_ATTR_2( \
    DE_INSTANCE_ID, DE_COMP_INSTANCE_ID, DE_COMP_TYPE) \
    ((((DE_INSTANCE_ID) << SCMI_TELEMETRY_DE_INSTANCE_ID_POS) & \
      SCMI_TELEMETRY_DE_INSTANCE_ID_MASK) | \
     (((DE_COMP_INSTANCE_ID) << SCMI_TELEMETRY_DE_COMP_INSTANCE_ID_POS) & \
      SCMI_TELEMETRY_DE_COMP_INSTANCE_ID_MASK) | \
     (((DE_COMP_TYPE) << SCMI_TELEMETRY_DE_COMP_TYPE_POS) & \
      SCMI_TELEMETRY_DE_COMP_TYPE_MASK))

/*!
 * \brief Size of DE name field in Bytes.
 */
#define SCMI_TELEMETRY_DE_NAME_SIZE 16
#define SCMI_TELEMETRY_DE_DESC_MAX_SIZE \
    (sizeof(struct mod_telemetry_de_desc) + \
     sizeof(struct mod_telemetry_de_fch_attr) + SCMI_TELEMETRY_DE_NAME_SIZE)

/*!
 * \brief SCMI response structure for DE descriptions.
 *
 * This structure is used in the SCMI Telemetry DE Description response,
 * containing the number of available DE descriptions and their descriptors.
 */
struct scmi_telemetry_de_desc_p2a {
    int status; /*!< Status of the SCMI request */
    uint32_t num_desc; /*!< Number of DE descriptions in the response */
    struct mod_telemetry_de_desc de_desc[]; /*!< Array of DE descriptors */
};

/*!
 * \brief SCMI Telemetry Update Intervals
 *
 * Defines macros and structures to represent telemetry update intervals
 * in the SCMI telemetry protocol. These intervals determine how often
 * telemetry data is refreshed.
 */
/* Bit positions for the update intervals request and response. */
#define SCMI_TELEMETRY_UPDATE_INTERVALS_GROUP_ID_SELECTOR_POS 0
#define SCMI_TELEMETRY_UPDATE_INTERVALS_NUM_REMAIN_POS \
    16 /*!< Remaining intervals */
#define SCMI_TELEMETRY_UPDATE_INTERVALS_FORMAT_POS \
    12 /*!< Format of the interval */
#define SCMI_TELEMETRY_UPDATE_INTERVALS_NUM_POS 0 /*!< Number of intervals */

/* Bit Mask for Group ID selector. */
#define SCMI_TELEMETRY_UPDATE_INTERVALS_GROUP_ID_SELECTOR_MASK \
    (UINT32_C(0xF) << SCMI_TELEMETRY_UPDATE_INTERVALS_GROUP_ID_SELECTOR_POS)

/* Bit masks for extracting interval properties */
#define SCMI_TELEMETRY_UPDATE_INTERVALS_NUM_REMAIN_MASK \
    (UINT32_C(0xFFFF) << SCMI_TELEMETRY_UPDATE_INTERVALS_NUM_REMAIN_POS)
#define SCMI_TELEMETRY_UPDATE_INTERVALS_FORMAT_MASK \
    (UINT32_C(0x1) << SCMI_TELEMETRY_UPDATE_INTERVALS_FORMAT_POS)
#define SCMI_TELEMETRY_UPDATE_INTERVALS_NUM_MASK \
    (UINT32_C(0xFFF) << SCMI_TELEMETRY_UPDATE_INTERVALS_NUM_POS)

#define SCMI_TELEMETRY_UPDATE_INTERVALS_GET_GROUP_ID_SELECTOR(flags) \
    ((flags)&SCMI_TELEMETRY_UPDATE_INTERVALS_GROUP_ID_SELECTOR_MASK)

#define SCMI_TELEMETRY_UPDATE_INTERVALS_GROUP_ID_SELECTOR_DE          0U
#define SCMI_TELEMETRY_UPDATE_INTERVALS_GROUP_ID_SELECTOR_EVENT_GROUP 1U
#define SCMI_TELEMETRY_UPDATE_INTERVALS_GROUP_ID_SELECTOR_ALL         2U

/*!
 * \brief Constructs a telemetry update interval flag.
 *
 * \param[in] NUM_REMAIN     Number of remaining update intervals.
 * \param[in] INTERVAL_FORMAT Format of the update intervals (Discrete/Linear).
 * \param[in] NUM_INTERVALS  Number of update intervals in this response.
 *
 * \return Encoded update interval flag.
 */
#define SCMI_TELEMETRY_UPDATE_INTERVALS_FLAG( \
    NUM_REMAIN, INTERVAL_FORMAT, NUM_INTERVALS) \
    ((((NUM_REMAIN) << SCMI_TELEMETRY_UPDATE_INTERVALS_NUM_REMAIN_POS) & \
      SCMI_TELEMETRY_UPDATE_INTERVALS_NUM_REMAIN_MASK) | \
     (((INTERVAL_FORMAT) << SCMI_TELEMETRY_UPDATE_INTERVALS_FORMAT_POS) & \
      SCMI_TELEMETRY_UPDATE_INTERVALS_FORMAT_MASK) | \
     (((NUM_INTERVALS) << SCMI_TELEMETRY_UPDATE_INTERVALS_NUM_POS) & \
      SCMI_TELEMETRY_UPDATE_INTERVALS_NUM_MASK))

/*!
 * \brief SCMI request structure for telemetry update intervals.
 *
 * This structure is used in SCMI requests to retrieve the telemetry update
 * intervals.
 */
struct scmi_telemetry_list_update_intervals_a2p {
    uint32_t index; /*!< Index of the first update interval to be listed */
    uint32_t group_id; /*!< Group identifier */
    uint32_t flags; /*!< Group identifier selector flags */
};

/*!
 * \brief SCMI response structure for telemetry update intervals.
 *
 * This structure is used in the SCMI telemetry response to provide
 * a list of available update intervals.
 */
struct scmi_telemetry_list_update_intervals_p2a {
    int32_t status; /*!< Status of the SCMI request (SCMI_SUCCESS, etc.) */
    uint32_t flags; /*!< Encoded flags representing update intervals */
};

/*!
 * \brief SCMI Telemetry Data Event (DE) Configuration
 *
 * Defines macros and structures for configuring Data Events (DE)
 * in the SCMI telemetry protocol.
 */

/*!
 * \brief DE_CONFIGURE selector.
 *
 * The selector specifies if agent has provided a Data Event or an Event Group
 * in the request.
 */
#define SCMI_TELEMETRY_DE_CONFIGURE_ID_SELECTOR_DE          0
#define SCMI_TELEMETRY_DE_CONFIGURE_ID_SELECTOR_EVENT_GROUP 1

/*!
 * \brief Data Event (DE) Modes
 *
 * These modes specify whether a Data Event is enabled, disabled,
 * or enabled with a timestamp.
 */

#define SCMI_TELEMETRY_DE_DISABLE 0 /*!< Disable Data Event */
#define SCMI_TELEMETRY_DE_ENABLE_NON_TS \
    1 /*!< Enable Data Event without timestamp */
#define SCMI_TELEMETRY_DE_ENABLE_TS 2 /*!< Enable Data Event with timestamp */

/*!
 * \brief SHMTI ID Constants
 *
 * These constants indicate unavailable or unsupported SHMTI IDs.
 */
#define SCMI_TELEMETRY_DE_SHMTI_ID_NOT_SUPPORTED        UINT32_C(0xFFFFFFFF)
#define SCMI_TELEMETRY_DE_SHMTI_ID_OFFSET_NOT_SUPPORTED UINT32_C(0xFFFFFFFF)

/*!
 * \brief Bit positions and masks for DE configuration flags
 */
#define SCMI_TELEMETRY_DE_CONFIGURE_ID_SELECTOR_POS 3
#define SCMI_TELEMETRY_DE_CONFIGURE_ID_SELECTOR_MASK \
    (UINT32_C(0x1) << SCMI_TELEMETRY_DE_CONFIGURE_ID_SELECTOR_POS)
#define SCMI_TELEMETRY_DE_CONFIGURE_ALL_DE_DISABLE_POS \
    2 /*!< Disable flag position */
#define SCMI_TELEMETRY_DE_CONFIGURE_ALL_DE_DISABLE_MASK \
    (UINT32_C(0x1) << SCMI_TELEMETRY_DE_CONFIGURE_ALL_DE_DISABLE_POS)

#define SCMI_TELEMETRY_DE_CONFIGURE_DE_MODE_POS 0 /*!< Mode field position */
#define SCMI_TELEMETRY_DE_CONFIGURE_DE_MODE_MASK \
    (UINT32_C(0x3) \
     << SCMI_TELEMETRY_DE_CONFIGURE_DE_MODE_POS) /*!< Uses 2 bits */

/*!
 * \brief Extract selector from configuration flags
 *
 * \param[in] DE_CONFIGURE_FLAGS The flags containing selector.
 *
 * \return The extracted selector value.
 */
#define SCMI_TELEMETRY_DE_CONFIGURE_ID_SELECTOR(DE_CONFIGURE_FLAGS) \
    (((DE_CONFIGURE_FLAGS)&SCMI_TELEMETRY_DE_CONFIGURE_ID_SELECTOR_MASK) >> \
     SCMI_TELEMETRY_DE_CONFIGURE_ID_SELECTOR_POS)

/*!
 * \brief Check if all DEs are disabled
 *
 * \param[in] DE_CONFIGURE_FLAGS The flags to check.
 *
 * \return Nonzero if all DEs are disabled, zero otherwise.
 */
#define SCMI_TELEMETRY_ALL_DE_DISABLED(DE_CONFIGURE_FLAGS) \
    ((DE_CONFIGURE_FLAGS)&SCMI_TELEMETRY_DE_CONFIGURE_ALL_DE_DISABLE_MASK)

/*!
 * \brief Extract DE mode from configuration flags
 *
 * \param[in] DE_CONFIGURE_FLAGS The flags containing DE mode.
 *
 * \return The extracted DE mode.
 */
#define SCMI_TELEMETRY_DE_CONFIGURE_DE_MODE(DE_CONFIGURE_FLAGS) \
    (((DE_CONFIGURE_FLAGS)&SCMI_TELEMETRY_DE_CONFIGURE_DE_MODE_MASK) >> \
     SCMI_TELEMETRY_DE_CONFIGURE_DE_MODE_POS)

/*!
 * \brief SHMTI ID Unavailable Constants
 *
 * These constants indicate that SHMTI IDs and offsets are unavailable.
 */
#define SCMI_TELEMETRY_SHMTI_ID_UNAVAILABLE        UINT32_C(0xFFFFFFFF)
#define SCMI_TELEMETRY_SHMTI_ID_OFFSET_UNAVAILABLE UINT32_C(0xFFFFFFFF)

/*!
 * \brief SCMI request structure for DE configuration.
 *
 * This structure is used in SCMI requests to configure Data Events (DEs).
 */
struct scmi_telemetry_de_configure_a2p {
    uint32_t id; /*!< ID of the Data Event/Event group to configure */
    uint32_t flags; /*!< Configuration flags (enable/disable/mode) */
};

/*!
 * \brief SCMI response structure for DE configuration.
 *
 * This structure is used in SCMI responses to indicate
 * the status of a DE configuration request.
 */
struct scmi_telemetry_de_configure_p2a {
    int32_t status; /*!< Status of the request (SCMI_SUCCESS, etc.) */
    uint32_t shmti_id; /*!< Assigned SHMTI ID for the Data Event */
    uint32_t shmti_de_offset; /*!< Offset of the Data Event within the SHMTI */
};

/*!
 * \brief SCMI Telemetry Enabled Data Events List
 *
 * Defines macros, enums, and structures used to retrieve a list of
 * enabled Data Events (DE) in SCMI Telemetry.
 */

/*! \brief Bit position and mask for the ID Selector. */
#define SCMI_TELEMETRY_DE_ENABLED_LIST_ID_SELECTOR_DE          0
#define SCMI_TELEMETRY_DE_ENABLED_LIST_ID_SELECTOR_EVENT_GROUP 1
#define SCMI_TELEMETRY_DE_ENABLED_LIST_ID_SELECTOR_POS         0
#define SCMI_TELEMETRY_DE_ENABLED_LIST_ID_SELECTOR_MASK \
    (UINT32_C(0x1) << SCMI_TELEMETRY_DE_CONFIGURE_ID_SELECTOR_POS)

/*! \brief Bit position and mask for the number of remaining DEs */
#define SCMI_TELEMETRY_DE_ENABLED_LIST_NUM_REMAIN_POS 16
#define SCMI_TELEMETRY_DE_ENABLED_LIST_NUM_REMAIN_MASK \
    (UINT32_C(0xFFFF) << SCMI_TELEMETRY_DE_ENABLED_LIST_NUM_REMAIN_POS)

/*! \brief Bit position and mask for the number of DEs in response */
#define SCMI_TELEMETRY_DE_ENABLED_LIST_NUM_POS 0
#define SCMI_TELEMETRY_DE_ENABLED_LIST_NUM_MASK \
    (UINT32_C(0xFFFF) << SCMI_TELEMETRY_DE_ENABLED_LIST_NUM_POS)

/*!
 * \brief Extract selector from identifier flags
 *
 * \param[in] DE_IDENTIFIER_FLAGS The flags containing selector.
 *
 * \return The extracted selector value.
 */
#define SCMI_TELEMETRY_DE_ENABLED_LIST_ID_SELECTOR(DE_IDENTIFIER_FLAGS) \
    (((DE_IDENTIFIER_FLAGS)&SCMI_TELEMETRY_DE_ENABLED_LIST_ID_SELECTOR_MASK) >> \
     SCMI_TELEMETRY_DE_ENABLED_LIST_ID_SELECTOR_POS)

/*!
 * \brief Macro to construct the enabled DE list flag.
 *
 * \param[in] NUM_REMAIN Number of remaining enabled DEs.
 * \param[in] NUM_DE     Number of DEs in the current response.
 *
 * \return Encoded flag representing the DE list response.
 */
#define SCMI_TELEMETRY_DE_ENABLED_LIST_FLAG(NUM_REMAIN, NUM_DE) \
    ((((NUM_REMAIN) << SCMI_TELEMETRY_DE_ENABLED_LIST_NUM_REMAIN_POS) & \
      SCMI_TELEMETRY_DE_ENABLED_LIST_NUM_REMAIN_MASK) | \
     (((NUM_DE) << SCMI_TELEMETRY_DE_ENABLED_LIST_NUM_POS) & \
      SCMI_TELEMETRY_DE_ENABLED_LIST_NUM_MASK))

/*!
 * \brief Enumeration for Data Event timestamp modes.
 *
 * Used to specify whether timestamps are enabled or disabled for a DE.
 */
#define SCMI_TELEMETRY_DE_TS_DISABLED 1U /*!< Timestamp disabled */
#define SCMI_TELEMETRY_DE_TS_ENABLED  2U /*!< Timestamp enabled */

/*!
 * \brief SCMI request structure for enabled DE list.
 */
struct scmi_telemetry_de_enabled_list_a2p {
    uint32_t index; /*!< Index of the first enabled Data Event or Event Group */
    uint32_t flags; /*!< Identifier flags (Data Event/Event Group) */
};

/*!
 * \brief Response structure for enabled DE list.
 *
 * Contains the list of enabled Data Events and relevant flags.
 */
struct scmi_telemetry_de_enabled_list_p2a {
    int status; /*!< Status of the SCMI request (SCMI_SUCCESS, etc.) */
    uint32_t flags; /*!< Encoded flags indicating DE count information */
    struct mod_telemetry_de_status array[]; /*!< Array of enabled DE statuses */
};

/*!
 * \brief SCMI Telemetry Configuration Macros
 *
 * These macros define the control parameters for configuring
 * telemetry settings such as sampling rate and operational modes.
 */

/*! \brief Bit position and mask for enabling/disabling telemetry */
#define SCMI_TELEMETRY_CONFIG_CONTROL_EN_POS 0
#define SCMI_TELEMETRY_CONFIG_CONTROL_EN_MASK \
    (UINT32_C(0x1) << SCMI_TELEMETRY_CONFIG_CONTROL_EN_POS)

/*! \brief Bit position and mask for configuring telemetry mode */
#define SCMI_TELEMETRY_CONFIG_CONTROL_MODE_POS 1
#define SCMI_TELEMETRY_CONFIG_CONTROL_MODE_MASK \
    (UINT32_C(0xF) << SCMI_TELEMETRY_CONFIG_CONTROL_MODE_POS)

/*!
 * \brief Extracts telemetry control mode from configuration flags.
 *
 * \param[in] CONTROL_VAL The control value containing mode settings.
 *
 * \return The extracted telemetry control mode.
 */
#define SCMI_TELEMETRY_CONFIG_SET_CONTROL_MODE(CONTROL_VAL) \
    (((CONTROL_VAL)&SCMI_TELEMETRY_CONFIG_CONTROL_MODE_MASK) >> \
     SCMI_TELEMETRY_CONFIG_CONTROL_MODE_POS)

/*! \brief Bit position and mask for configuring telemetry selector */
#define SCMI_TELEMETRY_CONFIG_CONTROL_ID_SELECTOR_POS 5
#define SCMI_TELEMETRY_CONFIG_CONTROL_ID_SELECTOR_MASK \
    (UINT32_C(0xF) << SCMI_TELEMETRY_CONFIG_CONTROL_ID_SELECTOR_POS)

/*!
 * \brief Extracts telemetry control id selector configuration flags.
 *
 * \param[in] CONTROL_VAL The control value containing mode settings.
 *
 * \return The extracted ID selector.
 */
#define SCMI_TELEMETRY_CONFIG_SET_ID_SELECTOR(CONTROL_VAL) \
    (((CONTROL_VAL)&SCMI_TELEMETRY_CONFIG_CONTROL_ID_SELECTOR_MASK) >> \
     SCMI_TELEMETRY_CONFIG_CONTROL_ID_SELECTOR_POS)

/*!
 * \brief Enumeration for Telemetry CONFIG_SET ID selector.
 *
 * Used to specify the ID selector interpretation.
 */
#define SCMI_TELEMETRY_CONFIG_CONTROL_ID_SELECTOR_NON_GROUP_DE 0U
#define SCMI_TELEMETRY_CONFIG_CONTROL_ID_SELECTOR_EVENT_GROUP  1U
#define SCMI_TELEMETRY_CONFIG_CONTROL_ID_SELECTOR_ALL          2U

/*!
 * \brief Constructs telemetry control settings.
 *
 * \param[in] MODE Telemetry mode to configure.
 * \param[in] EN   Enable (1) or disable (0) telemetry.
 *
 * \return Encoded control value for telemetry settings.
 */
#define SCMI_TELEMETRY_CONFIG_SET_CONTROL_FLAGS(MODE, EN) \
    ((((MODE) << SCMI_TELEMETRY_CONFIG_CONTROL_MODE_POS) & \
      SCMI_TELEMETRY_CONFIG_CONTROL_MODE_MASK) | \
     (((EN) << SCMI_TELEMETRY_CONFIG_CONTROL_EN_POS) & \
      SCMI_TELEMETRY_CONFIG_CONTROL_EN_MASK))

/*! \brief Bit position and mask for exponent in sampling rate */
#define SCMI_TELEMETRY_SAMPLING_RATE_EXP_POS      0
#define SCMI_TELEMETRY_SAMPLING_RATE_SIGN_BIT_POS 4
/*! Bit position of second field in 32 bit value */
#define SCMI_TELEMETRY_SAMPLING_RATE_SECONDS_POS 5U
/*! Number of bits for exponent field in a 32 bit interval value */
#define SCMI_TELEMETRY_SAMPLING_RATE_NUM_EXPONENT_BITS 5U
/*! Number of bits for second field in a 32 bit interval value */
#define SCMI_TELEMETRY_SAMPLING_RATE_NUM_SECONDS_BITS 16U

/*! Bitmask for exponent field */
#define SCMI_TELEMETRY_SAMPLING_RATE_EXP_MASK \
    (INT32_C(0x1F) << SCMI_TELEMETRY_SAMPLING_RATE_EXP_POS)
#define SCMI_TELEMETRY_SAMPLING_RATE_EXPONENT_SIGN_BIT \
    (INT32_C(0x1) << (SCMI_TELEMETRY_SAMPLING_RATE_NUM_EXPONENT_BITS - 1))

/*!
 * \brief Macros for setting telemetry sampling rate.
 *
 * Defines bit positions and masks for configuring the telemetry sampling rate.
 */
#define SCMI_TELEMETRY_SAMPLING_RATE_SEC_MASK \
    (UINT32_C(0xFFFF) << SCMI_TELEMETRY_SAMPLING_RATE_SECONDS_POS)

/*!
 * \brief Constructs the telemetry sampling rate value.
 *
 * \param[in] SEC Sampling rate in seconds.
 * \param[in] EXP Exponent for scaling the sampling rate.
 *
 * \return Encoded sampling rate value.
 */
#define SCMI_TELEMETRY_SAMPLING_RATE(SEC, EXP) \
    ((((SEC) << SCMI_TELEMETRY_SAMPLING_RATE_SECONDS_POS) & \
      SCMI_TELEMETRY_SAMPLING_RATE_SEC_MASK) | \
     (((EXP)) & SCMI_TELEMETRY_SAMPLING_RATE_EXP_MASK))

/*! Extract second field */
#define SCMI_TELEMETRY_SAMPLING_RATE_SECONDS(rate_value) \
    (((rate_value)&SCMI_TELEMETRY_SAMPLING_RATE_SEC_MASK) >> \
     SCMI_TELEMETRY_SAMPLING_RATE_SECONDS_POS)

/*!
 * \brief Enumeration for telemetry configuration control modes.
 *
 * Defines the possible operation modes for telemetry data collection.
 */
enum scmi_telemetry_config_control_modes {
    SCMI_TELEMETRY_CONFIG_CONTROL_MODE_SHMTI = 0, /*!< Use SHMTI */
    SCMI_TELEMETRY_CONFIG_CONTROL_MODE_NOTIFICATIONS =
        1, /*!< Use notifications */
    SCMI_TELEMETRY_CONFIG_CONTROL_MODE_ASYNC_READ = 2, /*!< Use async read */
};

/*!
 * \brief Request structure for telemetry configuration settings.
 *
 * This structure is used in SCMI requests to set telemetry configuration.
 */
struct scmi_telemetry_config_set_a2p {
    uint32_t group_identifier; /*!< Event Group identifier */
    uint32_t control; /*!< Encoded control flags (mode, enable/disable) */
    uint32_t sampling_rate; /*!< Encoded sampling rate value */
};

/*!
 * \brief Response structure for telemetry configuration settings.
 *
 * This structure is used in SCMI responses to confirm telemetry settings.
 */
struct scmi_telemetry_config_set_p2a {
    int32_t status; /*!< Status of the request (SCMI_SUCCESS, etc.) */
};

/*! \brief Bit position and mask for configuring telemetry selector */
#define SCMI_TELEMETRY_CONFIG_GET_FLAGS_ID_SELECTOR_POS 0
#define SCMI_TELEMETRY_CONFIG_GET_FLAGS_ID_SELECTOR_MASK \
    (UINT32_C(0xF) << SCMI_TELEMETRY_CONFIG_GET_FLAGS_ID_SELECTOR_POS)

/*!
 * \brief Extracts id selector from request configuration flags.
 *
 * \param[in] FLAGS The flags value containing ID selector .
 *
 * \return The extracted ID selector.
 */
#define SCMI_TELEMETRY_CONFIG_GET_ID_SELECTOR(FLAGS) \
    (((FLAGS)&SCMI_TELEMETRY_CONFIG_GET_FLAGS_ID_SELECTOR_MASK) >> \
     SCMI_TELEMETRY_CONFIG_GET_FLAGS_ID_SELECTOR_POS)

/*!
 * \brief Enumeration for Telemetry CONFIG_GET ID selector.
 *
 * Used to specify the ID selector interpretation.
 */
#define SCMI_TELEMETRY_CONFIG_GET_FLAGS_ID_SELECTOR_NON_GROUP_DE 0U
#define SCMI_TELEMETRY_CONFIG_GET_FLAGS_ID_SELECTOR_EVENT_GROUP  1U
#define SCMI_TELEMETRY_CONFIG_GET_FLAGS_ID_SELECTOR_ALL          2U

/*!
 * \brief Request structure for retrieving telemetry configuration.
 *
 * This structure is used in SCMI requests to get telemetry configuration.
 */
struct scmi_telemetry_config_get_a2p {
    uint32_t group_identifier; /*!< Event Group identifier */
    uint32_t flags; /*!< Encoded flags (ID selector) */
};

/*!
 * \brief Response structure for retrieving telemetry configuration.
 *
 * This structure is used in SCMI responses to fetch the current
 * telemetry configuration settings.
 */
struct scmi_telemetry_config_get_p2a {
    int32_t status; /*!< Status of the request */
    uint32_t control; /*!< Current control settings */
    uint32_t sampling_rate; /*!< Current sampling rate settings */
};

/*!
 * \brief Request structure for resetting telemetry.
 *
 * This structure is used in SCMI request to reset telemetry.
 */
struct scmi_telemetry_reset_a2p {
    uint32_t flags; /*!< Reserved, Must be 0. */
};

/*!
 * \brief Response structure for resetting telemetry.
 *
 * This structure is used in SCMI responses to reset the telemetry.
 */
struct scmi_telemetry_reset_p2a {
    int32_t status; /*!< Status of the request */
};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* INTERNAL_SCMI_TELEMETRY_H */
