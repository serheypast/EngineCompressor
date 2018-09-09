#include <stdlib.h>

#include "Options.hpp"
#include "Engine.hpp"

int main(int argc, char *argv[]) {
	Options options(argc, argv);
	if ( options.InputFolder == NULL ) {
		fprintf(stderr, "Input folder was not set\n");
		return -1;
	}

	// настройка параметров сжатия
	TransformSettings settings;
	settings.lumaBitLevelCount  = options.OutputBitSize;
	settings.lumaMatrixType  = INTEGER;
		
	settings.blockSize       = options.BlockSize;
	settings.decLevel		 = options.DecompositionLevel;
	settings.lumaWavelet	 = options.WaveletType;
	settings.compressionMode = options.Mode;

	//
	settings.Log = options.Log;
	settings.PSNR = options.PSNR;

	//
	settings.BlockIndex = options.BlockIndex;
	settings.RefinementSectionTest = options.RefinementSectionTest;
	settings.RefSection = options.RefSection;
	settings.SortingSectionTest = options.SortingSectionTest;
	settings.SortSection = options.SortSection;
	settings.BitIndex = options.BitIndex;

	Engine<short> engine( options.StartIndex, options.BearingFrameIndex, options.BitSize, options.InputFolder, &settings );
	engine.ProcessFrames( options.StartIndex, options.StopIndex, options.OutputFolder, options.ValidationMode );

	return 0;
}