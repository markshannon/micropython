


extern "C" {

#include "microbit/modmicrobit.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"
#include "microbit/modsound.h"

static const int8_t sample_microbit[] = {
-4, -5, -5, -5, -4, -4, -4, -4, -3, -4, -3, -3, -3, -4, -4, -4,
-4, -5, -5, -4, -4, -4, -3, -4, -4, -4, -5, -4, -3, -4, -3, -2,
-3, -3, -2, -2, -3, -4, -3, -3, -4, -4, -3, -5, -4, -2, -3, -4,
-3, -4, -5, -5, -6, -7, -7, -6, -5, -5, -5, -4, -5, -5, -5, -6,
-5, -6, -6, -5, -5, -5, -4, -4, -5, -5, -4, -4, -3, -3, -4, -3,
-4, -5, -5, -5, -5, -4, -4, -3, -3, -5, -4, -2, -4, -3, -3, -4,
-5, -5, -5, -5, -4, -3, -3, -5, -6, -7, -6, -2, -1, -2, -3, 0,
-2, -2, -1, -2, -2, -1, -2, -3, -2, -2, -2, -2, -3, -5, -4, -2,
-1, -2, -3, -3, -3, -4, -3, -1, -3, -5, -5, -6, -4, -4, -5, -3,
-3, -4, -4, -3, -4, -4, -5, -5, -5, -4, -4, -6, -6, -5, -6, -6,
-6, -6, -5, -4, -4, -5, -5, -5, -5, -4, -3, -4, -4, -4, -5, -5,
-4, -3, -3, -4, -2, -2, -3, -2, -2, -3, -4, -1, -1, -3, -3, -4,
-4, -5, -6, -6, -6, -8, -7, -8, -10, -8, -7, -8, -7, -6, -6, -6,
-5, -4, -4, -5, -6, -4, -4, -6, -5, -4, -4, -3, -2, -2, -4, -3,
-3, -4, -3, -2, -1, -1, -1, -2, -1, -1, -2, -1, 1, -1, -2, -1,
-1, -4, -6, -4, -6, -10, -9, -7, -8, -8, -10, -11, -11, -12, -11, -11,
-10, -11, -11, -9, -10, -12, -9, -7, -8, -8, -8, -7, -6, -5, -5, -6,
-6, -6, -6, -5, -3, -2, -2, -2, -3, -4, -1, 1, 0, 0, 2, 0,
-2, -1, -2, -2, -3, -6, -7, -8, -8, -9, -9, -10, -12, -12, -13, -15,
-17, -17, -17, -17, -16, -17, -17, -16, -15, -14, -13, -13, -12, -9, -7, -7,
-6, -5, -4, -5, -4, -2, -1, 0, 1, 2, 3, 2, 3, 5, 5, 5,
5, 5, 6, 6, 5, 4, 5, 6, 5, 4, 2, 2, 0, -2, -4, -6,
-8, -9, -12, -14, -18, -20, -19, -19, -20, -16, -14, -15, -12, -10, -10, -7,
-5, -5, -3, -3, -2, 0, 2, 2, 0, 0, 2, 1, -1, 0, 1, -1,
-2, -1, -2, -5, -5, -5, -7, -7, -5, -7, -6, -5, -5, -5, -4, -3,
-3, -1, 0, 0, 2, 1, 1, 2, 2, 2, 3, 2, 1, -1, -1, 0,
-2, -3, -2, -4, -5, -7, -7, -5, -4, -6, -8, -7, -6, -6, -6, -5,
-5, -5, -6, -7, -6, -7, -9, -10, -8, -8, -9, -10, -11, -10, -8, -9,
-12, -11, -9, -9, -8, -6, -7, -9, -9, -7, -6, -6, -4, -4, -3, -1,
-2, -1, 1, 0, 2, 2, 1, 1, 3, 2, 2, 3, 1, -3, -4, -2,
-4, -7, -8, -9, -13, -14, -16, -18, -20, -21, -21, -22, -22, -18, -16, -16,
-16, -13, -10, -9, -7, -7, -7, -4, -3, -4, -3, -2, -1, -4, -5, -4,
-4, -5, -6, -6, -8, -10, -8, -8, -8, -10, -9, -5, -5, -5, -3, -2,
0, 2, 2, 2, 2, 2, 4, 6, 7, 8, 9, 7, 6, 7, 7, 6,
4, 1, -3, -7, -9, -10, -13, -17, -20, -25, -26, -24, -23, -21, -19, -18,
-18, -16, -13, -9, -7, -6, -5, -5, -4, -4, -3, -1, 1, 1, 1, 0,
0, 0, -1, -4, -5, -8, -14, -13, -12, -13, -16, -18, -18, -19, -17, -13,
-8, -6, -4, -4, -2, 4, 9, 13, 15, 13, 12, 14, 14, 14, 17, 16,
13, 9, 4, -1, -5, -9, -7, -7, -17, -26, -27, -30, -33, -25, -17, -20,
-24, -22, -18, -17, -13, -5, -3, -5, -5, -4, -4, -3, 1, 4, 4, 3,
1, 0, -1, 0, -2, -5, -8, -9, -8, -12, -17, -17, -17, -17, -16, -15,
-12, -11, -10, -7, -4, -1, 4, 6, 8, 9, 10, 13, 15, 15, 15, 16,
15, 12, 10, 8, 6, 3, -3, -8, -9, -14, -20, -20, -21, -27, -31, -27,
-25, -24, -20, -16, -13, -14, -12, -9, -8, -4, -1, -1, 0, -1, -2, 1,
3, 3, 6, 5, 2, 0, -2, -5, -5, 0, 0, -7, -13, -16, -16, -13,
-8, -5, -6, -7, -6, -5, 2, 9, 12, 14, 14, 13, 13, 13, 15, 19,
22, 20, 16, 11, 10, 9, 4, -3, -5, -4, -10, -15, -17, -21, -25, -25,
-24, -24, -24, -20, -17, -15, -14, -11, -8, -5, -2, 1, 3, 3, 3, 2,
3, 5, 6, 7, 6, 3, 1, -2, -6, -3, 2, -1, -6, -9, -13, -15,
-12, -7, -6, -4, -2, -3, -2, 4, 10, 13, 17, 18, 15, 13, 16, 18,
20, 23, 21, 18, 15, 11, 9, 7, 2, -3, -3, -5, -12, -17, -18, -23,
-25, -25, -23, -22, -21, -17, -15, -13, -10, -10, -6, -2, 0, 3, 5, 3,
3, 5, 7, 9, 6, 8, 8, 3, -2, -2, -2, -4, -4, -7, -11, -14,
-15, -11, -8, -5, -2, -2, 0, 1, 3, 8, 11, 13, 16, 18, 16, 16,
17, 20, 21, 20, 17, 11, 6, 7, 9, 2, -3, -4, -8, -16, -17, -17,
-17, -20, -21, -20, -21, -18, -15, -13, -9, -7, -5, -5, -3, -1, 2, 3,
1, 4, 6, 3, 2, 3, 2, 1, -5, -6, -4, -5, -5, -7, -10, -14,
-16, -13, -12, -9, -4, -3, -4, -2, 1, 6, 10, 13, 15, 15, 13, 15,
17, 20, 19, 17, 15, 14, 10, 7, 5, 0, -6, -8, -8, -12, -16, -18,
-23, -29, -28, -25, -24, -23, -21, -17, -16, -15, -13, -12, -10, -8, -6, -2,
-2, -3, -1, -1, 0, 1, 1, -1, -6, -7, -5, -5, -8, -8, -10, -13,
-16, -15, -13, -10, -8, -9, -8, -5, -1, 2, 7, 10, 12, 13, 15, 15,
12, 15, 17, 16, 16, 15, 13, 10, 7, 2, -6, -11, -9, -10, -13, -16,
-20, -26, -30, -29, -26, -25, -23, -20, -15, -14, -14, -10, -6, -6, -4, -1,
-1, 1, 2, 1, 2, 3, 4, 4, 0, -8, -9, -7, -13, -13, -10, -14,
-20, -18, -19, -22, -14, -8, -3, 3, 7, 12, 19, 24, 29, 28, 22, 19,
16, 13, 13, 12, 14, 12, 8, 3, -10, -24, -33, -36, -34, -27, -18, -8,
-4, -1, 0, -5, -12, -19, -24, -25, -23, -18, -10, -5, -4, 1, 7, 10,
13, 12, 10, 5, -6, -14, -18, -24, -31, -34, -33, -27, -15, -6, 0, 2,
-1, -8, -14, -17, -16, -9, 0, 9, 19, 25, 26, 25, 22, 19, 12, 10,
11, 13, 15, 15, 15, 8, -4, -30, -55, -61, -56, -37, -10, 10, 24, 28,
12, -7, -23, -38, -43, -39, -25, -10, 2, 13, 17, 16, 14, 5, -4, -11,
-19, -22, -22, -19, -9, 0, -3, -8, -13, -23, -25, -23, -17, -8, -5, -5,
-6, -8, -7, -1, 6, 16, 20, 19, 15, 9, 10, 11, 12, 15, 18, 21,
22, 16, 7, -1, -23, -58, -70, -61, -40, -4, 25, 39, 37, 13, -22, -46,
-58, -56, -42, -22, -1, 9, 17, 22, 13, 4, -2, -18, -27, -31, -31, -20,
-2, 14, 24, 23, -2, -34, -58, -70, -53, -20, 9, 29, 29, 9, -15, -34,
-34, -22, -2, 20, 25, 22, 18, 13, 13, 15, 16, 18, 19, 18, 10, 6,
-8, -50, -72, -68, -55, -12, 30, 46, 46, 15, -26, -50, -62, -53, -39, -21,
-2, 2, 11, 21, 19, 13, 0, -20, -40, -51, -41, -16, 14, 36, 40, 30,
5, -19, -38, -60, -63, -46, -21, 4, 22, 26, 15, -5, -26, -38, -35, -22,
-4, 16, 30, 30, 25, 20, 15, 14, 16, 20, 20, 23, 24, 22, 4, -51,
-74, -70, -52, 10, 53, 58, 47, -1, -41, -58, -61, -40, -33, -23, -9, -6,
16, 40, 46, 29, -7, -42, -68, -63, -26, 15, 47, 54, 34, 9, -13, -24,
-33, -39, -46, -54, -35, -7, 20, 38, 25, -7, -37, -52, -44, -22, 5, 22,
27, 26, 20, 20, 23, 26, 22, 13, 12, 15, 15, 22, 3, -57, -73, -65,
-34, 28, 50, 46, 15, -31, -38, -45, -35, -26, -45, -36, -18, 8, 44, 56,
41, -4, -43, -54, -49, -16, 13, 25, 28, 15, 15, 16, 11, 0, -42, -70,
-71, -50, 4, 36, 41, 21, -7, -23, -35, -27, -22, -17, 2, 22, 43, 49,
43, 30, 16, 12, 10, 15, 16, 15, 0, -53, -73, -41, -8, 40, 41, 10,
-12, -30, -8, -7, -19, -36, -69, -50, -7, 39, 55, 33, 8, -29, -29, -7,
-6, -10, -25, -21, 5, 29, 49, 38, 7, -22, -38, -46, -51, -43, -36, -14,
14, 32, 35, 14, -12, -34, -38, -25, -15, 2, 19, 33, 42, 47, 45, 33,
20, 9, 4, 4, -10, -57, -72, -31, 3, 41, 30, 1, -13, -13, 22, -8,
-39, -53, -65, -29, 13, 39, 19, 5, 12, 2, 14, 2, -30, -44, -34, 3,
15, 21, 16, 6, 17, 19, 8, -34, -67, -65, -50, -13, 10, 13, 12, 17,
20, 6, -8, -28, -39, -23, 3, 20, 29, 44, 51, 56, 54, 38, 11, -13,
-44, -76, -43, -7, 12, 13, 14, 38, 10, 34, 13, -56, -63, -58, -31, -21,
4, 7, 0, 47, 48, 14, -16, -29, -33, -38, -6, -7, -13, 11, 28, 32,
21, 18, -20, -45, -30, -41, -46, -31, -2, 9, 25, 40, 19, 16, 11, -5,
-20, -16, -2, -1, 24, 41, 50, 54, 49, 42, 15, -45, -73, -46, -41, -39,
-36, 19, 55, 37, 48, 6, -19, -19, -39, -62, -67, -26, -7, 15, 45, 45,
39, 21, 10, -27, -39, -25, -37, -34, -13, 15, 22, 26, 33, 3, -14, -20,
-35, -50, -48, -24, -17, 1, 16, 24, 27, 26, 20, -1, 0, -3, -5, 4,
8, 22, 37, 43, 44, 30, -48, -52, -11, -51, -75, -50, 23, 14, 5, 26,
25, 33, 15, -18, -55, -33, -24, -61, -31, 17, 32, 20, 23, 30, 14, 7,
-20, -37, -26, -10, -20, -26, 9, 23, 6, -24, -9, 8, -21, -33, -33, -6,
-2, -10, -6, 4, 24, 9, -1, 7, 20, 17, 1, 10, 25, 24, 22, 2,
-57, -8, 10, -62, -74, -21, 28, -32, -22, 15, 23, 21, -8, -5, 1, 5,
-37, -63, -12, 2, -18, -18, 25, 37, 10, 6, 1, 6, -7, -32, -40, -19,
0, -23, -16, 10, 12, -9, -34, -5, 1, -24, -29, -9, 11, -15, -11, 12,
18, 2, -11, 13, 19, 8, -3, 9, 25, 5, 2, -6, -16, -6, -32, -48,
-32, -1, -17, -36, 0, 10, -4, -3, 15, 18, -2, -10, -17, -12, -14, -29,
-18, 4, 9, -7, 4, 24, 11, -3, -4, -3, -13, -15, -10, -12, -7, -7,
-10, -3, -5, -15, -8, 5, -6, -12, 3, 8, -2, -5, 5, 9, 5, 3,
7, 13, 8, 7, 16, 25, 7, -10, 10, -7, -32, -20, -11, -19, -23, -2,
1, -5, 4, 15, 23, 14, 3, 3, 4, -4, -19, -9, -1, -13, -14, 3,
12, 3, 5, 10, 5, -1, -2, 0, -2, -5, -6, -5, -2, -6, -4, 1,
-3, -18, 2, 9, -15, 1, 17, 2, -8, 9, 16, 1, 6, 12, 6, 4,
7, 9, 5, 4, 1, -10, -13, -6, -12, -15, -9, -4, -4, 2, 10, 10,
9, 8, 4, 3, -3, -4, -1, -4, -8, -8, -3, -3, -4, 0, 2, -1,
-2, 1, 0, -3, 0, -3, -4, -1, 1, 1, 1, 0, -7, -5, -5, -5,
0, 1, 1, 3, 11, 8, 10, 10, 4, 7, 2, -2, -2, -2, -7, -7,
-3, -11, -14, -10, -11, -15, -10, 1, 0, 1, 6, 5, 7, 4, -1, -1,
1, -1, -4, 0, -1, -3, -4, -3, -3, -3, -1, -3, -3, -2, -2, -6,
-9, -5, -5, -5, -4, -2, -3, -4, -1, -1, -1, -1, 1, 1, 0, -3,
-5, -3, -4, -4, -6, -5, -2, -4, -1, 1, 1, -2, -1, 5, -5, -7,
-3, -7, -8, -3, 0, -4, -1, 6, 6, 0, 1, 2, -3, -2, 0, -2,
-1, 0, -1, -2, -2, -2, -2, -3, -3, 0, 0, -2, -3, -3, -4, -4,
-1, -2, -2, 0, 0, -3, -2, 3, 2, -1, -2, 1, 1, -2, -3, -4,
-6, -6, -4, -4, -4, -3, -3, -1, 0, -1, 0, 1, -1, -2, -2, -3,
-2, -1, -3, -2, -2, -3, -1, -2, -5, -5, -3, -3, -3, 0, 0, -3,
-2, -1, -4, -3, -2, -4, -2, -1, -1, -4, -4, -3, -5, -4, -4, -5,
-3, -3, -5, -5, -3, -2, -2, -2, -2, -2, -2, -2, -3, -5, -4, -4,
-2, -2, -2, -3, -4, -4, -6, -4, -4, -4, -4, -6, -4, -5, -5, -3,
-4, -5, -4, -4, -6, -8, -6, -5, -6, -3, -4, -6, -3, -1, -3, -5,
-3, -2, -5, -6, -5, -4, -5, -6, -7, -6, -4, -2, -3, -4, -4, -5,
-4, -4, -5, -6, -7, -5, -7, -6, -4, -6, -8, -6, -4, -4, -3, -2,
-3, -3, -3, -5, -5, -4, -6, -6, -6, -8, -7, -5, -5, -6, -7, -5,
-5, -3, -3, -5, -5, -5, -5, -4, -3, -2, -2, -3, -4, -3, -5, -4,
-3, -5, -5, -6, -6, -4, -4, -4, -4, -5, -6, -4, -4, -5, -4, -5,
-4, -5, -5, -3, -5, -5, -3, -2, -1, -2, -2, -2, -3, -4, -5, -5,
-3, -2, -1, -1, -2, -3, -4, -5, -4, -5, -7, -6, -6, -5, -4, -4,
-5, -4, -4, -6, -4, -5, -4, -6, -7, -4, -5, -3, 0, -1, -2, -5,
-4, -5, -6, -3, -3, -4, -3, -2, -4, -3, -2, -4, -6, -6, -6, -6,
-5, -5, -5, -3, -3, -3, -3, -4, -3, -3, -4, -3, -3, -3, -2, -3,
-5, -2, 0, -2, -3, -1, -3, -4, -1, -2, -3, -3, -2, 0, 0, -2,
-2, -2, -2, -2, -2, -1, -2, -4, -2, 0, -2, -3, -2, -2, -3, -3,
-1, -1, -1, -1, -1, -2, 0, 0, -1, 0, -1, -1, -1, -1, 0, -1,
-1, -1, -1, -1, 1, 0, -2, -3, -2, -2, -2, -1, -1, -2, -2, 0,
0, -3, -2, -2, -3, -2, -3, -1, 0, -1, -1, 0, 0, 0, 0, -1,
-3, -4, -4, -4, -3, -2, -3, -2, -2, -1, -1, 0, 0, -1, 0, 1,
0, -2, -1, -1, -2, -1, -2, -3, -3, -3, -2, 0, 0, -1, 0, -2,
-1, 0, 0, -3, -3, -2, -1, -1, -1, -1, -1, -2, 0, 0, 0, 0,
-1, 0, 0, -1, 0, 0, -1, 0, -1, -2, -2, -2, 0, -2, -3, -1,
-1, -1, -2, -2, -2, -1, 0, 0, 0, 1, 0, -3, 0, -1, -2, -1,
-2, -2, -2, -3, -3, -1, -2, -2, -1, -2, -3, -4, -2, -1, -1, -1,
-2, -2, -2, -4, -3, -3, -1, 0, 0, -2, -1, -1, -1, -1, 0, 2,
2, 0, 1, 2, -1, -1, -1, -1, -2, -3, -5, -5, -2, -2, -3, -2,
-4, -3, -3, -1, -1, -1, 0, 0, -1, 1, 0, -1, -2, -3, -2, -1,
0, 1, 1, 2, 3, 1, -1, 1, 1, 0, 0, 0, -2, 0, 2, 0,
0, -1, 0, 1, 1, -1, -2, -1, -2, -3, -3, -2, -1, 0, 0, -1,
0, 0, 0, 0, 0, -1, -1, -1, -1, 0, -2, 0, -1, -2, 0, -1,
0, -1, -1, 0, 1, 1, 0, -1, 0, -1, -2, 0, -2, -2, 0, 1,
1, 0, 1, -1, -2, -1, -1, -2, -2, -1, 0, 1, 1, 1, 1, 0,
-1, 0, 1, -1, -1, 1, 0, -1, -1, -1, -1, -3, -3, 0, -1, -1,
-2, -1, 0, -1, -2, -3, -1, -2, -3, -1, -1, -2, -1, -1, 0, 0,
-1, 0, 0, -1, 1, 1, 0, 1, 1, 1, 2, 0, 0, 1, 1, 3,
2, 2, 2, 1, 1, 1, 0, 0, 1, 1, 1, 3, 3, 2, 2, 1,
1, 1, 2, 2, 1, 1, 1, 0, -2, 0, -1, -17, -11, 10, 23, 22,
-2, -12, -10, 4, 17, 11, 1, -14, -12, -2, 7, 10, 3, -3, -9, -2,
4, 6, 4, -2, -2, -3, 1, -7, -24, -9, 16, 32, 25, -2, -13, -11,
1, 8, 6, -1, -8, -6, -3, 4, 7, 4, 2, -3, 0, 5, 3, 3,
2, -15, -20, -1, 14, 22, -5, -34, -24, -5, 21, 26, 7, -5, -12, 3,
22, 19, 6, -15, -17, -8, -1, 2, -6, -11, -9, -2, 8, 4, -1, -3,
-6, -1, 3, 2, 0, -6, -7, -3, 0, 3, 1, -3, -5, -4, 2, 3,
4, 1, -4, -5, -4, 1, 2, -4, -8, -8, -4, 0, 0, -2, -3, -1,
0, -1, 0, -16, -44, -38, -16, 6, 31, 25, 11, 9, 1, 8, 13, 2,
-5, -17, -25, -17, -12, -4, 2, -2, 1, 8, 7, 10, 9, 4, 2, -7,
-9, -8, -12, -7, -6, -10, -5, 3, 7, 4, 2, 3, 1, 0, 1, 0,
-4, -5, -3, 0, 3, 2, -1, 1, -2, -7, 2, 4, -6, -7, -5, -6,
-5, -5, -5, -1, -3, -1, 0, -3, 2, 8, 6, 1, -5, -2, 1, -8,
-11, -7, -8, -4, -1, 0, 2, 2, 3, -1, 0, 7, 3, -1, -1, -4,
-5, -6, -1, 2, -2, -5, -3, 4, 3, 0, 0, 0, 3, 0, -8, 0,
6, 2, 4, 3, -4, -2, 2, 0, -5, -5, -6, -5, -3, -3, 0, 6,
6, 4, 4, -3, -5, 4, 2, -3, 0, -9, -15, -8, -6, -6, -4, 1,
7, 10, 5, 1, 1, -3, -4, 1, -2, -11, -10, -6, -5, 0, 6, 5,
3, 0, 0, -1, -5, -7, -4, 1, -8, -9, 0, -6, -6, 4, 0, -4,
-1, 2, 2, 0, -6, -9, -8, -5, -6, -5, -4, -7, -4, -5, -7, -2,
1, 5, 2, -2, -4, -12, -15, -7, 1, -3, -18, -18, -11, -9, -1, 4,
11, 10, -2, -2, -6, -13, -6, -4, -6, -5, -6, -4, -4, -5, -3, 3,
4, -3, -4, -3, -3, -1, -1, 1, -1, -8, -10, -7, 0, 2, 0, 2,
-1, -9, -6, 1, 0, -3, -4, -5, -5, -3, -5, -7, -2, -2, -1, -1,
-3, -1, 1, -3, -7, -10, -15, -12, -3, -2, -6, -10, -8, -1, 7, 2,
-2, 3, -4, -13, -13, -11, -8, -2, -8, -10, -5, -7, -4, 3, 5, 0,
-12, -10, -2, -4, -6, -7, -10, -9, -10, -11, -8, -1, 3, -7, -12, -6,
-3, -6, -4, -1, -4, -8, -11, -12, -7, -1, 4, -1, -8, -7, -9, -8,
-3, -2, -4, -5, -3, -1, -5, -4, 0, 0, 0, -6, -10, -8, -6, -7,
-10, -6, 0, -4, -6, -4, -4, -1, -4, -4, 0, -2, -6, -11, -11, -6,
-4, -4, -3, 0, -7, -9, -5, -1, 1, -3, -5, -3, -2, -7, -11, -7,
-3, 3, 3, -7, -12, -8, -1, 4, 4, 2, -6, -12, -9, 0, 6, -1,
-7, -8, -8, -3, 0, -2, -4, -3, -5, -8, -6, -3, -1, 0, -1, -3,
-5, -5, -3, -1, 0, -2, -7, -7, -7, -9, -5, -1, 2, 4, -2, -8,
-7, 0, 8, 4, -3, -5, -6, -6, -5, -2, 3, 4, 0, -4, -5, -4,
-3, 2, 4, 3, 1, -1, -1, -4, -1, 0, -5, -5, 1, -1, -2, 0,
3, 3, 0, -4, -2, 1, 0, 2, -4, -8, -4, 3, 6, 1, -2, -3,
-2, -4, -4, 3, 9, 5, -2, -7, -6, -1, 0, 0, 1, -3, -6, -6,
-6, -4, -2, -1, -3, -6, -7, -5, -3, -3, -3, -2, -5, -4, -2, -6,
-5, 0, 0, -3, -7, -7, -3, -1, -3, -3, -2, 1, -1, -8, -8, -5,
-6, -5, -5, 0, 3, -5, -9, -10, -9, -4, -1, 0, -5, -8, -7, -3,
-1, -1, -5, -10, -8, -4, -5, -4, -3, -2, -8, -10, -2, -4, -8, -3,
-1, -3, -7, -9, -6, -6, -10, -9, -7, -6, -5, -9, -6, -5, -9, -5,
-2, -5, -9, -10, -9, 1, 4, -3, -11, -9, -4, -5, -5, -2, -3, -5,
-6, -7, -6, -6, -5, -2, -1, -3, -8, -7, -4, -3, -5, -8, -9, -8,
-7, -6, -1, -1, -5, -7, -5, -3, -3, -4, -6, -7, -8, -6, -3, -4,
-6, -7, -4, -2, -5, -5, -3, -2, -4, -7, -8, -5, -4, -8, -9, -6,
-5, -3, -5, -7, -7, -5, -2, -3, -7, -6, -3, -6, -7, -7, -7, -5,
-5, -5, 0, -1, -5, -9, -7, -1, -5, -9, -8, -5, -6, -4, -7, -9,
-6, -1, 0, -4, -5, -5, -5, -5, -6, -8, -8, -3, -2, -5, -6, -7,
-7, -6, -4, -2, -3, -6, -8, -8, -6, -4, 0, 0, -3, -4, -6, -6,
-3, -1, -3, -5, -5, -3, -4, -6, -6, -4, -2, -1, -1, -3, -5, -7,
-6, -2, -6, -7, -6, -6, -5, -6, -6, -3, -7, -9, -3, 1, -1, -5,
-5, -7, -9, -9, -7, -3, 1, 0, -4, -6, -7, -6, -2, 1, 3, 2,
0, -4, -6, -7, -3, 1, 2, 0, -4, -5, -4, -4, -2, -1, 1, 3,
-2, -6, -6, -4, -1, 1, 2, -1, -3, -3, -4, -2, 0, 2, -1, -3,
-2, -1, 0, -1, -1, -2, -2, 0, -1, -1, -2, -3, -3, -2, -3, -5,
-4, -3, -3, -2, -5, -6, -3, -2, 0, 0, -1, -3, -5, -6, -4, -5,
-5, -3, -3, -3, -3, -1, 1, 1, 0, -1, 0, 1, -1, -2, -3, -5,
-1, 2, 0, -2, -3, -3, -3, -2, 0, 1, -1, -3, -3, -2, -1, 1,
2, -1, -5, -8, -7, -7, -6, -5, -5, -6, -7, -7, -5, -3, -2, -2,
-5, -6, -7, -8, -8, -6, -5, -5, -4, -5, -5, -5, -2, -2, -2, 0,
0, -1, 2, 4, 3, 5, 5, 4, 4, 2, 0, 0, -2, -4, -5, -7,
-6, -5, -11, -17, -17, -18, -15, -10, -7, -4, -3, -2, -4, -5, -1, 0,
0, -2, -6, -9, -11, -10, -8, -4, -1, 1, 2, 4, 6, 10, 12, 13,
15, 14, 14, 15, 15, 16, 17, 15, 12, 11, 7, 4, -4, -18, -30, -33,
-27, -18, -11, -3, 3, 4, 6, 11, 13, 18, 21, 11, 1, -8, -14, -17,
-17, -14, -10, -8, -5, -3, 0, 7, 14, 18, 20, 16, 10, 3, -3, -10,
-18, -23, -26, -26, -21, -13, -2, 9, 16, 20, 21, 19, 21, 21, 16, 11,
8, 3, 2, 5, 7, 11, 13, 15, 15, 10, 0, -18, -35, -39, -35, -26,
-16, -10, -5, -1, 2, 5, 14, 21, 20, 9, -5, -17, -22, -24, -25, -23,
-20, -21, -19, -12, -5, 5, 13, 11, 7, 0, -9, -13, -15, -17, -22, -33,
-45, -46, -35, -19, -5, 5, 6, 3, 2, 3, 6, 8, 6, -3, -14, -16,
-11, -4, 6, 12, 11, 7, 3, 2, 2, -10, -35, -57, -62, -53, -35, -22,
-12, -3, 0, 3, 11, 16, 17, 9, -8, -25, -39, -44, -41, -36, -30, -25,
-21, -15, -4, 9, 16, 15, 6, -7, -17, -24, -28, -30, -31, -30, -32, -37,
-35, -25, -12, -3, -1, -4, -6, -9, -11, -8, -7, -11, -16, -19, -15, -7,
0, 5, 9, 10, 7, 5, 3, 0, -5, -13, -36, -62, -69, -64, -42, -16,
-10, -7, -1, 4, 12, 15, 13, 3, -19, -42, -56, -57, -49, -37, -30, -27,
-20, -11, 0, 10, 13, 7, -10, -24, -34, -36, -36, -36, -32, -29, -27, -24,
-21, -19, -16, -13, -13, -20, -26, -27, -24, -21, -18, -13, -11, -8, -4, -1,
3, 4, 2, 0, -4, -8, -7, -8, -9, -10, -14, -30, -56, -68, -62, -40,
-19, -14, -15, -11, -5, 1, 4, 1, -8, -25, -44, -52, -48, -37, -29, -25,
-20, -14, -5, 2, 6, 3, -6, -17, -27, -30, -29, -27, -25, -25, -22, -13,
-7, -8, -13, -22, -26, -26, -26, -21, -20, -17, -10, -6, 2, 10, 11, 11,
7, 3, 2, 1, 3, 4, 3, 7, 10, 12, 12, 8, -4, -31, -60, -63,
-45, -23, -14, -13, -7, 4, 15, 23, 22, 13, -6, -27, -39, -39, -30, -24,
-22, -18, -10, 3, 14, 16, 14, 8, -5, -14, -18, -23, -26, -27, -26, -17,
-8, -2, 3, -2, -13, -19, -20, -18, -16, -21, -20, -9, 2, 12, 16, 14,
8, 2, 0, 4, 7, 5, 0, 0, 7, 15, 18, 19, 14, -4, -29, -53,
-60, -45, -28, -18, -15, -10, 4, 20, 31, 30, 18, -1, -22, -35, -38, -35,
-32, -33, -29, -15, 0, 15, 23, 21, 13, 3, -7, -15, -21, -27, -33, -29,
-24, -17, -6, 1, 7, 10, 8, -3, -13, -18, -22, -20, -23, -23, -19, -14,
-3, 5, 10, 13, 13, 14, 15, 12, 8, 6, 8, 8, 9, 12, 14, 17,
18, 15, -3, -34, -56, -53, -34, -19, -13, -9, -2, 10, 26, 34, 32, 16,
-10, -26, -30, -29, -29, -31, -29, -20, -7, 8, 19, 24, 20, 12, 7, 3,
-6, -16, -25, -31, -30, -22, -12, -3, 4, 7, 10, 15, 18, 9, -13, -29,
-32, -27, -22, -21, -21, -16, -6, 7, 19, 23, 19, 17, 18, 18, 17, 13,
6, 1, 0, 8, 15, 17, 17, 11, -11, -37, -43, -32, -20, -18, -26, -24,
-10, 8, 24, 32, 28, 12, -1, -5, -9, -15, -26, -33, -34, -27, -14, -2,
8, 13, 15, 16, 20, 18, 10, -1, -11, -18, -19, -16, -17, -17, -12, -3,
7, 17, 19, 8, -5, -11, -9, -7, -11, -21, -27, -24, -11, 6, 16, 19,
18, 19, 24, 30, 31, 28, 19, 10, 8, 13, 16, 12, 7, -6, -27, -41,
-36, -22, -14, -15, -14, -9, 3, 20, 31, 32, 22, 9, 0, -6, -11, -19,
-29, -31, -30, -24, -9, 4, 9, 13, 18, 20, 21, 17, 8, -1, -10, -14,
-14, -15, -17, -15, -15, -8, 1, 2, 1, -2, -5, -4, -3, -2, -6, -9,
-9, -4, 3, 7, 8, 9, 14, 19, 25, 28, 29, 27, 23, 22, 22, 21,
17, 4, -16, -37, -43, -37, -33, -27, -20, -15, -8, 10, 28, 33, 31, 26,
17, 7, 2, -7, -21, -32, -36, -32, -28, -21, -10, -3, 8, 18, 25, 26,
24, 17, 10, 5, 0, -5, -14, -20, -22, -24, -22, -20, -14, -13, -9, -1,
4, 8, 10, 11, 8, 8, 9, 12, 8, 4, 6, 5, 9, 18, 20, 21,
22, 22, 21, 21, 9, -14, -30, -33, -33, -34, -34, -32, -30, -22, -4, 11,
19, 22, 21, 19, 21, 20, 11, -4, -17, -24, -27, -27, -25, -21, -20, -16,
-5, 5, 14, 21, 20, 17, 17, 16, 13, 7, -3, -11, -21, -30, -29, -30,
-27, -22, -15, -6, -1, 9, 17, 21, 23, 24, 23, 21, 21, 19, 15, 13,
11, 9, 11, 14, 15, 15, 12, 1, -15, -21, -21, -23, -23, -23, -23, -23,
-16, -4, 5, 9, 12, 14, 13, 16, 16, 11, 2, -6, -12, -16, -19, -21,
-23, -20, -17, -13, -5, 2, 6, 10, 13, 16, 15, 13, 10, 5, -2, -6,
-13, -23, -27, -27, -27, -23, -15, -11, -6, 5, 13, 19, 22, 24, 22, 24,
25, 22, 18, 14, 11, 10, 11, 12, 12, 11, 4, -9, -20, -22, -24, -25,
-21, -21, -26, -23, -11, -1, 6, 9, 10, 10, 12, 15, 14, 9, 1, -5,
-10, -13, -16, -19, -22, -21, -18, -14, -8, -1, 3, 6, 9, 12, 14, 14,
12, 9, 4, 1, -3, -11, -18, -22, -25, -25, -23, -18, -14, -9, -1, 5,
13, 20, 25, 28, 29, 28, 26, 24, 22, 18, 15, 14, 11, 9, 8, 3,
-9, -19, -22, -24, -24, -22, -25, -28, -24, -16, -7, 1, 7, 9, 8, 10,
13, 15, 13, 7, 2, -6, -13, -15, -17, -18, -19, -20, -17, -13, -8, -1,
3, 6, 9, 10, 10, 11, 12, 11, 7, 3, -5, -16, -20, -23, -25, -22,
-21, -21, -19, -13, -3, 7, 12, 17, 19, 20, 23, 25, 26, 26, 22, 18,
14, 14, 14, 13, 10, 4, -3, -14, -20, -22, -24, -24, -24, -25, -25, -22,
-15, -6, 1, 6, 9, 9, 9, 13, 13, 11, 9, 3, -4, -9, -11, -12,
-12, -12, -11, -11, -9, -5, 2, 8, 11, 10, 9, 11, 12, 13, 10, 8,
6, 1, -9, -15, -15, -15, -16, -14, -12, -13, -8, -2, 4, 10, 15, 20,
21, 24, 27, 29, 28, 27, 26, 22, 17, 13, 14, 16, 15, 11, 3, -5,
-12, -16, -16, -14, -15, -19, -21, -18, -13, -6, 0, 5, 10, 10, 12, 15,
14, 15, 15, 10, 5, 2, -1, -4, -3, -1, 0, 0, 1, 3, 7, 8,
9, 11, 12, 9, 8, 9, 7, 8, 9, 4, -4, -6, -7, -8, -7, -5,
-5, -5, -5, -3, 1, 6, 13, 16, 19, 21, 23, 25, 24, 25, 23, 20,
16, 13, 12, 14, 15, 12, 9, 5, -2, -7, -9, -11, -10, -11, -12, -13,
-14, -13, -6, 0, 3, 5, 7, 9, 7, 6, 10, 10, 7, 7, 5, 3,
4, 5, 6, 6, 6, 7, 5, 3, 4, 4, 4, 6, 5, 4, 4, 5,
7, 4, 5, 7, 3, 1, 0, -1, -1, 0, 2, 3, 4, 5, 6, 9,
13, 14, 15, 15, 16, 15, 14, 15, 16, 14, 14, 13, 13, 13, 11, 12,
14, 13, 12, 10, 5, 2, 0, 0, -2, -3, -4, -5, -8, -9, -9, -8,
-4, -1, 1, 4, 3, 3, 5, 5, 7, 8, 7, 8, 9, 7, 7, 8,
6, 6, 5, 3, 4, 3, 2, 3, 2, 0, -2, -1, -1, 1, 3, 1,
2, 2, 0, 1, 1, 2, 3, 2, 2, 0, -1, 0, 0, 2, 2, 0,
-1, 0, 1, 1, 3, 5, 5, 6, 6, 7, 10, 10, 11, 12, 12, 11,
10, 12, 11, 9, 8, 5, 4, 1, -1, -1, -1, -4, -4, -3, -3, -5,
-3, -1, -3, -2, 0, 1, 3, 5, 4, 2, 4, 5, 6, 5, 6, 6,
6, 6, 6, 4, 3, 4, 4, 4, 5, 6, 6, 4, 5, 5, 5, 5,
5, 4, 3, 2, 0, 1, 0, -1, -1, -3, -2, -2, -3, -3, -4, -6,
-6, -3, -3, -3, 0, 3, 2, 2, 3, 4, 6, 6, 7, 7, 7, 7,
7, 9, 8, 7, 7, 5, 3, 3, 4, 3, 1, 2, 2, 2, 3, 1,
-2, -3, -3, -2, -5, -7, -7, -9, -9, -7, -8, -8, -5, -6, -7, -6,
-5, -3, -2, -2, -1, 1, 1, 2, 4, 4, 2, 2, 2, 1, 1, 2,
3, 4, 3, 3, 3, 2, 1, -1, 0, 1, -1, -2, -2, -2, -2, -3,
-2, -2, -3, -2, 0, -1, -2, -1, -2, -3, -4, -2, -2, -5, -5, -4,
-5, -4, -3, -3, -4, -3, -3, -4, -3, -2, 0, 3, 2, 3, 4, 4,
4, 5, 4, 3, 3, 3, 4, 4, 3, 3, 0, 0, 1, 1, 0, 1,
1, -2, 0, -1, -3, -1, -1, -1, -2, -2, -1, -3, -2, -1, -3, -2,
-3, -3, -2, 0, 1, -1, -1, 1, 0, 0, 0, 1, 1, 1, 1, 2,
4, 3, 4, 4, 4, 5, 7, 4, 3, 2, 0, 2, 0, -1, 0, 0,
-1, -2, -2, -1, -2, -3, -2, -3, -2, -1, -2, -3, -2, -1, 0, -1,
0, 1, 2, 2, 0, -2, -2, -1, -1, -4, -5, -3, -3, -4, -3, -3,
-5, -4, -3, -3, -2, -2, 0, -2, -2, 0, 0, -1, 0, 2, 1, 1,
1, 3, 3, 4, 5, 3, 3, 3, 2, 3, 2, 2, 2, 2, 1, -1,
-1, -2, -3, -4, -4, -3, -5, -7, -7, -6, -6, -7, -7, -7, -6, -6,
-6, -3, -1, -1, -1, -1, 1, 0, -1, -1, -2, -2, -1, -2, -2, -2,
1, 2, 1, 1, 2, -1, -1, 0, -1, 1, 0, -2, -2, -3, -2, -1,
-3, -2, -1, -3, -3, -1, -1, -1, 1, 0, 0, 1, -1, -1, -1, -1,
-1, 0, 0, 0, -1, -1, 1, 1, 1, 1, 0, -1, 1, 1, 0, 1,
0, -1, -1, -2, -1, -1, -2, 0, 1, 2, 3, 3, 3, 4, 1, 0,
2, 3, 2, 3, 3, 1, 0, 1, 2, 2, 2, 0, -1, -1, -1, -1,
-1, -3, -3, -3, -4, -4, -4, -5, -4, -3, -3, -2, -2, -2, -1, 0,
1, 0, 0, 0, 1, 2, 2, 3, 2, 1, 0, -1, -2, 1, 1, 0,
0, 0, -2, -2, -3, -2, 0, -1, -1, 1, 2, 2, 2, 2, 3, 5,
6, 5, 4, 4, 5, 7, 6, 3, 4, 4, 3, 2, 1, 2, 2, 1,
-1, -1, -2, -3, -2, -3, -4, -3, -4, -5, -4, -5, -4, -3, -2, -1,
-1, -1, -1, -1, 0, 0, 0, 1, 1, 2, 3, 4, 4, 3, 3, 3,
3, 2, 3, 2, 0, 1, 0, 0, 0, -1, 1, 0, 0, 0, 0, 0,
-1, 0, 0, -1, -1, -1, -1, -1, -1, 0, 1, 1, -1, -1, 0, 1,
1, 2, 1, -1, 1, 1, 1, 2, 0, 1, 0, 3, 5, 3, 1, 1,
1, 1, 0, -1, 0, 1, 0, -1, 0, -1, 2, 3, 1, 0, 0, -1,
-1, 0, 2, 3, 2, 1, 4, 3, 2, 3, 4, 2, 2, 2, 2, 2,
0, 3, 4, 3, 4, 3, 3, 3, 2, 3, 2, 2, 3, 3, 3, 3,
3, 2, 3, 4, 2, 2, 3, 3, 2, 2, 2, 2, 3, 3, 4, 4,
4, 5, 4, 3, 4, 5, 4, 3, 4, 5, 6, 7, 7, 4, 4, 3,
4, 5, 5, 5, 6, 4, 3, 4, 3, 5, 4, 2, 3, 4, 6, 7,
6, 6, 6, 6, 7, 7, 6, 8, 9, 9, 9, 8, 9, 10, 8, 7,
8, 7, 9, 7, 5, 6, 6, 4, 4, 5, 5, 5, 6, 5, 6, 5,
3, 6, 4, 4, 5, 5, 5, 5, 6, 6, 8, 7, 7, 7, 6, 8,
7, 6, 7, 7, 7, 7, 6, 6, 6, 7, 7, 9, 9, 7, 7, 8,
8, 8, 9, 10, 11, 10, 9, 8, 10, 9, 9, 10, 9, 8, 7, 7,
7, 6, 7, 4, 6, 6, -14, -9, -6, -7, 0, -6, -1, 11, 17, 16,
14, 15, 17, 16, 11, 7, 5, 3, 3, 0, 1, 2, 1, 2, 1, 2,
3, 1, 1, -1, 1, 1, -3, -1, 2, 3, 1, 1, 4, 5, 6, 8,
10, 13, 13, 14, 15, 13, 15, 14, 12, 12, 13, 11, 10, 12, 8, 4,
6, -4, -14, -9, -6, -10, -14, -7, 1, 1, 3, 8, 10, 11, 15, 13,
8, 7, 4, -1, -4, -5, -5, -8, -8, -2, -1, 2, 3, 5, 7, 10,
11, 7, 8, 4, 4, -6, -30, -17, -12, -30, -29, -12, 4, -1, 11, 27,
29, 39, 38, 30, 25, 34, 31, 9, 12, 16, 9, 1, 2, -1, -30, -37,
-14, -15, -36, -34, -5, 3, -3, 5, 17, 27, 31, 26, 11, 7, 14, 2,
-18, -23, -15, -17, -24, -19, -10, -2, 5, 11, 13, 19, 27, 21, 9, 6,
7, -1, -12, -15, -16, -15, -13, -10, -4, -4, -2, 6, 3, -1, 2, 4,
-1, -6, -3, -1, -1, 2, 7, 11, 15, 22, 21, 25, 31, 27, 22, 23,
25, 15, 8, 10, 7, -14, -40, -32, -15, -30, -44, -26, 5, 10, 6, 18,
30, 38, 29, 15, 11, 10, 1, -20, -31, -26, -20, -27, -24, -6, 9, 15,
18, 24, 30, 30, 22, 11, 6, 1, -9, -21, -24, -20, -20, -27, -31, -14,
4, -2, -1, 16, 25, 20, 13, 15, 16, 14, 3, -3, 7, 13, 9, 12,
26, 31, 23, 27, 37, 31, 23, 18, -11, -42, -28, -20, -52, -57, -23, -4,
-9, 6, 30, 42, 47, 39, 29, 25, 21, -3, -30, -28, -25, -36, -40, -27,
-10, 0, 12, 22, 33, 41, 37, 27, 22, 16, 0, -13, -21, -24, -23, -25,
-16, -15, -22, 3, 13, 2, 6, 20, 21, 9, 11, 11, 12, 9, 4, 8,
16, 19, 15, 24, 31, 30, 28, 28, 28, 24, 23, 4, -34, -41, -15, -31,
-62, -41, -4, 0, -5, 16, 37, 48, 44, 29, 24, 26, 8, -21, -24, -19,
-28, -36, -25, -7, 3, 12, 17, 30, 43, 38, 28, 22, 18, 7, -7, -14,
-15, -15, -16, -9, -1, 2, 8, 11, 11, 17, 16, 10, 9, 12, 9, 8,
10, 12, 15, 20, 26, 31, 34, 39, 40, 39, 36, 39, 37, 31, 21, -3,
-28, -26, -16, -35, -38, -11, 11, 14, 22, 42, 53, 54, 44, 32, 27, 16,
-6, -20, -18, -17, -23, -17, 1, 14, 25, 33, 40, 47, 48, 38, 25, 19,
9, -4, -10, -11, -10, -7, -1, 8, 23, 32, 38, 23, 1, 24, 29, -8,
-20, 8, 14, -1, 6, 21, 38, 41, 37, 46, 58, 51, 34, 38, 42, 29,
23, 27, 23, 19, -11, -36, -5, 3, -29, -26, 15, 27, 15, 25, 43, 51,
43, 28, 21, 22, 12, -12, -16, -6, -7, -13, -4, 13, 22, 26, 32, 39,
40, 34, 24, 16, 12, 4, -5, -6, -1, 1, 3, 11, 21, 29, 30, 36,
34, 4, 8, 23, -6, -24, -7, 7, -2, 6, 22, 35, 45, 45, 45, 53,
53, 34, 31, 35, 29, 22, 21, 27, 32, 41, 11, -29, 4, 26, -19, -39,
3, 24, 8, 9, 26, 46, 50, 33, 23, 33, 31, 1, -11, -1, -1, -10,
-10, 2, 16, 22, 22, 31, 45, 43, 29, 27, 27, 16, 7, 1, -1, 1,
2, 3, 12, 24, 26, 28, 35, 33, 32, 18, -23, -8, 13, -22, -37, 2,
22, 7, 17, 38, 56, 50, 37, 40, 49, 36, 14, 22, 26, 18, 13, 23,
35, 46, 4, -37, 22, 26, -33, -40, 11, 17, -4, 7, 27, 51, 46, 25,
20, 40, 28, -8, -11, 1, -1, -13, -10, 5, 21, 22, 21, 33, 43, 38,
27, 24, 23, 14, 4, -2, -2, -2, -1, 3, 12, 21, 26, 26, 31, 32,
28, 25, 19, 11, -7, -17, -5, -6, -18, -11, 11, 19, 18, 25, 38, 44,
35, 26, 32, 35, 16, 4, 18, 20, 10, 14, 26, 31, 36, 39, 41, 24,
-2, 6, 3, -23, -29, -14, -9, -7, 3, 15, 30, 38, 36, 30, 33, 24,
5, -4, -2, -8, -14, -11, -3, 10, 17, 21, 29, 39, 37, 26, 22, 20,
9, -2, -9, -12, -10, -10, -7, 8, 20, 24, 29, 33, 33, 28, 20, 11,
6, 3, -6, -8, -4, 0, 5, 14, 20, 23, 23, 10, 15, 19, 1, -11,
-7, -4, -9, -7, 1, 13, 22, 28, 32, 38, 41, 36, 36, 34, 24, 20,
17, 17, 22, -17, -40, 2, 4, -30, -24, 13, 19, 15, 19, 29, 46, 40,
15, 8, 21, 9, -15, -14, -1, 0, -4, 1, 12, 24, 26, 20, 25, 33,
22, 9, 6, 6, 2, -5, -7, -3, 3, 6, 9, 16, 26, 27, 23, 24,
23, 17, 11, 4, -1, -3, -4, -6, -1, 6, 6, 12, 19, 20, 20, 20,
16, 12, 8, 1, -3, -2, -3, -4, -1, 4, 5, 9, 14, 16, 17, 17,
16, 14, 10, 4, 0, 1, 2, -1, -1, 1, 5, 9, 9, 13, 11, -10,
-1, 12, -4, -13, 1, 7, 3, 9, 10, 19, 24, 19, 18, 29, 29, 19,
22, 28, 20, 4, -8, -8, -4, -18, -28, -14, 0, -4, -2, 15, 22, 23,
19, 14, 14, 12, -3, -12, -6, -9, -12, -9, -2, 8, 14, 15, 19, 25,
23, 16, 12, 8, 1, -7, -10, -9, -7, -5, -4, 3, 12, 15, 16, 18,
20, 18, 13, 7, 3, 0, -3, -4, -2, 2, 4, 6, 11, 14, 16, 18,
18, 14, 9, 4, 3, -2, -6, -5, -6, -5, 0, 2, 5, 9, 12, 10,
13, 15, 11, 7, 5, 3, 1, 0, -1, 0, 0, 1, 2, 2, 8, -6,
-16, 8, 7, -12, -8, 11, 9, 6, 8, 16, 21, 15, 13, 21, 30, 12,
-12, -11, 0, -11, -33, -26, -5, 1, -6, -2, 16, 27, 17, 9, 15, 18,
2, -14, -16, -11, -14, -22, -16, -1, 6, 4, 9, 20, 26, 21, 13, 10,
8, -1, -12, -17, -14, -13, -16, -10, 0, 4, 7, 12, 15, 16, 15, 9,
4, 4, -2, -10, -11, -7, -6, -5, 1, 6, 10, 13, 13, 14, 13, 10,
5, 0, -3, -6, -9, -8, -5, -4, 0, 5, 7, 8, 10, 10, 9, 7,
4, 1, -1, -4, -7, -7, -9, -8, -4, -1, 1, 2, 4, 4, 5, 5,
3, 3, 2, 0, 0, -1, -3, -3, -3, -2, 1, 1, 1, 3, 4, 3,
2, 2, 2, -1, -4, -3, -4, -3, -2, -3, -1, 0, 0, -2, 1, 4,
3, 3, 5, 3, 2, 2, -1, -2, -1, -1, -1, 0, 1, 2, 4, 4,
4, 6, 7, 4, 2, 1, -1, -2, -1, 0, -1, 0, -1, 0, 2, 3,
4, 6, 5, 4, 5, 3, 3, 4, 3, 2, 4, 5, 4, 5, 6, 6,
8, 6, 5, 5, 2, 0, 1, 0, -2, -1, -1, -2, -1, 3, 4, 4,
6, 7, 6, 4, 4, 4, 3, 4, 2, 3, 4, 1, 2, 1, 2, 5,
5, 6, 7, 5, 4, 4, 5, 4, 5, 4, 4, 5, 5, 4, 5, 7,
7, 6, 8, 8, 5, 4, 5, 3, 3, 4, 4, 6, 5, 3, 5, 8,
7, 6, 5, 5, 7, 7, 5, 4, 6, 7, 5, 6, 8, 7, 6, 5,
6, 8, 7, 7, 6, 6, 5, 3, 4, 4, 5, 7, 8, 7, 5, 5,
4, 4, 5, 4, 4, 4, 4, 5, 3, 3, 3, 3, 4, 4, 4, 5,
3, 4, 3, 2, 3, 3, 1, 0, 1, 1, 3, 4, 4, 5, 6, 6,
4, 5, 5, 5, 3, 3, 3, 2, 3, 4, 5, 5, 3, 3, 6, 6,
5, 6, 5, 4, 4, 5, 5, 4, 2, 1, 4, 5, 4, 5, 7, 7,
6, 7, 6, 5, 6, 5, 3, 2, 2, 2, 3, 4, 3, 2, 0, 1,
1, 3, 4, 4, 6, 5, 2, 2, 3, 1, 1, 1, 0, 1, 2, 1,
1, 3, 4, 3, 4, 3, 3, 3, 3, 4, 2, 3, 4, 4, 4, 6,
6, 6, 5, 5, 4, 4, 4, 3, 2, 1, 1, 1, 1, 1, 0, 1,
1, 1, 4, 3, 1, 2, 1, 1, 0, 1, 2, 1, 1, 1, 0, 1,
2, 1, -1, 1, 1, 0, -2, -2, 2, 1, -1, -2, 0, 2, 2, 1,
1, 0, 0, 0, 0, -1, -1, -1, 1, -1, -4, 2, 3, 0, -2, -2,
1, 3, 1, -3, -3, -1, -2, -2, -1, -1, -1, 0, -1, -1, -1, -1,
-2, -2, -2, -3, -3, -2, -3, -3, -2, -1, 0, -1, 0, 0, -1, -1,
-2, -4, -3, -2, 0, -1, -1, 0, -1, -1, -2, -1, -1, 0, -1, -1,
-3, -3, -1, -1, -2, -3, -4, -3, -1, -2, -2, -2, -1, -1, -2, -1,
0, -1, 0, 0, -1, -1, -1, 0, 0, 1, 0, -1, 0, 1, 1, 2,
1, -1, -1, -2, -2, -1, -1, -1, -1, 1, 2, 1, 1, 0, 0, 0,
-1, -1, 0, 0, 0, -1, 0, -1, -1, 1, 1, 0, 1, 2, 0, -1,
1, 1, 2, 3, 3, 3, 2, 3, 1, 1, 3, 3, 2, 0, 1, 3,
2, 1, 2, 4, 4, 3, 2, 1, 2, 2, 1, 1, 4, 2, 1, 3,
1, 3, 5, 2, 3, 2, 2, 3, 3, 3, 3, 2, 1, 1, 2, 3,
0, 0, 0, 0, 1, 0, 0, 1, -1, -1, 0, -2, -3, -2, -2, -3,
-3, -5, -6, -5, -6, -7, -7, -8, -9, -9, -11, -10, -8, -10, -12, -13,
-14, -15, -14, -13, -13, -14, -15, -13, -14, -15, -15, -17, -18, -19, -20, -22,
-22, -20, -21, -24, -23, -21, -23, -24, -24, -25, -25, -25, -26, -27, -29, -29,
-28, -27, -28, -28, -29, -31, -31, -31, -31, -30, -30, -32, -35, -34, -35, -36,
-35, -37, -37, -36, -37, -37, -37, -38, -38, -37, -40, -40, -40, -40, -40, -41,
-41, -42, -44, -43, -42, -45, -45, -44, -44, -44, -44, -45, -45, -45, -46, -48,
-48, -48, -48, -46, -47, -47, -47, -47, -47, -47, -47, -48, -49, -50, -49, -48,
-49, -49, -49, -48, -49, -51, -53, -52, -52, -53, -51, -52, -52, -51, -52, -55,
-53, -53, -56, -55, -54, -55, -55, -56, -56, -55, -56, -57, -57, -56, -54, -56,
-57, -56, -57, -57, -58, -57, -56, -58, -56, -54, -55, -56, -56, -55, -56, -57,
-58, -57, -58, -59, -58, -58, -59, -59, -60, -59, -58, -59, -60, -59, -60, -60,
-60, -59, -60, -61, -61, -60, -59, -61, -61, -61, -63, -62, -60, -60, -63, -63,
-62, -62, -63, -64, -63, -61, -59, -59, -58, -59, -59, -58, -59, -59, -60, -61,
-62, -61, -60, -61, -62, -62, -61, -61, -62, -62, -61, -62, -64, -64, -63, -63,
-63, -63, -62, -63, -63, -62, -63, -63, -63, -62, -62, -61, -63, -63, -62, -62,
-63, -62, -62, -63, -63, -62, -63, -64, -64, -65, -65, -64, -64, -62, -63, -62,
-61, -62, -62, -62, -61, -60, -60, -61, -62, -62, -61, -62, -61, -63, -63, -62,
-61, -60, -60, -61, -61, -61, -60, -61, -61, -59, -58, -59, -60, -60, -60, -62,
-62, -61, -62, -61, -60, -62, -62, -62, -62, -61, -63, -62, -60, -60, -59, -59,
-61, -61, -59, -59, -60, -61, -60, -59, -58, -59, -59, -59, -58, -58, -58, -57,
-57, -57, -57, -59, -57, -57, -57, -56, -55, -55, -57, -56, -56, -57, -56, -55,
-57, -56, -56, -57, -57, -57, -57, -58, -56, -54, -56, -55, -55, -55, -55, -56,
-56, -56, -55, -56, -55, -54, -55, -55, -54, -54, -53, -54, -52, -55, -55, -61,
-69, -41, -50, -54, -51, -58, -49, -54, -55, -52, -55, -55, -80, -67, -25, -55,
-47, -57, -62, -34, -57, -45, -45, -60, -53, -50, -50, -50, -54, -55, -52, -53,
-64, -53, -54, -48, -43, -51, -40, -46, -50, -55, -71, -56, -47, -44, -47, -43,
-36, -40, -42, -42, -41, -46, -47, -50, -46, -45, -52, -48, -46, -44, -49, -48,
-42, -41, -37, -39, -42, -40, -41, -43, -43, -41, -46, -45, -49, -51, -43, -48,
-49, -42, -44, -44, -41, -45, -40, -41, -43, -44, -43, -45, -48, -46, -46, -47,
-45, -46, -43, -44, -41, -43, -43, -41, -45, -39, -43, -39, -45, -37, -44, -47,
-37, -46, -38, -44, -38, -44, -40, -37, -45, -38, -41, -43, -40, -42, -38, -38,
-40, -41, -41, -39, -38, -41, -41, -39, -38, -37, -40, -39, -40, -38, -40, -36,
-36, -34, -36, -40, -33, -38, -34, -35, -38, -35, -38, -32, -36, -36, -35, -36,
-33, -33, -36, -36, -34, -40, -39, -34, -38, -37, -35, -34, -38, -35, -35, -34,
-35, -35, -38, -34, -34, -34, -35, -36, -33, -38, -31, -35, -30, -32, -33, -29,
-34, -30, -35, -34, -33, -36, -34, -36, -33, -33, -28, -36, -30, -31, -34, -33,
-31, -30, -34, -30, -33, -32, -29, -31, -30, -30, -26, -31, -27, -28, -27, -26,
-32, -26, -28, -26, -34, -33, -32, -31, -30, -33, -24, -28, -29, -28, -28, -24,
-31, -28, -29, -27, -26, -33, -27, -30, -26, -34, -28, -28, -29, -27, -28, -27,
-29, -25, -28, -27, -28, -21, -32, -25, -29, -27, -24, -27, -24, -25, -18, -30,
-20, -26, -22, -27, -24, -23, -28, -21, -26, -24, -29, -22, -21, -19, -21, -22,
-23, -20, -24, -24, -22, -26, -24, -29, -21, -19, -22, -21, -23, -16, -19, -19,
-19, -13, -15, -23, -21, -15, -15, -23, -20, -19, -18, -21, -20, -18, -13, -13,
-17, -19, -19, -15, -22, -21, -19, -17, -21, -20, -17, -18, -17, -17, -14, -15,
-15, -17, -16, -16, -16, -17, -20, -19, -20, -18, -17, -16, -13, -16, -13, -15,
-17, -14, -16, -15, -15, -13, -15, -15, -11, -18, -16, -16, -12, -11, -17, -13,
-17, -14, -18, -16, -16, -16, -11, -21, -14, -14, -12, -15, -16, -9, -15, -11,
-12, -11, -11, -15, -10, -16, -17, -11, -11, -15, -16, -16, -19, -13, -13, -16,
-8, -7, -15, -16, -8, -13, -15, -10, -18, -10, -15, -18, -13, -7, -5, -15,
-7, -13, -9, -9, -20, -13, -14, -12, -22, -13, -9, -13, -7, -11, -7, -10,
-12, -14, -13, -11, -18, -12, -8, -12, -11, -12, -11, -7, -6, -6, -8, -4,
-10, -11, -11, -8, -8, -14, -14, -13, -9, -11, -9, -11, -5, -9, -13, -7,
-7, -1, -13, -8, -4, -5, -7, -14, -6, -6, -8, -13, -9, -2, -10, -10,
-11, -4, -5, -11, -9, -9, 0, -11, -8, -7, -7, -3, -13, -3, -6, 1,
-5, -9, 0, -8, -2, -11, -6, -1, -8, -1, -11, -3, -6, -14, -8, -9,
-1, -6, -2, -4, -3, -1, -8, -1, -8, -5, -8, -3, -3, -8, 0, -9,
-3, -4, -3, -5, -6, 0, -8, -2, -5, -2, -2, -10, -3, -4, -1, -7,
-9, 1, -2, -1, -5, -6, 1, -5, -2, -5, -2, 2, -7, -3, -5, 2,
-2, -6, -2, -4, 4, -5, -5, 0, -1, 1, -7, -5, 0, -3, -3, -7,
-1, -3, -2, -3, -5, 3, -2, -4, -8, 0, 2, -6, -1, -3, 0, -3,
-4, 0, -1, 4, -1, 0, 4, 3, 3, -1, 2, -1, -3, -2, -4, 2,
-3, -2, -1, -1, 0, -3, 1, 0, 4, -2, 0, -2, -1, 2, -6, -1,
-4, 0, 2, -3, 2, 0, -1, -3, -4, -2, -3, 1, -4, -1, 1, 0,
2, -2, 2, 5, 4, 5, 3, 7, 6, 1, 4, 1, 4, 2, -4, 2,
0, 2, -1, -2, 1, 1, 3, 0, -3, 2, -1, -1, 1, -2, 1, -1,
0, 1, 0, 5, 3, 5, 5, 2, 7, 0, 2, 1, 0, 2, -3, 3,
0, 3, 3, 0, 3, -1, 4, 0, 0, -1, -2, 2, -2, 4, 0, 0,
4, 0, 6, 1, 1, 5, 2, 5, 1, 0, 1, 0, 0, 0, 2, 1,
0, -1, 1, 3, 0, 2, 0, 1, 2, 0, 1, -2, -1, 1, -4, 0,
1, 1, 3, -1, 0, 0, -1, -1, -1, 2, -1, 0, 2, 2, 5, 2,
-1, -2, -1, 0, -4, -1, 2, -2, -2, -4, -1, 1, -1, 1, 3, 6,
5, 2, 0, 2, 3, 1, 2, 2, 3, 1, -2, 3, 3, 2, 3, 0,
2, 5, 3, 2, 2, 4, 3, -2, -1, 2, 4, 3, 0, 1, 2, 3,
2, 2, 3, 3, 3, 0, 1, 2, 1, 1, 0, 1, 3, 3, 3, 3,
2, 4, 5, 2, 2, 2, 1, 0, 0, 0, 2, 4, 3, 2, 4, 2,
0, 1, 2, 2, 2, 1, 2, 2, 2, 4, 2, 1, 3, 0, 1, 3,
3, 2, 1, 1, 1, 1, 1, 3, 3, 3, 2, -1, 0, 2, 2, 1,
0, 0, -1, -1, -1, 0, 1, 0, 0, -1, 0, 2, 1, -1, -2, 0,
-1, -3, -1, 0, 0, -1, -1, 1, 0, 0, 1, -1, -1, -1, -3, -1,
-2, -4, -1, -2, -2, -1, -1, -1, -1, 0, 0, 0, 0, -2, -1, 0,
-2, -2, -2, -1, 0, -1, -1, 1, 2, -1, -2, 1, 1, 0, 0, 0,
0, -1, -3, -1, 1, 0, 0, 0, -1, -3, -3, -2, -2, -2, -3, -2,
0, 0, 1, 0, -2, 0, -1, -1, -1, 0, 0, -2, -2, 0, 1, 0,
0, -1, -1, -1, -3, -1, -1, -2, -2, -2, -3, -3, -2, -2, -3, -4,
-3, -2, -1, 0, 1, 1, 0, -2, -2, 0, -1, -3, -2, -1, -3, -2,
1, 0, -1, -2, -1, -1, -2, 0, 0, -2, -2, -2, -2, -1, -2, -3,
-2, -3, -3, -3, -3, -2, -1, -4, -6, -3, -4, -2, -2, -3, -2, -3,
-2, -2, -1, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -4, -2, -3,
-2, -1, -1, 0, -1, -1, -2, -4, -3, -1, 0, -2, -3, -3, -1, -1,
-4, -3, -1, -2, -2, -2, -2, 0, -1, -1, -1, -2, -3, -1, 0, 0,
-1, -3, -1, -1, 0, 0, 0, 0, 0, -2, -2, -2, -3, -2, 0, 0,
-1, -1, -3, -5, -3, -2, -2, -1, -1, -1, -2, -3, -4, -5, -6, -6,
-5, -5, -4, -4, -4, -3, -2, -3, -4, -2, -3, -4, -2, -4, -4, -2,
-3, -4, -4, -2, -2, -2, 0, -1, -3, -2, 0, 0, 0, 0, -1, -1,
-2, -2, -1, -2, -4, -4, -5, -4, -3, -4, -6, -4, -2, -4, -5, -3,
-4, -3, -2, -5, -6, -4, -4, -3, -3, -4, -3, -5, -5, -3, -2, -2,
-4, -2, -1, -3, -2, -3, -3, -2, -4, -4, -3, -4, -4, -3, -6, -4,
-2, -4, -3, -2, -3, -3, -4, -4, -3, -2, -2, -3, -3, -1, -3, -3,
-1, -1, -2, -4, -3, -1, -1, -2, -2, -1, 0, -1, -1, -2, -2, -2,
-3, -2, -1, -1, -2, -2, -1, -1, -1, -2, -1, -2, -1, 2, 1, -1,
-1, 1, 2, 2, 0, -1, 1, 1, 1, 3, 3, 1, 1,
};

typedef struct _sample_t {
    mp_obj_base_t base;
    const int8_t *data;
    uint32_t len;
} sample_t;

typedef struct _sample_iter_t {
    mp_obj_base_t base;
    microbit_sound_bytes_obj_t *buffer;
    const int8_t *data;
    uint32_t len;
    uint32_t index;
} sample_iter_t;

STATIC mp_obj_t sample_next(mp_obj_t o_in) {
    sample_iter_t *iter = (sample_iter_t *)o_in;
    const int8_t *src = iter->data + iter->index;
    int8_t *dest = iter->buffer->data;
    if (iter->index + SOUND_CHUNK_SIZE > iter->len) {
        unsigned len = iter->len-iter->index;
        if (len == 0) {
            return MP_OBJ_STOP_ITERATION;
        }
        unsigned int i;
        for (i = 0; i < len; i++) {
            dest[i] = src[i];
        }
        for (; i < SOUND_CHUNK_SIZE; i++) {
            dest[i] = 0;
        }
        iter->index += len;
    } else {
        for (unsigned int i = 0; i < SOUND_CHUNK_SIZE; i++) {
            dest[i] = src[i];
        }
        iter->index += SOUND_CHUNK_SIZE;
    }
    return iter->buffer;
}

STATIC mp_obj_t sample_get_buffer(mp_obj_t self_in) {
    sample_iter_t *self = (sample_iter_t *)self_in;
    return self->buffer;
}
MP_DEFINE_CONST_FUN_OBJ_1(sample_get_buffer_obj, sample_get_buffer);

STATIC const mp_map_elem_t local_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_buffer), (mp_obj_t)&sample_get_buffer_obj },
};

STATIC MP_DEFINE_CONST_DICT(local_dict, local_dict_table);

const mp_obj_type_t microbit_sample_iter_type = {
    { &mp_type_type },
    .name = MP_QSTR_iterator,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = mp_identity,
    .iternext = sample_next,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = (mp_obj_dict_t*)&local_dict,
};

STATIC mp_obj_t sample_iter(mp_obj_t o_in) {
    sample_t *obj = (sample_t *)o_in;
    sample_iter_t *iter = m_new_obj(sample_iter_t);
    iter->base.type = &microbit_sample_iter_type;
    iter->data = obj->data;
    iter->len = obj->len;
    iter->index = 0;
    iter->buffer = new_microbit_sound_bytes();
    return iter;
}

const mp_obj_type_t microbit_sample_type = {
    { &mp_type_type },
    .name = MP_QSTR_Sample,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .unary_op = NULL,
    .binary_op = NULL,
    .attr = NULL,
    .subscr = NULL,
    .getiter = sample_iter,
    .iternext = NULL,
    .buffer_p = {NULL},
    .stream_p = NULL,
    .bases_tuple = NULL,
    .locals_dict = NULL,
};

STATIC const sample_t microbit_sample_microbit_sample_obj = {
    .base = { &microbit_sample_type },
    .data = sample_microbit,
    .len = sizeof(sample_microbit)
};


STATIC const mp_map_elem_t globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_samples) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_microbit), (mp_obj_t)&microbit_sample_microbit_sample_obj },
};


STATIC MP_DEFINE_CONST_DICT(sample_module_globals, globals_table);

const mp_obj_module_t samples_module = {
    .base = { &mp_type_module },
    .name = MP_QSTR_samples,
    .globals = (mp_obj_dict_t*)&sample_module_globals,
};



}
