#ifndef __TIFF_PROCESSOR__
#define __TIFF_PROCESSOR__

#include <stdlib.h>

typedef struct {
	unsigned Width;
	unsigned Height;
	unsigned short SamplesPerPixel;
	unsigned short BitsPerSample;

	unsigned short PhotoMetric;
	bool PhotoMetricEnabled;
	float ResolutionX;
	float ResolutionY;
	unsigned short ResolutionUnit;
	bool ResolutionUnitEnabled;
} tiffParameters_t;

template<typename T> T **OpenTiffFile( const char *fileName, const unsigned bitSize, tiffParameters_t *tiffParams );
template<typename T> int SaveTiffFile( const char *fileName, T **img, const unsigned bitSize,  tiffParameters_t *tiffParams );

#endif // __TIFF_PROCESSOR__
