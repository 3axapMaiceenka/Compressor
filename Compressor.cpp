#include "Compressor.h"
#include "Huffman.h"
#include "LZW.h"

Compressor::~Compressor()
{
	CloseHandle(m_hSourceFile);
	CloseHandle(m_hDestinationFile);
	delete m_pCompAlgo;
}

Compressor::Compressor(BYTE btAlg)
	: m_btAlgType(btAlg)
{
	btAlg ? m_pCompAlgo = new LZW : m_pCompAlgo = new Huffman;
}

Compressor::Compressor()
	: m_pCompAlgo(NULL)
{ }

BOOLEAN Compressor::compress(LPTSTR* lpSourceFilesNames, LPCTSTR lpDestinationFileName)
{
	if (!(m_hDestinationFile = createFile(lpDestinationFileName)))
	{
		return FALSE;
	}

	DWORD dwNumberOfBytes;
	WriteFile(m_hDestinationFile, &m_btAlgType, sizeof(BYTE), &dwNumberOfBytes, NULL);

	for (int i = 0; lpSourceFilesNames[i]; i++)
	{
		BYTE btOpType = FALSE; // compress file

		if (!(m_hSourceFile = openCompressFile(lpSourceFilesNames[i])))
		{
			if (!(m_hSourceFile = openDirectory(lpSourceFilesNames[i])))
			{
				CloseHandle(m_hDestinationFile);
				DeleteFile(lpDestinationFileName);
				return FALSE;
			}

			btOpType = TRUE; // compress directory
		}

		encode(btOpType, lpSourceFilesNames[i]);
		CloseHandle(m_hSourceFile);
	}

	CloseHandle(m_hDestinationFile);
	m_hSourceFile = m_hDestinationFile = NULL;

	return TRUE;
}

HANDLE Compressor::openDirectory(LPCTSTR lpDerictoryName)
{
	return openFile(lpDerictoryName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS);
}

HANDLE Compressor::openCompressFile(LPCTSTR lpFileName)
{
	return openFile(lpFileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
}

HANDLE Compressor::createFile(LPCTSTR lpFileName)
{
	return openFile(lpFileName, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
}

void Compressor::compressFile(HANDLE hSourceFile, HANDLE hDestinationFile)
{
	m_pCompAlgo->compress(hSourceFile, hDestinationFile);
}

void Compressor::decompressFile(HANDLE hSourceFile, HANDLE hDestinationFile)
{
	m_pCompAlgo->decompress(hSourceFile, hDestinationFile);
}

void Compressor::decode()
{
	LPTSTR lpFileName = NULL;
	BYTE btRecordType;

	readRecordInfo(btRecordType, lpFileName);

	if (btRecordType == BEGIN_FILE)
	{
		m_hDestinationFile = createFile(lpFileName);
		decompressFile(m_hSourceFile, m_hDestinationFile);
	}
	else
	{
		LPTSTR lpCurrentDirectory = new TCHAR[BUF_SIZE];
		GetCurrentDirectory(BUF_SIZE, lpCurrentDirectory);	
		CreateDirectory(lpFileName, NULL);
		m_hDestinationFile = openDirectory(lpFileName);
		SetCurrentDirectory(lpFileName);
		decompressDirectory(lpFileName, lpCurrentDirectory);
	}

	delete lpFileName;
}

void Compressor::encode(BYTE btFileType, LPCTSTR lpSourceFileName)
{
	if (!btFileType)
	{
		writeRecordInfo(btFileType, lpSourceFileName);
		compressFile(m_hSourceFile, m_hDestinationFile);
	}
	else
	{
		WIN32_FIND_DATA ffd;
		LPTSTR lpCurrentDirectory = new TCHAR[BUF_SIZE];

		GetCurrentDirectory(BUF_SIZE, lpCurrentDirectory);
		SetCurrentDirectory(lpSourceFileName);
		compressDirectory(lpSourceFileName, lpCurrentDirectory, NULL, ffd);

		delete lpCurrentDirectory;
	}
}

void Compressor::compressDirectory(LPCTSTR lpCurrentDirectory, LPCTSTR lpPreviousDirectory, HANDLE hFind, WIN32_FIND_DATA& ffd)
{	
	if (hFind == NULL)
	{
		writeRecordInfo(BEGIN_DIRECTORY, lpCurrentDirectory);
		LPTSTR lpFindPath = formPathForSearhing(lpPreviousDirectory, lpCurrentDirectory);
		hFind = FindFirstFile(lpFindPath, &ffd);
		delete lpFindPath;
		compressDirectory(lpCurrentDirectory, lpPreviousDirectory, hFind, ffd);
	}
	else
	{										       
		if (hFind != INVALID_HANDLE_VALUE && _tcscmp(TEXT("."), ffd.cFileName) && _tcscmp(TEXT(".."), ffd.cFileName))
		{
			if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				writeRecordInfo(BEGIN_FILE, ffd.cFileName);
				HANDLE hFileToCompress = openCompressFile(ffd.cFileName);
				compressFile(hFileToCompress, m_hDestinationFile);
				CloseHandle(hFileToCompress);
			}
			else
			{
				WIN32_FIND_DATA newFfd;
				LPTSTR lpCurrentDirectoryFullPath = getFullPath(lpPreviousDirectory, lpCurrentDirectory);
				LPTSTR lpNewDirectory = copy(ffd.cFileName);
				SetCurrentDirectory(lpNewDirectory);
				compressDirectory(lpNewDirectory, lpCurrentDirectoryFullPath, NULL, newFfd);
				delete lpCurrentDirectoryFullPath;
				delete lpNewDirectory;
			}
		}

		if (hFind != INVALID_HANDLE_VALUE && FindNextFile(hFind, &ffd))
		{
			compressDirectory(lpCurrentDirectory, lpPreviousDirectory, hFind, ffd);
		}
		else
		{
			FindClose(hFind);
			writeRecordInfo(END_DIRECTORY);
			SetCurrentDirectory(lpPreviousDirectory);
		}
	}
}

void Compressor::decompressDirectory(LPCTSTR lpCurrentDirectory, LPCTSTR lpPreviousDirectory)
{
	DWORD dwBytesRead = 0, dwFileNameLenght = 0;
	BYTE btRecordType = 0;
	HANDLE hNewFile = NULL;
	LPTSTR lpFileName = NULL, lpCurDirFullPath = NULL;

	if (ReadFile(m_hSourceFile, &btRecordType, sizeof(BYTE), &dwBytesRead, NULL) && dwBytesRead)
	{
		if (btRecordType != END_DIRECTORY)
		{
			ReadFile(m_hSourceFile, &dwFileNameLenght, sizeof(DWORD), &dwBytesRead, NULL);
			lpFileName = new TCHAR[dwFileNameLenght];
			ReadFile(m_hSourceFile, lpFileName, dwFileNameLenght, &dwBytesRead, NULL);
		}

		switch (btRecordType)
		{
		case BEGIN_FILE:
			hNewFile = createFile(lpFileName);
			decompressFile(m_hSourceFile, hNewFile);
			CloseHandle(hNewFile);
			decompressDirectory(lpCurrentDirectory, lpPreviousDirectory);
			break;
		case BEGIN_DIRECTORY:
			CreateDirectory(lpFileName, NULL);
			SetCurrentDirectory(lpFileName);
			lpCurDirFullPath = getFullPath(lpPreviousDirectory, lpCurrentDirectory);
			decompressDirectory(lpFileName, lpCurDirFullPath);
			delete lpCurDirFullPath;
			decompressDirectory(lpCurrentDirectory, lpPreviousDirectory);
			break;
		case END_DIRECTORY:
			SetCurrentDirectory(lpPreviousDirectory);
			break;
		}

		delete lpFileName;
	}
}

void Compressor::writeRecordInfo(BYTE btRecordType, LPCTSTR lpszFileName)
{
	DWORD dwBytesWritten;

	WriteFile(m_hDestinationFile, &btRecordType, sizeof(BYTE), &dwBytesWritten, NULL);

	if (btRecordType != END_DIRECTORY)
	{
		DWORD dwFileNameLenght = _tcsclen(lpszFileName) + 1;	
		WriteFile(m_hDestinationFile, &dwFileNameLenght, sizeof(DWORD), &dwBytesWritten, NULL);
		WriteFile(m_hDestinationFile, lpszFileName, sizeof(TCHAR) * dwFileNameLenght, &dwBytesWritten, NULL);
	}
}

void Compressor::readRecordInfo(BYTE& btRecordType, LPTSTR& lpFileName)
{
	DWORD dwBytesRead;
	DWORD dwFileNameLenght;

	ReadFile(m_hSourceFile, &btRecordType, sizeof(BYTE), &dwBytesRead, NULL);
	ReadFile(m_hSourceFile, &dwFileNameLenght, sizeof(DWORD), &dwBytesRead, NULL);

	lpFileName = new TCHAR[dwFileNameLenght];
	ReadFile(m_hSourceFile, lpFileName, dwFileNameLenght, &dwBytesRead, NULL);
}

BOOLEAN Compressor::decompress(LPCTSTR lpSourceFileName, LPCTSTR lpDestinationFileName)
{
	if (!(m_hSourceFile = openCompressFile(lpSourceFileName)))
	{
		return FALSE;
	}

	DWORD dwNumberOfBytes;
	ReadFile(m_hSourceFile, &m_btAlgType, sizeof(BYTE), &dwNumberOfBytes, NULL);
	m_btAlgType ? m_pCompAlgo = new LZW : m_pCompAlgo = new Huffman;

	LPTSTR lpCurrentDirectory = new TCHAR[BUF_SIZE];
	GetCurrentDirectory(BUF_SIZE, lpCurrentDirectory);
	CreateDirectory(lpDestinationFileName, NULL);
	SetCurrentDirectory(lpDestinationFileName);

	DWORD dwFileStartLow = 0, dwFileEndLow = 0;
	LONG lFileCurHigh = 0, lFileEndHigh = 0;

	dwFileEndLow = SetFilePointer(m_hSourceFile, 0, &lFileEndHigh, FILE_END);
	SetFilePointer(m_hSourceFile, sizeof(BYTE), NULL, FILE_BEGIN);

	while (!(SetFilePointer(m_hSourceFile, 0, &lFileCurHigh, FILE_CURRENT) == dwFileEndLow && lFileCurHigh == lFileEndHigh))
	{
		decode();
		CloseHandle(m_hDestinationFile);
	}

	SetCurrentDirectory(lpCurrentDirectory);
	delete lpCurrentDirectory;
	CloseHandle(m_hSourceFile);
	m_hSourceFile = m_hDestinationFile = NULL;

	return TRUE;
}

HANDLE Compressor::openFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition,
							DWORD dwFlagsAndAttributes)
{
	HANDLE hFile = CreateFile(lpFileName, dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, dwFlagsAndAttributes, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}

	return hFile;
}

LPTSTR Compressor::concatenate(LPCTSTR lpFirst, LPCTSTR lpSecond)
{
	DWORD dwFirstLength = _tcslen(lpFirst);
	DWORD dwSecondLenght = _tcslen(lpSecond);
	LPTSTR lpNew = new TCHAR[dwFirstLength + dwSecondLenght + 1];
	_tcsnccpy_s(lpNew, dwFirstLength + 1, lpFirst, dwFirstLength + 1);
	_tcscat_s(lpNew, dwSecondLenght + dwFirstLength + 1, lpSecond);

	return lpNew;
}

LPTSTR Compressor::getFullPath(LPCTSTR lpParentDirectory, LPCTSTR lpChildDirectory)
{
	LPTSTR lpInbetween = concatenate(lpParentDirectory, TEXT("\\"));
	LPTSTR lpFullPath = concatenate(lpInbetween, lpChildDirectory);
	delete lpInbetween;

	return lpFullPath;
}

LPTSTR Compressor::formPathForSearhing(LPCTSTR lpParentDirectory, LPCTSTR lpChildDirectory)
{
	LPTSTR lpFullPath = getFullPath(lpParentDirectory, lpChildDirectory);
	LPTSTR lpPathForSearhing = concatenate(lpFullPath, TEXT("\\*"));
	delete lpFullPath;
	
	return lpPathForSearhing;
}

LPTSTR Compressor::copy(LPCTSTR lpStringToCopy)
{
	DWORD dwLength = _tcslen(lpStringToCopy) + 1;
	LPTSTR lpCopy = new TCHAR[dwLength];
	_tcsnccpy_s(lpCopy, dwLength, lpStringToCopy, dwLength);

	return lpCopy;
}

