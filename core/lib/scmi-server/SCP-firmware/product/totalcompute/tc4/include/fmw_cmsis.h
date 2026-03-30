/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FMW_CMSIS_H
#define FMW_CMSIS_H

#include <stdint.h>

#define __CHECK_DEVICE_DEFINES
#define __CM85_REV             0x0000U
#define __FPU_PRESENT          0U
#define __MPU_PRESENT          1U
#define __VTOR_PRESENT         1U
#define __PMU_PRESENT          0U
#define __DSP_PRESENT          0U
#define __ICACHE_PRESENT       0U
#define __DCACHE_PRESENT       0U
#define __DTCM_PRESENT         0U
#define __NVIC_PRIO_BITS       3U
#define __SAUREGION_PRESENT    0U
#define __Vendor_SysTickConfig 0U

extern uint32_t SystemCoreClock; /*!< System Clock Frequency (Core Clock)*/

typedef enum IRQn {
    Reset_IRQn = -15,
    NonMaskableInt_IRQn = -14,
    HardFault_IRQn = -13,
    MemoryManagement_IRQn = -12,
    BusFault_IRQn = -11,
    UsageFault_IRQn = -10,
    SecureFault_IRQn = -9,
    SVCall_IRQn = -5,
    DebugMonitor_IRQn = -4,
    PendSV_IRQn = -2,
    SysTick_IRQn = -1,

    WDG_RST_RQST_IRQ = 0, /* Watchdog reset request */
    WDG_INT_IRQ = 1, /* Watchdog interrupt */
    SLOWCLK_TIMER_IRQ = 2, /* Slow clock timer */
    TIMER0_IRQ = 3, /* Timer 0 */
    TIMER1_IRQ = 4, /* Timer 1 */
    TIMER2_IRQ = 5, /* Timer 2 */
    RESERVED6_IRQ = 6, /* Reserved */
    RESERVED7_IRQ = 7, /* Reserved */
    RESERVED8_IRQ = 8, /* Reserved */
    MPC_COMBD_S_IRQ = 9, /* MPC Combined Secure */
    PPC_COMBD_S_IRQ = 10, /* PPC Combined Secure */
    MSC_COMBD_S_IRQ = 11, /* MSC Combined Secure */
    BRIDGE_ERR_COMBD_S_IRQ = 12, /* Bridge Error Combined Secure */
    RESERVED13_IRQ = 13, /* Reserved */
    PPU_COMBINED_IRQ = 14, /* PPU Combined */

    SOC_WAKEUP0_IRQ = 16, /* SoC Expansion Wakeup */

    TIMER3_AON_IRQ = 27, /* Timer 3 AON */
    CPU0_CTI_IRQ0 = 28, /* CPU0 CTI IRQ0 */
    CPU0_CTI_IRQ1 = 29, /* CPU0 CTI IRQ1 */

    TIMREFCLK_IRQ = 33, /* REFCLK Physical Timer */

    SCP_AP_MHU0_SEND_IRQ = 88, /* MHU0 SCP AP Sender */
    AP_SCP_MHU0_REC_IRQ = 89, /* MHU0 AP SCP Receiver */
    SCP_AP_MHU1_SEND_IRQ = 90, /* MHU1 SCP AP Sender */
    AP_SCP_MHU1_REC_IRQ = 91, /* MHU1 AP SCP Receiver */
    SCP_AP_MHU2_SEND_IRQ = 92, /* MHU2 SCP AP Sender */
    AP_SCP_MHU2_REC_IRQ = 93, /* MHU2 AP SCP Receiver */
    SCP_AP_MHU3_SEND_IRQ = 94, /* MHU3 SCP AP Sender */
    AP_SCP_MHU3_REC_IRQ = 95, /* MHU3 AP SCP Receiver */
    SCP_AP_MHU4_SEND_IRQ = 96, /* MHU4 SCP AP Sender */
    AP_SCP_MHU4_REC_IRQ = 97, /* MHU4 AP SCP Receiver */
    SCP_AP_MHU5_SEND_IRQ = 98, /* MHU5 SCP AP Sender */
    AP_SCP_MHU5_REC_IRQ = 99, /* MHU5 AP SCP Receiver */
    SCP_AP_MHU6_SEND_IRQ = 100, /* MHU6 SCP AP Sender */
    AP_SCP_MHU6_REC_IRQ = 101, /* MHU6 AP SCP Receiver */

    ATU_IRQ = 116, /* ATU IRQ */

    CLUS_PPU_IRQ = 121, /* Cluster PPU */
    CORE0_PPU_IRQ = 122, /* Core0 PPU */
    CORE1_PPU_IRQ = 123, /* Core1 PPU */
    CORE2_PPU_IRQ = 124, /* Core2 PPU */
    CORE3_PPU_IRQ = 125, /* Core3 PPU */
    CORE4_PPU_IRQ = 126, /* Core4 PPU */
    CORE5_PPU_IRQ = 127, /* Core5 PPU */
    CORE6_PPU_IRQ = 128, /* Core6 PPU */
    CORE7_PPU_IRQ = 129, /* Core7 PPU */
    CORE8_PPU_IRQ = 130, /* Core8 PPU */
    CORE9_PPU_IRQ = 131, /* Core9 PPU */
    CORE10_PPU_IRQ = 132, /* Core10 PPU */
    CORE11_PPU_IRQ = 133, /* Core11 PPU */
    CORE12_PPU_IRQ = 134, /* Core12 PPU */
    CORE13_PPU_IRQ = 135, /* Core13 PPU */
    CORE14_PPU_IRQ = 136, /* Core14 PPU */
    CORE15_PPU_IRQ = 137, /* Core15 PPU */

    GPU_PPU_IRQ = 163, /* GPU PPU */

    SOC_SYS_TIM0_S_IRQ = 250, /* SoC System Timer 0 Secure */
    SOC_SYS_TIM1_NS_IRQ = 251, /* SoC System Timer 1 Non Secure */

    WATCHDOG0_S_IRQ = 253, /* Watchdog 0 Secure */
    WATCHDOG1_S_IRQ = 254, /* Watchdog 1 Secure */
    WATCHDOG0_NS_IRQ = 255, /* Watchdog 0 Non Secure */
    WATCHDOG1_NS_IRQ = 256, /* Watchdog 1 Non Secure */
    DMC0_COMBD_RAS_IRQ = 257, /* DMC0 Combined RAS */
    DMC0_COMBD_PMU_IRQ = 258, /* DMC0 Combined PMU */
    DMC0_COMBD_MISC_IRQ = 259, /* DMC0 Combined MISC */
    DMC1_COMBD_RAS_IRQ = 260, /* DMC1 Combined RAS */
    DMC1_COMBD_PMU_IRQ = 261, /* DMC1 Combined PMU */
    DMC1_COMBD_MISC_IRQ = 262, /* DMC1 Combined MISC */
    DMC2_COMBD_RAS_IRQ = 263, /* DMC2 Combined RAS */
    DMC2_COMBD_PMU_IRQ = 264, /* DMC2 Combined PMU */
    DMC2_COMBD_MISC_IRQ = 265, /* DMC2 Combined MISC */
    DMC3_COMBD_RAS_IRQ = 266, /* DMC3 Combined RAS */
    DMC3_COMBD_PMU_IRQ = 267, /* DMC3 Combined PMU */
    DMC3_COMBD_MISC_IRQ = 268, /* DMC3 Combined MISC */

    GIC_ECC_FATAL_IRQ = 281, /* GIC ECC Fatal */
    GIC_AXIM_ERR_IRQ = 282, /* GIC Axim Error */

    MCN0_COMBD_S_IRQ = 321, /* MCN0 Combined Secure */
    MCN0_COMBD_NS_IRQ = 322, /* MCN0 Combined Non Secure */
    MCN1_COMBD_S_IRQ = 323, /* MCN1 Combined Secure */
    MCN1_COMBD_NS_IRQ = 324, /* MCN1 Combined Non Secure */
    MCN2_COMBD_S_IRQ = 325, /* MCN2 Combined Secure */
    MCN2_COMBD_NS_IRQ = 326, /* MCN2 Combined Non Secure */
    MCN3_COMBD_S_IRQ = 327, /* MCN3 Combined Secure */
    MCN3_COMBD_NS_IRQ = 328, /* MCN3 Combined Non Secure */

    DATA_NCI_PMU_SYSCLK_IRQ = 337, /* Data NCI PMU SysClk */
    DATA_NCI_PMU_PERIPHCLK_IRQ = 338, /* Data NCI PMU Periphclk */
    DATA_NCI_PMU_GPU_COREGRPCLK_IRQ = 339, /* Data NCI PMU GPU core group clk */
    DATA_NCI_PMU_SMSCLK_IRQ = 340, /* Data NCI PMU smsclk */
    DATA_NCI_PMU_ROS_NCICLK_IRQ = 341, /* Data NCI PMU RoS nciclk */
    PERIPH_NCI_PMU_SYSCLK_IRQ = 342, /* Peripheral NCI PMU SysClk */
    PERIPH_NCI_PMU_PERIPHCLK_IRQ = 343, /* Peripheral NCI PMU Periphclk */
    PERIPH_NCI_PMU_GPU_CLK_IRQ = 344, /* Peripheral NCI PMU GPU clk */
    PERIPH_NCI_PMU_DSU_PPUCLK_IRQ = 345, /* Peripheral NCI DSU PPU clk */
    PERIPH_NCI_PMU_SMSCLK_IRQ = 346, /* Peripheral NCI PMU smsclk */

    DATA_PD_SYSTOP_S_IRQ = 353, /* Data PD SYSTOP Secure */
    DATA_PD_SYSTOP_NS_IRQ = 354, /* Data PD SYSTOP Non Secure */
    DATA_PD_GPUGLBL_S_IRQ = 355, /* Data PD GPU glbl Secure */
    DATA_PD_GPUGLBL_NS_IRQ = 356, /* Data PD GPU glbl Non Secure */
    PERIPH_PD_SYSTOP_S_IRQ = 357, /* Peripheral PD SYSTOP Secure */
    PERIPH_PD_SYSTOP_NS_IRQ = 358, /* Peripheral PD SYSTOP Non Secure */
    PERIPH_PD_GPUGLBL_S_IRQ = 359, /* Peripheral PD GPU glbl Secure */
    PERIPH_PD_GPUGLBL_NS_IRQ = 360, /* Peripheral PD GPU glbl Non Secure */
    PERIPH_PD_DSUTOP_S_IRQ = 361, /* Peripheral PD DSUTOP Secure */
    PERIPH_PD_DSUTOP_NS_IRQ = 362, /* Peripheral PD DSUTOP Non Secure */

    SCP_RSS_MHU0_SEN_IRQ = 431, /* SCP RSS MHU0 Sender */
    RSS_SCP_MHU0_REC_IRQ = 432, /* RSS SCP MHU0 Receiver */

    IRQn_MAX = INT16_MAX,
} IRQn_Type;

#include <core_cm85.h>

#endif /* FMW_CMSIS_H */
