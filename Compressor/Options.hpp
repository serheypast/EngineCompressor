#ifndef __OPTIONS__
#define __OPTIONS__

#include <string>
#include "Defines.hpp"

class Options
{
private:
	bool ParseCommandLine(const int argc, const char **argv, const std::string string_ref, char **string_retval);
	int ParseCommandLine(const int argc, const char **argv, const std::string string_ref);
	bool CheckFlag(const int argc, const char **argv, const std::string string_ref);
	
public:
	char *InputFolder;
	char *OutputFolder;
	
	unsigned StartIndex;
	unsigned StopIndex;
	unsigned BitSize;

	unsigned BearingFrameIndex;

	unsigned BlockSize;
	unsigned OutputBitSize;
	unsigned DecompositionLevel;

	bool ValidationMode;
	bool Log;
	bool PSNR;

	unsigned BlockIndex;
	bool RefinementSectionTest;
	unsigned RefSection;
	bool SortingSectionTest;
	unsigned SortSection;
	unsigned BitIndex;

	MODE Mode;
	Wavelet WaveletType;
	
	Options(int argc, char *argv[]);
	~Options(void) { };
};

#endif // __OPTIONS__