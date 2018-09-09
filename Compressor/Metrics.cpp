#include <math.h>

#include "Metrics.hpp"

template<typename T>
double MSE(T **etalonImg, T **img, unsigned width, unsigned height) {
	double e = 0.0;
	double wh = width * height;

	for (unsigned i = 0; i < height; i++)
	  for (unsigned j = 0; j < width; j++)
			e += ( etalonImg[i][j] - img[i][j] ) * ( etalonImg[i][j] - img[i][j] );
	
	return e / wh;
}

template<typename T>
double PSNR (T **etalonImg, T **img, unsigned width, unsigned height, unsigned maxValue) {
	double mse = MSE(etalonImg, img, width, height);
	if (mse == 0)
		return 1000.0;
	return 10 * log10(float(maxValue) * float(maxValue) / mse);
}

template double PSNR (short **etalonImg, short **img, unsigned width, unsigned height, unsigned maxValue);
template double PSNR (int **etalonImg, int **img, unsigned width, unsigned height, unsigned maxValue);