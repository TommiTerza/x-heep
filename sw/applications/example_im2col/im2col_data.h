#ifndef TEST_DATA_H
#define TEST_DATA_H

#include <stdint.h>

#define IH 10
#define IW 10
#define CH 3
#define BATCH 1
#define FW 3
#define FH 3
#define TOP_PAD 3
#define BOTTOM_PAD 3
#define LEFT_PAD 3
#define RIGHT_PAD 4
#define STRIDE_D1 1
#define STRIDE_D2 1
#define INPUT_DATATYPE 0

extern const uint32_t input_image_nchw[300];
extern const uint32_t golden_im2col_nchw[5670];

#endif // TEST_DATA_H
