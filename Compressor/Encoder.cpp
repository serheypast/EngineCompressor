#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cstring>

#include "Compressor.hpp"
#include "Encoder.hpp"

#include "Timer.hpp"

struct CompressedStream lumaStreamEncoder;

struct ByteSection *sortSectEncoderPtr; // pointer to current sorting section

unsigned char *sortByteEncoderPtr; // current position in sorting stream
unsigned char sortByteEncoderMask;

//############## Lists #################
struct Node* *lisEncoderNext;
struct Node* *lspEncoderCurrent;

signed char iPositionEncoder; // position of i node
unsigned char currentBitLevelEncoder;

//############## Subband arrays #################
struct Node** hlEncoder;
struct Node** lhEncoder;
struct Node** hhEncoder;
struct Node* llEncoder;

struct Node** codeStackBufEncoder;

unsigned int levelMaskEncoder;

//###############################################

bool isISignificant(void) {
	for (int i = iPositionEncoder; i >= 0; i--) {
		if( lhEncoder[i]->lev >= currentBitLevelEncoder ||
		    hlEncoder[i]->lev >= currentBitLevelEncoder ||
		    hhEncoder[i]->lev >= currentBitLevelEncoder ) {
			   return true;
		}
	}
	return false;
}

void codeS(Node* cnode) {
	struct Node** codeStack = codeStackBufEncoder;
	*codeStack++ = cnode;

	while(codeStackBufEncoder < codeStack) {
		cnode = *--codeStack;
		for (int i = 0; i < 4; i++) {
			bool currentBit = cnode->child[i]->lev >= currentBitLevelEncoder;

			 if (currentBit)
				*sortByteEncoderPtr |= sortByteEncoderMask;
 
			if( !( sortByteEncoderMask <<= 1 ) ) {
				*( ++sortByteEncoderPtr ) = 0x00;
				sortByteEncoderMask = 0x01;
				sortSectEncoderPtr->size++;
			}

			if (currentBit) {
				if(cnode->child[i]->size == 1) 	{
					if ( cnode->child[i]->sign )
						*sortByteEncoderPtr |= sortByteEncoderMask;
						
					if( !( sortByteEncoderMask <<=  1 ) ) {
						*( ++sortByteEncoderPtr ) = 0x00;
						sortByteEncoderMask = 0x01;
						sortSectEncoderPtr->size++;
					}
					
					*lspEncoderCurrent++ = cnode->child[i];
				} else {
					*codeStack++ = cnode->child[i];
				}
			} else {
				*lisEncoderNext++ = cnode->child[i];
			}
		}
	}
}

void processS(Node* node) {
	bool currentBit = node->lev >= currentBitLevelEncoder;

	 if ( currentBit )
		*sortByteEncoderPtr |= sortByteEncoderMask;
		
	if( !( sortByteEncoderMask <<= 1 ) ) {
		*( ++sortByteEncoderPtr ) = 0x00;
		sortByteEncoderMask = 0x01;
		sortSectEncoderPtr->size++;
	}

	if (currentBit) {
		if (node->size == 1) {
			 if ( node->sign )
				*sortByteEncoderPtr |= sortByteEncoderMask;
 
			if( !( sortByteEncoderMask <<= 1 ) ) {
				*( ++sortByteEncoderPtr ) = 0x00;
				sortByteEncoderMask = 0x01;
				sortSectEncoderPtr->size++;
			}
			
			*lspEncoderCurrent++ = node;
		} else {
			codeS(node);
		}
	} else {
		*lisEncoderNext++ = node;
	}
}

void processI(void) {
	if (iPositionEncoder < 0)
		return;

	bool currentBit = isISignificant();
	if ( currentBit )
		*sortByteEncoderPtr |= sortByteEncoderMask;
		
	if( !( sortByteEncoderMask <<= 1 ) ) {
		*( ++sortByteEncoderPtr ) = 0x00;
		sortByteEncoderMask = 0x01;
		sortSectEncoderPtr->size++;
	}

	if (currentBit) {
		processS( lhEncoder[iPositionEncoder] );
		processS( hlEncoder[iPositionEncoder] );
		processS( hhEncoder[iPositionEncoder] );
		iPositionEncoder --;
		processI();
	}
}

int channelTx(int **data, struct CompressedStream *stream, TransformSettings *settings) { // returns 0 if ok
	// creating tree structure
	unsigned treeDepth = unsigned( log( float( settings->blockSize ) ) / log( 2.0 ) );
	struct Node ***tree = new Node**[treeDepth];

	for (unsigned treeIdx = 0; treeIdx < treeDepth; treeIdx++) {
		tree[treeIdx] = new Node*[settings->blockSize >> treeIdx]; // Tree level for nodes with size of 1x1
	
		for (int i = 0; i < settings->blockSize >> treeIdx; i++) {
			tree[treeIdx][i] = new Node[settings->blockSize >> treeIdx];
		}
	}

	//
	codeStackBufEncoder = new Node*[settings->maxListSize];

	// initialization of tree
	unsigned char maxBitLevel = 0;
	// Pixel level
	for (int i = 0; i < settings->blockSize; i++) {
		for (int j = 0; j < settings->blockSize; j++) {
			tree[0][i][j].val  = abs( data[i][j] );
			tree[0][i][j].sign = data[i][j] < 0;

			tree[0][i][j].size = 1;

			int k = 0;
			for (; tree[0][i][j].val >> k; k++);
			
			tree[0][i][j].lev = k ? k - 1 : -1;
			maxBitLevel = MAX( tree[0][i][j].lev, maxBitLevel );
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

				int currentVal = tree[k][i][j].child[0]->lev;
				currentVal = MAX( tree[k][i][j].child[1]->lev, currentVal );
				currentVal = MAX( tree[k][i][j].child[2]->lev, currentVal );
				currentVal = MAX( tree[k][i][j].child[3]->lev, currentVal );

				tree[k][i][j].lev = currentVal;
				tree[k][i][j].sign = NULL;
				tree[k][i][j].val = NULL;
			}
		}
	}

	if ( settings->maxSectionsCount <= maxBitLevel ) {
		fprintf(stderr, "Incorrect input stream: max bit size is %d (options set %d bits)\n", maxBitLevel, settings->maxSectionsCount);
		return -1;
	}

	// Initialization of byte stream

	sortByteEncoderPtr = stream->sortStream;
	unsigned char *refBytePtr = stream->refStream;
	*sortByteEncoderPtr = 0x00;
	*refBytePtr = 0x00;

	sortByteEncoderMask = 0x01;
	unsigned char refByteMask = 0x01;

	sortSectEncoderPtr = stream->sortSections;
	struct ByteSection *refSectPtr = stream->refSections;
	sortSectEncoderPtr->begin = sortByteEncoderPtr;
	refSectPtr->begin = refBytePtr;
	sortSectEncoderPtr->size = 1;
	refSectPtr->size = 1;

	stream->sortSectionCount = 1;
	stream->refSectionCount  = 1;

	// Initialization of lists

	struct Node **lisBuffer1 = new Node*[settings->maxListSize]; 
	struct Node **lisBuffer2 = new Node*[settings->maxListSize];
	struct Node **lspBuffer = new Node*[settings->maxListSize];

	struct Node **lisCurrentEncoderBegin = lisBuffer1;
	struct Node** lisCurrent = lisCurrentEncoderBegin;

	struct Node** lisNextBegin = lisBuffer2;
	lisEncoderNext = lisNextBegin;

	struct Node** lspBegin = lspBuffer;
	struct Node** lspThis = NULL;
	lspEncoderCurrent = lspBegin;

	// Initialization of subbands

	llEncoder = &tree[treeDepth - settings->decLevel][0][0];

	hlEncoder = new Node*[settings->decLevel];
	lhEncoder = new Node*[settings->decLevel];
	hhEncoder = new Node*[settings->decLevel];
	
	for (int i = 0; i < settings->decLevel; i++) {
		hhEncoder[settings->decLevel - i - 1] = &tree[treeDepth-settings->decLevel + i][1][1];
		hlEncoder[settings->decLevel - i - 1] = &tree[treeDepth-settings->decLevel + i][0][1];
		lhEncoder[settings->decLevel - i - 1] = &tree[treeDepth-settings->decLevel + i][1][0];
	}

	// LIS initialization
	*lisEncoderNext++ = llEncoder;

	// I-SET initialization
	iPositionEncoder = settings->decLevel -1;
	//isI = true;
	bool isLev = true;

	stream->maxLevel = maxBitLevel;

	for (currentBitLevelEncoder = maxBitLevel; isLev; currentBitLevelEncoder--) { // Loop by bitlevels
		if (settings->lumaBitLevelCount > 0)
			if (maxBitLevel-currentBitLevelEncoder >= settings->lumaBitLevelCount)
				break;

		isLev = (bool)currentBitLevelEncoder;

		// Swapping of LIS buffers
		lisCurrent = lisCurrentEncoderBegin;
		lisCurrentEncoderBegin = lisNextBegin;
		lisNextBegin = lisCurrent;
		lisCurrent = lisEncoderNext;
		lisEncoderNext = lisNextBegin;

		while (lisCurrentEncoderBegin < lisCurrent) {
			processS( *--lisCurrent );
		}
		processI();

		if (currentBitLevelEncoder < maxBitLevel) {
			levelMaskEncoder = 0x01 << currentBitLevelEncoder;
			while (lspBegin < lspThis) {
				Node* tempNode = *--lspThis;
				
				if ( tempNode->val & levelMaskEncoder )
					*refBytePtr |= refByteMask;
				
				if( !( refByteMask <<= 1 ) ) {
					*( ++refBytePtr ) = 0x00;
					refByteMask = 0x01;
					refSectPtr->size++;
				}
			}
		}

		lspThis = lspEncoderCurrent;

		// modification of bit section
		if ( currentBitLevelEncoder ) {
			if(sortByteEncoderMask != 0x01) {
				sortByteEncoderPtr++;
				sortByteEncoderMask = 0x01;
				*sortByteEncoderPtr = 0x00;
			} else {
				sortSectEncoderPtr->size--;
			}
			
			sortSectEncoderPtr++; //new section
			stream->sortSectionCount++; //inc section count
			sortSectEncoderPtr->begin = sortByteEncoderPtr; // begin of new section
			sortSectEncoderPtr->size = 1; // init size of section

			if(currentBitLevelEncoder < maxBitLevel) {
				if(refByteMask != 0x01) {
					refBytePtr++;
					refByteMask = 0x01;
					*refBytePtr = 0x00;
				} else {
					refSectPtr->size--;
				}
				
				refSectPtr++; //new section
				stream->refSectionCount++; //inc section count
				refSectPtr->begin = refBytePtr; // begin of new section
				refSectPtr->size = 1; // init size of section
			}
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

	delete codeStackBufEncoder;

	delete hlEncoder;
	delete lhEncoder;
	delete hhEncoder;

	return 0;
}

// return size of out bytestream
int compressTx (
	int **r_channel,
	unsigned char *outStream,
	double *encodeTime,
	
	unsigned *sortStreamTotalSize,
	unsigned *refStreamTotalSize,
	
	bool sortingNoiseTest,
	bool refinementNoiseTest,
	TransformSettings *settings
) {
	lumaStreamEncoder.sortStream = new unsigned char[settings->maxStreamSize];
	 lumaStreamEncoder.refStream = new unsigned char[settings->maxStreamSize];

	memset( lumaStreamEncoder.sortStream, 0, settings->maxStreamSize * sizeof(unsigned char) );
	memset( lumaStreamEncoder.refStream,  0, settings->maxStreamSize * sizeof(unsigned char) );

	lumaStreamEncoder.sortSections = new ByteSection[settings->maxSectionsCount];
	 lumaStreamEncoder.refSections = new ByteSection[settings->maxSectionsCount];

	memset( lumaStreamEncoder.sortSections, 0, settings->maxSectionsCount * sizeof(ByteSection) );
	memset( lumaStreamEncoder.refSections,  0, settings->maxSectionsCount * sizeof(ByteSection) );

	Timer compressionEncodeTimer;
	compressionEncodeTimer.Start();
	
	if ( channelTx(r_channel, &lumaStreamEncoder, settings) == -1 ) {
		return -1;
	}
	
	*encodeTime = compressionEncodeTimer.GetTimeInMs();

	*outStream++ = lumaStreamEncoder.maxLevel;
	*outStream++ = lumaStreamEncoder.refSectionCount;
	unsigned long int streamSize = 2;

	*outStream++ =   lumaStreamEncoder.sortSections[0].size         & 0x00FF;
	*outStream++ = ( lumaStreamEncoder.sortSections[0].size >>  8 ) & 0x00FF;
	*outStream++ = ( lumaStreamEncoder.sortSections[0].size >> 16 ) & 0x00FF;
	streamSize += 3;
	
	memcpy( outStream, lumaStreamEncoder.sortSections[0].begin, lumaStreamEncoder.sortSections[0].size * sizeof(unsigned char) );

	if ( settings->Log ) {
		printf("\nMax bit level: %d\n", lumaStreamEncoder.maxLevel);
		printf("Sorting section count: %d\n", lumaStreamEncoder.refSectionCount + 1);
		printf("Refinement section count: %d\n\n", lumaStreamEncoder.refSectionCount);

		printf("Sorting section [0]: %d bytes\n", lumaStreamEncoder.sortSections[0].size);
		
		sortStreamTotalSize[0] += lumaStreamEncoder.sortSections[0].size;
	}

	if ( sortingNoiseTest ) {
		if ( settings->SortSection == 0 ) {
			// modify stream
			unsigned pos = settings->BitIndex >> 3;
			unsigned bitPos = settings->BitIndex & 7;

			// toggle a bit
			outStream[pos] ^= 1 << bitPos;
		}
	}

	outStream += lumaStreamEncoder.sortSections[0].size;
	streamSize += lumaStreamEncoder.sortSections[0].size;

	for (unsigned char j = 0; j < lumaStreamEncoder.refSectionCount; j++) {
		*outStream++ =   lumaStreamEncoder.sortSections[j + 1].size         & 0x00FF;
		*outStream++ = ( lumaStreamEncoder.sortSections[j + 1].size >>  8 ) & 0x00FF;
		*outStream++ = ( lumaStreamEncoder.sortSections[j + 1].size >> 16 ) & 0x00FF;
		streamSize += 3;

		if ( ( streamSize + lumaStreamEncoder.sortSections[j + 1].size ) >= settings->maxStreamSize ) {
			fprintf(stderr, "ERROR: Incorrect output stream (sortSections): maxStreamSize = %lu, requested size = %lu\n", settings->maxStreamSize, streamSize + lumaStreamEncoder.sortSections[j + 1].size);
			return -1;
		}

		memcpy( outStream, lumaStreamEncoder.sortSections[j + 1].begin, lumaStreamEncoder.sortSections[j + 1].size * sizeof(unsigned char) );

		if ( sortingNoiseTest ) {
			if ( settings->SortSection == ( j + 1 ) ) {
				// modify stream
				unsigned pos = settings->BitIndex >> 3;
				unsigned bitPos = settings->BitIndex & 7;

				// toggle a bit
				outStream[pos] ^= 1 << bitPos;
			}
		}

		outStream += lumaStreamEncoder.sortSections[j + 1].size;
		streamSize += lumaStreamEncoder.sortSections[j + 1].size;

		*outStream++ =   lumaStreamEncoder.refSections[j].size         & 0x00FF;
		*outStream++ = ( lumaStreamEncoder.refSections[j].size >>  8 ) & 0x00FF;
		*outStream++ = ( lumaStreamEncoder.refSections[j].size >> 16 ) & 0x00FF;
		streamSize += 3;
		
		if ( ( streamSize + lumaStreamEncoder.refSections[j].size ) >= settings->maxStreamSize ) {
			fprintf(stderr, "ERROR: Incorrect output stream (sortSections): maxStreamSize = %lu, requested size = %lu\n", settings->maxStreamSize, streamSize + lumaStreamEncoder.refSections[j].size);
			return -1;
		}

		memcpy( outStream, lumaStreamEncoder.refSections[j].begin, lumaStreamEncoder.refSections[j].size * sizeof(unsigned char) );
		
		if ( refinementNoiseTest ) {
			if ( settings->RefSection == j ) {
				// modify stream
				unsigned pos = settings->BitIndex >> 3;
				unsigned bitPos = settings->BitIndex & 7;

				// toggle a bit
				outStream[pos] ^= 1 << bitPos;
			}
		}

		outStream += lumaStreamEncoder.refSections[j].size;
		streamSize += lumaStreamEncoder.refSections[j].size;

		if ( settings->Log ) {
			printf("Sorting section [%d]: %d bytes\n", j + 1, lumaStreamEncoder.sortSections[j + 1].size);
			printf("Refinement section [%d]: %d bytes\n", j, lumaStreamEncoder.refSections[j].size);
			
			sortStreamTotalSize[j + 1] += lumaStreamEncoder.sortSections[j + 1].size;
			 refStreamTotalSize[j    ] += lumaStreamEncoder.refSections [j    ].size;
		}
	}
	
	delete lumaStreamEncoder.sortStream;
	delete lumaStreamEncoder.refStream;

	delete lumaStreamEncoder.sortSections;
	delete lumaStreamEncoder.refSections;
	
	return streamSize;
}
