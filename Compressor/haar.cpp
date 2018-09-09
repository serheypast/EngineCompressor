#include "haar.hpp"

#include <stdlib.h>
#include <cstring>

/* Calculate one iteration of the Haar forward FWT in 1-dimension */
void Haar_forward_pass_1d (int halfSize, int* s) {
	int *a = (int *) malloc (sizeof(int) * halfSize);
	int *c = (int *) malloc (sizeof(int) * halfSize);
	
	for (int i = 0;	i < halfSize; i++) {
		a[i] = ( s[2 * i] + s[2 * i + 1] ) >> 1;
		c[i] = ( s[2 * i] - s[2 * i + 1] ) >> 1;
	}

	memcpy(  s,           a, sizeof(int) * halfSize );
	memcpy( &s[halfSize], c, sizeof(int) * halfSize );

	free(a);
	free(c);
}

/* Calculate one iteration of the Haar forward FWT in 2-dimensions */
void Haar_forward_pass_2d (int size, int **s) {
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			// корректировка для работы в целочисленной арифметике
			s[i][j] = s[i][j] << 2;
		}
	}

	for (int i = 0;	i < size; i++) {
		Haar_forward_pass_1d (size / 2, s[i]);
	}

	int *c = (int *) malloc (sizeof(int) * size);

	for (int j = 0;	j < size; j++) {
		for (int i = 0; i < size; i++)
			c[i] = s[i][j];
		
		Haar_forward_pass_1d (size / 2, c);
		
		for (int i = 0; i < size; i++)
			s[i][j] = c[i];
	}

	free (c);
}

/* Calculate one iteration of the Haar inverse FWT in 1-dimension */
void Haar_inverse_pass_1d (int size, int *s) {
	int *r = (int *) malloc (sizeof(int) * size);
	
	for (int i = 0;	i < size / 2; i++) {
		r[2 * i    ] = s[i] + s[i + size / 2];
		r[2 * i + 1] = s[i] - s[i + size / 2]; 
	}

	memcpy( s, r, sizeof(int) * size );

	free(r);
}

/* Calculate one iteration of the Haar inverse FWT in 2-dimensions */
void Haar_inverse_pass_2d (int size, int **s) {
	for (int i = 0;	i < size; i++) {
		Haar_inverse_pass_1d (size, s[i]);
	}

	int *c = (int *) malloc (sizeof(int) * size);

	for (int j = 0;	j < size; j++) {
		for (int i = 0;	i < size; i++)
			c[i] = s[i][j];
		
		Haar_inverse_pass_1d (size, c);
		
		for (int i = 0;	i < size; i++)
			s[i][j] = c[i];
	}

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			// корректировка для работы в целочисленной арифметике
			s[i][j] = s[i][j] >> 2;
		}
	}

	free (c);
}

void applyHaarWavelet(int** data, TransformSettings *settings) {
	if ( settings->transformDirection == TransformDirection::FORWARD ) {
		for(int k = 0; k < settings->decLevel; k++) {
			Haar_forward_pass_2d(settings->blockSize >> k, data);
		}
	} else {
		for(int k = settings->decLevel - 1 ; k >= 0; k--) {
			Haar_inverse_pass_2d(settings->blockSize >> k, data);
		}
	}
}