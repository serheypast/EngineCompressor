#ifndef __DECODER__
#define __DECODER__

#include "Defines.hpp"

int decompressRx (int **r_channel, unsigned char *inStream, double *decodeTime, TransformSettings *settings);

#endif // __DECODER__