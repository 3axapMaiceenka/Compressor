#pragma once

#include <windows.h>

class CompressionAlgorithm
{
public:
	virtual void compress(HANDLE hSourceFile, HANDLE hDestinationFile) = 0;

	virtual void decompress(HANDLE hSourceFile, HANDLE hDestinationFile) = 0;
};
