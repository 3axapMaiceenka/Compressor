#include "LZW.h"

LZW::LZW()
	: CompressionAlgorithm(), m_pTable(new std::vector<Node>(TABLE_SIZE)), m_uCurTableSize(256)
{ 
	initTable();
}

LZW::~LZW()
{
	delete m_pTable;
}

USHORT LZW::hash(USHORT uStringIndx, USHORT uSymbolIndx)
{
	INT nIndex = ((DWORD)uSymbolIndx << HASHING_SHIFT) ^ (int)uStringIndx;
	INT nOffset;

	if (!nIndex)
	{
		nOffset = 1;
	}
	else
	{
		nOffset = TABLE_SIZE - nIndex;
	}

	while (1)
	{
		if (m_pTable->at(nIndex).m_uIndex == 0xffff)
		{
			return nIndex;
		}

		if (m_pTable->at(nIndex).m_Symbol == uSymbolIndx && m_pTable->at(nIndex).m_uParent == uStringIndx)
		{
			return nIndex;
		}

		nIndex -= nOffset;
		if (nIndex < 0)
		{
			nIndex += TABLE_SIZE;
		}
	}
}

void LZW::compress(HANDLE hSourceFile, HANDLE hDestinationFile)
{
	ULONG64 uBitsToWrite = 0;
	DWORD dwRecordEndLow = 0, dwRecordStartLow = 0, dwBytesNumber;
	LONG lRecordStartHigh = 0, lRecordEndHigh = 0;
	USHORT uStringIndex, uIndex;
	TCHAR symbol, string;
	BYTE btBitsUsed = 0;

	dwRecordStartLow = SetFilePointer(hDestinationFile, 0, &lRecordStartHigh, FILE_CURRENT);
	writeHeader(hDestinationFile, btBitsUsed, dwRecordEndLow, lRecordEndHigh);
	ReadFile(hSourceFile, &string, sizeof(TCHAR), &dwBytesNumber, NULL);
	uStringIndex = string;

	while (ReadFile(hSourceFile, &symbol, sizeof(TCHAR), &dwBytesNumber, NULL) && dwBytesNumber)
	{
		uIndex = hash(uStringIndex, symbol);

		if (m_pTable->at(uIndex).m_uIndex != 0xffff)
		{
			uStringIndex = m_pTable->at(uIndex).m_uIndex;
		}
		else
		{
			if (m_uCurTableSize <= MAX_CODE)
			{
				emplace(symbol, uIndex, uStringIndex);
			}

			writeBits(hDestinationFile, uStringIndex, btBitsUsed, uBitsToWrite);
			uStringIndex = symbol;
		}
	}
	
	writeBits(hDestinationFile, uStringIndex, btBitsUsed, uBitsToWrite);
	if (btBitsUsed)
	{
		DWORD dwBytesWritten;
		WriteFile(hDestinationFile, &uBitsToWrite, sizeof(ULONG64), &dwBytesWritten, NULL);
	}

	dwRecordEndLow = SetFilePointer(hDestinationFile, 0, &lRecordEndHigh, FILE_CURRENT);
	SetFilePointer(hDestinationFile, dwRecordStartLow, &lRecordStartHigh, FILE_BEGIN);
	writeHeader(hDestinationFile, btBitsUsed, dwRecordEndLow, lRecordEndHigh);
	SetFilePointer(hDestinationFile, dwRecordEndLow, &lRecordEndHigh, FILE_BEGIN);
}

void LZW::decompress(HANDLE hSourceFile, HANDLE hDestinationFile)
{
	ULONG64 uReadBits = 0;
	DWORD dwEndRecordLow = 0, dwCurFilePointerLow = 0, dwNumberOfBytes;
	LONG lEndRecordHigh = 0, lCurFilePointerHigh = 0;
	USHORT uIndex = 0, uModulo = 0, uStringIndex;
	LPTSTR lpString = NULL;
	BYTE btLastQWordBitsUsed = 0, btMaxBitsUsed = 64, btBitsUsed = 0;
	BOOL bEndRecord = FALSE;

	readHeader(hSourceFile, btLastQWordBitsUsed, dwEndRecordLow, lEndRecordHigh);
	ReadFile(hSourceFile, &uReadBits, sizeof(ULONG64), &dwNumberOfBytes, NULL);
	getSymbolIndex(uIndex, uReadBits, uModulo, btBitsUsed, btMaxBitsUsed);
	WriteFile(hDestinationFile, &m_pTable->at(uIndex).m_Symbol, sizeof(TCHAR), &dwNumberOfBytes, NULL);
	uStringIndex = uIndex;

	do
	{
		if (SetFilePointer(hSourceFile, 0, &lCurFilePointerHigh, FILE_CURRENT) == dwEndRecordLow &&
			lEndRecordHigh == lCurFilePointerHigh)
		{
			btMaxBitsUsed = btLastQWordBitsUsed;
			bEndRecord = TRUE;
		}

		while (getSymbolIndex(uIndex, uReadBits, uModulo, btBitsUsed, btMaxBitsUsed))
		{
			if (uIndex >= m_uCurTableSize) // string + character + string + character + string exception
			{
				lpString = decodeString(uStringIndex);
				LPTSTR lpTemp = append(lpString, lpString[0]);
				std::swap(lpString, lpTemp);
				delete lpTemp;
			}
			else
			{
				lpString = decodeString(uIndex);
			}

			WriteFile(hDestinationFile, lpString, _tcslen(lpString) * sizeof(TCHAR), &dwNumberOfBytes, NULL);

			if (m_uCurTableSize <= MAX_CODE)
			{
				emplace(lpString[0], m_uCurTableSize, uStringIndex);
			}

			uStringIndex = uIndex;
			delete lpString;
		}

		btMaxBitsUsed = 64;
	} while (!bEndRecord && ReadFile(hSourceFile, &uReadBits, sizeof(ULONG64), &dwNumberOfBytes, NULL) && dwNumberOfBytes);
}

BOOL LZW::getSymbolIndex(USHORT& uIndex, ULONG64& uReadBits, USHORT& uModulo, BYTE& btBitsUsed, BYTE& btMaxBitsUse)
{
	if (btBitsUsed && btBitsUsed < BITS)
	{
		uIndex = uModulo;
		uIndex |= (uReadBits << btBitsUsed);
		uIndex &= 0x1fff;
		uReadBits >>= (BITS - btBitsUsed);
		btMaxBitsUse -= (BITS - btBitsUsed);
		btBitsUsed = 0;
		return TRUE;
	} 
	else
	{
		if (BITS <= btMaxBitsUse - btBitsUsed)
		{
			uIndex = (USHORT)uReadBits;
			uIndex &= 0x1fff;
			uReadBits >>= BITS;
			btBitsUsed += BITS;
			return TRUE;
		}
		// if (BITS > btMaxBitsUse - btBitsUsed)
		uModulo = (USHORT)uReadBits;
		btBitsUsed = btMaxBitsUse - btBitsUsed;
		return FALSE;
	}
}

void LZW::writeBits(HANDLE hFile, USHORT uBits, BYTE &btBitsUsed, ULONG64& uBitsToWrite)
{
	uBitsToWrite |= (((ULONG64)uBits) << btBitsUsed);
	btBitsUsed += BITS;

	if (btBitsUsed < 64)
	{
		return;
	}
	
	DWORD dwBytesWritten;
	WriteFile(hFile, &uBitsToWrite, sizeof(ULONG64), &dwBytesWritten, NULL);

	uBitsToWrite = uBits >> (64 - (btBitsUsed - BITS));
	btBitsUsed -= 64;
}

LPTSTR LZW::decodeString(USHORT uIndex)
{
	std::stack<TCHAR> symbols;

	do
	{
		symbols.push(m_pTable->at(uIndex).m_Symbol);
		uIndex = m_pTable->at(uIndex).m_uParent;
	} while (uIndex != 0xffff);

	LPTSTR lpString = new TCHAR[symbols.size() + 1];
	LPTSTR lpIterator = lpString;

	do
	{
		*lpIterator = symbols.top();
		symbols.pop();
		lpIterator++;
	} while (!symbols.empty());

	*lpIterator = '\0';

	return lpString;
}

void LZW::emplace(TCHAR symbol, USHORT uIndex, USHORT uParent)
{
	Node node;
	node.m_Symbol = symbol;
	node.m_uIndex = m_uCurTableSize;
	node.m_uParent = uParent;
	(*m_pTable)[uIndex] = node;
	m_uCurTableSize++;
}

void LZW::initTable()
{
	for (int i = 0; i < 256; i++)
	{
		Node node;
		node.m_Symbol = i;
		node.m_uIndex = i;
		(*m_pTable)[i] = node;
	}
}

void LZW::readHeader(HANDLE hFile, BYTE& btLastQWordBitsUsed, DWORD& dwEndRecordLow, LONG& lEndRecordHigh)
{
	DWORD dwBytesRead;

	ReadFile(hFile, &btLastQWordBitsUsed, sizeof(BYTE), &dwBytesRead, NULL);
	ReadFile(hFile, &dwEndRecordLow, sizeof(DWORD), &dwBytesRead, NULL);
	ReadFile(hFile, &lEndRecordHigh, sizeof(LONG), &dwBytesRead, NULL);
}

void LZW::writeHeader(HANDLE hFile, BYTE btLastQWordBitsUsed, DWORD dwEndRecordLow, LONG lEndRecordHigh)
{
	DWORD dwBytesWritten;

	WriteFile(hFile, &btLastQWordBitsUsed, sizeof(BYTE), &dwBytesWritten, NULL);
	WriteFile(hFile, &dwEndRecordLow, sizeof(DWORD), &dwBytesWritten, NULL);
	WriteFile(hFile, &lEndRecordHigh, sizeof(LONG), &dwBytesWritten, NULL);
}

LPTSTR LZW::append(LPTSTR lpString, TCHAR symbol)
{
	DWORD dwLength = _tcslen(lpString);
	LPTSTR lpNewString = new TCHAR[dwLength + 2];

	_tcsnccpy_s(lpNewString, dwLength + 1, lpString, dwLength + 1);
	lpNewString[dwLength] = symbol;
	lpNewString[dwLength + 1] = '\0';

	return lpNewString;
}