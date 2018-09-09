#ifndef __ENGINE_H__
#define __ENGINE_H__

#include "Defines.hpp"

template<typename T>
class Engine
{
private:
	char *filePath_;

	unsigned rowsCount_;
	unsigned columnsCount_;

	int frameWidth_;
	int frameHeight_;
	int frameBitsPerSample_;

	unsigned bearingFramesCount_;
	unsigned bearingFrameCount_;

	unsigned averageFrameCount_;
	
	TransformSettings *settings_;

	T **bearingFrame_;
	T **restoredImage_;
	int **bearingBlock_;
	int **processingBlock_;

	bool bearingProcess_;
	double waveletEncodingBearingTotalTime_;
	double waveletDecodingBearingTotalTime_;

	double encodeBearingTotalTime_;
	double decodeBearingTotalTime_;

	double crBearingTotal_;
	double compressedSizeBearingTotal_;

	double waveletEncodingTotalTime_;
	double waveletDecodingTotalTime_;

	double encodeTotalTime_;
	double decodeTotalTime_;

	double crTotal_;
	double compressedSizeTotal_;

	unsigned char ***outImageStream_;
	int **outStreamSize_;

	void imageToMatrix(T **src, int **dst, int x, int y);
	void matrixToImage(int **src, T **dst, int x, int y);

public:
	T **ProcessFrame(T **img);
	int ProcessFrames(unsigned frameIdxStart, unsigned frameIdxStop, char *outputPath, bool validationMode);
	int StoreToFile(const char* outputFile);
	int LoadFromFile(const char*  inputFile, unsigned bitSize);

	Engine(unsigned bearingFrameIdx, unsigned bearingFrameCount, int bitSize, char *filePath, TransformSettings *settings);
	~Engine();
};

#endif // __ENGINE_H__
