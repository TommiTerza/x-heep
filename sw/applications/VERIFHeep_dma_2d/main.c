/*
 *  Copyright EPFL contributors.
 *  Licensed under the Apache License, Version 2.0, see LICENSE for details.
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Author: Tommaso Terzano <tommaso.terzano@epfl.ch>
 *  
 *  Info: Verification script of the 2D DMA CH0.
 */

#include <stdio.h>
#include <stdlib.h>
#include "dma.h"
#include "core_v_mini_mcu.h"
#include "x-heep.h"
#include "csr.h"
#include "rv_plic.h"
#include "test_data.h"

/*  
 *  This code contains four different tests that can be run by defining the corresponding TEST_ID_* macro.
 *  - Extract a NxM matrix, perform optional padding and copy it to a AxB matrix, using HALs
 *  - Extract a NxM matrix and copy its transposed version to AxB matrix, using HALs
 *  - Extract a 1xN matrix (array), perform optional padding and copy it to an array, using HALs
 *  - Extract a NxM matrix, perform optional padding and copy it to a AxB matrix, using direct register operations
 */

/* Parameters */

/* Size of the extracted matrix (including strides on the input, excluding strides on the outputs) */
#define SIZE_EXTR_D1 4
#define SIZE_EXTR_D2 4

/* Mask for the direct register operations example */
#define DMA_CSR_REG_MIE_MASK (( 1 << 19 ) | (1 << 11 ))

/* Transposition example def */
#define TRANSPOSITION_EN 1

/* By default, printfs are activated for FPGA and disabled for simulation. */
#define PRINTF_IN_FPGA  1
#define PRINTF_IN_SIM   1

#if TARGET_SIM && PRINTF_IN_SIM
        #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#elif TARGET_PYNQ_Z2 && PRINTF_IN_FPGA
    #define PRINTF(fmt, ...)    printf(fmt, ## __VA_ARGS__)
#else
    #define PRINTF(...)
#endif

dma_config_flags_t res_valid, res_load, res_launch;

dma *peri = dma_peri(0);

dma_target_t tgt_src;
dma_target_t tgt_dst;
dma_trans_t trans;

uint32_t dst_ptr = 0, src_ptr = 0;
uint32_t cycles_dma, cycles_cpu;
uint32_t size_dst_trans_d1;
uint32_t dst_stride_d1;
uint32_t dst_stride_d2;
uint32_t size_src_trans_d1;
uint32_t src_stride_d1;
uint32_t src_stride_d2;
uint32_t i_in;
uint32_t j_in;
uint32_t i_in_last;
uint16_t left_pad_cnt = 0;
uint16_t top_pad_cnt = 0;
uint8_t stride_1d_cnt = 0;
uint8_t stride_2d_cnt = 0;
char passed = 1;
int res;

dma_input_data_type copied_data_2D_DMA[SIZE_IN_D1*SIZE_IN_D2];
dma_input_data_type copied_data_2D_CPU[SIZE_IN_D1*SIZE_IN_D2];

int test_0(int top_pad, int bottom_pad, int left_pad, int right_pad, int stride_src_d1, int stride_src_d2, int stride_dst_d1, int stride_dst_d2)
{
    /* Reset for second test */
    passed = 1;
    i_in = 0;
    j_in = 0;
    left_pad_cnt = 0;
    top_pad_cnt = 0;
    stride_1d_cnt = 0;
    stride_2d_cnt = 0;
    for (int i = 0; i < SIZE_IN_D1 * SIZE_IN_D2; i++)
    {
        copied_data_2D_DMA[i] = 0;
        copied_data_2D_CPU[i] = 0;
    }

    /* Testing copy and padding of a NxM matrix using HALs */

    int src_inc_d1 = stride_src_d1;

    int dst_inc_d1 = stride_dst_d1;

    int src_inc_d2 = (stride_src_d2 * SIZE_IN_D1) - (SIZE_EXTR_D1 - 1 + (stride_src_d1 - 1) * (SIZE_EXTR_D1 - 1));

    int out_d1_pad = SIZE_EXTR_D1 + left_pad + right_pad;

    int out_d2_pad = SIZE_EXTR_D2 + top_pad + bottom_pad;

    int out_d1_pad_stride = (out_d1_pad * stride_dst_d1) - (stride_dst_d1 - 1);

    int out_d2_pad_stride = (out_d2_pad * stride_dst_d2) - (stride_dst_d2 - 1);

    int out_dim_1d = out_d1_pad_stride;

    int dst_inc_d2 = ((stride_dst_d2 - 1) * out_dim_1d + 1);

    int src_inc_trsp_d1 = src_inc_d1;

    int src_inc_trsp_d2 = stride_src_d2 * SIZE_IN_D1;

    int out_dim_2d = out_d1_pad_stride * out_d2_pad_stride;

    tgt_src.ptr = &test_data[0];
    tgt_src.inc_du = src_inc_d1;
    tgt_src.inc_d2_du = src_inc_d2;
    tgt_src.size_du = SIZE_EXTR_D1;
    tgt_src.size_d2_du = SIZE_EXTR_D2;
    tgt_src.trig = DMA_TRIG_MEMORY;
    tgt_src.type = DMA_DATA_TYPE;
    
    tgt_dst.ptr = copied_data_2D_DMA;
    tgt_dst.inc_du = dst_inc_d1;
    tgt_dst.inc_d2_du = dst_inc_d2;
    tgt_dst.trig = DMA_TRIG_MEMORY;

    trans.src = &tgt_src;
    trans.dst = &tgt_dst;
    trans.mode = DMA_TRANS_MODE_SINGLE;
    trans.dim = DMA_DIM_CONF_2D;
    trans.pad_top_du     = top_pad,
    trans.pad_bottom_du  = bottom_pad,
    trans.pad_left_du    = left_pad,
    trans.pad_right_du   = right_pad,
    trans.win_du         = 0,
    trans.end            = DMA_TRANS_END_INTR;
    
    dma_init(NULL);

    res_valid = dma_validate_transaction(&trans, DMA_ENABLE_REALIGN, DMA_PERFORM_CHECKS_INTEGRITY);
    res_load = dma_load_transaction(&trans);
    res_launch = dma_launch(&trans);

    while( ! dma_is_ready(0))

    /* Run the same computation on the CPU */
    for (int i=0; i < out_d2_pad_stride; i++)
    {
        stride_1d_cnt = 0;
        j_in = 0;

        for (int j=0; j < out_d1_pad_stride; j++)
        {
            dst_ptr = i * out_d1_pad_stride + j;
            src_ptr = (i_in - top_pad_cnt ) * stride_src_d2 * SIZE_IN_D1 + (j_in - left_pad_cnt) * stride_src_d1;
            if (i_in < top_pad || i_in >= SIZE_EXTR_D2 + top_pad || j_in < left_pad || j_in >= SIZE_EXTR_D1 + left_pad ||
                stride_1d_cnt != 0 || stride_2d_cnt != 0)
            {
                copied_data_2D_CPU[dst_ptr] = 0;
            }
            else
            {
                copied_data_2D_CPU[dst_ptr] = test_data[src_ptr];
            }

            if (j_in < left_pad && i_in >= top_pad && stride_1d_cnt == 0 && stride_2d_cnt == 0)
            {
                left_pad_cnt++;
            }

            if (stride_1d_cnt == stride_dst_d1 - 1)
            {
                stride_1d_cnt = 0;
                j_in++;
            }
            else
            {
                stride_1d_cnt++;
            }

        }

        if (i_in < top_pad && stride_2d_cnt == 0)
        {
            top_pad_cnt++;
        }
        
        if (stride_2d_cnt == stride_dst_d2 - 1)
        {
            stride_2d_cnt = 0;
            i_in++;
        }
        else
        {
            stride_2d_cnt++;
        }

        left_pad_cnt = 0;
    }
    
    /* Verify that the DMA and the CPU outputs are the same */
    for (int i = 0; i < out_d2_pad_stride; i++) {
        for (int j = 0; j < out_d1_pad_stride; j++) {
            if (copied_data_2D_DMA[i * out_d1_pad_stride + j] != copied_data_2D_CPU[i * out_d1_pad_stride + j]) {
                passed = 0;
            }
        }
    }

    if (passed) {
        return EXIT_SUCCESS;
    } 
    else 
    {
        PRINTF("Computed parameters: src_inc_d1:%d, src_inc_d2:%d, dst_inc_d1:%d, dst_inc_d2:%d, out_dim_2d: %d\n\r", src_inc_d1, src_inc_d2, dst_inc_d1, dst_inc_d2, out_dim_2d);
        return EXIT_FAILURE;
    }
}

int main()
{    
    int count = 0;
    for (int i=0; i < 5; i++)
    {
        for (int j=0; j < 5; j++)
        {
            for (int k=0; k < 5; k++)
            {
                for (int l=0; l < 5; l++)
                {
                    for (int m=1; m < 5; m++)
                    {
                        for (int n=1; n < 5; n++)
                        {
                            for (int o=1; o < 5; o++)
                            {
                                for (int p=1; p < 5; p++)
                                {
                                    res = test_0(i, j, k, l, m, n, o, p);
                                    if (res == EXIT_FAILURE)
                                    {
                                        printf("Test failed with parameters: top_pad = %d, bottom_pad = %d, left_pad = %d, right_pad = %d, stride_src_d1 = %d, stride_src_d2 = %d, stride_dst_d1 = %d, stride_dst_d2 = %d\n\r", i, j, k, l, m, n, o, p);
                                    } else 
                                    {
                                        printf("%d/%d\n\r", count, 5*5*5*5*4*4*4*4);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}