/*
    Copyright EPFL contributors.
    Licensed under the Apache License, Version 2.0, see LICENSE for details.
    SPDX-License-Identifier: Apache-2.0

    Author: Tommaso Terzano <tommaso.terzano@epfl.ch>
                            <tommaso.terzano@gmail.com>

    Info: This simple HAL is used to load the im2col SPC and to run it.
*/

#include "im2col.h"

void handler_irq_im2col_spc( void )
{
  /* Read the IFR to lower the interrupt flag */
  * (volatile uint32_t * )(IM2COL_SPC_BASE_ADDR + IM2COL_SPC_SPC_IFR_REG_OFFSET);
  return;
}

/* Simple function to extract the channel from the channel mask */
int get_channel_number(uint32_t mask) {
    int channel = 0;
    
    // Check the position of the first set bit
    while (mask > 0) {
        channel++;
        if (mask & 1) {  // If the least significant bit is set
            return channel - 1;
        }
        mask >>= 1;  // Right shift the mask to check the next bit
    }
    
    // If no bit is set, return -1 indicating no valid channel
    return -1;
}

int run_im2col(im2col_trans_t trans){

  /* Initializing PLIC */
  if(plic_Init()) 
  {
      return EXIT_FAILURE;
  };

  if(plic_irq_set_priority(EXT_INTR_2, 1)) 
  {
      return EXIT_FAILURE;
  };

  if(plic_irq_set_enabled(EXT_INTR_2, kPlicToggleEnabled)) 
  {
      return EXIT_FAILURE;
  };

  plic_assign_external_irq_handler(EXT_INTR_2, &handler_irq_im2col_spc);

  /* Enable global interrupt for machine-level interrupts */
  CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);

  /* Set mie.MEIE bit to one to enable machine-level external interrupts */
  const uint32_t mask = 1 << 11;
  CSR_SET_BITS(CSR_REG_MIE, mask);

  dma_init(NULL);

  /* Write the DMA channel mask that the SPC has access to */
  write_register( trans.ch_mask,
                  IM2COL_SPC_SPC_CH_MASK_REG_OFFSET,
                  0xffffffff,
                  0,
                  IM2COL_SPC_BASE_ADDR );

  /* Write the offset of the DMA channel the SPC has access to */
  write_register( get_channel_number(trans.ch_mask) * DMA_CH_SIZE + DMA_START_ADDRESS,
                  IM2COL_SPC_SPC_CH_OFFSET_REG_OFFSET,
                  0xffffffff,
                  0,
                  IM2COL_SPC_BASE_ADDR );

  /* Write the source */
  write_register( trans.src,
                  IM2COL_SPC_SRC_PTR_REG_OFFSET,
                  0xffffffff,
                  0,
                  IM2COL_SPC_BASE_ADDR );

  /* Write the destination */
  write_register( trans.dst,
                  IM2COL_SPC_DST_PTR_REG_OFFSET,
                  0xffffffff,
                  0,
                  IM2COL_SPC_BASE_ADDR );

  /* Write the datatype */
  write_register( DMA_DATA_TYPE_WORD,
                  IM2COL_SPC_DATA_TYPE_REG_OFFSET,
                  IM2COL_SPC_DATA_TYPE_DATA_TYPE_MASK,
                  IM2COL_SPC_DATA_TYPE_DATA_TYPE_OFFSET,
                  IM2COL_SPC_BASE_ADDR );

  /* Write the filter dimensions */
  write_register( trans.filter_width,
                  IM2COL_SPC_FW_REG_OFFSET,
                  IM2COL_SPC_FW_SIZE_MASK,
                  IM2COL_SPC_FW_SIZE_OFFSET,
                  IM2COL_SPC_BASE_ADDR );

  write_register( trans.filter_height,
                  IM2COL_SPC_FH_REG_OFFSET,
                  IM2COL_SPC_FH_SIZE_MASK,
                  IM2COL_SPC_FH_SIZE_OFFSET,
                  IM2COL_SPC_BASE_ADDR );

  /* Write the image dimensions */
  write_register( trans.im_width,
                  IM2COL_SPC_IW_REG_OFFSET,
                  0xffffffff,
                  0,
                  IM2COL_SPC_BASE_ADDR );

  write_register( trans.im_height,
                  IM2COL_SPC_IH_REG_OFFSET,
                  0xffffffff,
                  0,
                  IM2COL_SPC_BASE_ADDR );

  /* Write the CH_COL */
  write_register( trans.num_channels_col,
                  IM2COL_SPC_CH_COL_REG_OFFSET,
                  IM2COL_SPC_CH_COL_NUM_MASK,
                  IM2COL_SPC_CH_COL_NUM_OFFSET,
                  IM2COL_SPC_BASE_ADDR );

  /* Write n_patches */
  write_register( trans.n_patches_w,
                  IM2COL_SPC_N_PATCHES_W_REG_OFFSET,
                  IM2COL_SPC_N_PATCHES_W_NUM_MASK,
                  IM2COL_SPC_N_PATCHES_W_NUM_OFFSET,
                  IM2COL_SPC_BASE_ADDR );

  write_register( trans.n_patches_h,
                  IM2COL_SPC_N_PATCHES_H_REG_OFFSET,
                  IM2COL_SPC_N_PATCHES_H_NUM_MASK,
                  IM2COL_SPC_N_PATCHES_H_NUM_OFFSET,
                  IM2COL_SPC_BASE_ADDR );

  /* Write the padding */
  write_register( trans.left_pad,
                  IM2COL_SPC_PAD_LEFT_REG_OFFSET,
                  IM2COL_SPC_PAD_LEFT_PAD_MASK,
                  IM2COL_SPC_PAD_LEFT_PAD_OFFSET,
                  IM2COL_SPC_BASE_ADDR );

  write_register( trans.right_pad,
                  IM2COL_SPC_PAD_RIGHT_REG_OFFSET,
                  IM2COL_SPC_PAD_RIGHT_PAD_MASK,
                  IM2COL_SPC_PAD_RIGHT_PAD_OFFSET,
                  IM2COL_SPC_BASE_ADDR );

  write_register( trans.top_pad,
                  IM2COL_SPC_PAD_TOP_REG_OFFSET,
                  IM2COL_SPC_PAD_TOP_PAD_MASK,
                  IM2COL_SPC_PAD_TOP_PAD_OFFSET,
                  IM2COL_SPC_BASE_ADDR );

  write_register( trans.bottom_pad,
                  IM2COL_SPC_PAD_BOTTOM_REG_OFFSET,
                  IM2COL_SPC_PAD_BOTTOM_PAD_MASK,
                  IM2COL_SPC_PAD_BOTTOM_PAD_OFFSET,
                  IM2COL_SPC_BASE_ADDR );

  /* 
    * Write the strides. With respect to test_2 these are the application-point-of-view
    * strides, so they are the same as STRIDE_D1 and STRIDE_D2.
    */
  write_register( (int) log2(trans.stride_d1),
                  IM2COL_SPC_LOG_STRIDES_D1_REG_OFFSET,
                  IM2COL_SPC_LOG_STRIDES_D1_SIZE_MASK,
                  IM2COL_SPC_LOG_STRIDES_D1_SIZE_OFFSET,
                  IM2COL_SPC_BASE_ADDR );

  write_register( (int) log2(trans.stride_d2),
                  IM2COL_SPC_LOG_STRIDES_D2_REG_OFFSET,
                  IM2COL_SPC_LOG_STRIDES_D2_SIZE_MASK,
                  IM2COL_SPC_LOG_STRIDES_D2_SIZE_OFFSET,
                  IM2COL_SPC_BASE_ADDR );

  /* Write the batch size */
  write_register( trans.batch,
                  IM2COL_SPC_BATCH_REG_OFFSET,
                  IM2COL_SPC_BATCH_SIZE_MASK,
                  IM2COL_SPC_BATCH_SIZE_OFFSET,
                  IM2COL_SPC_BASE_ADDR );

  /* Write the adapted pad regions */
  write_register( trans.adpt_pad_right,
                  IM2COL_SPC_ADPT_PAD_RIGHT_REG_OFFSET,
                  0xffffffff,
                  0,
                  IM2COL_SPC_BASE_ADDR );

  write_register( trans.adpt_pad_bottom,
                  IM2COL_SPC_ADPT_PAD_BOTTOM_REG_OFFSET,
                  0xffffffff,
                  0,
                  IM2COL_SPC_BASE_ADDR );

  /* Enable the interrupt logic */
  write_register( 0x1,
                  IM2COL_SPC_INTERRUPT_EN_REG_OFFSET,
                  0x1,
                  IM2COL_SPC_INTERRUPT_EN_EN_BIT,
                  IM2COL_SPC_BASE_ADDR );

  /* Write the number of channels to start the process */
  write_register( trans.num_channels,
                  IM2COL_SPC_NUM_CH_REG_OFFSET,
                  IM2COL_SPC_NUM_CH_NUM_MASK,
                  IM2COL_SPC_NUM_CH_NUM_OFFSET,
                  IM2COL_SPC_BASE_ADDR );
}
