#ifndef TEST_DATA_H
#define TEST_DATA_H

#include <stdint.h>

#define IH 30
#define IW 30
#define CH 3
#define BATCH 1
#define FW 3
#define FH 3
#define TOP_PAD 1
#define BOTTOM_PAD 1
#define LEFT_PAD 1
#define RIGHT_PAD 1
#define STRIDE_D1 4
#define STRIDE_D2 4
#define INPUT_DATATYPE 0

extern const uint32_t input_image_nchw[2700];
extern const uint32_t golden_im2col_nchw[1728];

#endif // TEST_DATA_H
