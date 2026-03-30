/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Telemetry SHMTI Manager (Shared Memory Telemetry Interface).
 *
 */

#include <internal/telemetry.h>

#include <fwk_log.h>
#include <fwk_mm.h>

/* SHMTI (Shared Memory Telemetry Interface) Management */
#define BITS_PER_BYTE                   8
#define BITMAP_ALLOC_BLOCK_SIZE         8
#define BIT_POS(bit_num)                ((bit_num) % (BITS_PER_BYTE))
#define BITMAP_IDX(bit_num)             ((bit_num) / (BITS_PER_BYTE))
#define BIT_NUM_FOR_BYTE_OFFSET(offset) ((offset) / (BITMAP_ALLOC_BLOCK_SIZE))
#define BITMAP_GET_MIN_BITS_FOR_SIZE(size) \
    ((FWK_ALIGN_TO_PREVIOUS_PWR_OF_2(size, BITMAP_ALLOC_BLOCK_SIZE)) / \
     (BITMAP_ALLOC_BLOCK_SIZE))
#define BITMAP_GET_MAX_BITS_FOR_SIZE(size) \
    ((FWK_ALIGN_TO_NEXT_PWR_OF_2(size, BITMAP_ALLOC_BLOCK_SIZE)) / \
     (BITMAP_ALLOC_BLOCK_SIZE))
#define BITMAP_GET_MIN_BYTES_FOR_BIT_LEN(num_bits) \
    (((num_bits) + (BITS_PER_BYTE - 1)) / (BITS_PER_BYTE))

/* Macros to set and clear bits in the allocation bitmap */
#define BITMAP_SET_BITS(bitmap_byte, bit_pos_begin, bit_pos_end) \
    ((bitmap_byte) |= \
     ((0xFF >> (7 - (bit_pos_end))) & (0xFF << (bit_pos_begin))))

#define BITMAP_CLEAR_BITS(bitmap_byte, bit_pos_begin, bit_pos_end) \
    ((bitmap_byte) &= \
     ~(((0xFF >> (7 - (bit_pos_end))) & (0xFF << (bit_pos_begin)))))

#define QWORD_SIZE                   (8U)
#define SHMTI_HEADER_METADATA_OFFSET (8U)
#define SHMTI_HEADER_METADATA_SIZE   (8U)
#define SHMTI_MATCH_SEQUENCE_SIZE    (8U)

/* Macro to write SHMTI header metadata */
#define WRITE_SHMTI_HEADER_METADATA(header_address, N_QWORDS) \
    (*((uint32_t *)(header_address)) = (N_QWORDS))

/* Helper functions */
/*
 * Searches the free space in [search_region_start_bit, search_region_end_bit].
 */
static int shmti_bitmap_check_for_free(
    uint8_t *allocation_bitmap,
    size_t required_bits,
    size_t bitmap_len,
    uint32_t search_region_start_bit,
    uint32_t search_region_end_bit,
    uint32_t *allocated_region_start_bit)
{
    size_t remaining_bits_needed = required_bits;
    uint32_t index, bit_pos;

    /* Validate input parameters */
    if (allocation_bitmap == NULL || allocated_region_start_bit == NULL) {
        return FWK_E_PARAM;
    }

    /* Ensure Search range is valid and requested space does not exceed bitmap
     * length
     */
    if (search_region_start_bit > search_region_end_bit ||
        remaining_bits_needed > bitmap_len) {
        return FWK_E_PARAM;
    }

    while (search_region_start_bit <= search_region_end_bit) {
        index = BITMAP_IDX(search_region_start_bit);
        bit_pos = BIT_POS(search_region_start_bit);

        /* Check if the current bit is free */
        if (!(allocation_bitmap[index] & (0x1 << bit_pos))) {
            /* Start tracking free block */
            if (remaining_bits_needed == required_bits) {
                *allocated_region_start_bit = search_region_start_bit;
            }
            remaining_bits_needed -= 1;
        } else {
            /* Reset counter if an allocated bit is found */
            remaining_bits_needed = required_bits;
        }

        search_region_start_bit++;

        /* If we found enough contiguous free bits, mark end and return success
         */
        if (remaining_bits_needed == 0) {
            return FWK_SUCCESS;
        }
    }

    /* No free block of required size found */
    return FWK_E_NOMEM;
}

/*
 * Mark the space in [start_bit_num, end_bit_num] as used.
 */
static void shmti_bitmap_mark_used(
    uint8_t *bitmap,
    uint32_t start_bit_num,
    uint32_t end_bit_num,
    size_t bitmap_len)
{
    uint32_t start_bitmap_index, end_bitmap_index;
    uint32_t start_bit_pos, end_bit_pos;

    /* Validate parameters to prevent out-of-bounds access */
    if (bitmap == NULL || start_bit_num > end_bit_num ||
        end_bit_num >= bitmap_len) {
        return;
    }

    /* Compute byte indices and bit positions */
    start_bitmap_index = BITMAP_IDX(start_bit_num);
    end_bitmap_index = BITMAP_IDX(end_bit_num);
    start_bit_pos = BIT_POS(start_bit_num);
    end_bit_pos = BIT_POS(end_bit_num);

    /* If the start and end bits are in the same byte, set bits directly */
    if (start_bitmap_index == end_bitmap_index) {
        BITMAP_SET_BITS(bitmap[start_bitmap_index], start_bit_pos, end_bit_pos);
        return;
    }

    /* Mark bits in the first byte from start_bit_pos to the end of the byte */
    BITMAP_SET_BITS(
        bitmap[start_bitmap_index], start_bit_pos, BITMAP_ALLOC_BLOCK_SIZE - 1);
    start_bitmap_index++;

    /* Set all bits in full bytes between start and end indices */
    while (start_bitmap_index < end_bitmap_index) {
        bitmap[start_bitmap_index++] = 0xFF;
    }

    /* Mark bits in the last byte from bit 0 to end_bit_pos */
    BITMAP_SET_BITS(bitmap[end_bitmap_index], 0, end_bit_pos);
}

static void shmti_bitmap_mark_free(
    uint8_t *bitmap,
    uint32_t start_bit_num,
    uint32_t end_bit_num,
    size_t bitmap_len)
{
    uint32_t start_bitmap_index, end_bitmap_index;
    uint32_t start_bit_pos, end_bit_pos;

    /* Validate parameters to prevent out-of-bounds access */
    if (bitmap == NULL || start_bit_num > end_bit_num ||
        end_bit_num >= bitmap_len) {
        return;
    }

    /* Compute byte indices and bit positions */
    start_bitmap_index = BITMAP_IDX(start_bit_num);
    end_bitmap_index = BITMAP_IDX(end_bit_num);
    start_bit_pos = BIT_POS(start_bit_num);
    end_bit_pos = BIT_POS(end_bit_num);

    /* If the start and end bits are in the same byte, clear bits directly */
    if (start_bitmap_index == end_bitmap_index) {
        BITMAP_CLEAR_BITS(
            bitmap[start_bitmap_index], start_bit_pos, end_bit_pos);
        return;
    }

    /* Clear bits in the first byte from start_bit_pos to the end of the byte */
    BITMAP_CLEAR_BITS(bitmap[start_bitmap_index], start_bit_pos, 7);
    start_bitmap_index++;

    /* Clear all bits in full bytes between start and end indices */
    while (start_bitmap_index < end_bitmap_index) {
        bitmap[start_bitmap_index++] = 0x00;
    }

    /* Clear bits in the last byte from bit 0 to end_bit_pos */
    BITMAP_CLEAR_BITS(bitmap[end_bitmap_index], 0, end_bit_pos);
}

static int shmti_init_bitmap(
    uint8_t **bitmap,
    size_t mem_size,
    uint32_t *bitmap_len)
{
    /* Validate input parameters */
    if (bitmap == NULL || bitmap_len == NULL || mem_size == 0) {
        return FWK_E_PARAM;
    }

    /* Calculate bitmap length in bits, ensuring alignment */
    *bitmap_len = BITMAP_GET_MIN_BITS_FOR_SIZE(mem_size);

    /* Allocate memory for the bitmap */
    *bitmap = fwk_mm_alloc(1, BITMAP_GET_MIN_BYTES_FOR_BIT_LEN(*bitmap_len));
    if (*bitmap == NULL) {
        return FWK_E_NOMEM;
    }

    /* Initialize the bitmap to zero (all memory blocks are free) */
    memset(*bitmap, 0, *bitmap_len);

    return FWK_SUCCESS;
}

int shmti_create(struct telemetry_shmti_context *shmti_ctx)
{
    int status;

    /* Validate input */
    if (shmti_ctx == NULL || shmti_ctx->shmti_info == NULL) {
        return FWK_E_PARAM;
    }

    /* Initialize the bitmap for tracking memory allocation */
    status = shmti_init_bitmap(
        &shmti_ctx->allocation_map,
        shmti_ctx->shmti_info->length,
        &shmti_ctx->bitmap_len);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /*!
     * Mark the beginning of the SHMTI region as used for metadata storage.
     * This includes the header and the start match sequence.
     */
    shmti_bitmap_mark_used(
        shmti_ctx->allocation_map,
        0,
        BITMAP_GET_MIN_BITS_FOR_SIZE(
            SHMTI_MATCH_SEQUENCE_SIZE + SHMTI_HEADER_METADATA_SIZE) -
            1,
        shmti_ctx->bitmap_len);
    /* Compute the end sequence address */
    shmti_ctx->end_seq_addr = shmti_ctx->shmti_info->start_addr +
        FWK_ALIGN_TO_PREVIOUS_PWR_OF_2(
                                  shmti_ctx->shmti_info->length, QWORD_SIZE) -
        SHMTI_MATCH_SEQUENCE_SIZE;

    /*!
     * Mark the last QWORD of the SHMTI region as used for the end match
     * sequence.
     */
    shmti_bitmap_mark_used(
        shmti_ctx->allocation_map,
        shmti_ctx->bitmap_len -
            BITMAP_GET_MIN_BITS_FOR_SIZE(SHMTI_MATCH_SEQUENCE_SIZE),
        shmti_ctx->bitmap_len - 1,
        shmti_ctx->bitmap_len);

    /* Zero down Prologue and Epilogue. */
    memset(
        (void *)shmti_ctx->shmti_info->start_addr,
        0,
        SHMTI_HEADER_METADATA_SIZE + SHMTI_MATCH_SEQUENCE_SIZE);

    memset((void *)shmti_ctx->end_seq_addr, 0, SHMTI_MATCH_SEQUENCE_SIZE);

    /* Write metadata header in SHMTI memory */
    WRITE_SHMTI_HEADER_METADATA(
        shmti_ctx->shmti_info->start_addr + SHMTI_HEADER_METADATA_OFFSET,
        FWK_ALIGN_TO_PREVIOUS_PWR_OF_2(
            shmti_ctx->shmti_info->length, QWORD_SIZE) /
            QWORD_SIZE);

    /* Mark SHMTI as successfully created */
    shmti_ctx->shmti_created = true;

    return FWK_SUCCESS;
}

int shmti_alloc_pool(
    struct telemetry_shmti_context *shmti_ctx,
    size_t size,
    uint32_t *offset)
{
    int status;
    uint32_t start_bit_num = 0, end_bit_num;

    /* Validate input parameters */
    if (size == 0 || offset == NULL) {
        return FWK_E_PARAM;
    }

    /* Ensure that the SHMTI context is properly initialized */
    if (!shmti_ctx->shmti_created) {
        return FWK_E_STATE;
    }

    /* Attempt to find a contiguous free block in the bitmap */
    status = shmti_bitmap_check_for_free(
        shmti_ctx->allocation_map,
        BITMAP_GET_MAX_BITS_FOR_SIZE(size),
        shmti_ctx->bitmap_len,
        0,
        shmti_ctx->bitmap_len,
        &start_bit_num);

    if (status != FWK_SUCCESS) {
        return FWK_E_NOMEM;
    }

    end_bit_num = start_bit_num + BITMAP_GET_MAX_BITS_FOR_SIZE(size) - 1;
    /* Mark the allocated space as used */
    shmti_bitmap_mark_used(
        shmti_ctx->allocation_map,
        start_bit_num,
        end_bit_num,
        shmti_ctx->bitmap_len);

    /* Compute the allocated memory address */
    *offset = start_bit_num * 8;
    return FWK_SUCCESS;
}

int shmti_free_pool(
    struct telemetry_shmti_context *shmti_ctx,
    size_t size,
    uint32_t offset)
{
    uint32_t start_bit_num, end_bit_num;

    if (size == 0) {
        return FWK_E_PARAM;
    }

    if (!shmti_ctx->shmti_created) {
        return FWK_E_STATE;
    }

    if (offset >= shmti_ctx->shmti_info->length) {
        return FWK_E_PARAM;
    }

    /* Compute start and end offsets in the bitmap */
    start_bit_num = BIT_NUM_FOR_BYTE_OFFSET(offset);
    end_bit_num = BIT_NUM_FOR_BYTE_OFFSET(offset + size) - 1;

    /* Mark the memory block as free */
    shmti_bitmap_mark_free(
        shmti_ctx->allocation_map,
        start_bit_num,
        end_bit_num,
        shmti_ctx->bitmap_len);

    return FWK_SUCCESS;
}

int shmti_seq_inc(const struct telemetry_shmti_context *shmti_ctx)
{
    uint64_t *start_seq_addr;
    uint64_t *end_seq_addr;

    /* Get the memory addresses for start and end sequence markers */
    start_seq_addr = (uint64_t *)shmti_ctx->shmti_info->start_addr;
    end_seq_addr = (uint64_t *)shmti_ctx->end_seq_addr;

    /* Ensure valid memory addresses before modifying values */
    if (start_seq_addr == NULL || end_seq_addr == NULL) {
        return FWK_E_PARAM;
    }

    /*!
     * Increment both start and end sequence numbers.
     * This ensures the telemetry system can detect changes in data updates.
     */
    (*start_seq_addr)++;
    (*end_seq_addr)++;

    return FWK_SUCCESS;
}
