#include "TiffProcessor.hpp"

#include "tiffio.h"

template<typename T>
void ShiftArrayLeft( T *arr, const unsigned bitSize, const unsigned size ) {
	const unsigned shift = sizeof(T) * 8 - bitSize;
	const unsigned correctionCoeff = ( 1 << shift ) - 1;

	for (unsigned i = 0; i < size; i++) {
		arr[i] = ( arr[i] << shift ) + correctionCoeff;
	}
}

template<typename T>
void ShiftArrayRight( T *arr, const unsigned bitSize, const unsigned size ) {
	const unsigned shift = sizeof(T) * 8 - bitSize;
	const unsigned correctionCoeff = ( 1 << bitSize ) - 1;

	for (unsigned i = 0; i < size; i++) {
		arr[i] = ( arr[i] >> shift ) & correctionCoeff;
	}
}

template<typename T>
T **OpenTiffFile( const char *fileName, const unsigned bitSize, tiffParameters_t *tiffParams ) {
	if ( tiffParams == NULL ) {
		fprintf(stderr, "tiff parameters was not set\n");
		return NULL;
	}
	
	T **img = NULL;

	TIFF* tif = TIFFOpen(fileName, "r");
	if ( tif ) {
		uint16 config;

		// обязательные параметры
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &tiffParams->Height);
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &tiffParams->Width);
		TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &config);
		TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &tiffParams->SamplesPerPixel);
		TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &tiffParams->BitsPerSample);
		
		// дополнительные найденные параметры
		tiffParams->PhotoMetricEnabled = true;
		if ( TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &tiffParams->PhotoMetric) != 1 ) {
			// текущее значение не определено
			tiffParams->PhotoMetricEnabled = false;
		}
		if ( TIFFGetField(tif, TIFFTAG_XRESOLUTION, &tiffParams->ResolutionX) != 1 ) {
			tiffParams->ResolutionX = -1;
		}
		if ( TIFFGetField(tif, TIFFTAG_YRESOLUTION, &tiffParams->ResolutionY) != 1 ) {
			tiffParams->ResolutionY = -1;
		}
		tiffParams->ResolutionUnitEnabled = true;
		if ( TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &tiffParams->ResolutionUnit) != 1 ) {
			tiffParams->ResolutionUnitEnabled = false;
		}

		printf("\nInput file name: %s\n", fileName);
		if ( tiffParams->SamplesPerPixel != 1 && tiffParams->BitsPerSample != (sizeof(T) * 8) && config != PLANARCONFIG_CONTIG ) {
			fprintf(stderr, "Unsupported image parameters (just 1 sample per pixel, %d bits per sample, single image plane)\n", sizeof(T) * 8);
			return NULL;
		}

		img = new T*[tiffParams->Height];
		for (unsigned i = 0; i < tiffParams->Height; i++) {
			img[i] = new T[tiffParams->Width];

			TIFFReadScanline(tif, img[i], i);
			ShiftArrayRight<T>( img[i], bitSize, tiffParams->Width );
		}
		
		TIFFClose(tif);
	} else {
		fprintf(stderr, "Cannot read input file: %s\n", fileName);
	}
	return img;
}

template<typename T>
int SaveTiffFile( const char *fileName, T **img, const unsigned bitSize, tiffParameters_t *tiffParams ) {
	if ( tiffParams == NULL ) {
		fprintf(stderr, "tiff parameters was not set\n");
		return -1;
	}

	TIFF* tif = TIFFOpen(fileName, "w+");
	if ( tif ) {
		printf("Output file name: %s\n", fileName);

		TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, tiffParams->Width);
		TIFFSetField(tif, TIFFTAG_IMAGELENGTH, tiffParams->Height);
		TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, tiffParams->BitsPerSample);
		if ( tiffParams->PhotoMetricEnabled ) {
			TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, tiffParams->PhotoMetric);
		}
		TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, tiffParams->SamplesPerPixel);
		if ( tiffParams->ResolutionX != -1 ) {
			TIFFSetField(tif, TIFFTAG_XRESOLUTION, tiffParams->ResolutionX);
		}
		if ( tiffParams->ResolutionY != -1 ) {
			TIFFSetField(tif, TIFFTAG_YRESOLUTION, tiffParams->ResolutionY);
		}
		TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		if ( tiffParams->ResolutionUnitEnabled ) {
			TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, tiffParams->ResolutionUnit);
		}
		
		for (unsigned i = 0; i < tiffParams->Height; i++){
			ShiftArrayLeft<T>( img[i], bitSize, tiffParams->Width );
			TIFFWriteScanline(tif, img[i], i);
		}
		
		TIFFClose(tif);
	} else {
		fprintf(stderr, "Cannot create output file: %s\n", fileName);
		return -1;
	}
	return 0;
}

template short **OpenTiffFile( const char *fileName, const unsigned bitSize, tiffParameters_t *tiffParams );
template int SaveTiffFile( const char *fileName, short **img, const unsigned bitSize,  tiffParameters_t *tiffParams );
