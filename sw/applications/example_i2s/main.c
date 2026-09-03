/*
 * Copyright EPFL contributors.
 * Licensed under the Apache License, Version 2.0, see LICENSE for details.
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2S example application.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitfield.h"
#include "dma.h"
#include "i2s.h"
#include "i2s_structs.h"
#include "i2s_tx_sink_regs.h"
#include "mmio.h"

#include "test_i2s.h"

int main(void)
{
    bool passed = true;

#ifdef TEST_ID_0
    PRINTF("TEST_ID_0: I2S RX-only DMA test\n\r");

#ifdef TARGET_IS_FPGA
    for (uint32_t i = 0; i < I2S_FPGA_WAIT_CYCLES; ++i) {
        asm volatile("nop");
    }

#pragma message("this application takes multiple I2S microphone batches")

    for (uint32_t batch = 0; batch < I2S_RX_FPGA_BATCHES; ++batch) {
        bool rx_stopped = false;

        PRINTF("starting\r\n\r");

        clear_samples(rx_only_samples, I2S_RX_ONLY_SAMPLES);
        dma_init(NULL);

        passed = configure_rx_dma(rx_only_samples, I2S_RX_ONLY_SAMPLES,
                                  I2S_RX_ONLY_DMA_CH, "I2S RX-only");

        if (passed && (i2s_init(I2S_FPGA_CLK_DIV, I2S_32_BITS) != kI2sOk)) {
            printf("I2S init failed\n");
            passed = false;
        }

        if (passed) {
            passed = launch_dma_transaction(&rx_trans, "I2S RX-only");
        }

        if (passed) {
            i2s_result_t rx_start_res = i2s_rx_start(I2S_LEFT_CH);
            if (rx_start_res != kI2sOk) {
                printf("I2S RX start failed with %d\n", rx_start_res);
                passed = false;
            }
        }

        if (passed) {
            passed = wait_dma_ready(I2S_RX_ONLY_DMA_CH, "I2S RX-only");
        }

        if (passed && i2s_rx_overflow()) {
            printf("I2S RX FIFO overflowed\n");
            passed = false;
        }

        if (i2s_is_running() &&
            (bitfield_field32_read(i2s_peri->CONTROL,
                                   I2S_CONTROL_EN_RX_FIELD) != I2S_DISABLE)) {
            passed = stop_rx(&rx_stopped) && passed;
        }

        i2s_terminate();

        if (!passed) {
            PRINTF("TEST_ID_0 failed\n\r");
            return EXIT_FAILURE;
        }

        for (uint32_t i = 0; i < I2S_RX_ONLY_SAMPLES; ++i) {
            PRINTF("%d\r\n\r", (int16_t)(rx_only_samples[i] >> 16));
        }
        PRINTF("Batch done!\r\n\r");
    }
#else
    bool rx_only_stopped = false;

    clear_samples(rx_only_samples, I2S_RX_ONLY_SAMPLES);
    dma_init(NULL);

    passed = configure_rx_dma(rx_only_samples, I2S_RX_ONLY_SAMPLES,
                              I2S_RX_ONLY_DMA_CH, "I2S RX-only");

    if (passed && (i2s_init(I2S_SIM_CLK_DIV, I2S_32_BITS) != kI2sOk)) {
        printf("I2S init failed\n");
        passed = false;
    }

    if (passed) {
        passed = launch_dma_transaction(&rx_trans, "I2S RX-only");
    }

    if (passed) {
        i2s_result_t rx_start_res = i2s_rx_start(I2S_BOTH_CH);
        if (rx_start_res != kI2sOk) {
            printf("I2S RX start failed with %d\n", rx_start_res);
            passed = false;
        }
    }

    if (passed) {
        passed = wait_dma_ready(I2S_RX_ONLY_DMA_CH, "I2S RX-only");
    }

    if (passed && i2s_rx_overflow()) {
        printf("I2S RX FIFO overflowed\n");
        passed = false;
    }

    if (i2s_is_running() &&
        (bitfield_field32_read(i2s_peri->CONTROL, I2S_CONTROL_EN_RX_FIELD) !=
         I2S_DISABLE)) {
        passed = stop_rx(&rx_only_stopped) && passed;
    }

    i2s_terminate();

#if TARGET_SIM
    if (passed && !check_rx_samples(rx_only_samples, I2S_RX_ONLY_SAMPLES)) {
        passed = false;
    }
#endif
#endif

    if (!passed) {
        PRINTF("TEST_ID_0 failed\n\r");
        return EXIT_FAILURE;
    }
#endif

#ifdef TEST_ID_1
    PRINTF("TEST_ID_1: I2S TX-only DMA test\n\r");

#if TARGET_SIM
    bool tx_only_enabled = false;
    bool tx_only_completed = false;
    mmio_region_t tx_only_sink =
        mmio_region_from_addr((uintptr_t)I2S_TX_SINK_START_ADDRESS);

    select_gpio_13_pad(1);
    dma_init(NULL);
    mmio_region_write32(tx_only_sink, I2S_TX_SINK_CONTROL_REG_OFFSET, 0);

    passed = configure_tx_dma(I2S_TX_DMA_CH);

    if (passed) {
        i2s_result_t tx_start_res = i2s_tx_start();
        tx_only_enabled = tx_start_res == kI2sOk;
        if (tx_start_res != kI2sOk) {
            printf("I2S TX start failed with %d\n", tx_start_res);
            passed = false;
        }
    }

    if (passed) {
        passed = launch_dma_transaction(&tx_trans, "I2S TX");
    }

    if (passed) {
        mmio_region_write32(tx_only_sink, I2S_TX_SINK_CONTROL_REG_OFFSET,
                            I2S_TX_SINK_SINK_EN);
        if (i2s_init(I2S_SIM_CLK_DIV, I2S_32_BITS) != kI2sOk) {
            printf("I2S init failed\n");
            passed = false;
        }
    }

    if (passed) {
        tx_only_completed =
            wait_tx_dma_and_sink(tx_only_sink, I2S_TX_DMA_CH);
        passed = tx_only_completed;
    }

    passed = stop_tx(tx_only_enabled, tx_only_completed) && passed;
    i2s_terminate();
    mmio_region_write32(tx_only_sink, I2S_TX_SINK_CONTROL_REG_OFFSET, 0);
    select_gpio_13_pad(0);
#else
    PRINTF("Skipping I2S TX-only test outside simulation.\n\r");
#endif

    if (!passed) {
        PRINTF("TEST_ID_1 failed\n\r");
        return EXIT_FAILURE;
    }
#endif

#ifdef TEST_ID_2
    PRINTF("TEST_ID_2: simultaneous I2S RX/TX DMA test\n\r");

#if TARGET_SIM
    bool rx_tx_rx_dma_done = false;
    bool rx_tx_tx_dma_done = false;
    bool rx_tx_stopped = false;
    bool rx_tx_tx_enabled = false;
    bool rx_tx_tx_completed = false;
    uint32_t rx_tx_sink_read = 0;
    mmio_region_t rx_tx_sink =
        mmio_region_from_addr((uintptr_t)I2S_TX_SINK_START_ADDRESS);

    clear_samples(rx_tx_samples, I2S_RX_TX_SAMPLES);
    select_gpio_13_pad(1);
    dma_init(NULL);
    mmio_region_write32(rx_tx_sink, I2S_TX_SINK_CONTROL_REG_OFFSET, 0);

    passed = configure_rx_dma(rx_tx_samples, I2S_RX_TX_SAMPLES, I2S_RX_DMA_CH,
                              "I2S RX");
    passed = passed && configure_tx_dma(I2S_TX_DMA_CH);

    if (passed) {
        passed = arm_i2s_rx_tx();
        rx_tx_tx_enabled = passed;
    }

    if (passed) {
        passed = launch_dma_transaction(&rx_trans, "I2S RX") &&
                 launch_dma_transaction(&tx_trans, "I2S TX");
    }

    if (passed) {
        mmio_region_write32(rx_tx_sink, I2S_TX_SINK_CONTROL_REG_OFFSET,
                            I2S_TX_SINK_SINK_EN);
        if (i2s_init(I2S_SIM_CLK_DIV, I2S_32_BITS) != kI2sOk) {
            printf("I2S init failed\n");
            passed = false;
        }
    }

    for (uint32_t timeout = I2S_POLL_TIMEOUT; passed && timeout; --timeout) {
        uint32_t sink_status =
            mmio_region_read32(rx_tx_sink, I2S_TX_SINK_STATUS_REG_OFFSET);
        if (sink_status & (1u << I2S_TX_SINK_STATUS_OVERFLOW_BIT)) {
            printf("I2S TX sink overflowed\n");
            passed = false;
            break;
        }

        if (sink_status & (1u << I2S_TX_SINK_STATUS_AVAILABLE_BIT)) {
            uint32_t sample =
                mmio_region_read32(rx_tx_sink,
                                   I2S_TX_SINK_RXDATA_REG_OFFSET);
            if (rx_tx_sink_read >= I2S_TX_SAMPLES) {
                printf("I2S TX sink received more samples than expected\n");
                passed = false;
                break;
            }
            if (!sink_sample_matches(sample, rx_tx_sink_read)) {
                passed = false;
                break;
            }
            ++rx_tx_sink_read;
        }

        rx_tx_rx_dma_done = dma_is_ready(I2S_RX_DMA_CH) != 0;
        rx_tx_tx_dma_done = dma_is_ready(I2S_TX_DMA_CH) != 0;
        rx_tx_tx_completed =
            rx_tx_tx_dma_done && (rx_tx_sink_read >= I2S_TX_SAMPLES);

        if (rx_tx_rx_dma_done && !rx_tx_stopped &&
            !stop_rx(&rx_tx_stopped)) {
            passed = false;
            break;
        }

        if (rx_tx_stopped && rx_tx_tx_completed) {
            break;
        }

        if (!rx_tx_stopped && i2s_rx_overflow()) {
            printf("I2S RX FIFO overflowed\n");
            passed = false;
            break;
        }

        if (i2s_tx_overflow()) {
            printf("I2S TX FIFO overflowed\n");
            passed = false;
            break;
        }

        if (i2s_tx_underflow() && !rx_tx_tx_completed) {
            printf("I2S TX underflowed before all samples were sent\n");
            passed = false;
            break;
        }
    }

    rx_tx_rx_dma_done = dma_is_ready(I2S_RX_DMA_CH) != 0;
    rx_tx_tx_dma_done = dma_is_ready(I2S_TX_DMA_CH) != 0;
    rx_tx_tx_completed =
        rx_tx_tx_dma_done && (rx_tx_sink_read >= I2S_TX_SAMPLES);

    if (passed && !rx_tx_rx_dma_done) {
        printf("I2S RX DMA timed out\n");
        passed = false;
    }

    if (passed && !rx_tx_tx_dma_done) {
        printf("I2S TX DMA timed out\n");
        passed = false;
    }

    if (passed && (rx_tx_sink_read < I2S_TX_SAMPLES)) {
        printf("I2S TX sink timed out after %u samples\n", rx_tx_sink_read);
        passed = false;
    }

    if (passed && !check_rx_samples(rx_tx_samples, I2S_RX_TX_SAMPLES)) {
        passed = false;
    }

    if (i2s_is_running() && !rx_tx_stopped) {
        passed = stop_rx(&rx_tx_stopped) && passed;
    }

    passed = stop_tx(rx_tx_tx_enabled, rx_tx_tx_completed) && passed;
    i2s_terminate();
    i2s_peri->CONTROL &=
        ~(I2S_CONTROL_EN_RX_MASK << I2S_CONTROL_EN_RX_OFFSET);
    mmio_region_write32(rx_tx_sink, I2S_TX_SINK_CONTROL_REG_OFFSET, 0);
    select_gpio_13_pad(0);
#else
    PRINTF("Skipping simultaneous I2S RX/TX test outside simulation.\n\r");
#endif

    if (!passed) {
        PRINTF("TEST_ID_2 failed\n\r");
        return EXIT_FAILURE;
    }
#endif

    PRINTF("Success.\n\r");
    return EXIT_SUCCESS;
}
