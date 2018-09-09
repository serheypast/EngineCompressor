#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include "dlwt97.hpp"

void cdf_97_f ( double **data, int width, int height ) {
	const double a1 = -1.586134342;
	const double a2 = -0.05298011854;
	const double a3 =  0.8829110762;
	const double a4 =  0.4435068522;

	const double k1 = 1.14960439; //0.81289306611596146;
	const double k2 = 1 / 1.14960439; //0.61508705245700002;

	for (int col = 0; col < width; col++) {
		//predict 1
		for (int row = 1; row < height - 1; row += 2) {
			data[row][col] += a1 * ( data[row - 1][col] + data[row + 1][col] );
		}
		data[height - 1][col] += 2 * a1 * data[height - 2][col];

		//update 1
		for (int row = 2; row < height; row += 2) {
                data[row][col] += a2 * ( data[row - 1][col] + data[row + 1][col] );
		}
		data[0][col] += 2 * a2 * data[1][col];

		//predict 2
		for (int row = 1; row < height - 1; row += 2) {
			data[row][col] += a3 * ( data[row - 1][col] + data[row + 1][col] );
		}
		data[height - 1][col] += 2 * a3 * data[height - 2][col];

		//update 2
		for (int row = 2; row < height; row += 2) {
			data[row][col] += a4 * ( data[row - 1][col] + data[row + 1][col] );
		}
		data[0][col] += 2 * a4 * data[1][col];
	}

	double **temp_bank = new double*[height];
	for (int i = 0; i < height; i++) {
		temp_bank[i] = new double[width];
	}

	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			if ( ( row % 2 ) == 0 ) {
				temp_bank[col][int( row / 2 )] = k1 * data[row][col];
			} else {
				temp_bank[col][int( row / 2 ) + int( height / 2 )] = k2 * data[row][col];
			}
		}
	}

	for (int row = 0; row < width; row++) {
		for (int col = 0; col < height; col++) {
			data[row][col] = temp_bank[row][col];
		}
	}

	for (int i = 0; i < height; i++) {
		delete[] temp_bank[i];
	}
	delete [] temp_bank;
}

void cdf_97_i ( double **data, int width, int height ) {
	double a1 =  1.586134342;
	double a2 =  0.05298011854;
	double a3 = -0.8829110762;
	double a4 = -0.4435068522;

	double k1 = 1/1.14960439;
	double k2 = 1.14960439;

	double **temp_bank = new double*[height];
	for (int i = 0; i < height; i++) {
		temp_bank[i] = new double [width];
	}
	
	for (int col = 0; col < int( width / 2 ); col++) {
		for (int row = 0; row < height; row++) {
			temp_bank[col * 2    ][row] = k1 * data[row][col                   ];
			temp_bank[col * 2 + 1][row] = k2 * data[row][col + int( width / 2 )];
		}
	}

	for (int row = 0; row < width; row++) {
        for (int col = 0; col < height; col++) {
			data[row][col] = temp_bank[row][col];
		}
	}

	for (int col = 0; col < width; col++) {
		// Inverse update 2
		for (int row = 2; row < height; row += 2) {
			data[row][col] += a4 * ( data[row - 1][col] + data[row + 1][col] );
		}
		data[0][col] += 2 * a4 * data[1][col];

		//Inverse predict 2
		for (int row = 1; row < ( height - 1 ); row += 2) {
			data[row][col] += a3 * ( data[row - 1][col] + data[row + 1][col] );
		}
		data[height - 1][col] += 2 * a3 * data[height - 2][col];
		
		// Inverse update 1
		for (int row = 2; row < height; row += 2) {
			data[row][col] += a2 * ( data[row - 1][col] + data[row + 1][col] );
		}
		data[0][col] += 2 * a2 * data[1][col];
		
		// Inverse predict 1
		for (int row = 1; row < ( height - 1 ); row += 2 ) {
			data[row][col] += a1 * ( data[row - 1][col] + data[row + 1][col] );
		}
		data[height - 1][col] += 2 * a1 * data[height - 2][col]; 
	}
}

void fwt97(double **data, int areaWidth, int areaHeight) {
	cdf_97_f(data, areaWidth, areaHeight);
	cdf_97_f(data, areaWidth, areaHeight);
}

void iwt97( double** data, int areaWidth, int areaHeight ) {
	cdf_97_i(data, areaWidth, areaHeight);
	cdf_97_i(data, areaWidth, areaHeight);
}

void applyWavelet9_7 (int **data, TransformSettings *settings) {
	double **ddata = new double*[settings->blockSize];
	for (int i = 0; i < settings->blockSize; i++) {
		ddata[i] = new double[settings->blockSize];
	}

	if ( settings->transformDirection == FORWARD ) {
		for (int i = 0; i < settings->blockSize; i++) {
			for (int j = 0; j < settings->blockSize; j++) {
				ddata[i][j] = (double)( data[i][j] /*/ 100.0*/ );
			}
		}

		for(int k = 0; k < settings->decLevel; k++) {
			fwt97(ddata, settings->blockSize >> k, settings->blockSize >> k);
		}

		for (int i = 0; i < settings->blockSize; i++) {
			for (int j = 0; j < settings->blockSize; j++) {
				data[i][j] = (int)floor( ddata[i][j] + 0.5 );
			}
		}
	} else {
		for (int i = 0; i < settings->blockSize; i++) {
			for (int j = 0; j < settings->blockSize; j++) {
				ddata[i][j] = (double)data[i][j];
			}
		}

		for(int k = settings->decLevel - 1 ; k >= 0; k--) {
			iwt97(ddata, settings->blockSize >> k, settings->blockSize >> k);
		}

		for (int i = 0; i < settings->blockSize; i++) {
			for (int j = 0; j < settings->blockSize; j++) {
				data[i][j] = (int)floor( ddata[i][j] /** 100.0*/ + 0.5 );
			}
		}
	}

	for (int i = 0; i < settings->blockSize; i++) {
		delete[] ddata[i];
	}
	delete []ddata;
}