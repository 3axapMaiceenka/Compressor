#pragma once

#include "CompressionAlgorithm.h"
#include <tchar.h>
#include <vector>
#include <stack>

#define BITS 13
#define TABLE_SIZE 9029
#define HASHING_SHIFT 5
#define MAX_CODE ((1 << BITS) - 2)

class LZW : public CompressionAlgorithm
{
public:

	LZW();

	~LZW();

	virtual void compress(HANDLE hSourceFile, HANDLE hDestinationFile) override;

	virtual void decompress(HANDLE hSourceFile, HANDLE hDestinationFile) override;

private:
	
	struct Node
	{
		TCHAR m_Symbol = -1;
		USHORT m_uIndex = 0xffff; // that value indicates, that object is uninitialized
		USHORT m_uParent = 0xffff;
	};

	USHORT hash(USHORT uStringIndx, USHORT uSymbolIndx);

	void initTable();

	void writeHeader(HANDLE hFile, BYTE btLastQWordBitsUsed, DWORD dwEndRecordLow, LONG lEndRecordHigh);

	void readHeader(HANDLE hFile, BYTE& btLastQWordBitssUsed, DWORD& dwEndRecordLow, LONG& lEndRecordHigh);

	void writeBits(HANDLE hFile, USHORT uBits, BYTE &btBitsUsed, ULONG64& uBitsToWrite);

	void emplace(TCHAR symbol, USHORT uIndex, USHORT uParent);

	BOOL getSymbolIndex(USHORT& uIndex, ULONG64& uReadBits, USHORT& uModulo, BYTE& btBitsUsed, BYTE& btMaxBitsUse);

	LPTSTR decodeString(USHORT uIndex);

	LPTSTR append(LPTSTR lpString, TCHAR symbol);

	std::vector<Node>* m_pTable;
	USHORT m_uCurTableSize;
};

