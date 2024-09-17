#ifndef INPUT_IMAGE_NCHW_H
#define INPUT_IMAGE_NCHW_H

/*
	Copyright EPFL contributors.
	Licensed under the Apache License, Version 2.0, see LICENSE for details.
	SPDX-License-Identifier: Apache-2.0
*/

#include <stdint.h>

#define IH 40
#define IW 40
#define CH 2
#define BATCH 1
#define FH 3
#define FW 3
#define TOP_PAD 1
#define BOTTOM_PAD 1
#define LEFT_PAD 1
#define RIGHT_PAD 1
#define STRIDE_D1 1
#define STRIDE_D2 1

extern const uint8_t input_image_nchw[3200];

#endif // INPUT_IMAGE_NCHW_H
