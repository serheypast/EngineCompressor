#ifndef __FRAME_DECORRELATE__
#define __FRAME_DECORRELATE__

template<typename T>
int SubtractionForward( T **bearingImage, T **currentImage, const int width, const int height, const int offset );

template<typename T>
int SubtractionBackward( T **bearingImage, T **currentImage, const int width, const int height, const int offset );

#endif // __FRAME_DECORRELATE__
