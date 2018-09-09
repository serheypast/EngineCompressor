#ifndef __COMPRESSOR__
#define __COMPRESSOR__

#include "Defines.hpp"

#define MAX( A, B) ((A)>(B) ? (A) : (B))

struct Node {
	unsigned int size; // 2^номер узла
	unsigned int val; // модуль значения пикселя
	signed char lev; // число бит, кодирующих val
	bool sign; // знак числа (отрицательное / положительное)
	Node* child[4]; // ссылки на дочерние узлы
};

struct ByteSection {
	unsigned char* begin; // begin of section
	unsigned long int size;	// size of section
};

struct CompressedStream {
	unsigned char *sortStream;	 // sorting stream
	unsigned char *refStream;	// refinement stream
	struct ByteSection *sortSections;   // array of pointers to begin of each sections in sortStream
	struct ByteSection *refSections;	// array of pointers to begin of each sections in refStream
	unsigned char sortSectionCount;
	unsigned char refSectionCount;
	unsigned char maxLevel;
};

#endif // __COMPRESSOR__
