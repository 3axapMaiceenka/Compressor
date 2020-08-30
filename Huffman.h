#pragma once

#include "CompressionAlgorithm.h"
#include <tchar.h>
#include <iterator>
#include <map>
#include <list>
#include <utility>

class Huffman : public CompressionAlgorithm
{
public:
	Huffman();

	virtual void compress(HANDLE hSourceFile, HANDLE hDestinationFile) override;

	virtual void decompress(HANDLE hSourceFile, HANDLE hDestinationFile) override;

	~Huffman();

private:

	struct Key
	{
		TCHAR m_Symb;
		DWORD m_dwFrequency;
	};

	struct Node
	{		
		Node* m_pLeft = NULL;
		Node* m_pRight = NULL;
		Key* m_pKey = NULL;
	};

	struct Unapplied
	{
		Key* remove();
		void push(const Key& key);

		Key* m_pFirst  = NULL;
		Key* m_pSecond = NULL;
	};

	BOOL getSymbol(DWORD& dwHuffmanCode, TCHAR& symbol, BYTE& btBitsUsed, BYTE btMaxBitsUsed);

	void buildHuffmanTree();

	void add(const Key& key);

	void priorityPush(Node* pNode);

	void addIfOneUnapplied();

	void addIfTwoUnapplied();

	void createParentNode(Node* pParent, Node* pLeft, Node* pRight);

	void finishTreeBuilding();

	void generateCodes(std::map<TCHAR, std::pair<DWORD, BYTE>>& table, Node* pRoot, DWORD dwCode, BYTE btDepth);

	Node* applyKey();

	void free(Node* pRoot);

	void priorityAdd(const Key& s);

	void pickStat(HANDLE hFile);

	void writeHuffmanCodesToFile(const std::map<TCHAR, std::pair<DWORD, BYTE>>& table, HANDLE hFileToCompress, HANDLE hNewFile);

	void decodeFile(HANDLE hCompressedFile, HANDLE hNewFile);
	
	void writeListToFile(HANDLE hFile);

	void readListFromFile(HANDLE hFile);

	void copyMapToList();

	void readHeader(HANDLE hFile, BYTE& btLastDwordBitsUsed, DWORD& dwRecordEndLow, LONG& lRecordEndHigh);

	void writeHeader(HANDLE hFile, BYTE& btLastDwordBitsUsed, DWORD& dwRecordEndLow, LONG& lRecordEndHigh);
	
	Node* m_pRoot;
	Node* m_pIterNode;
	Unapplied m_Unapplied;
	std::list<Node*> m_pUnlinkedNodes;
	std::list<Key>* m_pSymbList;     // always sorted list (by frequances)
	std::map<TCHAR, DWORD>* m_pStat; // symbol + frequency
};

