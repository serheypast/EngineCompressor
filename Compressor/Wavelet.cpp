#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "Wavelet.hpp"

#include "dlwt97.hpp"
#include "dlwt53.hpp"
#include "haar.hpp"

double **IntDoubleConverter( int **int_, double **double_, int width, int height, bool forward) {
	if ( forward ) {
		double_ = (double **)malloc( height * sizeof( double * ) );
		for (int i = 0; i < height; i++) {
			double_[i] = (double *)malloc( width * sizeof( double ) );
			for (int j = 0; j < width; j++) {
				double_[i][j] = (double)int_[i][j];
			}
		}
	} else {
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				int_[i][j] = abs( double_[i][j] );
			}
			free( double_[i] );
		}
		free( double_ );
		double_ = NULL;
	}

	return double_;
}

void waveletTransform (int** data, TransformSettings *settings) {
	switch( settings->lumaWavelet ) {
		case Wavelet::CDF_53:
			applyWavelet5_3(data, settings);
			break;
		case Wavelet::CDF_97:
			{
				//double **newData = NULL;
				//newData = IntDoubleConverter( data, newData, settings->blockSize, settings->blockSize, true );
				applyWavelet9_7(data, settings);
				//newData = IntDoubleConverter( data, newData, settings->blockSize, settings->blockSize, false );
				//if ( newData != NULL ) {
				//	fprintf(stderr, "Double to int conversion failed\n");
				//}
			}
			break;
		case Wavelet::HAAR:
			applyHaarWavelet(data, settings);
			break;
	}
}

char *AnalyzeWavelet( Wavelet wavelet, MatrixType matrixType ) {
	switch( wavelet ) {
		case Wavelet::CDF_53:
			return "CDF 5/3";
		case Wavelet::CDF_97:
			return "CDF 9/7";
		case HAAR:
			return "Haar";
		default:
			return "Unknown";
	}
	
	return "Unknown";
}