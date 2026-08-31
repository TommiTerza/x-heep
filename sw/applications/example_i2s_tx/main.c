/*
 * Copyright EPFL contributors.
 * Licensed under the Apache License, Version 2.0, see LICENSE for details.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "core_v_mini_mcu.h"
#include "i2s.h"
#include "i2s_tx_sink_regs.h"
#include "mmio.h"
#include "x-heep.h"

#ifdef SERIAL_LINK_REG_IS_INCLUDED
#define I2S_TX_SINK_START_ADDRESS (EXT_PERIPHERAL_START_ADDRESS + 0x7000)
#else
#define I2S_TX_SINK_START_ADDRESS (EXT_PERIPHERAL_START_ADDRESS + 0x6000)
#endif

#define I2S_TX_SINK_CONTROL_SINK_EN_BIT 0
#define I2S_TX_SINK_SINK_EN (1u << I2S_TX_SINK_CONTROL_SINK_EN_BIT)

#define I2S_TX_CLK_DIV 32
#define SAMPLE_COUNT 4
#define POLL_TIMEOUT 1000000

static const uint32_t kTxSamples[SAMPLE_COUNT] = {
    0x01234567,
    0x89abcdef,
    0x0badcafe,
    0x13579bdf,
};

static inline uint32_t sink_read32(mmio_region_t sink, ptrdiff_t offset)
{
    return mmio_region_read32(sink, offset);
}

static inline void sink_write32(mmio_region_t sink, ptrdiff_t offset, uint32_t value)
{
    mmio_region_write32(sink, offset, value);
}

static bool wait_tx_ready(void)
{
    for (uint32_t i = 0; i < POLL_TIMEOUT; ++i) {
        if (i2s_tx_ready()) {
            return true;
        }
    }
    return false;
}

static bool wait_sink_available(mmio_region_t sink)
{
    for (uint32_t i = 0; i < POLL_TIMEOUT; ++i) {
        uint32_t status = sink_read32(sink, I2S_TX_SINK_STATUS_REG_OFFSET);
        if (status & (1u << I2S_TX_SINK_STATUS_OVERFLOW_BIT)) {
            return false;
        }
        if (status & (1u << I2S_TX_SINK_STATUS_AVAILABLE_BIT)) {
            return true;
        }
    }
    return false;
}

int main(void)
{
#if TARGET_SIM
    bool success = true;
    mmio_region_t sink =
        mmio_region_from_addr((uintptr_t)I2S_TX_SINK_START_ADDRESS);

    // Keep the TX sink off while I2S is idle.
    sink_write32(sink, I2S_TX_SINK_CONTROL_REG_OFFSET, 0);

    for (uint32_t i = 0; i < SAMPLE_COUNT; ++i) {
        if (!wait_tx_ready() || i2s_tx_write_data(kTxSamples[i]) != kI2sOk) {
            success = false;
            break;
        }
    }

    if (success && i2s_tx_overflow()) {
        success = false;
    }

    if (success && i2s_tx_start() != kI2sOk) {
        success = false;
    }

    if (success) {
        sink_write32(sink, I2S_TX_SINK_CONTROL_REG_OFFSET, I2S_TX_SINK_SINK_EN);
        if (i2s_init(I2S_TX_CLK_DIV, I2S_32_BITS) != kI2sOk) {
            success = false;
        }
    }

    for (uint32_t i = 0; success && i < SAMPLE_COUNT; ++i) {
        if (!wait_sink_available(sink)) {
            success = false;
            break;
        }

        uint32_t rx_sample = sink_read32(sink, I2S_TX_SINK_RXDATA_REG_OFFSET);
        if (rx_sample != kTxSamples[i]) {
            success = false;
        }
    }

    if (sink_read32(sink, I2S_TX_SINK_STATUS_REG_OFFSET) &
        (1u << I2S_TX_SINK_STATUS_OVERFLOW_BIT)) {
        success = false;
    }

    i2s_tx_stop();
    i2s_terminate();
    sink_write32(sink, I2S_TX_SINK_CONTROL_REG_OFFSET, 0);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
#else
    return EXIT_SUCCESS;
#endif
}
