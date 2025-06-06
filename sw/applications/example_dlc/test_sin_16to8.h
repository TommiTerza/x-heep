/*
 *  Copyright EPFL contributors.
 *  Licensed under the Apache License, Version 2.0, see LICENSE for details.
 *  SPDX-License-Identifier: Apache-2.0
 *
 */
/**
* @file   sin_1_12_LC_5_32_FM_8_l_0_3_2.h
* @brief  
	A sine of frequency 0.01 Hz, sampled at 0.001 kHz.
	The data is encoded in 12 bits, signed, of amplitude 2047 LSBs
	The input data has a data rate of 0 kbps
	The signal is passed through a digital LC (dLC) block. 
	Each level is 32 LSBs wide (i.e. the 5 less significant bits are ignored).
* @date   2025-02-19
* @author Juan Sapriza (juan.sapriza@epfl.ch)
*/

#ifndef _TEST_SIN_16TO8_
#define _TEST_SIN_16TO8_

#include <stdint.h>


#define LC_PARAMS_LC_LSBS_TO_USE_AS_HYSTERESIS 0
#define LC_PARAMS_LC_LEVEL_WIDTH_BY_BITS 5
#define LC_PARAMS_DATA_IN_TWOS_COMPLEMENT 0
#define LC_PARAMS_LC_ACQUISITION_WORD_SIZE_OF_AMPLITUDE 3
#define LC_PARAMS_LC_ACQUISITION_WORD_SIZE_OF_TIME 2
#define LC_PARAMS_LC_LEVEL_WIDTH_BY_FRACTION 128
#define LC_PARAMS_SIZE_PER_SAMPLE_BITS 8
#define LC_STATS_CROSSINGS 242
#define LC_STATS_D_LVL_OVERFLOW_WORDS 55
#define LC_STATS_D_T_OVERFLOW_WORDS 4
#define FORM_STATS_WORD_SIZE_BITS 8


int16_t sin_data [200] = {0, 128, 256, 383, 509, 632, 753, 871, 986, 1096, 1203, 1304, 1401, 1492, 1577, 1656, 1728, 1793, 1852, 1903, 1946, 1982, 2010, 2030, 2042, 2047, 2042, 2030, 2010, 1982, 1946, 1903, 1852, 1793, 1728, 1656, 1577, 1492, 1401, 1304, 1203, 1096, 986, 871, 753, 632, 509, 383, 256, 128, -1, -129, -257, -384, -510, -633, -754, -872, -987, -1097, -1204, -1305, -1402, -1493, -1578, -1657, -1729, -1794, -1853, -1904, -1947, -1983, -2011, -2031, -2043, -2047, -2043, -2031, -2011, -1983, -1947, -1904, -1853, -1794, -1729, -1657, -1578, -1493, -1402, -1305, -1204, -1097, -987, -872, -754, -633, -510, -384, -257, -129, 0, 128, 256, 383, 509, 632, 753, 871, 986, 1096, 1203, 1304, 1401, 1492, 1577, 1656, 1728, 1793, 1852, 1903, 1946, 1982, 2010, 2030, 2042, 2047, 2042, 2030, 2010, 1982, 1946, 1903, 1852, 1793, 1728, 1656, 1577, 1492, 1401, 1304, 1203, 1096, 986, 871, 753, 632, 509, 383, 256, 128, 0, -129, -257, -384, -510, -633, -754, -872, -987, -1097, -1204, -1305, -1402, -1493, -1578, -1657, -1729, -1794, -1853, -1904, -1947, -1983, -2011, -2031, -2043, -2047, -2043, -2031, -2011, -1983, -1947, -1904, -1853, -1794, -1729, -1657, -1578, -1493, -1402, -1305, -1204, -1097, -987, -872, -754, -633, -510, -384, -257, -129};
uint8_t sin_time [200] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199};
int16_t lc_subs_data [242] = {3, 1, 3, 1, 3, 3, 1, 3, 1, 3, 1, 3, 1, 3, 3, 1, 3, 3, 3, 3, 3, 2, 3, 2, 1, 2, 1, 1, 1, 1, 0, -1, -1, -1, -1, -2, -1, -2, -3, -2, -3, -3, -3, -3, -3, -3, -1, -3, -3, -1, -3, -1, -3, -1, -3, -1, -3, -3, -1, -3, -2, -3, -1, -3, -1, -3, -3, -1, -3, -1, -3, -1, -3, -1, -3, -3, -1, -3, -3, -3, -3, -3, -2, -3, -2, -1, -2, -1, -1, -1, -1, 0, 1, 1, 1, 1, 2, 1, 2, 3, 2, 3, 3, 3, 3, 3, 3, 1, 3, 3, 1, 3, 1, 3, 1, 3, 1, 3, 3, 1, 3, 2, 3, 1, 3, 1, 3, 3, 1, 3, 1, 3, 1, 3, 1, 3, 3, 1, 3, 3, 3, 3, 3, 2, 3, 2, 1, 2, 1, 1, 1, 1, 0, -1, -1, -1, -1, -2, -1, -2, -3, -2, -3, -3, -3, -3, -3, -3, -1, -3, -3, -1, -3, -1, -3, -1, -3, -1, -3, -3, -1, -3, -1, -3, -2, -3, -1, -3, -3, -1, -3, -1, -3, -1, -3, -1, -3, -3, -1, -3, -3, -3, -3, -3, -2, -3, -2, -1, -2, -1, -1, -1, -1, 0, 1, 1, 1, 1, 2, 1, 2, 3, 2, 3, 3, 3, 3, 3, 3, 1, 3, 3, 1, 3, 1, 3, 1, 3, 1, 3, 3, 1};
uint8_t lc_subs_time [242] = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0};
int16_t lc_subs_lcrectime_data [188] = {0, 128, 256, 352, 480, 608, 736, 864, 960, 1088, 1184, 1280, 1376, 1472, 1568, 1632, 1728, 1792, 1824, 1888, 1920, 1952, 1984, 2016, 2016, 1984, 1952, 1920, 1888, 1824, 1792, 1728, 1632, 1568, 1472, 1376, 1280, 1184, 1088, 960, 864, 736, 608, 480, 352, 256, 128, -32, -160, -288, -384, -512, -640, -768, -896, -992, -1120, -1216, -1312, -1408, -1504, -1600, -1664, -1760, -1824, -1856, -1920, -1952, -1984, -2016, -2048, -2048, -2016, -1984, -1952, -1920, -1856, -1824, -1760, -1664, -1600, -1504, -1408, -1312, -1216, -1120, -992, -896, -768, -640, -512, -384, -288, -160, 0, 128, 256, 352, 480, 608, 736, 864, 960, 1088, 1184, 1280, 1376, 1472, 1568, 1632, 1728, 1792, 1824, 1888, 1920, 1952, 1984, 2016, 2016, 1984, 1952, 1920, 1888, 1824, 1792, 1728, 1632, 1568, 1472, 1376, 1280, 1184, 1088, 960, 864, 736, 608, 480, 352, 256, 128, 0, -160, -288, -384, -512, -640, -768, -896, -992, -1120, -1216, -1312, -1408, -1504, -1600, -1664, -1760, -1824, -1856, -1920, -1952, -1984, -2016, -2048, -2048, -2016, -1984, -1952, -1920, -1856, -1824, -1760, -1664, -1600, -1504, -1408, -1312, -1216, -1120, -992, -896, -768, -640, -512, -384, -288, -160};
uint8_t lc_subs_lcrectime_time [188] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 26, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 76, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 126, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 176, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199};
uint8_t lc_data_for_storage_data [242] = {19, 1, 19, 1, 19, 19, 1, 19, 1, 19, 1, 19, 1, 19, 19, 1, 19, 19, 19, 19, 19, 18, 19, 18, 17, 18, 17, 17, 17, 17, 48, 37, 21, 21, 21, 22, 21, 22, 23, 22, 23, 23, 23, 23, 23, 23, 5, 23, 23, 5, 23, 5, 23, 5, 23, 5, 23, 23, 5, 23, 6, 23, 5, 23, 5, 23, 23, 5, 23, 5, 23, 5, 23, 5, 23, 23, 5, 23, 23, 23, 23, 23, 22, 23, 22, 21, 22, 21, 21, 21, 21, 48, 33, 17, 17, 17, 18, 17, 18, 19, 18, 19, 19, 19, 19, 19, 19, 1, 19, 19, 1, 19, 1, 19, 1, 19, 1, 19, 19, 1, 19, 2, 19, 1, 19, 1, 19, 19, 1, 19, 1, 19, 1, 19, 1, 19, 19, 1, 19, 19, 19, 19, 19, 18, 19, 18, 17, 18, 17, 17, 17, 17, 48, 37, 21, 21, 21, 22, 21, 22, 23, 22, 23, 23, 23, 23, 23, 23, 5, 23, 23, 5, 23, 5, 23, 5, 23, 5, 23, 23, 5, 23, 5, 23, 6, 23, 5, 23, 23, 5, 23, 5, 23, 5, 23, 5, 23, 23, 5, 23, 23, 23, 23, 23, 22, 23, 22, 21, 22, 21, 21, 21, 21, 48, 33, 17, 17, 17, 18, 17, 18, 19, 18, 19, 19, 19, 19, 19, 19, 1, 19, 19, 1, 19, 1, 19, 1, 19, 1, 19, 19, 1};
#endif // _TEST_SIN_16TO8_
