#include <stdio.h>
#include <string.h>

#include "Options.hpp"

bool Options::CheckFlag(const int argc, const char **argv, const std::string string_ref) {
	if (argc >= 1) {
		for (int i = 1; i < argc; i++) {
			std::string arg(argv[i]);
			std::size_t pos = arg.find( string_ref );
			if ( pos != std::string::npos ) {
				return true;
			}
		}
	}

	return false;
}

bool Options::ParseCommandLine(const int argc, const char **argv, const std::string string_ref, char **string_retval) {
	bool bFound = false;

	if (argc >= 1) {
		for (int i = 1; i < argc; i++) {
			std::string arg(argv[i]);
			std::size_t pos = arg.find( string_ref );
			if ( pos != std::string::npos ) {
				// found item
				if ( ( i + 1 ) < argc) {
					*string_retval = (char *)argv[i + 1];
					std::size_t length = strlen(*string_retval);
					while( isspace( (*string_retval)[length - 1] ) ) {
						(*string_retval)[length - 1] = '\0';
						length--;
					}

					bFound = true;
					break;
				}
			}
		}
	}

	if ( !bFound ) {
		*string_retval = NULL;
	}

	return bFound;
}

int Options::ParseCommandLine(const int argc, const char **argv, const std::string string_ref) {
	char *tmp = NULL;
	if ( !ParseCommandLine( argc, argv, string_ref, &tmp ) ) {
		return 0;
	}
	
	return atoi( tmp );
}

Options::Options(int argc, char *argv[]) {
	InputFolder = OutputFolder = NULL;
	if (!ParseCommandLine(argc, (const char **)argv, "input", &InputFolder)) {
		fprintf(stderr, "Input path parameter is not found\n");
		throw;
	}

	if (!ParseCommandLine(argc, (const char **)argv, "output", &OutputFolder)) {
		fprintf(stderr, "Output file parameter is not found\n");
		throw;
	}
	
	StartIndex = ParseCommandLine(argc, (const char **)argv, "startIdx");
	
	StopIndex = ParseCommandLine(argc, (const char **)argv, "stopIdx");
	if ( StopIndex == 0 ) {
		StopIndex = StartIndex + 1;
	} else {
		StopIndex++;
	}

	BitSize = ParseCommandLine(argc, (const char **)argv, "bits");

	BearingFrameIndex = ParseCommandLine(argc, (const char **)argv, "bearingFrame");
	if ( BearingFrameIndex <= 0 ) {
		BearingFrameIndex = 100;
	}

	BlockSize = ParseCommandLine(argc, (const char **)argv, "blockSize");
	OutputBitSize = ParseCommandLine(argc, (const char **)argv, "outputBitsize");
	DecompositionLevel = ParseCommandLine(argc, (const char **)argv, "decLevel");

	char *tmp = NULL;
	ValidationMode = CheckFlag(argc, (const char **)argv, "validation");
	Log = CheckFlag(argc, (const char **)argv, "log");
	PSNR = CheckFlag(argc, (const char **)argv, "psnr");

	SortingSectionTest = CheckFlag(argc, (const char **)argv, "sortSect");
	RefinementSectionTest = CheckFlag(argc, (const char **)argv, "refSect");
	BlockIndex = ParseCommandLine(argc, (const char **)argv, "blockIndex");
	RefSection = ParseCommandLine(argc, (const char **)argv, "refSect");
	SortSection = ParseCommandLine(argc, (const char **)argv, "sortSect");
	BitIndex = ParseCommandLine(argc, (const char **)argv, "bitIdx");

	if (!ParseCommandLine(argc, (const char **)argv, "wavelet", &tmp)) {
		WaveletType = CDF_53;
	} else {
		if ( strcmp( tmp, "5_3" ) == 0 ) {
			WaveletType = CDF_53;
		} else if ( strcmp( tmp, "9_7" ) == 0 ) {
			WaveletType = CDF_97;
		} else if ( strcmp( tmp, "haar" ) == 0 ) {
			WaveletType = HAAR;
		} else {
			WaveletType = CDF_53;
		}
	}

	if (!ParseCommandLine(argc, (const char **)argv, "mode", &tmp)) {
		Mode = NONE;
	} else {
		if ( strcmp( tmp, "image_sub" ) == 0 ) {
			Mode = IMAGE_SUB;
		} else if ( strcmp( tmp, "wavelet_sub" ) == 0 ) {
			Mode = WAVELET_SUB;
		} else if ( strcmp( tmp, "image_wavelet_sub" ) == 0 ) {
			Mode = IMAGE_WAVELET_SUB;
		} else if ( strcmp( tmp, "offset" ) == 0 ) {
			Mode = IMAGE_SUB_WITH_OFFSET;
		} else {
			Mode = NONE;
		}
	}
}
