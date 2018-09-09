#ifndef __DEFINES__
#define __DEFINES__

enum Wavelet { HAAR, CDF_53, CDF_97 };
enum MatrixType { INTEGER, REAL };
enum TransformDirection { FORWARD, BACKWARD };
enum MODE { NONE, IMAGE_SUB, WAVELET_SUB, IMAGE_WAVELET_SUB, IMAGE_SUB_WITH_OFFSET };

struct TransformSettings {
	int blockSize;
	TransformDirection transformDirection;
	int decLevel;
	MODE compressionMode;

	Wavelet lumaWavelet;
	int lumaBitLevelCount;
	MatrixType lumaMatrixType;

	unsigned long maxStreamSize;
	long maxSectionsCount;
	long maxListSize;

	bool Log;
	bool PSNR;

	unsigned BlockIndex;
	bool RefinementSectionTest;
	unsigned RefSection;
	bool SortingSectionTest;
	unsigned SortSection;
	unsigned BitIndex;
};

struct ImageSettings {
	unsigned Width;
	unsigned Height;

	unsigned short SamplesPerPixel;
	unsigned short BitsPerPixel;
};

typedef struct {
	unsigned short PhotoMetric;
	bool PhotoMetricEnabled;
	float ResolutionX;
	float ResolutionY;
	unsigned short ResolutionUnit;
	bool ResolutionUnitEnabled;

	ImageSettings Image;
} Tiff_t;

struct CompressionResult {
	int compressedFileSize;
	
	double waveletEncodeTime;
	double waveletDecodeTime;
	
	double encodeTime;
	double decodeTime;

	unsigned originalFileSize;
	unsigned originalBitstreamSize;
};

#endif // __DEFINES__