#ifndef __METRICS__
#define __METRICS__

template<typename T>
double PSNR (T **etalonImg, T **img, unsigned width, unsigned height, unsigned maxValue);

#endif // __METRICS__