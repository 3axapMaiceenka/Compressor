#pragma once

#include <windows.h>
#include <tchar.h>

#define BUF_SIZE 1024

class CompressionAlgorithm;

class Compressor
{
public:	

	enum Algos
	{
		Huff,
		Lzw 
	};

	~Compressor();

	Compressor(BYTE btAlg);

	Compressor();

	BOOLEAN compress(LPTSTR* lpSourceFilesNames, LPCTSTR lpDestinationFileName); 

	BOOLEAN decompress(LPCTSTR lpSourceFileName, LPCTSTR lpDestinationFileName);

private:

	enum Delimiters
	{
		BEGIN_FILE,
		BEGIN_DIRECTORY,
		END_DIRECTORY
	};

	HANDLE openFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes);
	
	HANDLE openDirectory(LPCTSTR lpDerictoryName);

	HANDLE openCompressFile(LPCTSTR lpFileName);

	HANDLE createFile(LPCTSTR lpFileName);

	void writeRecordInfo(BYTE btRecordType, LPCTSTR lpFileName = NULL);

	void compressFile(HANDLE hSourceFile, HANDLE hDestinationFile);

	void compressDirectory(LPCTSTR lpCurrentDirectory, LPCTSTR lpPreviousDirectory, HANDLE hFind, WIN32_FIND_DATA& ffd);

	void decompressFile(HANDLE hSourceFile, HANDLE hDestinationFile);

	void decompressDirectory(LPCTSTR lpCurrentDirectory, LPCTSTR lpPreviousDirectory);

	void encode(BYTE btFileType, LPCTSTR lpSourceFileName);

	void decode();

	void readRecordInfo(BYTE& btRecordType, LPTSTR& lpFileName);

	LPTSTR concatenate(LPCTSTR lpFirst, LPCTSTR lpSecond);

	LPTSTR formPathForSearhing(LPCTSTR lpParentDirectory, LPCTSTR lpChildDirectory);

	LPTSTR getFullPath(LPCTSTR lpParentDirectory, LPCTSTR lpChildDirectory);

	LPTSTR copy(LPCTSTR lpStringToCopy);

	HANDLE m_hSourceFile;
	HANDLE m_hDestinationFile;
	CompressionAlgorithm* m_pCompAlgo;
	BYTE m_btAlgType;
};