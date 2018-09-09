#include "FrameDecorrelate.hpp"

template<typename T>
int SubtractionForward( T **bearingImage, T **currentImage, const int width, const int height, const int offset ) {
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width - offset; j++) {
			T res = currentImage[i][j] - bearingImage[i][j + offset];
			currentImage[i][j] = res;
		}
	}
	return 0;
}

template<typename T>
int SubtractionBackward( T **bearingImage, T **currentImage, const int width, const int height, const int offset ) {
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width - offset; j++) {
			T res = currentImage[i][j] + bearingImage[i][j + offset];
			currentImage[i][j] = res;
		}
	}
	return 0;
}

template int SubtractionForward( short **bearingImage, short **currentImage, const int width, const int height, const int offset );
template int SubtractionBackward( short **bearingImage, short **currentImage, const int width, const int height, const int offset );

template int SubtractionForward( int **bearingImage, int **currentImage, const int width, const int height, const int offset );
template int SubtractionBackward( int **bearingImage, int **currentImage, const int width, const int height, const int offset );
