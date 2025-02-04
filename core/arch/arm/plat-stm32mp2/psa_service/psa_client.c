// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2025, STMicroelectronics
 */

#include <drivers/mailbox.h>
#include <drivers/stm32mp_dt_bindings.h>
#include <initcall.h>
#include <kernel/dt.h>
#include <kernel/misc.h>
#include <libfdt.h>
#include <mm/core_memprot.h>
#include <rse_comms.h>
#include <trace.h>
#include <util.h>

#define ST_PSA_CLIENT_COMPAT	"st,psa-client"

/* Arbitrary default Client */
#define ST_PSA_CLIENT_ID (-19)

static struct mbox_chan *handle_send_receive;

struct stm32_rse_shmem {
	uint32_t size;
	uint8_t payload[];
};

static struct stm32_rse_shmem *shmem;
static size_t shmem_size;

static int rse_mbox_send_data(const uint8_t *send_buffer, size_t size)
{
	TEE_Result res = TEE_ERROR_NO_DATA;

	memcpy(shmem->payload, send_buffer, size);
	shmem->size = size;
	res = mbox_send(handle_send_receive, false, NULL, 0);
	if (res != TEE_SUCCESS) {
		EMSG("Send/receive IPCC : send failed");
		return res;
	}
	return 0;
}

static int rse_mbox_rcv_data(const uint8_t *rcv_buffer, size_t *size)
{
	TEE_Result res = TEE_ERROR_NO_DATA;

	do {
		res = mbox_recv(handle_send_receive, false, NULL, 0);
	} while (res == TEE_ERROR_NO_DATA);
	if (res != TEE_SUCCESS) {
		EMSG("Send/receive : failed to receive message");
		return res;
	}
	*size = shmem->size;
	memcpy((void *)rcv_buffer, shmem->payload, *size);
	return 0;
}

static int rse_mbox_size_data(size_t  *size)
{
	*size = shmem_size;
	if (!shmem_size)
		return -1;
	else
		return 0;
}

static TEE_Result stm32mp_start_psa_service(void)
{
	TEE_Result res = TEE_SUCCESS;
	void *fdt = NULL;
	const fdt32_t *prop = NULL;
	int node = 0;
	int len = 0;
	int pnode = 0;
	paddr_t pa = 0;

	fdt = get_embedded_dt();
	node = fdt_node_offset_by_compatible(fdt, 0, ST_PSA_CLIENT_COMPAT);
	if (node < 0)
		/*  Test not activated in this dt configuration */
		return TEE_SUCCESS;

	if (fdt_get_status(fdt, node) == DT_STATUS_DISABLED)
		return TEE_SUCCESS;
	res = mbox_dt_register_chan_by_name(NULL, NULL, NULL, fdt, node,
					    "psa_tfm",
					    &handle_send_receive);
	if (res) {
		DMSG("No PSA mailbox channel %d", res);
		return TEE_ERROR_DEFER_DRIVER_INIT;
	}

	prop = fdt_getprop(fdt, node, "shmem", &len);
	if (!prop) {
		EMSG("No Shared memory");
		panic();
	}

	pnode = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(prop[0]));
	if (pnode > 0) {
		pa = fdt_reg_base_address(fdt, pnode);
		shmem_size = fdt_reg_size(fdt, pnode);
		shmem = core_mmu_add_mapping(MEM_AREA_IO_SEC, pa, shmem_size);
	}
	if (!shmem) {
		EMSG("No Shared memory");
		panic();
	}

	rse_register_cb(rse_mbox_send_data, rse_mbox_rcv_data,
			rse_mbox_size_data);
	/*  set a default client id */
	rse_set_client_id(ST_PSA_CLIENT_ID);

	return res;
}
service_init_crypto(stm32mp_start_psa_service);
