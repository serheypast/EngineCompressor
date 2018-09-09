#ifndef __WAVELET_H__
#define __WAVELET_H__

#include "Defines.hpp"

void waveletTransform (int** data, TransformSettings *settings);
char *AnalyzeWavelet( Wavelet wavelet, MatrixType matrixType );

#endif // __WAVELET_H__
