#ifndef __ENCODER__
#define __ENCODER__

#include "Defines.hpp"

int compressTx (
	int **r_channel,
	unsigned char *outStream,
	double *encodeTime,
	
	unsigned *sortStreamTotalSize,
	unsigned *refStreamTotalSize,
	
	bool sortingNoiseTest,
	bool refinementNoiseTest,
	TransformSettings *settings
);

#endif // __ENCODER__