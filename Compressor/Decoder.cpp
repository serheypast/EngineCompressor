#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cstring>

#include "Compressor.hpp"
#include "Decoder.hpp"

#include "Timer.hpp"

struct CompressedStream lumaStreamDecoder;

struct ByteSection *sortSectDecoderPtr; // pointer to current sorting section

unsigned char *sortByteDecoderPtr; // current position in sorting stream
unsigned char sortByteDecoderMask;

//############## Lists #################
struct Node* *lisCurrentDecoderBegin;
struct Node* *lisNextDecoder;
struct Node* *lspCurrentDecoder;

signed char iPositionDecoder; // position of i node
unsigned char currentBitLevelDecoder;

//############## Subband arrays #################
struct Node** hlDecoder;
struct Node** lhDecoder;
struct Node** hhDecoder;
struct Node* llDecoder;

struct Node** codeStackBufDecoder;

unsigned int levelMaskDecoder;

//###############################################

void codeS_RX (Node* cnode) {
	register int i;

	struct Node** codeStack = codeStackBufDecoder;
	*codeStack++ = cnode;

	while(codeStackBufDecoder < codeStack) {
		cnode = *--codeStack;
		for (i = 0; i < 4; i++) {
			bool currentBit = ( *sortByteDecoderPtr ) & sortByteDecoderMask;
			if( !( sortByteDecoderMask <<= 1 ) ) {
				++sortByteDecoderPtr;
				sortByteDecoderMask = 0x01;
			};

			if (currentBit) {
				if(cnode->child[i]->size == 1) {
					cnode->child[i]->val |= levelMaskDecoder;
					cnode->child[i]->sign = ( *sortByteDecoderPtr ) & sortByteDecoderMask;
					if( !( sortByteDecoderMask <<= 1 ) ) {
						++sortByteDecoderPtr;
						sortByteDecoderMask = 0x01;
					};
					*lspCurrentDecoder++ = cnode->child[i];
				} else {
					*codeStack++ = cnode->child[i];
				}
			} else {
				*lisNextDecoder++ = cnode->child[i];
			}
		}
	}
}

void processS_RX (Node* node) {
	bool currentBit = ( *sortByteDecoderPtr ) & sortByteDecoderMask;
	
	if( !( sortByteDecoderMask <<= 1 ) ) {
		++sortByteDecoderPtr;
		sortByteDecoderMask = 0x01;
	};

	if (currentBit) {
		if (node->size == 1) {
			node->val |= levelMaskDecoder;
			node->sign = ( *sortByteDecoderPtr ) & sortByteDecoderMask;
			
			if( !( sortByteDecoderMask <<= 1 ) ) {
				++sortByteDecoderPtr;
				sortByteDecoderMask = 0x01;
			};

			*lspCurrentDecoder++ = node;
		} else {
			codeS_RX(node);
		}
	} else {
		*lisNextDecoder++ = node;
	}
}

void processI_RX(void) {
	if ( iPositionDecoder < 0 )
		return;
	bool currentBit = ( *sortByteDecoderPtr ) & sortByteDecoderMask;
	if( !( sortByteDecoderMask <<= 1 ) ) {
		++sortByteDecoderPtr;
		sortByteDecoderMask = 0x01;
	};

	if (currentBit) {
		processS_RX( lhDecoder[iPositionDecoder] );
		processS_RX( hlDecoder[iPositionDecoder] );
		processS_RX( hhDecoder[iPositionDecoder] );
		iPositionDecoder --;
		processI_RX();
	}
}

int channeldecompressRx(int **data, struct CompressedStream *stream, TransformSettings *settings) {
	Node* testNode;

	// creating tree structure
	unsigned treeDepth = unsigned( log( float( settings->blockSize ) ) / log( 2.0 ) );
	struct Node ***tree = new Node**[treeDepth];

	for (unsigned treeIdx = 0; treeIdx < treeDepth; treeIdx++) {
		tree[treeIdx] = new Node*[settings->blockSize >> treeIdx]; // Tree level for nodes with size of 1x1
	
		for (int i = 0; i < settings->blockSize >> treeIdx; i++) {
			tree[treeIdx][i] = new Node[settings->blockSize >> treeIdx];
		}
	}

	codeStackBufDecoder = new Node*[settings->maxListSize];

	// Pixel level
	for (int i = 0; i < settings->blockSize; i++) {
		for (int j = 0; j < settings->blockSize; j++) {
			tree[0][i][j].val = 0;
			tree[0][i][j].sign = false;
			tree[0][i][j].size = 1;
			tree[0][i][j].child[0] = NULL;
			tree[0][i][j].child[1] = NULL;
			tree[0][i][j].child[2] = NULL;
			tree[0][i][j].child[3] = NULL;
		}
	}
	
	// Block levels
	for (unsigned k = 1; k < treeDepth; k++) {
		int currentSize = settings->blockSize >> k;
		for (int i = 0; i < currentSize; i++) {
			for (int j = 0; j < currentSize; j++) {
				tree[k][i][j].size = 0x01 << k;
				tree[k][i][j].child[0] = &tree[k - 1][  i << 1      ][  j << 1      ];
				tree[k][i][j].child[1] = &tree[k - 1][( i << 1 ) + 1][  j << 1      ];
				tree[k][i][j].child[2] = &tree[k - 1][  i << 1      ][( j << 1 ) + 1];
				tree[k][i][j].child[3] = &tree[k - 1][( i << 1 ) + 1][( j << 1 ) + 1];
				tree[k][i][j].sign = NULL;
				tree[k][i][j].val = NULL;
			}
		}
	}
	
	// Initialization of byte stream
	sortByteDecoderPtr = stream->sortStream;
	unsigned char *refBytePtr = stream->refStream;
	
	sortByteDecoderMask = 0x01;
	unsigned char refByteMask = 0x01;

	// Initialization of lists
	struct Node **lisBuffer1 = new Node*[settings->maxListSize]; 
	struct Node **lisBuffer2 = new Node*[settings->maxListSize];
	struct Node **lspBuffer  = new Node*[settings->maxListSize];

	lisCurrentDecoderBegin = lisBuffer1;
	struct Node** lisCurrent = lisCurrentDecoderBegin;
	
	struct Node** lisNextBegin = lisBuffer2;
	lisNextDecoder = lisNextBegin;
	
	struct Node** lspBegin = lspBuffer;
	struct Node** lspThis = NULL;
	lspCurrentDecoder = lspBegin;
	
	// Initialization of subbands
	
	llDecoder = &tree[treeDepth - settings->decLevel][0][0];
	
	hlDecoder = new Node*[settings->decLevel];
	lhDecoder = new Node*[settings->decLevel];
	hhDecoder = new Node*[settings->decLevel];

	for (int i = 0; i < settings->decLevel; i++) {
		hhDecoder[settings->decLevel - i - 1] = &tree[treeDepth-settings->decLevel + i][1][1];
		hlDecoder[settings->decLevel - i - 1] = &tree[treeDepth-settings->decLevel + i][0][1];
		lhDecoder[settings->decLevel - i - 1] = &tree[treeDepth-settings->decLevel + i][1][0];
	}
	
	// LIS initialization
	*lisNextDecoder++ = llDecoder;
	
	// I-SET initialization
	iPositionDecoder = settings->decLevel -1;
	//isI = true;
	bool isLev = true;
	
	// Compression algorithm
	unsigned char maxBitLevel = stream->maxLevel;
	for (currentBitLevelDecoder = maxBitLevel; isLev; currentBitLevelDecoder--) { // Loop by bitlevels
		if (settings->lumaBitLevelCount > 0)
			if ( ( maxBitLevel - currentBitLevelDecoder ) >= settings->lumaBitLevelCount)
				break;
		isLev = (bool)currentBitLevelDecoder;
		
		// Swapping of LIS buffers
		lisCurrent = lisCurrentDecoderBegin;
		lisCurrentDecoderBegin = lisNextBegin;
		lisNextBegin = lisCurrent;
		lisCurrent = lisNextDecoder;
		lisNextDecoder = lisNextBegin;
		
		levelMaskDecoder = 0x01 << currentBitLevelDecoder;
		
		while (lisCurrentDecoderBegin < lisCurrent) {
			processS_RX( *--lisCurrent );
		}
		processI_RX();
		
		if (currentBitLevelDecoder < maxBitLevel) {
			while (lspBegin < lspThis) {
				testNode = *--lspThis;
				if( ( *refBytePtr ) & refByteMask )
					testNode->val |= levelMaskDecoder;
				
				if( !( refByteMask <<= 1 ) ) {
					++refBytePtr;
					refByteMask = 0x01;
				};
			}
		}
		
		lspThis = lspCurrentDecoder;
		
		// modification of bit section
		
		if (currentBitLevelDecoder) {
			if(sortByteDecoderMask != 0x01) {
				sortByteDecoderPtr++;
				sortByteDecoderMask = 0x01;
			}
			
			if(currentBitLevelDecoder < maxBitLevel) {
				if(refByteMask != 0x01) {
					refBytePtr++;
					refByteMask = 0x01;
				}
			}
		}
	}
		
	for (int i = 0; i < settings->blockSize; i++) {
		for (int j = 0; j < settings->blockSize; j++) {
			data[i][j] = tree[0][i][j].sign ? -int(tree[0][i][j].val) : tree[0][i][j].val;
		}
	}
	
	for (unsigned treeIdx = 0; treeIdx < treeDepth; treeIdx++) {
		for (int i = 0; i < settings->blockSize >> treeIdx; i++) {
			delete tree[treeIdx][i];
		}
		delete tree[treeIdx];
	}
	delete tree;

	delete lisBuffer1;
	delete lisBuffer2;
	delete lspBuffer;

	delete codeStackBufDecoder;

	delete hlDecoder;
	delete lhDecoder;
	delete hhDecoder;

	return 0;
}

int decompressRx(int **r_channel, unsigned char *inStream, double *decodeTime, TransformSettings *settings) {
	lumaStreamDecoder.sortStream = new unsigned char[settings->maxStreamSize];
	 lumaStreamDecoder.refStream = new unsigned char[settings->maxStreamSize];

	memset( lumaStreamDecoder.sortStream, 0, settings->maxStreamSize * sizeof(unsigned char) );
	memset( lumaStreamDecoder.refStream,  0, settings->maxStreamSize * sizeof(unsigned char) );

	lumaStreamDecoder.sortSections = new ByteSection[settings->maxSectionsCount];
	 lumaStreamDecoder.refSections = new ByteSection[settings->maxSectionsCount];

	memset( lumaStreamDecoder.sortSections, 0, settings->maxSectionsCount * sizeof(ByteSection) );
	memset( lumaStreamDecoder.refSections,  0, settings->maxSectionsCount * sizeof(ByteSection) );

	unsigned char *r_ref_str = lumaStreamDecoder.refStream;
	unsigned char *r_sort_str = lumaStreamDecoder.sortStream;

	lumaStreamDecoder.maxLevel = *(inStream++);
	lumaStreamDecoder.refSectionCount = *(inStream++);

	lumaStreamDecoder.sortSections[0].begin = r_sort_str;
	long size = *( inStream++ );
	size += ( *( inStream++ ) ) <<  8;
	size += ( *( inStream++ ) ) << 16;

	lumaStreamDecoder.sortSections[0].size = size;

	memcpy(r_sort_str, inStream, lumaStreamDecoder.sortSections[0].size * sizeof(unsigned char));
	r_sort_str += lumaStreamDecoder.sortSections[0].size;
	inStream   += lumaStreamDecoder.sortSections[0].size;

	for (unsigned char j = 0; j < lumaStreamDecoder.refSectionCount; j++) {
		lumaStreamDecoder.sortSections[j + 1].begin = r_sort_str;

		size  =   *( inStream++ );
		size += ( *( inStream++ ) ) <<  8;
		size += ( *( inStream++ ) ) << 16;

		lumaStreamDecoder.sortSections[j + 1].size = size;

		memcpy(r_sort_str, inStream, lumaStreamDecoder.sortSections[j + 1].size * sizeof(unsigned char));
		r_sort_str += lumaStreamDecoder.sortSections[j + 1].size;
		inStream   += lumaStreamDecoder.sortSections[j + 1].size;

		lumaStreamDecoder.refSections[j].begin = r_ref_str;

		size  =   *( inStream++ );
		size += ( *( inStream++ ) ) <<  8;
		size += ( *( inStream++ ) ) << 16;

		lumaStreamDecoder.refSections[j].size = size;

		memcpy(r_ref_str, inStream, lumaStreamDecoder.refSections[j].size * sizeof(unsigned char));
		r_ref_str += lumaStreamDecoder.refSections[j].size;
		inStream  += lumaStreamDecoder.refSections[j].size;
	}

	Timer compressionDecodeTimer;
	compressionDecodeTimer.Start();

	if ( channeldecompressRx(r_channel, &lumaStreamDecoder, settings) < 0 ) {
		return -1;
	}

	*decodeTime = compressionDecodeTimer.GetTimeInMs();

	delete lumaStreamDecoder.refStream;
	delete lumaStreamDecoder.sortStream;

	delete lumaStreamDecoder.sortSections;
	delete lumaStreamDecoder.refSections;
	
	return 0;
}
