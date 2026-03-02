// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021-2024, Linaro Limited
 */

#include <assert.h>
#include <bitstring.h>
#include <config.h>
#include <initcall.h>
#include <kernel/interrupt.h>
#include <kernel/notif.h>
#include <kernel/spinlock.h>
#include <kernel/virtualization.h>
#include <sm/optee_smc.h>
#include <trace.h>
#include <types_ext.h>

/* Async notif ID related to interrupt event triggered in non-secure world */
#define NOTIF_VALUE_DO_IT		1

static_assert(NOTIF_VALUE_DO_IT == OPTEE_SMC_ASYNC_NOTIF_VALUE_DO_IT);

struct notif_vm_bitmap {
	bool alloc_values_inited;
	bitstr_t bit_decl(values, NOTIF_ASYNC_VALUE_MAX + 1);
	bitstr_t bit_decl(alloc_values, NOTIF_ASYNC_VALUE_MAX + 1);

	bitstr_t bit_decl(it_pending, NOTIF_IT_VALUE_MAX + 1);
	bitstr_t bit_decl(it_masked, NOTIF_IT_VALUE_MAX + 1);
};

static unsigned int notif_default_lock = SPINLOCK_UNLOCK;
static unsigned int it_lock = SPINLOCK_UNLOCK;

/* Id used to look up the guest specific struct notif_vm_bitmap */
static unsigned int notif_vm_bitmap_id __nex_bss;
/* Notification state when ns-virtualization isn't enabled */
static struct notif_vm_bitmap default_notif_vm_bitmap;

static struct notif_vm_bitmap *get_notif_vm_bitmap(struct guest_partition *prtn)
{
	if (IS_ENABLED(CFG_NS_VIRTUALIZATION)) {
		if (!prtn)
			return NULL;
		return virt_get_guest_spec_data(prtn, notif_vm_bitmap_id);
	}
	return &default_notif_vm_bitmap;
}

TEE_Result notif_alloc_async_value(uint32_t *val)
{
	struct guest_partition *prtn = NULL;
	struct notif_vm_bitmap *nvb = NULL;
	TEE_Result res = TEE_SUCCESS;
	uint32_t old_itr_status = 0;
	int bit = 0;

	assert(interrupt_can_raise_pi(interrupt_get_main_chip()));

	prtn = virt_get_current_guest();
	nvb = get_notif_vm_bitmap(prtn);
	if (!nvb) {
		res = TEE_ERROR_BAD_PARAMETERS;
		goto out;
	}

	old_itr_status = cpu_spin_lock_xsave(&notif_default_lock);

	if (!nvb->alloc_values_inited) {
		bit_set(nvb->alloc_values, NOTIF_VALUE_DO_BOTTOM_HALF);
		bit_set(nvb->alloc_values, NOTIF_VALUE_DO_IT);
		bit_nset(nvb->it_masked, 0, (int)NOTIF_IT_VALUE_MAX);
		nvb->alloc_values_inited = true;
	}

	bit_ffc(nvb->alloc_values, (int)NOTIF_ASYNC_VALUE_MAX + 1, &bit);
	if (bit < 0) {
		res = TEE_ERROR_OUT_OF_MEMORY;
		goto out_unlock;
	}
	*val = bit;
	bit_set(nvb->alloc_values, bit);

out_unlock:
	cpu_spin_unlock_xrestore(&notif_default_lock, old_itr_status);
out:
	virt_put_guest(prtn);

	return res;
}

void notif_free_async_value(uint32_t val)
{
	struct guest_partition *prtn = NULL;
	struct notif_vm_bitmap *nvb = NULL;
	uint32_t old_itr_status = 0;

	prtn = virt_get_current_guest();
	nvb = get_notif_vm_bitmap(prtn);
	if (!nvb)
		goto out;

	old_itr_status = cpu_spin_lock_xsave(&notif_default_lock);

	assert(val < NOTIF_ASYNC_VALUE_MAX);
	assert(bit_test(nvb->alloc_values, val));
	bit_clear(nvb->alloc_values, val);

	cpu_spin_unlock_xrestore(&notif_default_lock, old_itr_status);
out:
	virt_put_guest(prtn);
}

uint32_t notif_get_value(bool *value_valid, bool *value_pending)
{
	struct guest_partition *prtn = NULL;
	struct notif_vm_bitmap *nvb = NULL;
	uint32_t old_itr_status = 0;
	uint32_t res = 0;
	int bit = -1;

	prtn = virt_get_current_guest();
	nvb = get_notif_vm_bitmap(prtn);
	if (!nvb) {
		*value_valid = false;
		goto out;
	}

	old_itr_status = cpu_spin_lock_xsave(&notif_default_lock);

	bit_ffs(nvb->values, (int)NOTIF_ASYNC_VALUE_MAX + 1, &bit);
	*value_valid = (bit >= 0);
	if (!*value_valid)
		goto out_unlock;

	res = bit;
	bit_clear(nvb->values, res);
	bit_ffs(nvb->values, (int)NOTIF_ASYNC_VALUE_MAX + 1, &bit);

out_unlock:
	cpu_spin_unlock_xrestore(&notif_default_lock, old_itr_status);
out:
	virt_put_guest(prtn);
	*value_pending = (bit >= 0);

	return res;
}

uint32_t it_get_value(bool *value_valid, bool *value_pending)
{
	struct guest_partition *prtn = NULL;
	struct notif_vm_bitmap *nvb = NULL;
	uint32_t old_itr_status = 0;
	uint32_t res = 0;
	int bit = 0;

	prtn = virt_get_current_guest();
	if (virt_get_guest_id(prtn) == HYP_CLNT_ID) {
		nvb = get_notif_vm_bitmap(prtn);
	} else {
		DMSG("Interrupt notif to NS guest not supported");
	}
	if (!nvb) {
		*value_valid = false;
		goto out;
	}

	old_itr_status = cpu_spin_lock_xsave(&it_lock);

	while (1) {
		bit_ffs_from(nvb->it_pending, (int)NOTIF_IT_VALUE_MAX, bit,
			     &bit);
		if (bit < 0 || !bit_test(nvb->it_masked, bit))
			break;

		++bit;
	};

	*value_valid = (bit >= 0);
	if (!*value_valid) {
		*value_pending = false;
		goto out_unlock;
	}

	res = bit;
	bit_clear(nvb->it_pending, res);

	while (1) {
		bit_ffs_from(nvb->it_pending, (int)NOTIF_IT_VALUE_MAX, bit,
			     &bit);
		if (bit < 0 || !bit_test(nvb->it_masked, bit))
			break;

		++bit;
	};

	*value_pending = (bit >= 0);

out_unlock:
	cpu_spin_unlock_xrestore(&it_lock, old_itr_status);
out:
	virt_put_guest(prtn);

	return res;
}

uint32_t it_set_mask(uint32_t it_number, bool masked)
{
	struct guest_partition *prtn = NULL;
	struct notif_vm_bitmap *nvb = NULL;
	uint32_t old_itr_status = 0;

	prtn = virt_get_current_guest();
	if (virt_get_guest_id(prtn) == HYP_CLNT_ID) {
		nvb = get_notif_vm_bitmap(prtn);
	} else {
		DMSG("Interrupt notif (%"PRIu32") to NS guest not supported",
		     it_number);
	}
	if (!nvb)
		goto out;

	old_itr_status = cpu_spin_lock_xsave(&it_lock);

	DMSG("it_number=%"PRIu32" masked=%u", it_number, masked);

	if (it_number >= NOTIF_IT_VALUE_MAX)
		goto out_unlock;

	if (masked) {
		bit_set(nvb->it_masked, it_number);
	} else {
		bit_clear(nvb->it_masked, it_number);

		if (bit_test(nvb->it_pending, it_number))
			notif_send_async(NOTIF_VALUE_DO_IT,
					 virt_get_guest_id(prtn));
	}

out_unlock:

	cpu_spin_unlock_xrestore(&it_lock, old_itr_status);
out:
	virt_put_guest(prtn);

	return 0;
}

void notif_send_it(uint32_t it_number, uint16_t guest_id)
{
	struct guest_partition *prtn = NULL;
	struct notif_vm_bitmap *nvb = NULL;
	uint32_t old_itr_status = 0;

	if (guest_id != HYP_CLNT_ID) {
		EMSG("Interrupt notif (%"PRIu32") to NS guest not supported",
		     it_number);
		return;
	}

	prtn = virt_get_guest(guest_id);
	nvb = get_notif_vm_bitmap(prtn);
	if (!nvb)
		goto out;

	assert(it_number <= NOTIF_IT_VALUE_MAX);
	old_itr_status = cpu_spin_lock_xsave(&it_lock);

	DMSG("0x%"PRIx32, it_number);
	bit_set(nvb->it_pending, it_number);

	if (!bit_test(nvb->it_masked, it_number))
		notif_send_async(NOTIF_VALUE_DO_IT, virt_get_guest_id(prtn));

	cpu_spin_unlock_xrestore(&it_lock, old_itr_status);

out:
	virt_put_guest(prtn);
}

void notif_send_async(uint32_t value, uint16_t guest_id)
{
	struct guest_partition *prtn = NULL;
	struct notif_vm_bitmap *nvb = NULL;
	uint32_t old_itr_status = 0;
	struct itr_chip *itr_chip = interrupt_get_main_chip();

	assert(value <= NOTIF_ASYNC_VALUE_MAX);

	prtn = virt_get_guest(guest_id);
	nvb = get_notif_vm_bitmap(prtn);
	if (!nvb)
		goto out;

	old_itr_status = cpu_spin_lock_xsave(&notif_default_lock);

	bit_set(nvb->values, value);
	interrupt_raise_pi(itr_chip, CFG_CORE_ASYNC_NOTIF_GIC_INTID);

	cpu_spin_unlock_xrestore(&notif_default_lock, old_itr_status);
out:
	virt_put_guest(prtn);
}

static TEE_Result notif_init(void)
{
	if (IS_ENABLED(CFG_NS_VIRTUALIZATION) &&
	    virt_add_guest_spec_data(&notif_vm_bitmap_id,
				     sizeof(struct notif_vm_bitmap), NULL))
		panic("virt_add_guest_spec_data");
	return TEE_SUCCESS;
}
nex_service_init(notif_init);
