/*
 * Copyright EPFL contributors.
 * Licensed under the Apache License, Version 2.0, see LICENSE for details.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Simultaneous I2S RX/TX simulation test.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "core_v_mini_mcu.h"
#include "i2s.h"
#include "i2s_structs.h"
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

#define I2S_CLK_DIV 32
#define I2S_RX_CHECK_SAMPLES 8
#define I2S_TX_CHECK_SAMPLES 8
#define I2S_TX_TOTAL_SAMPLES (I2S_TX_CHECK_SAMPLES + 4)
#define I2S_TX_PRELOAD_SAMPLES 4
#define I2S_POLL_TIMEOUT 2000000

#define I2S_MIC_LEFT_SAMPLE 0x08765431u
#define I2S_MIC_RIGHT_SAMPLE 0x0fedcba9u

static inline uint32_t sink_read32(mmio_region_t sink, ptrdiff_t offset)
{
    return mmio_region_read32(sink, offset);
}

static inline void sink_write32(mmio_region_t sink, ptrdiff_t offset,
                                uint32_t value)
{
    mmio_region_write32(sink, offset, value);
}

static uint32_t tx_sample(uint32_t sample_idx)
{
    static const uint32_t kSamples[I2S_TX_TOTAL_SAMPLES] = {
        0x01234567u, 0x89abcdefu, 0x0badcafeu, 0x13579bdfu,
        0x2468ace0u, 0xfdb97531u, 0xa5a55a5au, 0xc001d00du,
        0x55aa00ffu, 0xff00aa55u, 0xdeadbeefu, 0x10203040u,
    };

    return kSamples[sample_idx];
}

static bool wait_tx_ready(void)
{
    for (uint32_t i = 0; i < I2S_POLL_TIMEOUT; ++i) {
        if (i2s_tx_ready()) {
            return true;
        }
    }

    return false;
}

static bool tx_write_sample(uint32_t sample_idx)
{
    if (!wait_tx_ready()) {
        printf("I2S TX FIFO was not ready\n");
        return false;
    }

    if (i2s_tx_write_data(tx_sample(sample_idx)) != kI2sOk) {
        printf("I2S TX write failed for sample %u\n", sample_idx);
        return false;
    }

    return true;
}

static bool preload_tx_fifo(uint32_t *tx_written)
{
    while (*tx_written < I2S_TX_PRELOAD_SAMPLES) {
        if (!tx_write_sample(*tx_written)) {
            return false;
        }
        ++(*tx_written);
    }

    return true;
}

static bool arm_i2s_rx_tx(void)
{
    uint32_t control = i2s_peri->CONTROL;

    if ((bitfield_field32_read(control, I2S_CONTROL_EN_RX_FIELD) !=
         I2S_DISABLE) ||
        (control & (1u << I2S_CONTROL_EN_TX_BIT))) {
        printf("I2S RX or TX was already enabled\n");
        return false;
    }

    // Arm both channels before enabling SCK/WS in i2s_init().
    control = bitfield_field32_write(control, I2S_CONTROL_EN_RX_FIELD,
                                     I2S_BOTH_CH);
    control |= (1u << I2S_CONTROL_EN_TX_BIT);
    control |= (1u << I2S_CONTROL_RESET_WATERMARK_BIT);
    i2s_peri->CONTROL = control;

    return true;
}

static bool rx_sample_matches(uint32_t sample, uint32_t sample_idx,
                              uint32_t *first_sample)
{
    if (sample_idx == 0) {
        if ((sample != I2S_MIC_LEFT_SAMPLE) &&
            (sample != I2S_MIC_RIGHT_SAMPLE)) {
            printf("RX sample 0 = 0x%08x, expected microphone pattern\n",
                   sample);
            return false;
        }

        *first_sample = sample;
        return true;
    }

    uint32_t expected =
        (*first_sample == I2S_MIC_LEFT_SAMPLE)
            ? ((sample_idx & 1u) ? I2S_MIC_RIGHT_SAMPLE : I2S_MIC_LEFT_SAMPLE)
            : ((sample_idx & 1u) ? I2S_MIC_LEFT_SAMPLE : I2S_MIC_RIGHT_SAMPLE);

    if (sample != expected) {
        printf("RX sample %u = 0x%08x, expected 0x%08x\n", sample_idx,
               sample, expected);
        return false;
    }

    return true;
}

static bool sink_sample_matches(uint32_t sample, uint32_t sample_idx)
{
    uint32_t expected = tx_sample(sample_idx);

    if (sample != expected) {
        printf("TX sink sample %u = 0x%08x, expected 0x%08x\n", sample_idx,
               sample, expected);
        return false;
    }

    return true;
}

static bool run_i2s_rx_tx_test(void)
{
    bool success = true;
    uint32_t tx_written = 0;
    uint32_t rx_read = 0;
    uint32_t sink_read = 0;
    uint32_t first_rx_sample = 0;
    mmio_region_t sink =
        mmio_region_from_addr((uintptr_t)I2S_TX_SINK_START_ADDRESS);

    sink_write32(sink, I2S_TX_SINK_CONTROL_REG_OFFSET, 0);

    success = preload_tx_fifo(&tx_written);
    success = success && arm_i2s_rx_tx();

    if (success) {
        sink_write32(sink, I2S_TX_SINK_CONTROL_REG_OFFSET,
                     I2S_TX_SINK_SINK_EN);
        success = (i2s_init(I2S_CLK_DIV, I2S_32_BITS) == kI2sOk);
        if (!success) {
            printf("I2S init failed\n");
        }
    }

    for (uint32_t timeout = I2S_POLL_TIMEOUT;
         success && timeout &&
         ((rx_read < I2S_RX_CHECK_SAMPLES) ||
          (sink_read < I2S_TX_CHECK_SAMPLES));
         --timeout) {
        if ((tx_written < I2S_TX_TOTAL_SAMPLES) && i2s_tx_ready()) {
            if (i2s_tx_write_data(tx_sample(tx_written)) != kI2sOk) {
                printf("I2S TX write failed for sample %u\n", tx_written);
                success = false;
                break;
            }
            ++tx_written;
        }

        if (i2s_rx_data_available()) {
            uint32_t sample = i2s_rx_read_data();
            if (!rx_sample_matches(sample, rx_read, &first_rx_sample)) {
                success = false;
                break;
            }
            ++rx_read;
        }

        uint32_t sink_status =
            sink_read32(sink, I2S_TX_SINK_STATUS_REG_OFFSET);
        if (sink_status & (1u << I2S_TX_SINK_STATUS_OVERFLOW_BIT)) {
            printf("I2S TX sink overflowed\n");
            success = false;
            break;
        }

        if (sink_status & (1u << I2S_TX_SINK_STATUS_AVAILABLE_BIT)) {
            uint32_t sample =
                sink_read32(sink, I2S_TX_SINK_RXDATA_REG_OFFSET);
            if (sink_read < I2S_TX_CHECK_SAMPLES) {
                if (!sink_sample_matches(sample, sink_read)) {
                    success = false;
                    break;
                }
            }
            ++sink_read;
        }

        if (i2s_rx_overflow()) {
            printf("I2S RX FIFO overflowed\n");
            success = false;
            break;
        }

        if (i2s_tx_overflow()) {
            printf("I2S TX FIFO overflowed\n");
            success = false;
            break;
        }

        if (i2s_tx_underflow() &&
            ((rx_read < I2S_RX_CHECK_SAMPLES) ||
             (sink_read < I2S_TX_CHECK_SAMPLES))) {
            printf("I2S TX underflowed before the checks completed\n");
            success = false;
            break;
        }
    }

    if (success && (rx_read < I2S_RX_CHECK_SAMPLES)) {
        printf("I2S RX timed out after %u samples\n", rx_read);
        success = false;
    }

    if (success && (sink_read < I2S_TX_CHECK_SAMPLES)) {
        printf("I2S TX sink timed out after %u samples\n", sink_read);
        success = false;
    }

    if (i2s_is_running()) {
        i2s_result_t rx_stop_res = i2s_rx_stop();
        if ((rx_stop_res != kI2sOk) && success) {
            printf("I2S RX stop failed with %d\n", rx_stop_res);
            success = false;
        }
    }

    i2s_result_t tx_stop_res = i2s_tx_stop();
    if ((tx_stop_res != kI2sOk) && success) {
        printf("I2S TX stop failed with %d\n", tx_stop_res);
        success = false;
    }

    i2s_terminate();
    i2s_peri->CONTROL &=
        ~(I2S_CONTROL_EN_RX_MASK << I2S_CONTROL_EN_RX_OFFSET);
    sink_write32(sink, I2S_TX_SINK_CONTROL_REG_OFFSET, 0);

    return success;
}

int main(void)
{
#if TARGET_SIM
    bool success = run_i2s_rx_tx_test();

    if (success) {
        printf("Success.\n");
        return EXIT_SUCCESS;
    }

    printf("Failure.\n");
    return EXIT_FAILURE;
#else
    return EXIT_SUCCESS;
#endif
}
