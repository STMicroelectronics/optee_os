// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2023, STMicroelectronics
 */

#include <assert.h>
#include <config.h>
#include <drivers/firewall_device.h>
#include <drivers/rstctrl.h>
#include <drivers/stm32_remoteproc.h>
#ifdef CFG_STM32MP15
#include <drivers/stm32mp1_rcc.h>
#endif
#include <kernel/cache_helpers.h>
#include <kernel/dt_driver.h>
#include <kernel/tee_misc.h>
#include <libfdt.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <stdint.h>
#if defined(CFG_STM32MP25) || defined(CFG_STM32MP23) || defined(CFG_STM32MP21)
#include <stm32_sysconf.h>
#endif
#include <stm32_util.h>

#define TIMEOUT_US_1MS	U(1000)

#define INITVTOR_MASK	GENMASK_32(31, 7)

#define FIREWALL_CONF_DEFAULT "default"
#define FIREWALL_CONF_LOAD    "load"

enum reset_id {
	MCU_RESET,
	MCU_HOLD_BOOT,
	NB_RESET_ID,
};

/**
 * struct stm32_rproc_mem - Memory regions used by the remote processor
 *
 * @addr:	physical base address from the CPU space perspective
 * @da:		device address corresponding to the physical base address
 *		from remote processor space perspective
 * @size:	size of the region
 * @default_conf: default firewall configuration applied on memory
 * @load_conf:	  firewall configuration applied on memory to give access to the
 *		  TEE remoteproc framework to load the firmware
 */
struct stm32_rproc_mem {
	paddr_t addr;
	paddr_t da;
	size_t size;
	struct firewall_alt_conf *default_conf;
	struct firewall_alt_conf *load_conf;
};

struct stm32_rproc_instance;

/**
 * struct stm32_rproc_reset - Reset line description, exposed to non secure
 *
 * @id:		reset identifier
 * @rproc:	remoteproc device pointer (stm32_rproc_instance)
 * @rstctrl:	backend reset control to use, when access is granted
 */
struct stm32_rproc_reset {
	unsigned int id;
	const struct stm32_rproc_instance *rproc;
	struct rstctrl rstctrl;
};

/**
 * struct stm32_rproc_instance - rproc instance context
 *
 * @cdata:	pointer to the device compatible data
 * @fdt:	device tree file to work on
 * @link:	the node in the rproc_list
 * @n_regions:	number of memory regions
 * @regions:	memory regions used
 * @mcu_rst:	remote processor reset control
 * @hold_boot:	remote processor hold boot control
 * @m33_cr_right: access rights for the M33 subsystem control registers
 * @boot_addr:	boot address
 * @tzen:	indicate if the remote processor should enable the TrustZone
 * @m_get_cnt:	counter used for the memory region get/release balancing
 * @reset:	array of reset line description (stm32_rproc_reset)
 */
struct stm32_rproc_instance {
	const struct stm32_rproc_compat_data *cdata;
	const void *fdt;
	SLIST_ENTRY(stm32_rproc_instance) link;
	size_t n_regions;
	struct stm32_rproc_mem *regions;
	struct rstctrl *mcu_rst;
	struct rstctrl *hold_boot;
	uint32_t m33_cr_right;
	paddr_t boot_addr;
	bool tzen;
	uint32_t m_get_cnt;
	struct stm32_rproc_reset reset[NB_RESET_ID];
};

/**
 * struct stm32_rproc_compat_data - rproc associated data for compatible list
 *
 * @rproc_id:	identifies the remote processor
 * @start:	remote processor start routine
 * @ns_loading: True if supports non-secure firmware loading, false otherwise
 */
struct stm32_rproc_compat_data {
	uint32_t rproc_id;
	TEE_Result (*start)(struct stm32_rproc_instance *rproc);
	bool ns_loading;
};

static SLIST_HEAD(, stm32_rproc_instance) rproc_list =
		SLIST_HEAD_INITIALIZER(rproc_list);

void *stm32_rproc_get(uint32_t rproc_id)
{
	struct stm32_rproc_instance *rproc = NULL;

	SLIST_FOREACH(rproc, &rproc_list, link)
		if (rproc->cdata->rproc_id == rproc_id)
			break;

	return rproc;
}

bool stm32_rproc_is_secure(uint32_t rproc_id)
{
	struct stm32_rproc_instance *rproc = stm32_rproc_get(rproc_id);

	if (rproc)
		return !rproc->cdata->ns_loading;

	return false;
}

/* Re-apply default access right on the memory regions */
static TEE_Result
stm32_rproc_release_mems_access(struct stm32_rproc_instance *rproc)
{
	struct stm32_rproc_mem *mems = rproc->regions;
	TEE_Result res = TEE_ERROR_GENERIC;
	unsigned int i = 0;

	rproc->m_get_cnt--;
	if (rproc->m_get_cnt)
		return TEE_SUCCESS;

	for (i = 0; i < rproc->n_regions; i++) {
		DMSG("Release access of the memory region %#"PRIxPA" size %#zx",
		     mems[i].addr, mems[i].size);
		res = firewall_set_memory_alternate_conf(mems[i].default_conf,
							 mems[i].addr,
							 mems[i].size);
		if (res) {
			EMSG("Failed to apply access rights on region %#"
			     PRIxPA" size %#zx", mems[i].addr, mems[i].size);
			return res;
		}
	}

	return TEE_SUCCESS;
}

/* Set the firewall configuration allowing to load the remoteproc firmware */
static TEE_Result
stm32_rproc_get_mems_access(struct stm32_rproc_instance *rproc)
{
	struct stm32_rproc_mem *mems = rproc->regions;
	TEE_Result res = TEE_ERROR_GENERIC;
	unsigned int i = 0;

	rproc->m_get_cnt++;
	if (rproc->m_get_cnt > 1)
		return TEE_SUCCESS;

	for (i = 0; i < rproc->n_regions; i++) {
		DMSG("Get access of the memory region %#"PRIxPA" size %#zx",
		     mems[i].addr, mems[i].size);
		res = firewall_set_memory_alternate_conf(mems[i].load_conf,
							 mems[i].addr,
							 mems[i].size);
		if (res) {
			EMSG("Failed to apply access rights on region %#"
			     PRIxPA" size %#zx", mems[i].addr, mems[i].size);
			goto err;
		}
	}

	return TEE_SUCCESS;

err:
	if (stm32_rproc_release_mems_access(rproc) != TEE_SUCCESS)
		panic();

	return res;
}

static TEE_Result stm32mp2_rproc_start(struct stm32_rproc_instance *rproc)
{
	struct stm32_rproc_mem *mems = NULL;
	unsigned int i = 0;
	bool boot_addr_valid = false;

	if (!rproc->boot_addr)
		return TEE_ERROR_GENERIC;

	mems = rproc->regions;

	/* Check that the boot address is in declared regions */
	for (i = 0; i < rproc->n_regions; i++) {
		if (!core_is_buffer_inside(rproc->boot_addr, 1, mems[i].addr,
					   mems[i].size))
			continue;

#if defined(CFG_STM32MP25) || defined(CFG_STM32MP23) || defined(CFG_STM32MP21)
		if (rproc->tzen) {
			stm32mp_syscfg_write(CA35SS_SYSCFG_M33_INITSVTOR_CR,
					     rproc->boot_addr, INITVTOR_MASK);

			stm32mp_syscfg_write(CA35SS_SYSCFG_M33_TZEN_CR,
					     CA35SS_SYSCFG_M33_TZEN_CR_CFG_SECEXT,
					     CA35SS_SYSCFG_M33_TZEN_CR_CFG_SECEXT);
		} else {
			stm32mp_syscfg_write(CA35SS_SYSCFG_M33_INITNSVTOR_CR,
					     rproc->boot_addr, INITVTOR_MASK);
		}
		boot_addr_valid = true;
		break;
#endif
	}

	if (!boot_addr_valid) {
		EMSG("Invalid boot address");
		return TEE_ERROR_GENERIC;
	}

	return TEE_SUCCESS;
}

TEE_Result stm32_rproc_start(uint32_t rproc_id)
{
	struct stm32_rproc_instance *rproc = stm32_rproc_get(rproc_id);
	TEE_Result res = TEE_ERROR_GENERIC;

	if (!rproc || !rproc->hold_boot)
		return TEE_ERROR_GENERIC;

	if (rproc->cdata->start) {
		res = rproc->cdata->start(rproc);
		if (res)
			return res;
	}

	/*
	 * The firmware is started by de-asserting the hold boot and
	 * asserting it back to avoid auto restart on a crash.
	 * No need to release the MCU reset as it is automatically released by
	 * the hardware.
	 */
	res = rstctrl_deassert_to(rproc->hold_boot, TIMEOUT_US_1MS);
	if (!res)
		res = rstctrl_assert_to(rproc->hold_boot, TIMEOUT_US_1MS);

	return res;
}

static TEE_Result rproc_stop(struct stm32_rproc_instance *rproc)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	if (!rproc->hold_boot || !rproc->mcu_rst)
		return TEE_ERROR_GENERIC;

	res = rstctrl_assert_to(rproc->hold_boot, TIMEOUT_US_1MS);
	if (res)
		return res;
	res = rstctrl_assert_to(rproc->mcu_rst, TIMEOUT_US_1MS);
	if (res)
		return res;

#if defined(CFG_STM32MP25) || defined(CFG_STM32MP23) || defined(CFG_STM32MP21)
	/* Disable the TrustZone */
	stm32mp_syscfg_write(CA35SS_SYSCFG_M33_TZEN_CR, 0,
			     CA35SS_SYSCFG_M33_TZEN_CR_CFG_SECEXT);
#endif

	rproc->boot_addr = 0;
	rproc->tzen = false;

	return TEE_SUCCESS;
}

TEE_Result stm32_rproc_stop(uint32_t rproc_id)
{
	struct stm32_rproc_instance *rproc = stm32_rproc_get(rproc_id);

	if (!rproc)
		return TEE_ERROR_BAD_PARAMETERS;

	return rproc_stop(rproc);
}

TEE_Result stm32_rproc_da_to_pa(uint32_t rproc_id, paddr_t da, size_t size,
				paddr_t *pa)
{
	struct stm32_rproc_instance *rproc = stm32_rproc_get(rproc_id);
	struct stm32_rproc_mem *mems = NULL;
	unsigned int i = 0;

	if (!rproc)
		return TEE_ERROR_BAD_PARAMETERS;

	mems = rproc->regions;

	for (i = 0; i < rproc->n_regions; i++) {
		if (core_is_buffer_inside(da, size, mems[i].da, mems[i].size)) {
			/*
			 * A match between the requested DA memory area and the
			 * registered regions has been found.
			 * The PA is the reserved-memory PA address plus the
			 * delta between the requested DA and the
			 * reserved-memory DA address.
			 */
			*pa = mems[i].addr + da - mems[i].da;
			return TEE_SUCCESS;
		}
	}

	return TEE_ERROR_ACCESS_DENIED;
}

TEE_Result stm32_rproc_map(uint32_t rproc_id, paddr_t pa, size_t size,
			   void **va)
{
	struct stm32_rproc_instance *rproc = stm32_rproc_get(rproc_id);
	struct stm32_rproc_mem *mems = NULL;
	unsigned int i = 0;

	if (!rproc)
		return TEE_ERROR_BAD_PARAMETERS;

	mems = rproc->regions;

	for (i = 0; i < rproc->n_regions; i++) {
		if (!core_is_buffer_inside(pa, size, mems[i].addr,
					   mems[i].size))
			continue;
		*va = core_mmu_add_mapping(MEM_AREA_RAM_SEC, pa, size);
		if (!*va) {
			EMSG("Can't map region %#"PRIxPA" size %zu", pa, size);
			return TEE_ERROR_GENERIC;
		}

		return TEE_SUCCESS;
	}

	return TEE_ERROR_ACCESS_DENIED;
}

TEE_Result stm32_rproc_unmap(uint32_t rproc_id, void *va, size_t size)
{
	struct stm32_rproc_instance *rproc = stm32_rproc_get(rproc_id);
	struct stm32_rproc_mem *mems = NULL;
	paddr_t pa = virt_to_phys(va);
	unsigned int i = 0;

	if (!rproc || !pa)
		return TEE_ERROR_BAD_PARAMETERS;

	mems = rproc->regions;

	for (i = 0; i < rproc->n_regions; i++) {
		if (!core_is_buffer_inside(pa, size, mems[i].addr,
					   mems[i].size))
			continue;

		/* Flush the cache before unmapping the memory */
		dcache_clean_range(va, size);

		if (core_mmu_remove_mapping(MEM_AREA_RAM_SEC, va, size)) {
			EMSG("Can't unmap region %#"PRIxPA" size %zu",
			     pa, size);
			return TEE_ERROR_GENERIC;
		}

		return TEE_SUCCESS;
	}

	return TEE_ERROR_ACCESS_DENIED;
}

static TEE_Result stm32_rproc_get_dma_range(struct stm32_rproc_mem *region,
					    const void *fdt, int node)
{
	const fdt32_t *list = NULL;
	uint32_t size = 0;
	int ahb_node = 0;
	int elt_size = 0;
	uint32_t da = 0;
	int nranges = 0;
	paddr_t pa = 0;
	int len = 0;
	int i = 0;
	int j = 0;

	/*
	 * The match between local and remote processor memory mapping is
	 * described in the dma-ranges defined by the bus parent node.
	 */
	ahb_node = fdt_parent_offset(fdt, node);

	list = fdt_getprop(fdt, ahb_node, "dma-ranges", &len);
	if (!list) {
		if (len != -FDT_ERR_NOTFOUND)
			return TEE_ERROR_GENERIC;
		/* Same memory mapping */
		DMSG("No dma-ranges found in DT");
		region->da = region->addr;
		return TEE_SUCCESS;
	}

	/* A dma-ranges element is constructed with:
	 *  - the 32-bit remote processor address
	 *  - the cpu address which depends on the CPU arch (32-bit or 64-bit)
	 *  - the 32-bit remote processor memory mapping size
	 */
	elt_size = sizeof(uint32_t) + sizeof(paddr_t) + sizeof(uint32_t);

	if (len % elt_size)
		return TEE_ERROR_BAD_PARAMETERS;

	nranges = len / elt_size;

	for (i = 0, j = 0; i < nranges; i++) {
		da = fdt32_to_cpu(list[j++]);
		pa = fdt32_to_cpu(list[j++]);
#if defined(__LP64__)
		pa = SHIFT_U64(pa, 32) | list[j++];
#endif
		size = fdt32_to_cpu(list[j++]);

		if (core_is_buffer_inside(region->addr, region->size,
					  pa, size)) {
			region->da = da + (region->addr - pa);
			return TEE_SUCCESS;
		}
	}

	return TEE_ERROR_BAD_PARAMETERS;
}

TEE_Result stm32_rproc_set_boot_address(uint32_t rproc_id, paddr_t address)
{
	struct stm32_rproc_instance *rproc = stm32_rproc_get(rproc_id);

	if (!rproc)
		return TEE_ERROR_BAD_PARAMETERS;

	if (rproc->boot_addr) {
		DMSG("Firmware boot address already set");
		return TEE_ERROR_GENERIC;
	}

	rproc->boot_addr = address;

	return TEE_SUCCESS;
}

TEE_Result stm32_rproc_enable_sec_boot(uint32_t rproc_id)
{
	struct stm32_rproc_instance *rproc = stm32_rproc_get(rproc_id);

	if (!rproc)
		return TEE_ERROR_BAD_PARAMETERS;

	if (rproc->tzen) {
		DMSG("Firmware TrustZone already enabled");
		return TEE_ERROR_GENERIC;
	}

	rproc->tzen = true;

	return TEE_SUCCESS;
}

static void stm32_rproc_free_regions(struct stm32_rproc_instance *rproc)
{
	struct stm32_rproc_mem *regions = rproc->regions;
	int i = 0;

	if (!regions)
		return;

	for (i = 0; i < (int)rproc->n_regions; i++) {
		if (regions[i].default_conf)
			firewall_alternate_conf_put(regions[i].default_conf);
		if (regions[i].load_conf)
			firewall_alternate_conf_put(regions[i].load_conf);
	}

	free(regions);
	rproc->n_regions = 0;
	rproc->regions = NULL;
}

/* Get device tree memory regions reserved for the Cortex-M and the IPC */
static TEE_Result stm32_rproc_parse_mems(struct stm32_rproc_instance *rproc,
					 const void *fdt, int node)
{
	const fdt32_t *list = NULL;
	TEE_Result res = TEE_ERROR_GENERIC;
	struct stm32_rproc_mem *regions = NULL;
	int len = 0;
	int n_regions = 0;
	int i = 0;

	/*
	 * In case of firmware loading by the non secure context no need to
	 * register memory regions, so we ignore them.
	 */
	if (rproc->cdata->ns_loading)
		return TEE_SUCCESS;

	list = fdt_getprop(fdt, node, "memory-region", &len);
	if (!list) {
		EMSG("No memory regions found in DT");
		return TEE_ERROR_GENERIC;
	}

	n_regions = len / sizeof(uint32_t);

	regions = calloc(n_regions, sizeof(*regions));
	if (!regions)
		return TEE_ERROR_OUT_OF_MEMORY;

	rproc->n_regions = n_regions;
	rproc->regions = regions;

	for (i = 0; i < n_regions; i++) {
		int pnode = 0;

		pnode = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(list[i]));
		if (pnode < 0) {
			res = TEE_ERROR_GENERIC;
			return res;
		}

		if (fdt_reg_info(fdt, pnode, &regions[i].addr,
				 &regions[i].size)) {
			res = TEE_ERROR_GENERIC;
			return res;
		}
		res = firewall_dt_get_alternate_conf(fdt, pnode,
						     FIREWALL_CONF_DEFAULT,
						     &regions[i].default_conf);
		if (res)
			return res;
		res = firewall_dt_get_alternate_conf(fdt, pnode,
						     FIREWALL_CONF_LOAD,
						     &regions[i].load_conf);
		if (res)
			return res;

#if defined(CFG_STM32MP15)
		if (stm32mp1_ram_intersect_pager_ram(regions[i].addr,
						     regions[i].size)) {
			EMSG("Region %#"PRIxPA"..%#"PRIxPA" intersects pager secure memory",
			     regions[i].addr,
			     regions[i].addr + regions[i].size);
			return TEE_ERROR_GENERIC;
		}
#endif

		res = stm32_rproc_get_dma_range(&regions[i], fdt, node);
		if (res)
			return res;

		if (!regions[i].addr || !regions[i].size) {
			res = TEE_ERROR_BAD_PARAMETERS;
			return res;
		}

		DMSG("register region %#"PRIxPA" size %#zx",
		     regions[i].addr, regions[i].size);
	}

	return TEE_SUCCESS;
}

TEE_Result stm32_rproc_clean_up_memories(uint32_t rproc_id)
{
	struct stm32_rproc_instance *rproc = stm32_rproc_get(rproc_id);
	struct stm32_rproc_mem *mems = NULL;
	TEE_Result res = TEE_ERROR_GENERIC;
	unsigned int i = 0;
	void *va = NULL;
	size_t size = 0;
	paddr_t pa = 0;

	if (!rproc)
		return TEE_ERROR_BAD_PARAMETERS;

	res = stm32_rproc_get_mems_access(rproc);
	if (res)
		return res;

	mems = rproc->regions;
	for (i = 0; i < rproc->n_regions; i++) {
		pa = mems[i].addr;
		size = mems[i].size;
		res = stm32_rproc_map(rproc_id, pa, size, &va);
		if (res)
			break;
		memset(va, 0, size);
		res = stm32_rproc_unmap(rproc_id, va, size);
		if (res)
			break;
	}

	if (stm32_rproc_release_mems_access(rproc) != TEE_SUCCESS)
		panic();

	return res;
}

TEE_Result stm32_rproc_get_mem(uint32_t rproc_id)
{
	struct stm32_rproc_instance *rproc = stm32_rproc_get(rproc_id);

	if (!rproc)
		return TEE_ERROR_BAD_PARAMETERS;

	return stm32_rproc_get_mems_access(rproc);
}

TEE_Result stm32_rproc_release_mem(uint32_t rproc_id)
{
	struct stm32_rproc_instance *rproc = stm32_rproc_get(rproc_id);

	if (!rproc)
		return TEE_ERROR_BAD_PARAMETERS;

	return stm32_rproc_release_mems_access(rproc);
}

static void stm32_rproc_cleanup(struct stm32_rproc_instance *rproc)
{
	stm32_rproc_free_regions(rproc);
	free(rproc);
}

static void stm32_rproc_a35ss_cfg(struct stm32_rproc_instance *rproc __unused)
{
#if defined(CFG_STM32MP25) || defined(CFG_STM32MP23) || defined(CFG_STM32MP21)
	stm32mp_syscfg_write(CA35SS_SYSCFG_M33_ACCESS_CR,
			     rproc->m33_cr_right,
			     CA35SS_SYSCFG_M33_ACCESS_CR_SEC |
			     CA35SS_SYSCFG_M33_ACCESS_CR_PRIV);
	/* Disable the TrustZone that is enabled by default */
	stm32mp_syscfg_write(CA35SS_SYSCFG_M33_TZEN_CR, 0,
			     CA35SS_SYSCFG_M33_TZEN_CR_CFG_SECEXT);

#endif
}

/* Functions to managed reset line exported to non secure world with SCMI */
static struct stm32_rproc_reset *to_rproc_reset(struct rstctrl *rstctrl)
{
	assert(rstctrl);

	return container_of(rstctrl, struct stm32_rproc_reset, rstctrl);
}

static TEE_Result stm32_rproc_reset_update(struct rstctrl *rstctrl, bool status,
					   unsigned int to_us)
{
	struct stm32_rproc_reset *reset = to_rproc_reset(rstctrl);
	const struct stm32_rproc_instance *rproc = reset->rproc;
	struct rstctrl *rproc_rstctrl  = NULL;
	TEE_Result res = TEE_ERROR_GENERIC;
	unsigned int id = reset->id;

	assert(rproc);

	switch (id) {
	case MCU_RESET:
		rproc_rstctrl = rproc->mcu_rst;
		/* Deassert not possible for mcu_rst, cleared by hardware */
		if (!status)
			return TEE_ERROR_NOT_SUPPORTED;
		break;
	case MCU_HOLD_BOOT:
		rproc_rstctrl = rproc->hold_boot;
		break;
	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}

	if (!rproc->cdata->ns_loading)
		return TEE_ERROR_ACCESS_DENIED;

	if (status)
		res = rstctrl_assert_to(rproc_rstctrl, to_us);
	else
		res = rstctrl_deassert_to(rproc_rstctrl, to_us);

	return res;
}

static TEE_Result stm32_rproc_reset_assert(struct rstctrl *rstctrl,
					   unsigned int to_us)
{
	return stm32_rproc_reset_update(rstctrl, true, to_us);
}

static TEE_Result stm32_rproc_reset_deassert(struct rstctrl *rstctrl,
					     unsigned int to_us)
{
	return stm32_rproc_reset_update(rstctrl, false, to_us);
}

static const char *stm32_rproc_reset_get_name(struct rstctrl *rstctrl)
{
	struct stm32_rproc_reset *reset = to_rproc_reset(rstctrl);
	const struct stm32_rproc_instance *rproc = reset->rproc;
	struct rstctrl *rproc_rstctrl = NULL;
	unsigned int id = reset->id;

	assert(rproc);

	switch (id) {
	case MCU_RESET:
		rproc_rstctrl = rproc->mcu_rst;
		break;
	case MCU_HOLD_BOOT:
		rproc_rstctrl = rproc->hold_boot;
		break;
	default:
		return NULL;
	}

	return rstctrl_name(rproc_rstctrl);
}

static const struct rstctrl_ops stm32_rproc_rstctrl_ops = {
	.assert_level = stm32_rproc_reset_assert,
	.deassert_level = stm32_rproc_reset_deassert,
	.get_name = stm32_rproc_reset_get_name,
};

static TEE_Result stm32_rproc_rstctrl_get_dev(struct dt_pargs *arg,
					      void *priv_data,
					      struct rstctrl **out_device)
{
	struct stm32_rproc_instance *rproc = priv_data;
	uint32_t id = 0;

	assert(arg);
	assert(out_device);

	if (arg->args_count != 1)
		return TEE_ERROR_BAD_PARAMETERS;

	id = arg->args[0];
	if (id >= NB_RESET_ID)
		return TEE_ERROR_BAD_PARAMETERS;

	*out_device = &rproc->reset[id].rstctrl;

	return TEE_SUCCESS;
}

static TEE_Result stm32_rproc_probe(const void *fdt, int node,
				    const void *comp_data)
{
	struct stm32_rproc_instance *rproc = NULL;
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t id = 0;

	rproc = calloc(1, sizeof(*rproc));
	if (!rproc)
		return TEE_ERROR_OUT_OF_MEMORY;

	rproc->cdata = comp_data;
	rproc->fdt = fdt;

#if defined(CFG_STM32MP25) || defined(CFG_STM32MP23) || defined(CFG_STM32MP21)
	if (rproc->cdata->ns_loading) {
		/*
		 * The remote firmware will be loaded by the non secure
		 * Provide access rights to A35SSC_M33 registers
		 * to the non secure context
		 */
		rproc->m33_cr_right = CA35SS_SYSCFG_M33_ACCESS_CR_PRIV;
	} else {
		rproc->m33_cr_right = CA35SS_SYSCFG_M33_ACCESS_CR_SEC |
				      CA35SS_SYSCFG_M33_ACCESS_CR_PRIV;
	}
#endif /* CFG_STM32MP25 || CFG_STM32MP23 || CFG_STM32MP21 */

	if (!rproc->cdata->ns_loading) {
		res = stm32_rproc_parse_mems(rproc, fdt, node);
		if (res)
			goto err;
	}

	res = rstctrl_dt_get_by_name(fdt, node, "mcu_rst", &rproc->mcu_rst);
	if (res)
		goto err;

	res = rstctrl_dt_get_by_name(fdt, node, "hold_boot", &rproc->hold_boot);
	if (res)
		goto err;

	/* Expose reset line to SCMI server for non-secure access */
	if (fdt_getprop(fdt, node, "#reset-cells", NULL)) {
		for (id = 0; id < NB_RESET_ID; id++) {
			rproc->reset[id].id = id;
			rproc->reset[id].rproc = rproc;
			rproc->reset[id].rstctrl.ops = &stm32_rproc_rstctrl_ops;
		}

		res = rstctrl_register_provider(fdt, node,
						stm32_rproc_rstctrl_get_dev,
						rproc);
		if (res)
			goto err;
	}

	/* Ensure that the MCU is HOLD */
	if (!rproc->cdata->ns_loading) {
		res = rproc_stop(rproc);
		if (res)
			goto err;
	}

	stm32_rproc_a35ss_cfg(rproc);
#ifdef CFG_STM32MP15
	if (!stm32_rcc_is_secure()) {
		IMSG("WARNING: insecure rproc isolation, RCC is not secure");
		if (!IS_ENABLED(CFG_INSECURE))
			panic();
	} else {
		stm32_rcc_set_mckprot(true);
	}
#endif


	if (!rproc->cdata->ns_loading)
		SLIST_INSERT_HEAD(&rproc_list, rproc, link);

	return TEE_SUCCESS;

err:
	stm32_rproc_cleanup(rproc);
	return res;
}

static const struct stm32_rproc_compat_data stm32_rproc_m4_tee_compat = {
	.rproc_id = STM32MP1_M4_RPROC_ID,
	.ns_loading = false,
};

static const struct stm32_rproc_compat_data stm32_rproc_m33_compat = {
	.rproc_id = STM32MP2_M33_RPROC_ID,
	.ns_loading = true,
	.start = stm32mp2_rproc_start,
};

static const struct stm32_rproc_compat_data stm32_rproc_m33_tee_compat = {
	.rproc_id = STM32MP2_M33_RPROC_ID,
	.ns_loading = false,
	.start = stm32mp2_rproc_start,
};

static const struct dt_device_match stm32_rproc_match_table[] = {
	{
		.compatible = "st,stm32mp1-m4-tee",
		.compat_data = &stm32_rproc_m4_tee_compat,
	},
	{
		.compatible = "st,stm32mp2-m33",
		.compat_data = &stm32_rproc_m33_compat,
	},
	{
		.compatible = "st,stm32mp2-m33-tee",
		.compat_data = &stm32_rproc_m33_tee_compat,
	},
	{ }
};

DEFINE_DT_DRIVER(stm32_rproc_dt_driver) = {
	.name = "stm32-rproc",
	.match_table = stm32_rproc_match_table,
	.probe = &stm32_rproc_probe,
};
