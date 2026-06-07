/*
 * Arm Musca-B1 test chip board emulation
 *
 * Copyright (c) 2019 Linaro Limited
 * Written by Peter Maydell
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 or
 *  (at your option) any later version.
 */

/*
 * The Musca boards are a reference implementation of a system using
 * the SSE-200 subsystem for embedded:
 * https://developer.arm.com/products/system-design/development-boards/iot-test-chips-and-boards/musca-a-test-chip-board
 * https://developer.arm.com/products/system-design/development-boards/iot-test-chips-and-boards/musca-b-test-chip-board
 * We model the A and B1 variants of this board, as described in the TRMs:
 * https://developer.arm.com/documentation/101107/latest/
 * https://developer.arm.com/documentation/101312/latest/
 */

#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "system/address-spaces.h"
#include "system/system.h"
#include "hw/arm/boot.h"
#include "hw/arm/armsse.h"
#include "hw/arm/machines-qom.h"
#include "hw/core/boards.h"
#include "hw/char/pl011.h"
#include "hw/core/split-irq.h"
#include "hw/misc/tz-mpc.h"
#include "hw/misc/tz-ppc.h"
#include "hw/misc/unimp.h"
#include "hw/rtc/pl031.h"
#include "hw/core/qdev-clock.h"
#include "qom/object.h"

#define IRQ_COUNT               32


// //------------------------------------------

#define TYPE_RP2350_MACHINE "rp2350"
#define TYPE_RP2350_PICO2_MACHINE MACHINE_TYPE_NAME("rp2350-pico2")

#define RP2350_NUMIRQ_MAX 96
#define RP2350_PPC_MAX 3
#define RP2350_MPC_MAX 5

typedef struct MPCInfo MPCInfo;

typedef enum Rp2350Type {
    RP2350_PICO2
} Rp2350Type;

// Devices



typedef struct Peripheral {
    const char *name;
    hwaddr base;
    hwaddr size;
} Peripheral;

static const Peripheral apbPeripherals[] = {
    {"sysinfo_base",                0x40000000, 0x1000},
    {"syscfg_base",                 0x40008000, 0x1000},
    {"clocks_base",                 0x40010000, 0x1000},
    {"psm_base",                    0x40018000, 0x1000},
    {"resets_base",                 0x40020000, 0x7000},
    {"io_bank0_base",               0x40028000, 0x1000},
    {"io_qspi_base",                0x40030000, 0x1000},
    {"pads_bank0_base",             0x40038000, 0x1000},
    {"pads_qspi_base",              0x40040000, 0x1000},
    {"xosc_base",                   0x40048000, 0x1000},
    {"pll_sys_base",                0x40050000, 0x1000},
    {"pll_usb_base",                0x40058000, 0x1000},
    {"accessctrl_base",             0x40060000, 0x1000},
    {"busctrl_base",                0x40068000, 0x1000},
    {"uart0_base",                  0x40070000, 0x1000},
    {"uart1_base ",                 0x40078000, 0x1000},
    {"spi0_base",                   0x40080000, 0x1000},
    {"spi1_base",                   0x40088000, 0x1000},
    {"i2c0_base",                   0x40090000, 0x1000},
    {"i2c1_base",                   0x40098000, 0x1000},
    {"adc_base",                    0x400a0000, 0x1000},
    {"wm_base",                     0x400a8000, 0x1000},
    {"timer0_base",                 0x400b0000, 0x1000},
    {"timer1_base",                 0x400b8000, 0x1000},
    {"hstx_ctrl_base",              0x400c0000, 0x1000},
    {"xip_ctrl_base",               0x400c8000, 0x1000},
    {"xip_qmi_base",                0x400d0000, 0x1000},
    {"watchdog_base",               0x400d8000, 0x1000},
    {"bootram_base",                0x400e0000, 0x1000},
    {"rosc_base",                   0x400e8000, 0x1000},
    {"trng_base",                   0x400f0000, 0x1000},
    {"sha256_base",                 0x400f8000, 0x1000},
    {"powman_base",                 0x40100000, 0x1000},
    {"ticks_base",                  0x40108000, 0x1000},
    {"otp_base",                    0x40120000, 0x1000},
    {"otp_data_base",               0x40130000, 0x1000},
    {"otp_data_raw_base",           0x40134000, 0x1000},
    {"otp_data_guarded_base",       0x40138000, 0x1000},
    {"otp_data_raw_guarded_base",   0x4013c000, 0x1000},
    {"cs_periph_base",              0x40140000, 0x1000},
    {"cs_romtable_base",            0x40140000, 0x1000},
    {"cs_ahb_ap_core0_base",        0x40142000, 0x1000},
    {"cs_ahb_ap_core1_base",        0x40144000, 0x1000},
    {"cs_timestamp_gen_base",       0x40146000, 0x1000},
    {"cs_atb_funnel_base",          0x40147000, 0x1000},
    {"cs_tpiu_base",                0x40148000, 0x1000},
    {"cs_cti_base",                 0x40149000, 0x1000},
    {"cs_apb_ap_riscv_base",        0x4014a000, 0x1000},
    {"glitch_detector_base",        0x40158000, 0x1000},
    {"tbman_base",                  0x40160000, 0x1000}
};

#define APB_COUNT   (sizeof(apbPeripherals) / sizeof(Peripheral))

static const Peripheral ahbPeripherals[] = {
    {"dma_base",                    0x50000000, 1000}, 
    {"usbctrl_base",                0x50100000, 1000}, 
    {"usbctrl_dpram_base",          0x50100000, 1000}, 
    {"usbctrl_regs_base",           0x50110000, 1000}, 
    {"pio0_base",                   0x50200000, 1000}, 
    {"pio1_base",                   0x50300000, 1000}, 
    {"pio2_base",                   0x50400000, 1000}, 
    {"xip_aux_base",                0x50500000, 1000}, 
    {"hstx_fifo_base",              0x50600000, 1000}, 
    {"coresight_trace_base",        0x50700000, 1000}, 
};
#define AHB_COUNT   ((int) sizeof(ahbPeripherals) / (int)sizeof(Peripheral))





struct Rp2350MachineClass {
    MachineClass parent;
    Rp2350Type type;
    uint32_t init_svtor;
    int sram_addr_width;
    int num_irqs;
    const MPCInfo *mpc_info;
    int num_mpcs;
};

struct Rp2350MachineState {
    MachineState parent;

    MemoryRegion lsrom;
    MemoryRegion lssram;
    MemoryRegion lsxip;
    MemoryRegion lsperipherals;



    ARMSSE sse;
    /* RAM and flash */
    MemoryRegion ram[RP2350_MPC_MAX];
    SplitIRQ cpu_irq_splitter[IRQ_COUNT];
    SplitIRQ sec_resp_splitter;
    TZPPC ppc[RP2350_PPC_MAX];
    MemoryRegion container;
    UnimplementedDeviceState eflash[2];
    UnimplementedDeviceState qspi;
    TZMPC mpc[RP2350_MPC_MAX];
    UnimplementedDeviceState mhu[2];
    UnimplementedDeviceState pwm[3];
    UnimplementedDeviceState i2s;
    PL011State uart[2];
    UnimplementedDeviceState i2c[2];
    UnimplementedDeviceState spi;
    UnimplementedDeviceState scc;
    UnimplementedDeviceState timer;
    PL031State rtc;
    UnimplementedDeviceState pvt;
    UnimplementedDeviceState sdio;
    UnimplementedDeviceState gpio;
    UnimplementedDeviceState cryptoisland;
    Clock *sysclk;
    Clock *s32kclk;
};

OBJECT_DECLARE_TYPE(Rp2350MachineState, Rp2350MachineClass, RP2350_MACHINE)

/*
 * Main SYSCLK frequency in Hz
 * TODO this should really be different for the two cores, but we
 * don't model that in our SSE-200 model yet.
 */
#define SYSCLK_FRQ 40000000
/* Slow 32Khz S32KCLK frequency in Hz */
#define S32KCLK_FRQ (32 * 1000)

static qemu_irq get_sse_irq_in(Rp2350MachineState *mms, int irqno)
{
    /* Return a qemu_irq which will signal IRQ n to all CPUs in the SSE. */
    assert(irqno < IRQ_COUNT);

    return qdev_get_gpio_in(DEVICE(&mms->cpu_irq_splitter[irqno]), 0);
}
typedef MemoryRegion *MakeDevFn(Rp2350MachineState *mms, void *opaque,
                                const char *name, hwaddr size);

typedef struct PPCPortInfo {
    const char *name;
    MakeDevFn *devfn;
    void *opaque;
    hwaddr addr;
    hwaddr size;
} PPCPortInfo;

typedef struct PPCInfo {
    const char *name;
    PPCPortInfo ports[TZ_NUM_PORTS];
} PPCInfo;


static MemoryRegion *make_unimp_dev(Rp2350MachineState *mms,
                                    void *opaque, const char *name, hwaddr size)
{
    /*
     * Initialize, configure and realize a TYPE_UNIMPLEMENTED_DEVICE,
     * and return a pointer to its MemoryRegion.
     */
    UnimplementedDeviceState *uds = opaque;

    object_initialize_child(OBJECT(mms), name, uds, TYPE_UNIMPLEMENTED_DEVICE);
    qdev_prop_set_string(DEVICE(uds), "name", name);
    qdev_prop_set_uint64(DEVICE(uds), "size", size);
    sysbus_realize(SYS_BUS_DEVICE(uds), &error_fatal);
    return sysbus_mmio_get_region(SYS_BUS_DEVICE(uds), 0);
}

typedef enum MPCInfoType {
    MPC_RAM,
    MPC_ROM,
    MPC_CRYPTOISLAND,
} MPCInfoType;

struct MPCInfo {
    const char *name;
    hwaddr addr;
    hwaddr size;
    MPCInfoType type;
};

static const MPCInfo pico2_mpc_info[] = { {
        .name = "qspi",
        .type = MPC_ROM,
        .addr = 0x00000000,
        .size = 0x02000000,
    }, {
        .name = "sram",
        .type = MPC_RAM,
        .addr = 0x0a400000,
        .size = 0x00080000,
    }, {
        .name = "eflash0",
        .type = MPC_ROM,
        .addr = 0x0a000000,
        .size = 0x00200000,
    }, {
        .name = "eflash1",
        .type = MPC_ROM,
        .addr = 0x0a200000,
        .size = 0x00200000,
    }, {
        .name = "cryptoisland",
        .type = MPC_CRYPTOISLAND,
        .addr = 0x0a000000,
        .size = 0x00200000,
    }
};

static MemoryRegion *make_mpc(Rp2350MachineState *mms, void *opaque,
                              const char *name, hwaddr size)
{
    /*
     * Create an MPC and the RAM or flash behind it.
     * MPC 0: eFlash 0
     * MPC 1: eFlash 1
     * MPC 2: SRAM
     * MPC 3: QSPI flash
     * MPC 4: CryptoIsland
     * For now we implement the flash regions as ROM (ie not programmable)
     * (with their control interface memory regions being unimplemented
     * stubs behind the PPCs).
     * The whole CryptoIsland region behind its MPC is an unimplemented stub.
     */
    Rp2350MachineClass *mmc = RP2350_MACHINE_GET_CLASS(mms);
    TZMPC *mpc = opaque;
    int i = mpc - &mms->mpc[0];
    MemoryRegion *downstream;
    MemoryRegion *upstream;
    UnimplementedDeviceState *uds;
    char *mpcname;
    const MPCInfo *mpcinfo = mmc->mpc_info;

    mpcname = g_strdup_printf("%s-mpc", mpcinfo[i].name);

    switch (mpcinfo[i].type) {
    case MPC_ROM:
        downstream = &mms->ram[i];
        memory_region_init_rom(downstream, NULL, mpcinfo[i].name,
                               mpcinfo[i].size, &error_fatal);
        break;
    case MPC_RAM:
        downstream = &mms->ram[i];
        memory_region_init_ram(downstream, NULL, mpcinfo[i].name,
                               mpcinfo[i].size, &error_fatal);
        break;
    case MPC_CRYPTOISLAND:
        /* We don't implement the CryptoIsland yet */
        uds = &mms->cryptoisland;
        object_initialize_child(OBJECT(mms), name, uds,
                                TYPE_UNIMPLEMENTED_DEVICE);
        qdev_prop_set_string(DEVICE(uds), "name", mpcinfo[i].name);
        qdev_prop_set_uint64(DEVICE(uds), "size", mpcinfo[i].size);
        sysbus_realize(SYS_BUS_DEVICE(uds), &error_fatal);
        downstream = sysbus_mmio_get_region(SYS_BUS_DEVICE(uds), 0);
        break;
    default:
        g_assert_not_reached();
    }

    object_initialize_child(OBJECT(mms), mpcname, mpc, TYPE_TZ_MPC);
    object_property_set_link(OBJECT(mpc), "downstream", OBJECT(downstream),
                             &error_fatal);
    sysbus_realize(SYS_BUS_DEVICE(mpc), &error_fatal);
    /* Map the upstream end of the MPC into system memory */
    upstream = sysbus_mmio_get_region(SYS_BUS_DEVICE(mpc), 1);
    memory_region_add_subregion(get_system_memory(), mpcinfo[i].addr, upstream);
    /* and connect its interrupt to the SSE-200 */
    qdev_connect_gpio_out_named(DEVICE(mpc), "irq", 0,
                                qdev_get_gpio_in_named(DEVICE(&mms->sse),
                                                       "mpcexp_status", i));

    g_free(mpcname);
    /* Return the register interface MR for our caller to map behind the PPC */
    return sysbus_mmio_get_region(SYS_BUS_DEVICE(mpc), 0);
}

static MemoryRegion *make_uart(Rp2350MachineState *mms, void *opaque,
                               const char *name, hwaddr size)
{
    PL011State *uart = opaque;
    int i = uart - &mms->uart[0];
    int irqbase = 7 + i * 6;
    SysBusDevice *sysbus;

    object_initialize_child(OBJECT(mms), name, uart, TYPE_PL011);
    qdev_prop_set_chr(DEVICE(uart), "chardev", serial_hd(i));
    sysbus_realize(SYS_BUS_DEVICE(uart), &error_fatal);
    sysbus = SYS_BUS_DEVICE(uart);
    sysbus_connect_irq(sysbus, 0, get_sse_irq_in(mms, irqbase + 5)); /* combined */
    sysbus_connect_irq(sysbus, 1, get_sse_irq_in(mms, irqbase + 0)); /* RX */
    sysbus_connect_irq(sysbus, 2, get_sse_irq_in(mms, irqbase + 1)); /* TX */
    sysbus_connect_irq(sysbus, 3, get_sse_irq_in(mms, irqbase + 2)); /* RT */
    sysbus_connect_irq(sysbus, 4, get_sse_irq_in(mms, irqbase + 3)); /* MS */
    sysbus_connect_irq(sysbus, 5, get_sse_irq_in(mms, irqbase + 4)); /* E */
    return sysbus_mmio_get_region(SYS_BUS_DEVICE(uart), 0);
}

static MemoryRegion *make_rtc(Rp2350MachineState *mms, void *opaque,
                              const char *name, hwaddr size)
{
    PL031State *rtc = opaque;

    object_initialize_child(OBJECT(mms), name, rtc, TYPE_PL031);
    sysbus_realize(SYS_BUS_DEVICE(rtc), &error_fatal);
    sysbus_connect_irq(SYS_BUS_DEVICE(rtc), 0, get_sse_irq_in(mms, 39));
    return sysbus_mmio_get_region(SYS_BUS_DEVICE(rtc), 0);
}

static void rp2350_init(MachineState *machine)
{
    Rp2350MachineState *mms = RP2350_MACHINE(machine);
    Rp2350MachineClass *mmc = RP2350_MACHINE_GET_CLASS(mms);
    MemoryRegion *system_memory = get_system_memory();
    DeviceState *ssedev;
    DeviceState *dev_splitter;
    const PPCInfo *ppcs;
    int num_ppcs;
    int i;

    assert(mmc->num_irqs <= RP2350_NUMIRQ_MAX);
    assert(mmc->num_mpcs <= RP2350_MPC_MAX);

    mms->sysclk = clock_new(OBJECT(machine), "SYSCLK");
    clock_set_hz(mms->sysclk, SYSCLK_FRQ);
    mms->s32kclk = clock_new(OBJECT(machine), "S32KCLK");
    clock_set_hz(mms->s32kclk, S32KCLK_FRQ);

    object_initialize_child(OBJECT(machine), "sse-200", &mms->sse,
                            TYPE_SSE200);
    ssedev = DEVICE(&mms->sse);
    object_property_set_link(OBJECT(&mms->sse), "memory", OBJECT(system_memory), &error_fatal);
    qdev_prop_set_uint32(ssedev, "EXP_NUMIRQ", mmc->num_irqs);
    qdev_prop_set_uint32(ssedev, "init-svtor", mmc->init_svtor);
    qdev_prop_set_uint32(ssedev, "SRAM_ADDR_WIDTH", mmc->sram_addr_width);
    qdev_connect_clock_in(ssedev, "MAINCLK", mms->sysclk);
    qdev_connect_clock_in(ssedev, "S32KCLK", mms->s32kclk);

    qdev_prop_set_bit(ssedev, "CPU0_FPU", true);
    qdev_prop_set_bit(ssedev, "CPU0_DSP", true);

    sysbus_realize(SYS_BUS_DEVICE(&mms->sse), &error_fatal);


    /*
     * We need to create splitters to feed the IRQ inputs
     * for each CPU in the SSE-200 from each device in the board.
     */
    for (i = 0; i < mmc->num_irqs; i++) {
        char *name = g_strdup_printf("rp2350-irq-splitter%d", i);
        SplitIRQ *splitter = &mms->cpu_irq_splitter[i];

        object_initialize_child_with_props(OBJECT(machine), name, splitter, sizeof(*splitter), TYPE_SPLIT_IRQ, &error_fatal, NULL);
        g_free(name);

        object_property_set_int(OBJECT(splitter), "num-lines", 2, &error_fatal);
        qdev_realize(DEVICE(splitter), NULL, &error_fatal);
        qdev_connect_gpio_out(DEVICE(splitter), 0, qdev_get_gpio_in_named(ssedev, "EXP_IRQ", i));
        qdev_connect_gpio_out(DEVICE(splitter), 1, qdev_get_gpio_in_named(ssedev, "EXP_CPU1_IRQ", i));
    }

    /*
     * The sec_resp_cfg output from the SSE-200 must be split into multiple
     * lines, one for each of the PPCs we create here.
     */
    object_initialize_child_with_props(OBJECT(machine), "sec-resp-splitter", &mms->sec_resp_splitter, sizeof(mms->sec_resp_splitter), TYPE_SPLIT_IRQ, &error_fatal, NULL);

    object_property_set_int(OBJECT(&mms->sec_resp_splitter), "num-lines", ARRAY_SIZE(mms->ppc), &error_fatal);
    qdev_realize(DEVICE(&mms->sec_resp_splitter), NULL, &error_fatal);
    dev_splitter = DEVICE(&mms->sec_resp_splitter);
    qdev_connect_gpio_out_named(ssedev, "sec_resp_cfg", 0, qdev_get_gpio_in(dev_splitter, 0));

    /*
     * Most of the devices in the board are behind Peripheral Protection
     * Controllers. The required order for initializing things is:
     *  + initialize the PPC
     *  + initialize, configure and realize downstream devices
     *  + connect downstream device MemoryRegions to the PPC
     *  + realize the PPC
     *  + map the PPC's MemoryRegions to the places in the address map
     *    where the downstream devices should appear
     *  + wire up the PPC's control lines to the SSE object
     *
     * The PPC mapping differs for the -A and -B1 variants; the -A version
     * is much simpler, using only a single port of a single PPC and putting
     * all the devices behind that.
     */

//***** Lets attempt to make the machine without all the insane amout of stuff */



#define LS_ROM_BASE 0x00000000
#define LS_ROM_SIZE 0x00200000
#define LS_XIP_BASE 0x10000000
#define LS_XIP_SIZE 0x00082000
#define LS_RAM_BASE 0x20000000
#define LS_RAM_SIZE 0x00082000

    memory_region_init_rom(&mms->lsrom, OBJECT(&mms->sse),  "lsrom",  LS_ROM_SIZE,  &error_fatal);
    memory_region_init_ram(&mms->lsxip, OBJECT(&mms->sse),  "lsxip",  LS_XIP_SIZE,  &error_fatal);
    memory_region_init_ram(&mms->lssram, OBJECT(&mms->sse), "lssram", LS_RAM_SIZE, &error_fatal);
    // memory_region_init_rom(&mms->lsrom, NULL,  "lsrom",  LS_ROM_SIZE,  &error_fatal);
    // memory_region_init_ram(&mms->lssram, NULL, "lssram", LS_RAM_SIZE, &error_fatal);
    // memory_region_init_ram(&mms->lsxip, NULL,  "lsxip",  LS_XIP_SIZE,  &error_fatal);
 
// printf("\n size %0llx\n", mms->lsrom.size);
// printf("\n size %0llx\n", mms->lsxip.size);
// printf("\n size %0llx\n", mms->lssram.size);


    memory_region_add_subregion(get_system_memory(), LS_ROM_BASE, &mms->lsrom);
    memory_region_add_subregion(get_system_memory(), LS_XIP_BASE, &mms->lsxip);
    memory_region_add_subregion(get_system_memory(), LS_RAM_BASE, &mms->lssram);

// printf("\n rom start %0llx  size %0llx\n", mms->lsrom.addr);
// printf("\n xip start %0llx  size %0llx\n", mms->lsxip.addr);
// printf("\n ram start %0llx  size %0llx\n", mms->lssram.addr);

printf("\n Welcome to the RP2350 Pico-2 Emulator\n");

create_unimplemented_device(apbPeripherals[0].name, apbPeripherals[0].base, apbPeripherals[0].size);

    // create the unused peripherals -- feel free to implement them
    for (int index = 0; index < APB_COUNT; ++index) {
        create_unimplemented_device(apbPeripherals[index].name, apbPeripherals[index].base, apbPeripherals[index].size);
    }

    // create the unused peripherals -- feel free to implement them
    // for (int index = 0; index < AHB_COUNT; ++index) {
    //     create_unimplemented_device(ahbPeripherals[index].name, ahbPeripherals[index].base, ahbPeripherals[index].size);
    // }

    // create_unimplemented_device("rp2350.sysinfo",    RP2350_SYSINFO_BASE, 0x1000);
    // create_unimplemented_device("rp2350.syscfg",     RP2350_SYSCFG_BASE, 0x1000);
    // create_unimplemented_device("rp2350.clocks",     RP2350_CLOCKS_BASE, 0x1000);
    // create_unimplemented_device("rp2350.resets",     RP2350_RESETS_BASE, 0x1000);
    // create_unimplemented_device("rp2350.psm",        RP2350_PSM_BASE, 0x1000);
    // create_unimplemented_device("rp2350.pads_bank0", RP2350_PADS_BANK0_BASE, 0x1000);
    // create_unimplemented_device("rp2350.watchdog",   RP2350_WATCHDOG_BASE, 0x1000);
    // create_unimplemented_device("rp2350.sio",        RP2350_SIO_BASE, 0x1000);


    armv7m_load_kernel(mms->sse.armv7m[0].cpu, machine->kernel_filename, 0x10000000, 0x2000000);

return;

    /*
     * Devices listed with an 0x4.. address appear in both the NS 0x4.. region
     * and the 0x5.. S region. Devices listed with an 0x5.. address appear
     * only in the S region.
     */
    const PPCInfo pico2_ppcs[] = { {
            .name = "apb_ppcexp0",
            .ports = {
                { "eflash0", make_unimp_dev, &mms->eflash[0], 0x52400000, 0x1000 },
                { "eflash1", make_unimp_dev, &mms->eflash[1], 0x52500000, 0x1000 },
                { "qspi", make_unimp_dev, &mms->qspi, 0x42800000, 0x100000 },
                { "mpc0", make_mpc, &mms->mpc[0], 0x52000000, 0x1000 },
                { "mpc1", make_mpc, &mms->mpc[1], 0x52100000, 0x1000 },
                { "mpc2", make_mpc, &mms->mpc[2], 0x52200000, 0x1000 },
                { "mpc3", make_mpc, &mms->mpc[3], 0x52300000, 0x1000 },
                { "mhu0", make_unimp_dev, &mms->mhu[0], 0x42600000, 0x100000 },
                { "mhu1", make_unimp_dev, &mms->mhu[1], 0x42700000, 0x100000 },
                { }, /* port 9: unused */
                { }, /* port 10: unused */
                { }, /* port 11: unused */
                { }, /* port 12: unused */
                { }, /* port 13: unused */
                { "mpc4", make_mpc, &mms->mpc[4], 0x52e00000, 0x1000 },
            },
        }, {
            .name = "apb_ppcexp1",
            .ports = {
                { "pwm0", make_unimp_dev, &mms->pwm[0], 0x40101000, 0x1000 },
                { "pwm1", make_unimp_dev, &mms->pwm[1], 0x40102000, 0x1000 },
                { "pwm2", make_unimp_dev, &mms->pwm[2], 0x40103000, 0x1000 },
                { "i2s", make_unimp_dev, &mms->i2s, 0x40104000, 0x1000 },
                { "uart0", make_uart, &mms->uart[0], 0x40105000, 0x1000 },
                { "uart1", make_uart, &mms->uart[1], 0x40106000, 0x1000 },
                { "i2c0", make_unimp_dev, &mms->i2c[0], 0x40108000, 0x1000 },
                { "i2c1", make_unimp_dev, &mms->i2c[1], 0x40109000, 0x1000 },
                { "spi", make_unimp_dev, &mms->spi, 0x4010a000, 0x1000 },
                { "scc", make_unimp_dev, &mms->scc, 0x5010b000, 0x1000 },
                { "timer", make_unimp_dev, &mms->timer, 0x4010c000, 0x1000 },
                { "rtc", make_rtc, &mms->rtc, 0x4010d000, 0x1000 },
                { "pvt", make_unimp_dev, &mms->pvt, 0x4010e000, 0x1000 },
                { "sdio", make_unimp_dev, &mms->sdio, 0x4010f000, 0x1000 },
            },
        }, {
            .name = "ahb_ppcexp0",
            .ports = {
                { }, /* port 0: unused */
                { "gpio", make_unimp_dev, &mms->gpio, 0x41000000, 0x1000 },
            },
        },
    };

    switch (mmc->type) {
        case RP2350_PICO2:
            ppcs = pico2_ppcs;
            num_ppcs = ARRAY_SIZE(pico2_ppcs);
            break;
        default:
            g_assert_not_reached();
    }
    assert(num_ppcs <= RP2350_PPC_MAX);

    for (i = 0; i < num_ppcs; i++) {
        const PPCInfo *ppcinfo = &ppcs[i];
        TZPPC *ppc = &mms->ppc[i];
        DeviceState *ppcdev;
        int port;
        char *gpioname;

        object_initialize_child(OBJECT(machine), ppcinfo->name, ppc, TYPE_TZ_PPC);
        ppcdev = DEVICE(ppc);

        for (port = 0; port < TZ_NUM_PORTS; port++) {
            const PPCPortInfo *pinfo = &ppcinfo->ports[port];
            MemoryRegion *mr;
            char *portname;

            if (!pinfo->devfn) {
                continue;
            }

            mr = pinfo->devfn(mms, pinfo->opaque, pinfo->name, pinfo->size);
            portname = g_strdup_printf("port[%d]", port);
            object_property_set_link(OBJECT(ppc), portname, OBJECT(mr), &error_fatal);
            g_free(portname);
        }

        sysbus_realize(SYS_BUS_DEVICE(ppc), &error_fatal);

        for (port = 0; port < TZ_NUM_PORTS; port++) {
            const PPCPortInfo *pinfo = &ppcinfo->ports[port];

            if (!pinfo->devfn) {
                continue;
            }
            sysbus_mmio_map(SYS_BUS_DEVICE(ppc), port, pinfo->addr);

            gpioname = g_strdup_printf("%s_nonsec", ppcinfo->name);
            qdev_connect_gpio_out_named(ssedev, gpioname, port, qdev_get_gpio_in_named(ppcdev, "cfg_nonsec", port));
            g_free(gpioname);
            gpioname = g_strdup_printf("%s_ap", ppcinfo->name);
            qdev_connect_gpio_out_named(ssedev, gpioname, port, qdev_get_gpio_in_named(ppcdev, "cfg_ap", port));
            g_free(gpioname);
        }

        gpioname = g_strdup_printf("%s_irq_enable", ppcinfo->name);
        qdev_connect_gpio_out_named(ssedev, gpioname, 0, qdev_get_gpio_in_named(ppcdev, "irq_enable", 0));
        g_free(gpioname);
        gpioname = g_strdup_printf("%s_irq_clear", ppcinfo->name);
        qdev_connect_gpio_out_named(ssedev, gpioname, 0, qdev_get_gpio_in_named(ppcdev, "irq_clear", 0));
        g_free(gpioname);
        gpioname = g_strdup_printf("%s_irq_status", ppcinfo->name);
        qdev_connect_gpio_out_named(ppcdev, "irq", 0, qdev_get_gpio_in_named(ssedev, gpioname, 0));
        g_free(gpioname);

        qdev_connect_gpio_out(dev_splitter, i, qdev_get_gpio_in_named(ppcdev, "cfg_sec_resp", 0));
    }

    armv7m_load_kernel(mms->sse.armv7m[0].cpu, machine->kernel_filename, 0, 0x2000000);
}

static void rp2350_class_init(ObjectClass *oc, const void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    static const char * const valid_cpu_types[] = {
        ARM_CPU_TYPE_NAME("cortex-m33"),
        NULL
    };

    mc->default_cpus = 2;
    mc->min_cpus = mc->default_cpus;
    mc->max_cpus = mc->default_cpus;
    mc->valid_cpu_types = valid_cpu_types;
    mc->init = rp2350_init;
}



static void rp2350_pico2_class_init(ObjectClass *oc, const void *data) {
    MachineClass *mc = MACHINE_CLASS(oc);
    Rp2350MachineClass *mmc = RP2350_MACHINE_CLASS(oc);

    mc->desc = "ARM Pico-2 board (dual Cortex-M33)";
    mmc->type = RP2350_PICO2;
    /*
     * This matches the DAPlink firmware which boots from QSPI. There
     * is also a firmware blob which boots from the eFlash, which
     * uses init_svtor = 0x1A000000. QEMU doesn't currently support that,
     * though we could in theory expose a machine property on the command
     * line to allow the user to request eFlash boot.
     */
    mmc->init_svtor = 0x10000000;
    mmc->sram_addr_width = 17;
    mmc->num_irqs = 96;
    mmc->mpc_info = pico2_mpc_info;
    mmc->num_mpcs = ARRAY_SIZE(pico2_mpc_info);
}

static const TypeInfo rp2350_info = {
    .name = TYPE_RP2350_MACHINE,
    .parent = TYPE_MACHINE,
    .abstract = true,
    .instance_size = sizeof(Rp2350MachineState),
    .class_size = sizeof(Rp2350MachineClass),
    .class_init = rp2350_class_init,
};

static const TypeInfo rp2350_pico_info = {
    .name = TYPE_RP2350_PICO2_MACHINE,
    .parent = TYPE_RP2350_MACHINE,
    .class_init = rp2350_pico2_class_init,
    .interfaces = arm_machine_interfaces,
};

static void rp2350_info_init(void) {
    type_register_static(&rp2350_info);
    type_register_static(&rp2350_pico_info);
}

type_init(rp2350_info_init);
