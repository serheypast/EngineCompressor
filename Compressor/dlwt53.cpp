#include "dlwt53.hpp"
#include "math.h"

void dlwt_53_F ( int **data, int xStart, int yStart, int areaWidth, int areaHeight) {
	unsigned int pos;
	unsigned int yStop = areaHeight + yStart;
	unsigned int xStop = areaWidth + xStart;
	unsigned int areaHalfWidth = areaWidth >> 1;
	unsigned int j,i;

	for (j = yStart; j < yStop; j++ ) { // Loop by rows
		data[xStart + 1][j] = data[xStart + 1][j] - ( ( data[xStart    ][j] + data[xStart + 2][j] ) >> 1 );
		data[xStart    ][j] = data[xStart    ][j] + ( ( data[xStart + 1][j] + data[xStart + 1][j] ) >> 2 );

		for (i = 1; i < areaHalfWidth - 1; i++) {
			pos = (i << 1) + xStart;
			data[pos + 1][j] = data[pos + 1][j] - ( ( data[pos    ][j] + data[pos + 2][j] ) >> 1 );
			data[pos    ][j] = data[pos    ][j] + ( ( data[pos - 1][j] + data[pos + 1][j] ) >> 2 );
		}
		data[xStop - 1][j] = data[xStop - 1][j] - ( ( data[xStop - 2][j] + data[xStop - 2][j] ) >> 1 );
		data[xStop - 2][j] = data[xStop - 2][j] + ( ( data[xStop - 3][j] + data[xStop - 1][j] ) >> 2 );
		
		int* temp = new int [areaHalfWidth];
		for (i = 0; i < areaHalfWidth; i++) {
			pos = ( i << 1 ) + xStart;
			temp[i] = data[pos + 1][j];
			data[i + xStart][j] = data[pos][j];
		}
		for (i = 0; i < areaHalfWidth; i++) {
			data[areaHalfWidth + i + xStart][j] = temp[i];
		}
		delete[] temp;
	}

	unsigned int areaHalfHeight = areaHeight >> 1;
	for (i = xStart; i < xStop; i++ ) { // Loop by columns
		data[i][1 + yStart] = data[i][1 + yStart] - ( ( data[i][yStart    ] + data[i][yStart + 2] ) >> 1 );
		data[i][    yStart] = data[i][    yStart] + ( ( data[i][yStart + 1] + data[i][yStart + 1] ) >> 2 );
		
		for (j = 1; j < areaHalfHeight - 1; j++) {
			pos = ( j << 1 ) + yStart;
			data[i][pos + 1] = data[i][pos + 1] - ( ( data[i][pos    ] + data[i][pos + 2] ) >> 1 );
			data[i][pos    ] = data[i][pos    ] + ( ( data[i][pos - 1] + data[i][pos + 1] ) >> 2 );
		}
		data[i][yStop - 1] = data[i][yStop - 1] - ( ( data[i][yStop - 2] + data[i][yStop - 2] ) >> 1 );
		data[i][yStop - 2] = data[i][yStop - 2] + ( ( data[i][yStop - 3] + data[i][yStop - 1] ) >> 2 );

		int* temp = new int[areaHalfHeight];
		for (j = 0; j < areaHalfHeight; j++) {
			pos = j << 1;
			temp[j] = data[i][pos + 1 + yStart];
			data[i][j + yStart] = data[i][pos + yStart];
		}
	
		for (j = 0; j < areaHalfHeight; j++) {
			data[i][areaHalfHeight + j + yStart] = temp[j];
		}
		delete[] temp;
	}
}

void dlwt_53_B ( int **data, int xStart, int yStart, int areaWidth, int areaHeight) {
	unsigned int pos;
	unsigned int yStop = areaHeight + yStart;
	unsigned int xStop =  areaWidth + xStart;
	unsigned int areaHalfHeight = areaHeight >> 1;
	unsigned int areaHalfWidth = areaWidth >> 1;
	unsigned int i,j;

	for (i = xStart; i < xStop; i++) { // Loop by columns
		int* temp = new int[areaHalfHeight];
		for (j = 0; j < areaHalfHeight; j++) {
			temp[j] = data[i][j + yStart];
		}
  
		for (j = 0; j < areaHalfHeight; j++) {
			pos = ( j << 1 ) + yStart;
			data[i][pos    ] = temp[j];
			data[i][pos + 1] = data[i][yStart + areaHalfHeight + j];
		}
		delete [] temp;
		data[i][yStart] = data[i][yStart] - ( ( data[i][yStart + 1] + data[i][yStart + 1] ) >> 2 );

		for (j = 1; j < areaHalfHeight; j++) {
			pos = ( j << 1 ) + yStart;
			data[i][pos    ] = data[i][pos    ] - ( ( data[i][pos - 1] + data[i][pos + 1] ) >> 2 );
			data[i][pos - 1] = data[i][pos - 1] + ( ( data[i][pos    ] + data[i][pos - 2] ) >> 1 );
		}
		data[i][yStop - 1] = data[i][yStop - 1] + ( ( data[i][yStop - 2] + data[i][yStop - 2] ) >> 1 );
	}

	for (j = yStart; j < yStop; j++ ) { // Loop by rows
		int* temp = new int [areaHalfWidth];
		for (i = 0; i < areaHalfWidth; i++) {
			temp[i] = data[i + xStart][j];
		}
		for (i = 0; i < areaHalfWidth; i++) {
			pos = ( i << 1 ) + xStart;
			data[pos    ][j] = temp[i];
			data[pos + 1][j] = data[areaHalfWidth + i + xStart][j];
		}
		delete [] temp;
		data[xStart][j] = data[xStart][j] - ( ( data[xStart + 1][j] + data[xStart + 1][j] ) >> 2);
		
		for (i = 1; i < areaHalfWidth; i++) {
			pos = ( i << 1 ) + xStart;
			data[pos    ][j] = data[pos    ][j] - ( ( data[pos - 1][j] + data[pos + 1][j] ) >> 2 );
			data[pos - 1][j] = data[pos - 1][j] + ( ( data[pos    ][j] + data[pos - 2][j] ) >> 1 );
		}
		data[xStop - 1][j] = data[xStop - 1][j] + ( ( data[xStop - 2][j] + data[xStop - 2][j] ) >> 1 );
	}
}

void applyWavelet5_3(int** data, TransformSettings *settings) {
	if ( settings->transformDirection == TransformDirection::FORWARD ) {
		for(int k = 0; k < settings->decLevel; k++) {
			dlwt_53_F (data, 0, 0, settings->blockSize >> k, settings->blockSize >> k);
		}
	} else {
		for(int k = settings->decLevel - 1 ; k >= 0; k--) {
			dlwt_53_B (data, 0, 0, settings->blockSize >> k, settings->blockSize >> k);
		}
	}
}