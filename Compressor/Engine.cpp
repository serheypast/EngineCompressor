#include "Engine.hpp"

#include <math.h>

#include "Encoder.hpp"
#include "Decoder.hpp"
#include "Wavelet.hpp"

#include "Defines.hpp"
#include "Timer.hpp"

#include "TiffProcessor.hpp"
#include "FrameDecorrelate.hpp"

#include "Metrics.hpp"

template<typename T>
Engine<T>::Engine(unsigned bearingFrameIdx, unsigned bearingFrameCount, int bitSize, char *filePath, TransformSettings *settings) {
	if ( settings == NULL ) {
		fprintf(stderr, "Settings parameters was not set\n");
		throw;
	}

	filePath_ = filePath;
	bearingFrameCount_ = bearingFrameCount;
	frameBitsPerSample_ = bitSize;

	char fileName[100];
	tiffParameters_t tiffParams;
	
	sprintf(fileName, "%s/%03d.tif", filePath_, bearingFrameIdx);
	bearingFrame_ = OpenTiffFile<T>( fileName, frameBitsPerSample_, &tiffParams );
	if ( bearingFrame_ == NULL || ( tiffParams.Width % settings->blockSize ) != 0 || ( tiffParams.Height % settings->blockSize ) != 0 ) {
		fprintf(stderr, "Incorrect bearing image parameters\n");
		throw;
	}

	frameWidth_ = tiffParams.Width;
	frameHeight_ = tiffParams.Height;

	settings_ = settings;

	rowsCount_ = int( double(frameHeight_) / double(settings_->blockSize) );
	columnsCount_ = int( double(frameWidth_) / double(settings_->blockSize) );

	// maxSectionsCount должно быть по максимальному числу бит в кодируемом потоке
	// после вейвлет преобразования
	settings_->maxSectionsCount = sizeof(int) * 8;
	//
	settings_->maxStreamSize = settings_->blockSize * settings_->blockSize * sizeof(T) * 5 /* основной поток */;
	settings_->maxListSize = 2641440;

	outImageStream_ = new unsigned char **[rowsCount_];
	outStreamSize_  = new int* [rowsCount_];
	for (unsigned i = 0; i < rowsCount_; i++) {
		outStreamSize_[i] = new int [columnsCount_];
		outImageStream_[i] = new unsigned char *[columnsCount_];

		for (unsigned j = 0; j < columnsCount_; j++) {
			outImageStream_[i][j] = new unsigned char [settings_->maxStreamSize];
		}
	}

	restoredImage_ = new T*[frameHeight_];
	for (int i = 0; i < frameHeight_; i++) {
		restoredImage_[i] = new T[frameWidth_];
	}
	
	processingBlock_ = new int*[settings_->blockSize];
	bearingBlock_ = new int*[settings_->blockSize];
	for (int i = 0; i < settings_->blockSize; i++) {
		processingBlock_[i] = new int[settings_->blockSize];
		bearingBlock_[i] = new int[settings_->blockSize];
	}

	printf("Image settings:\n");
	printf("\tImage size: %dx%d (%d bits per sample)\n", frameWidth_, frameHeight_, frameBitsPerSample_);
	printf("\tImage type: grayscale\n");

	printf("\nCompression settings:\n");
	printf("\tBlock size to compress: %d (%dx%d blocks)\n", settings_->blockSize, rowsCount_, columnsCount_);
	printf("\tDecomposition level: %d (max value: %.3f)\n", settings_->decLevel, ( log( float(settings->blockSize) ) / log(2.0) ) - 1);
	printf("\tBearing frame: each %d frame\n", bearingFrameCount_);
	printf("\n\tWavelet: %s\n", AnalyzeWavelet( settings_->lumaWavelet, settings_->lumaMatrixType ));
	printf("\tBit level count: %d\n", settings_->lumaBitLevelCount);
	printf("\tPixel bit size: %d\n", bitSize);

	printf("\nLog settings:\n");
	printf("\tLog mode enabled: %s\n", settings_->Log ? "true" : "false");
	printf("\tRefinement section: %d\n", settings_->RefSection);
	printf("\tSorting section: %d\n", settings_->SortSection);
	printf("\tValidate bit index: %d\n", settings_->BitIndex);
}

template<typename T>
Engine<T>::~Engine() {
	for (unsigned i = 0; i < rowsCount_; i++) {
		for (unsigned j = 0; j < columnsCount_; j++) {
			delete outImageStream_[i][j];
		}

		delete outStreamSize_[i];
		delete outImageStream_[i];
	}
	delete outImageStream_;
	delete outStreamSize_;

	for (int i = 0; i < settings_->blockSize; i++) {
		delete processingBlock_[i];
		delete bearingBlock_[i];
	}
	delete processingBlock_;
	delete bearingBlock_;

	for (int i = 0; i < frameHeight_; i++) {
		delete bearingFrame_[i];
		delete restoredImage_[i];
	}
	delete bearingFrame_;
	delete restoredImage_;
}

template<typename T>
T **Engine<T>::ProcessFrame(T **img) {
	CompressionResult result;
	result.compressedFileSize = 0;
	result.originalBitstreamSize = frameWidth_ * frameHeight_ * frameBitsPerSample_;
	result.originalFileSize = result.originalBitstreamSize / 8;

	result.waveletEncodeTime = 0.0f;
	result.encodeTime = 0.0f;

	result.waveletDecodeTime = 0.0f;
	result.decodeTime = 0.0f;

	Timer waveletEncodeTimer;
	Timer waveletDecodeTimer;

	unsigned *sortStreamTotalSize = NULL;
	unsigned *refStreamTotalSize = NULL;
	if ( settings_->Log ) {
		sortStreamTotalSize = new unsigned[settings_->maxSectionsCount];
		refStreamTotalSize = new unsigned[settings_->maxSectionsCount];
		for (int i = 0; i < settings_->maxSectionsCount; i++) {
			sortStreamTotalSize[i] = 0;
			refStreamTotalSize[i] = 0;
		}
	}

	// encode
	settings_->transformDirection = TransformDirection::FORWARD;
	for (unsigned rowIdx = 0; rowIdx < rowsCount_; rowIdx++) {
		for (unsigned colIdx = 0; colIdx < columnsCount_; colIdx++) {
			if ( settings_->Log ) {
				printf("\nBlock processing: row = %d column = %d\n", rowIdx, colIdx);
			}

			double encodeTime = 0.0f;
			imageToMatrix(img, processingBlock_, colIdx * settings_->blockSize, rowIdx * settings_->blockSize );

			waveletEncodeTimer.Start();
			waveletTransform (processingBlock_, settings_);
			result.waveletEncodeTime += waveletEncodeTimer.GetTimeInMs();

			if ( settings_->compressionMode == WAVELET_SUB && !bearingProcess_ ) {
				imageToMatrix(bearingFrame_, bearingBlock_, colIdx * settings_->blockSize, rowIdx * settings_->blockSize );
				waveletTransform (bearingBlock_, settings_);
				SubtractionForward( bearingBlock_, processingBlock_, settings_->blockSize, settings_->blockSize, 0 );
			}
			
			const bool sortSectionTest = settings_->SortingSectionTest && ( ( rowIdx * columnsCount_ + colIdx ) == settings_->BlockIndex );
			const bool refSectionTest = settings_->RefinementSectionTest && ( ( rowIdx * columnsCount_ + colIdx ) == settings_->BlockIndex );

			outStreamSize_[rowIdx][colIdx] = compressTx(
												processingBlock_,
												outImageStream_[rowIdx][colIdx],
												&encodeTime,
												sortStreamTotalSize,
												refStreamTotalSize,

												sortSectionTest,
												refSectionTest,
												settings_
											 );
			if ( outStreamSize_[rowIdx][colIdx] < 0 ) {
				return NULL;
			}
			result.encodeTime += encodeTime;

			result.compressedFileSize += outStreamSize_[rowIdx][colIdx];
		}
	}

	if (settings_->Log) {
		printf("\n\nTotal size parameters:\n");
		for (int i = 0; i < settings_->maxSectionsCount; i++) {
			printf("Sorting section [%d] size: %d bytes\n", i, sortStreamTotalSize[i]);
			printf("Refinement section [%d] size: %d bytes\n", i, refStreamTotalSize[i]);
		}
	}

	// decode
	settings_->transformDirection = TransformDirection::BACKWARD;
	for (unsigned rowIdx = 0; rowIdx < rowsCount_; rowIdx++) {
		for (unsigned colIdx = 0; colIdx < columnsCount_; colIdx++) {
			double decodeTime = 0.0f;
		
			if ( decompressRx( processingBlock_, outImageStream_[rowIdx][colIdx], &decodeTime, settings_ ) < 0 ) {
				return NULL;
			}
			result.decodeTime += decodeTime;

			if ( settings_->compressionMode == WAVELET_SUB && !bearingProcess_ ) {
				imageToMatrix(bearingFrame_, bearingBlock_, colIdx * settings_->blockSize, rowIdx * settings_->blockSize );
				settings_->transformDirection = TransformDirection::FORWARD;
				waveletTransform (bearingBlock_, settings_);
				settings_->transformDirection = TransformDirection::BACKWARD;
				SubtractionBackward( bearingBlock_, processingBlock_, settings_->blockSize, settings_->blockSize, 0 );
			}

			waveletDecodeTimer.Start();
			waveletTransform (processingBlock_, settings_);
			result.waveletDecodeTime += waveletDecodeTimer.GetTimeInMs();

			const bool sortSectionTest = settings_->SortingSectionTest && ( ( rowIdx * columnsCount_ + colIdx ) == settings_->BlockIndex );
			const bool refSectionTest = settings_->RefinementSectionTest && ( ( rowIdx * columnsCount_ + colIdx ) == settings_->BlockIndex );
			if ( sortSectionTest || refSectionTest ) {
				int **etalon = new int*[settings_->blockSize];
				for (int i = 0; i < settings_->blockSize; i++) {
					etalon[i] = new int[settings_->blockSize];
				}
				imageToMatrix(img, etalon, colIdx * settings_->blockSize, rowIdx * settings_->blockSize );

				printf("Noise test: PSNR = %.3f dB\n", PSNR( etalon, processingBlock_, settings_->blockSize, settings_->blockSize, ( 1 << frameBitsPerSample_ ) - 1 ));

				for (int i = 0; i < settings_->blockSize; i++) {
					delete etalon[i];
				}
				delete etalon;
			}

			matrixToImage( processingBlock_, restoredImage_, colIdx * settings_->blockSize, rowIdx * settings_->blockSize );
		}
	}

	printf("\nOriginal file size: %.3f MB (%d bits)\n", double( result.originalFileSize ) / double( 1024.0 * 1024.0 ), result.originalBitstreamSize);
	printf("Compressed file size: %.3f MB (CR = %.3f)\n\n", double( result.compressedFileSize ) / double( 1024.0 * 1024.0 ),
													   double( result.originalFileSize ) / double( result.compressedFileSize ) );
	printf("Encode time: %.3f ms (%.3f + %.3f)\n", result.waveletEncodeTime + result.encodeTime, result.waveletEncodeTime, result.encodeTime);
	printf("Decode time: %.3f ms (%.3f + %.3f)\n", result.waveletDecodeTime + result.decodeTime, result.waveletDecodeTime, result.decodeTime);
	
	if ( ( double( result.originalFileSize ) / double( result.compressedFileSize ) ) < 1.0 ) {
		fprintf( stderr, "Compressed file size is more than original\n");
	}

	if ( bearingProcess_ ) {
		printf("Bearing frame\n");
		bearingFramesCount_++;

		waveletEncodingBearingTotalTime_ += result.waveletEncodeTime;
		waveletDecodingBearingTotalTime_ += result.waveletDecodeTime;
		
		encodeBearingTotalTime_ += result.encodeTime;
		decodeBearingTotalTime_ += result.decodeTime;
		
		crBearingTotal_ += double( result.compressedFileSize ) / double( result.originalFileSize );
		compressedSizeBearingTotal_ += double(result.compressedFileSize) / 1024.0;
	} else {
		averageFrameCount_++;

		waveletEncodingTotalTime_ += result.waveletEncodeTime;
		waveletDecodingTotalTime_ += result.waveletDecodeTime;
		
		encodeTotalTime_ += result.encodeTime;
		decodeTotalTime_ += result.decodeTime;
		
		crTotal_ += double( result.compressedFileSize ) / double( result.originalFileSize );
		compressedSizeTotal_ += double(result.compressedFileSize) / 1024.0;
	}

	if ( refStreamTotalSize != NULL )
		delete refStreamTotalSize;
	if ( sortStreamTotalSize != NULL )
		delete sortStreamTotalSize;
	
	return restoredImage_;
}

template <typename T>
int Engine<T>::ProcessFrames(unsigned frameIdxStart, unsigned frameIdxStop, char *outputPath, bool validationMode) {
	tiffParameters_t tiffParams;
	char fileName[100];

	bearingFramesCount_ = 0;
	averageFrameCount_  = 0;

	encodeBearingTotalTime_ = 0.f;
	decodeBearingTotalTime_ = 0.f;

	waveletEncodingBearingTotalTime_ = 0.f;
	waveletDecodingBearingTotalTime_ = 0.f;
	
	waveletEncodingTotalTime_ = 0.f;
	waveletDecodingTotalTime_ = 0.f;
		
	encodeTotalTime_ = 0.f;
	decodeTotalTime_ = 0.f;
		
	crTotal_ = 0.f;
	crBearingTotal_ = 0.f;

	compressedSizeBearingTotal_ = 0.f;
	compressedSizeTotal_ = 0.f;

	for (unsigned i = frameIdxStart; i < frameIdxStop; i++ ) {
		sprintf(fileName, "%s/%03d.tif", filePath_, i);
		T **currentImage = OpenTiffFile<T>( fileName, frameBitsPerSample_, &tiffParams );
		if ( currentImage == NULL || tiffParams.Width != frameWidth_ || tiffParams.Height != frameHeight_ ) {
			fprintf(stderr, "Incorrect image parameters (%s)\n", fileName);
			return -1;
		}
		
		bearingProcess_ = ( i == frameIdxStart ) || ( ( ( i - 1 ) % bearingFrameCount_ ) == 0 );

		if ( ( ( i - 1 ) % bearingFrameCount_ ) == 0 ) {
			sprintf(fileName, "%s/%03d.tif", filePath_, i);
			bearingFrame_ = OpenTiffFile<T>( fileName, frameBitsPerSample_, &tiffParams );
			if ( tiffParams.Width != frameWidth_ || tiffParams.Height != frameHeight_ ) {
				fprintf(stderr, "Incorrect bearing frame size\n");
				return -1;
			}
		}
		
		if ( !bearingProcess_ ) {
			// осуществляет вычитание
			//		из текущего кадра опорный
			if ( settings_->compressionMode == IMAGE_SUB ) {
				SubtractionForward( bearingFrame_, currentImage, tiffParams.Width, tiffParams.Height, 0 );
			}
		}

		T **resultImg = ProcessFrame( currentImage );
		if ( resultImg == NULL ) {
			return -1;
		}

		sprintf(fileName, "%s/%03d.block.%d.dec.%d.dat", outputPath, i, settings_->blockSize, settings_->decLevel);
		StoreToFile( fileName );

		if ( !bearingProcess_ ) {
			// осуществляет восстановление
			//		текущего кадра из опорного
			if ( settings_->compressionMode == IMAGE_SUB ) {
				// восстанавливаем сжатое/восстановленное изображение
				SubtractionBackward( bearingFrame_, resultImg,    tiffParams.Width, tiffParams.Height, 0 );
				// восстанавливаем оригинальное изображение
				SubtractionBackward( bearingFrame_, currentImage, tiffParams.Width, tiffParams.Height, 0 );
			}
		}

		if ( settings_->PSNR ) {
			printf("PSNR = %.3f dB\n", PSNR( currentImage, resultImg, frameWidth_, frameHeight_, ( 1 << frameBitsPerSample_ ) - 1 ));
		}

		if ( validationMode ) {
			bool failed = false;
			for (int i = 0; i < frameHeight_ && !failed; i++) {
				for (int j = 0; j < frameWidth_ && !failed; j++) {
					if ( currentImage[i][j] != resultImg[i][j] ) {
						fprintf(stderr, "\t\t%s: validation failed in pos (h x w): %d x %d\n", fileName, i, j);
						failed = true;
					}
				}
			}

			if ( !failed ) {
				printf("\t\t%s: validation success\n", fileName);
			}
		} else {
			sprintf(fileName, "%s/%03d.block.%d.dec.%d.restored.tif", outputPath, i, settings_->blockSize, settings_->decLevel);
			if ( SaveTiffFile( fileName, resultImg, frameBitsPerSample_, &tiffParams ) < 0 ) {
				return -1;
			}
		}
		
		for (int h = 0; h < frameHeight_; h++) {
			delete currentImage[h];
		}
		delete currentImage;
	}

	double oneImageSize = ( frameWidth_ * frameHeight_ * frameBitsPerSample_ ) / ( 8.0 * 1024.0 );
	double originalBearingSize = oneImageSize * bearingFramesCount_;
	double originalSize = oneImageSize * averageFrameCount_;

	printf("\nCommon results:\n");
	printf("\tOriginal frame size: %.3f kB\n", oneImageSize);
	
	printf("Bearing results:\n");
	printf("\tTotal bearing frame size (original stream): %.3f kB * %d frames = %.3f kB\n", oneImageSize, bearingFramesCount_, originalBearingSize);
	printf("\tTotal bearing frame size (compressed stream): %.3f kB\n", compressedSizeBearingTotal_);
	printf("\tCompression ratio: %.3f\n", originalBearingSize / compressedSizeBearingTotal_);
	printf("\tEncoding processing time: %.3f + %.3f = %.3f ms\n",
		encodeBearingTotalTime_ / bearingFramesCount_, waveletEncodingBearingTotalTime_ / bearingFramesCount_,
		( encodeBearingTotalTime_ + waveletEncodingBearingTotalTime_ ) / bearingFramesCount_ );
	printf("\tDecoding processing time: %.3f + %.3f = %.3f ms\n",
		decodeBearingTotalTime_ / bearingFramesCount_, waveletDecodingBearingTotalTime_ / bearingFramesCount_,
		( decodeBearingTotalTime_ + waveletDecodingBearingTotalTime_ ) / bearingFramesCount_ );

	double totalFrames = averageFrameCount_ + bearingFramesCount_;
	if ( totalFrames != 0 ) {
		printf("Average results with other frames:\n");
		printf("\tTotal frame size (original stream): %.3f kB * %d frames = %.3f kB\n", oneImageSize, averageFrameCount_, originalSize);
		printf("\tTotal frame size (compressed stream): %.3f kB\n", compressedSizeTotal_);
		printf("\tCompression ratio: %.3f\n", originalSize / compressedSizeTotal_ );
		printf("\tEncoding processing time: %.3f + %.3f = %.3f ms\n",
			encodeTotalTime_ / averageFrameCount_, waveletEncodingTotalTime_ / averageFrameCount_,
			( encodeTotalTime_ + waveletEncodingTotalTime_ ) / averageFrameCount_);
		printf("\tDecoding processing time: %.3f + %.3f = %.3f ms\n",
			decodeTotalTime_ / averageFrameCount_, waveletDecodingTotalTime_ / averageFrameCount_,
			( decodeTotalTime_ + waveletDecodingTotalTime_ ) / averageFrameCount_);

		crTotal_ = ( originalBearingSize + originalSize ) / ( compressedSizeBearingTotal_ + compressedSizeTotal_ );
		encodeTotalTime_ = ( encodeTotalTime_ + encodeBearingTotalTime_ ) / totalFrames;
		waveletEncodingTotalTime_ = ( waveletEncodingTotalTime_ + waveletEncodingBearingTotalTime_ ) / totalFrames;
		decodeTotalTime_ = ( decodeTotalTime_ + decodeBearingTotalTime_ ) / totalFrames;
		waveletDecodingTotalTime_ = ( waveletDecodingTotalTime_ + waveletDecodingBearingTotalTime_ ) / totalFrames;

		const double megabitStreamSize = ( frameWidth_ * frameHeight_ * frameBitsPerSample_ ) / ( 1024.0 * 1024.0 );
		const double encodeProcessTimeSeconds = ( encodeTotalTime_ + waveletEncodingTotalTime_ ) / 1000.0;
	
		printf("Average results with bearing frame:\n");
		printf("\tTotal frame size (original stream): %.3f + %.3f = %.3f kB\n", originalBearingSize, originalSize, originalBearingSize + originalSize);
		printf("\tTotal frame size (compressed stream): %.3f + %.3f = %.3f kB\n", compressedSizeBearingTotal_, compressedSizeTotal_, compressedSizeBearingTotal_ + compressedSizeTotal_);
		printf("\tCompression ratio: %.3f\n", crTotal_);
		printf("\tEncoding processing time: %.3f + %.3f = %.3f ( throughput = %.3f Mbit/s )\n", encodeTotalTime_, waveletEncodingTotalTime_, encodeTotalTime_ + waveletEncodingTotalTime_, megabitStreamSize / encodeProcessTimeSeconds);
		printf("\tDecoding processing time: %.3f + %.3f = %.3f\n", decodeTotalTime_, waveletDecodingTotalTime_, decodeTotalTime_ + waveletDecodingTotalTime_);
	}
	
	return 0;
}

template <typename T>
void Engine<T>::imageToMatrix(T **src, int **dst, int x, int y) {
	for (int i = 0; i < settings_->blockSize /* height */; i++) {
		for (int j = 0; j < settings_->blockSize /* width */; j++) {
			if ( ( ( j + x ) >= frameWidth_ ) || ( i + y ) >= frameHeight_ ) {
				dst[i][j] = 0;
			} else {
				dst[i][j] = src[j + x][i + y];
			}
		}
	}
}

template <typename T>
void Engine<T>::matrixToImage(int **src, T **dst, int x, int y) {
	for (int i = 0; i < settings_->blockSize /* height */; i++) {
		for (int j = 0; j < settings_->blockSize /* width */; j++) {
			if ( ( j + x ) < frameWidth_ || ( i + y ) < frameHeight_ ) {
				dst[j + x][i + y] = src[i][j];
			}
		}
	}
}

// store compressed image to file
template<typename T>
int Engine<T>::StoreToFile(const char *outputFile) {
	FILE *fp = fopen( outputFile, "w+b" );
	if ( !fp ) {
		fprintf(stderr, "Cannot create output file\n");
		return -1;
	}

	// save image settings
	fwrite( &rowsCount_,       sizeof(rowsCount_      ), 1, fp );
	fwrite( &columnsCount_,    sizeof(columnsCount_   ), 1, fp );
	fwrite( &frameBitsPerSample_,   sizeof(frameBitsPerSample_  ), 1, fp );
	
	// save compression settings
	fwrite( &(settings_->blockSize     ), sizeof(int ), 1, fp );
	fwrite( &(settings_->decLevel      ), sizeof(int ), 1, fp );
	
	fwrite( &(settings_->lumaWavelet      ), sizeof(Wavelet), 1, fp );
	fwrite( &(settings_->lumaBitLevelCount), sizeof(int    ), 1, fp );
	fwrite( &(settings_->lumaMatrixType   ), sizeof(int    ), 1, fp );

	// save compressed stream
	for (unsigned i = 0; i < rowsCount_; i++) {
		for (unsigned j = 0; j < columnsCount_; j++) {
			fwrite( &(outStreamSize_[i][j]), sizeof(unsigned), 1, fp );
			fwrite( outImageStream_[i][j], sizeof(unsigned char), outStreamSize_[i][j], fp );
		}
	}
	fclose( fp );
	return 0;
}

// read compressed image to file
template<typename T>
int Engine<T>::LoadFromFile(const char *inputFile, unsigned bitSize) {
	FILE *fp = fopen( inputFile, "rb" );
	if ( !fp ) {
		fprintf(stderr, "Cannot create input file\n");
		return -1;
	}

	// load image settings
	fread( &rowsCount_,       sizeof(rowsCount_      ), 1, fp );
	fread( &columnsCount_,    sizeof(columnsCount_   ), 1, fp );
	fread( &frameBitsPerSample_,   sizeof(frameBitsPerSample_  ), 1, fp );
	
	// load compression settings
	fread( &(settings_->blockSize     ), sizeof(int ), 1, fp );
	fread( &(settings_->decLevel      ), sizeof(int ), 1, fp );
	
	fread( &(settings_->lumaWavelet      ), sizeof(Wavelet), 1, fp );
	fread( &(settings_->lumaBitLevelCount), sizeof(int    ), 1, fp );
	fread( &(settings_->lumaMatrixType   ), sizeof(int    ), 1, fp );

	for (unsigned i = 0; i < rowsCount_; i++) {
		for (unsigned j = 0; j < columnsCount_; j++) {
			fread( &(outStreamSize_[i][j]), sizeof(unsigned), 1, fp );
			fread( outImageStream_[i][j], sizeof(unsigned char), outStreamSize_[i][j], fp );
		}
	}
	fclose( fp );
	return 0;
}

template class Engine<short>;
