#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "dma.h"

#define TEST_WORDS 256

/* DDR buffers */
static uint32_t ddr_src[TEST_WORDS] __attribute__((aligned(4)));
static uint32_t ddr_dst[TEST_WORDS] __attribute__((aligned(4)));

/* SRAM buffers: plain static arrays -> default .bss, which lives in SRAM */
static uint32_t sram_src[TEST_WORDS] __attribute__((aligned(4)));
static uint32_t sram_dst[TEST_WORDS] __attribute__((aligned(4)));

/*
 * HAL structs declared globally so every field not set below is zero
 * (the HAL relies on that for unused fields).
 */
static dma_target_t tgt_src = {
    .inc_d1_du = 1,
    .type      = DMA_DATA_TYPE_WORD,
    .trig      = DMA_TRIG_MEMORY,
};

static dma_target_t tgt_dst = {
    .inc_d1_du = 1,
    .type      = DMA_DATA_TYPE_WORD,
    .trig      = DMA_TRIG_MEMORY,
};

static dma_trans_t trans = {
    .src        = &tgt_src,
    .dst        = &tgt_dst,
    .src_addr   = NULL,
    .size_d1_du = TEST_WORDS,
    .mode       = DMA_TRANS_MODE_SINGLE,
    .win_du     = 0,
    .end        = DMA_TRANS_END_INTR_WAIT,
};

static dma_config_flags_t run_dma_trans(uint32_t *src, uint32_t *dst)
{
    dma_config_flags_t res;

    tgt_src.ptr = (uint8_t *)src;
    tgt_dst.ptr = (uint8_t *)dst;

    res  = dma_validate_transaction(&trans, DMA_ENABLE_REALIGN,
                                    DMA_PERFORM_CHECKS_INTEGRITY);
    res |= dma_load_transaction(&trans);
    res |= dma_launch(&trans);

    return res;
}

/*
 * Generic test: fill src with pattern, clear dst, DMA copy, verify.
 * Returns number of mismatching words.
 */
static uint32_t run_test(const char *name, uint32_t *src, uint32_t *dst)
{
    uint32_t errors = 0;

    printf("\n=== %s ===\n", name);
    printf("src: 0x%08x\n", (unsigned)(uintptr_t)src);
    printf("dst: 0x%08x\n", (unsigned)(uintptr_t)dst);

    for (uint32_t i = 0; i < TEST_WORDS; i++) {
        src[i] = 0xA5000000u | i;
        dst[i] = 0;
    }

    for (uint32_t i = 0; i < TEST_WORDS; i++) {
        if (src[i] != (0xA5000000u | i) || dst[i] != 0) {
            printf("CPU read/write failed at %u\n", i);
            return TEST_WORDS;
        }
    }
    printf("CPU access OK\n");

    printf("Starting DMA copy...\n");
    dma_config_flags_t res = run_dma_trans(src, dst);
    if (res != DMA_CONFIG_OK) {
        printf("%s: DMA config failed, flags=0x%x\n", name, (unsigned)res);
        return TEST_WORDS;
    }
    printf("DMA copy finished\n");

    for (uint32_t i = 0; i < TEST_WORDS; i++) {
        if (dst[i] != src[i]) {
            if (errors < 20) {
                printf("ERROR [%u]: src=0x%08x dst=0x%08x\n",
                       i, src[i], dst[i]);
            }
            errors++;
        }
    }

    if (errors == 0) {
        printf("%s PASSED\n", name);
        return 0;
    }

    printf("%s FAILED: %u / %u words\n", name, errors, TEST_WORDS);
    for (uint32_t i = 0; i < TEST_WORDS; i++) {
        printf("Index: %u, SRC_VAL: %08x, DST_VAL: %08x\n",
               i, src[i], dst[i]);
    }
    return errors;
}

int main(void)
{
    uint32_t total = 0;

    printf("\nDMA memory-to-memory tests (HAL)\n");
    dma_init(NULL);

    total += run_test("SRAM -> SRAM", sram_src, sram_dst); /* control      */
    total += run_test("DDR -> SRAM",  ddr_src,  sram_dst); /* DDR read     */
    total += run_test("SRAM -> DDR",  sram_src, ddr_dst);  /* DDR write    */
    total += run_test("DDR -> DDR",   ddr_src,  ddr_dst);  /* original     */

    printf("\n=== SUMMARY ===\n");
    printf("Total errors: %u\n", total);

    return (total == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}