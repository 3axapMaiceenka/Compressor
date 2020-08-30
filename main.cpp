#include "Compressor.h"
#include <stdio.h>

LPTSTR copy(LPCTSTR lpStringToCopy);

LPTSTR* getFilesNames(int argc, _TCHAR** argv);

void freeFilesNames(LPTSTR* lpszFilesNames);

BYTE parseArguments(int argc, _TCHAR** argv);

void info();

void compress(BYTE btSpeed, int argc, _TCHAR** argv);

void decompress(_TCHAR** argv);

enum ARGS
{
	COMPRESS_SLOW,
	COMPRESS_FAST,
	DECOMPRESS,
	HELP,
	INVALID_INPUT
};

int _tmain(int argc, _TCHAR* argv[])
{
	BYTE btOpType = parseArguments(argc, argv);

	switch (btOpType)
	{
	case COMPRESS_FAST:
		compress(btOpType, argc, argv);
		break;
	case COMPRESS_SLOW:
		compress(btOpType, argc, argv);
		break;
	case DECOMPRESS:
		decompress(argv);
		break;
	case HELP:
		info();
		break;
	case INVALID_INPUT:
		_ftprintf(stdout, TEXT("Invalid input! Enter --Compressor help-- to know about how to use the compressor\n"));
		break;
	}

	return 0;
}

LPTSTR copy(LPCTSTR lpStringToCopy)
{
	DWORD dwLength = _tcslen(lpStringToCopy) + 1;
	LPTSTR lpCopy = new TCHAR[dwLength];
	_tcsnccpy_s(lpCopy, dwLength, lpStringToCopy, dwLength);

	return lpCopy;
}

LPTSTR* getFilesNames(int argc, _TCHAR** argv)
{
	LPTSTR* lpszFiles = new LPTSTR[argc - 2];

	for (int i = 1; i < argc - 2; i++)
	{
		lpszFiles[i - 1] = copy(argv[i]);
	}

	lpszFiles[argc - 3] = NULL;

	return lpszFiles;
}

void freeFilesNames(LPTSTR* lpszFilesNames)
{
	for (int i = 0; lpszFilesNames[i]; i++)
	{
		delete lpszFilesNames[i];
	}

	delete lpszFilesNames;
}

BYTE parseArguments(int argc, _TCHAR** argv)
{
	if (argc == 4 && !_tcscmp(argv[2], TEXT("d")))
	{
		return DECOMPRESS;
	}
	if (argc >= 4 && !_tcscmp(argv[argc - 2], TEXT("c")))
	{
		return COMPRESS_SLOW;
	}
	if (argc >= 4 && !_tcscmp(argv[argc - 2], TEXT("cf")))
	{
		return COMPRESS_FAST;
	}
	if (argc == 2 && !_tcscmp(argv[1], TEXT("help")))
	{
		return HELP;
	}

	return INVALID_INPUT;
}

void info()
{
	_ftprintf(stdout, "Enter --Compressor [list of files names you want to compress] c [arhive file name]-- to compress files\n");
	_ftprintf(stdout, "\tExample: Compressor FileToCompress1.txt FileToCompress2.txt c ArhieveFile\n");
	_ftprintf(stdout, "\t\tIf you want to compress faster, use --cf-- flag instead of --c--\n");
	_ftprintf(stdout, "Enter --Compressor [arhieve file name] d [output directory name]-- to decompress arhive\n");
	_ftprintf(stdout, "\tExample: Compressor ArhiveFile d OutputDirectory\n");
}

void compress(BYTE btSpeed, int argc, _TCHAR** argv)
{
	LPTSTR* lpszFilesNames = getFilesNames(argc, argv);
	Compressor* pCompressor = new Compressor(btSpeed);

	_ftprintf(stdout, "Compressing...\n");

	if (!pCompressor->compress(lpszFilesNames, argv[argc - 1]))
	{
		_ftprintf(stdout, TEXT("Unexisting files names entered!\n"));
	}
	else
	{
		_ftprintf(stdout, "Done!\n");
	}

	freeFilesNames(lpszFilesNames);
	delete pCompressor;
}

void decompress(_TCHAR** argv)
{
	Compressor* pCompressor = new Compressor;

	_ftprintf(stdout, "Decompressing...\n");

	if (!pCompressor->decompress(argv[1], argv[3]))
	{
		_ftprintf(stdout, TEXT("Unexisting arhive name entered\n"));
	}
	else
	{
		_ftprintf(stdout, "Done!\n");
	}

	delete pCompressor;
}