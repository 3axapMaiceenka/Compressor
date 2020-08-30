#include "Huffman.h"

Huffman::Huffman() 
	: CompressionAlgorithm(), m_pStat(new std::map<TCHAR, DWORD>), m_pSymbList(new std::list<Key>)
{ }

void Huffman::add(const Key& key)
{
	m_Unapplied.push(key);

	if (!m_Unapplied.m_pSecond)
	{
		addIfOneUnapplied();
	}
	else
	{
		addIfTwoUnapplied();
	}
}

Huffman::Node* Huffman::applyKey()
{
	Node* node = new Node;
	node->m_pKey = m_Unapplied.remove();
	node->m_pLeft = node->m_pRight = nullptr;
	return node;
}

void Huffman::buildHuffmanTree()
{
	for (auto it = m_pSymbList->cbegin(); it != m_pSymbList->cend(); it++)
	{
		add(*it);
	}

	finishTreeBuilding();
}

void Huffman::compress(HANDLE hSourceFile, HANDLE hDestinationFile)
{
	pickStat(hSourceFile);
	buildHuffmanTree();

	auto* table = new std::map<TCHAR, std::pair<DWORD, BYTE>>;

	generateCodes(*table, m_pRoot, 0, 0);
	writeListToFile(hDestinationFile);
	writeHuffmanCodesToFile(*table, hSourceFile, hDestinationFile);

	delete table;
}

void Huffman::decompress(HANDLE hSourceFile, HANDLE hDestinationFile)
{
	readListFromFile(hSourceFile);
	buildHuffmanTree();

	auto* table = new std::map<TCHAR, std::pair<DWORD, BYTE>>;

	decodeFile(hSourceFile, hDestinationFile);

	delete table;
}

void Huffman::priorityPush(Node* node)
{
	auto it = m_pUnlinkedNodes.cbegin();

	for (; it != m_pUnlinkedNodes.cend() && (*it)->m_pKey->m_dwFrequency < node->m_pKey->m_dwFrequency; it++)
		;

	m_pUnlinkedNodes.insert(it, node);
}

Huffman::Key* Huffman::Unapplied::remove()
{
	Key* pRetVal = m_pFirst;
	m_pFirst = m_pSecond;
	m_pSecond = nullptr;
	return pRetVal;
}

void Huffman::addIfOneUnapplied()
{
	if (!m_pUnlinkedNodes.empty() && m_pUnlinkedNodes.front()->m_pKey->m_dwFrequency <= m_Unapplied.m_pFirst->m_dwFrequency)
	{
		Node* pParentNode = new Node;
		Node* pRightNode = nullptr;

		if (m_pUnlinkedNodes.size() >= 2 &&
			(*(++m_pUnlinkedNodes.begin()))->m_pKey->m_dwFrequency <= m_Unapplied.m_pFirst->m_dwFrequency)
		{                                                 
			pRightNode = *(++m_pUnlinkedNodes.begin());
			m_pUnlinkedNodes.remove(pRightNode);
		}
		else
		{
			pRightNode = applyKey();
		}

		createParentNode(pParentNode, m_pUnlinkedNodes.front(), pRightNode);
		m_pUnlinkedNodes.pop_front();
		priorityPush(pParentNode);
	}
}

void Huffman::addIfTwoUnapplied()
{
	Node* pParentNode = new Node;
	Node* pRightNode = nullptr, *pLeftNode = nullptr;

	if (!m_pUnlinkedNodes.empty() && m_pUnlinkedNodes.front()->m_pKey->m_dwFrequency <= m_Unapplied.m_pSecond->m_dwFrequency)
	{
		pRightNode = applyKey();
		pLeftNode = m_pUnlinkedNodes.front();
		m_pUnlinkedNodes.pop_front();
	}
	else
	{
		pLeftNode = applyKey();
		pRightNode = applyKey();
	}

	createParentNode(pParentNode, pLeftNode, pRightNode);
	priorityPush(pParentNode);
}

void Huffman::createParentNode(Huffman::Node* pParent, Huffman::Node* pLeft, Huffman::Node* pRight)
{
	pParent->m_pLeft = pLeft;
	pParent->m_pRight = pRight;
	pParent->m_pKey = new Key;
	pParent->m_pKey->m_dwFrequency = pLeft->m_pKey->m_dwFrequency + pRight->m_pKey->m_dwFrequency;
}

void Huffman::finishTreeBuilding()
{
	while (m_pUnlinkedNodes.size() != 1 || m_Unapplied.m_pFirst)
	{
		Node* pParent = new Node;
		Node* pLeft = nullptr, *pRight = nullptr;

		if (m_Unapplied.m_pFirst)
		{
			pRight = applyKey();
			pLeft = m_pUnlinkedNodes.front();
			m_pUnlinkedNodes.pop_front();
		}
		else
		{
			pLeft = m_pUnlinkedNodes.front();
			m_pUnlinkedNodes.pop_front();
			pRight = m_pUnlinkedNodes.front();
			m_pUnlinkedNodes.pop_front();
		}

		createParentNode(pParent, pLeft, pRight);
		priorityPush(pParent);
	}

	m_pRoot = m_pUnlinkedNodes.front();
	m_pIterNode = m_pRoot;
	m_pUnlinkedNodes.pop_front();
}

//Initially m_pIterNode is equal to m_pRoot
BOOL Huffman::getSymbol(DWORD& dwHuffmanCode, TCHAR& symbol, BYTE& btBitsUsed, BYTE btMaxBitsUsed)
{
	while (m_pIterNode->m_pLeft && m_pIterNode->m_pRight && btBitsUsed < btMaxBitsUsed)
	{
		if (dwHuffmanCode & 1)
		{
			m_pIterNode = m_pIterNode->m_pRight;
		}
		else
		{
			m_pIterNode = m_pIterNode->m_pLeft;
		}

		dwHuffmanCode >>= 1;
		btBitsUsed++;
	}

	if (!m_pIterNode->m_pLeft && !m_pIterNode->m_pRight)
	{
		symbol = m_pIterNode->m_pKey->m_Symb;
		m_pIterNode = m_pRoot;
		return TRUE;
	}
	
	return FALSE;
}

void Huffman::Unapplied::push(const Key& key)
{
	Key* pKey = new Key;
	pKey->m_dwFrequency = key.m_dwFrequency;
	pKey->m_Symb = key.m_Symb;

	if (!m_pFirst)
	{
		m_pFirst = pKey;
	}
	else
	{
		m_pSecond = pKey;
	}
}

void Huffman::readHeader(HANDLE hFile, BYTE& btLastDwordBitsUsed, DWORD& dwRecordEndLow, LONG& lRecordEndHigh)
{
	DWORD dwBytesRead;

	ReadFile(hFile, &btLastDwordBitsUsed, sizeof(BYTE), &dwBytesRead, NULL);
	ReadFile(hFile, &dwRecordEndLow, sizeof(DWORD), &dwBytesRead, NULL);
	ReadFile(hFile, &lRecordEndHigh, sizeof(LONG), &dwBytesRead, NULL);
}

void Huffman::writeHeader(HANDLE hFile, BYTE& btLastDwordBitsUsed, DWORD& dwRecrodEndLow, LONG& lRecordEndHigh)
{
	DWORD dwNumberOfBytesWritten;

	WriteFile(hFile, &btLastDwordBitsUsed, sizeof(BYTE), &dwNumberOfBytesWritten, NULL); 
	WriteFile(hFile, &dwRecrodEndLow, sizeof(DWORD), &dwNumberOfBytesWritten, NULL);
	WriteFile(hFile, &lRecordEndHigh, sizeof(LONG), &dwNumberOfBytesWritten, NULL);
}

void Huffman::decodeFile(HANDLE hCompressedFile, HANDLE hNewFile)
{
	DWORD dwReadBits = 0, dwNumberOfBytes = 0, dwEndFilePointerLow = 0;
	LONG lEndFilePointerHigh = 0, lCurFilepointerHigh = 0;
	TCHAR symbol;
	BYTE btBitsUsed = 0, btMaxBitsUsed = 32, btLastDwordBitsUsed, btRecordRead = FALSE;

	readHeader(hCompressedFile, btLastDwordBitsUsed, dwEndFilePointerLow, lEndFilePointerHigh);

	while (!btRecordRead && ReadFile(hCompressedFile, &dwReadBits, sizeof(DWORD), &dwNumberOfBytes, NULL) && dwReadBits)
	{
		if (SetFilePointer(hCompressedFile, 0, &lCurFilepointerHigh, FILE_CURRENT) == dwEndFilePointerLow
			&& lCurFilepointerHigh == lEndFilePointerHigh)
		{
			btMaxBitsUsed = btLastDwordBitsUsed; 
			btRecordRead = TRUE; // to prevent while loop for one more iteration
		}

		btBitsUsed = 0;

		while (btBitsUsed < btMaxBitsUsed)
		{
			if (getSymbol(dwReadBits, symbol, btBitsUsed, btMaxBitsUsed))
			{
				WriteFile(hNewFile, &symbol, sizeof(TCHAR), &dwNumberOfBytes, NULL);
			}
		}
	}
}

void Huffman::readListFromFile(HANDLE hFile)
{
	DWORD dwNumberOfBytesRead;
	std::list<Key>::size_type ulSize;
	Key key;

	ReadFile(hFile, &ulSize, sizeof(std::list<Key>::size_type), &dwNumberOfBytesRead, NULL);

	for (; ulSize; ulSize--)
	{
		ReadFile(hFile, &key, sizeof(Key), &dwNumberOfBytesRead, NULL);
		m_pSymbList->emplace_back(key);
	}
}

void Huffman::writeListToFile(HANDLE hWriteFile)
{
	DWORD dwNumberOfBytesWritten;
	auto uUniqueSymbols = m_pSymbList->size();

	WriteFile(hWriteFile, &uUniqueSymbols, sizeof(std::list<Key>::size_type), &dwNumberOfBytesWritten, NULL);

	for (auto& key : *m_pSymbList)
	{
		WriteFile(hWriteFile, &key, sizeof(Key), &dwNumberOfBytesWritten, NULL);
	}
}

void Huffman::writeHuffmanCodesToFile(const std::map<TCHAR, std::pair<DWORD, BYTE>>& table,
	HANDLE hFileToCompress, HANDLE hNewFile)
{
	DWORD dwBitsToWrite = 0, dwBytesRead = 0, dwNumberOfBytesWritten = 0, dwFileStartLow = 0, dwFileEndLow = 0;
	LONG lFileStartHigh = 0, lFileEndHigh = 0;
	TCHAR symbol;
	BYTE btBitCount = 0;

	dwFileStartLow = SetFilePointer(hNewFile, 0, &lFileStartHigh, FILE_CURRENT); // get current fp position
	writeHeader(hNewFile, btBitCount, dwFileEndLow, lFileEndHigh);
	SetFilePointer(hFileToCompress, 0, NULL, FILE_BEGIN);

	while (ReadFile(hFileToCompress, &symbol, sizeof(TCHAR), &dwBytesRead, NULL) && dwBytesRead)
	{
		std::pair<DWORD, BYTE> code = table.at(symbol);
		dwBitsToWrite |= (code.first << btBitCount);
		btBitCount += code.second;

		if (btBitCount >= 32)
		{
			WriteFile(hNewFile, &dwBitsToWrite, sizeof(DWORD), &dwNumberOfBytesWritten, NULL);
			dwBitsToWrite = code.first >> (32 - (btBitCount - code.second));
			btBitCount -= 32;
		}
	}

	WriteFile(hNewFile, &dwBitsToWrite, sizeof(DWORD), &dwNumberOfBytesWritten, NULL);

	dwFileEndLow = SetFilePointer(hNewFile, 0, &lFileEndHigh, FILE_CURRENT); // get current fp position
	SetFilePointer(hNewFile, dwFileStartLow, &lFileStartHigh, FILE_BEGIN);   // set fp to the start of file
	writeHeader(hNewFile, btBitCount, dwFileEndLow, lFileEndHigh);
	SetFilePointer(hNewFile, dwFileEndLow, &lFileEndHigh, FILE_BEGIN);
}

// builds a model of a file: counts every occurred symbol and it's frequency, stores them to the map and sorted list
void Huffman::pickStat(HANDLE hFile)
{
	TCHAR symbol;
	DWORD dwNumberOfBytesRead, dwReadSymbols = 0;

	while (ReadFile(hFile, &symbol, sizeof(TCHAR), &dwNumberOfBytesRead, NULL) && dwNumberOfBytesRead) // test for EOF
	{
		(*m_pStat)[symbol]++; // if symbol encountered first time add it to the map, else increase it's frequency
	}

	copyMapToList();
}

void Huffman::copyMapToList()
{
	Key key;

	for (auto& i : *m_pStat)
	{
		key.m_dwFrequency = i.second;
		key.m_Symb = i.first;
		priorityAdd(key);
	}
}

void Huffman::priorityAdd(const Key& s) // adds elements to the list and keeps it sorted
{
	auto it = m_pSymbList->cbegin();

	for (; it != m_pSymbList->cend() && s.m_dwFrequency > it->m_dwFrequency; it++)
		;

	m_pSymbList->insert(it, s);
}

void Huffman::free(Node* pRoot)
{
	if (pRoot)
	{
		if (pRoot->m_pLeft && pRoot->m_pRight)
		{
			free(pRoot->m_pLeft);
			free(pRoot->m_pRight);
		}

		delete pRoot->m_pKey;
		delete pRoot;
	}
}

void Huffman::generateCodes(std::map<TCHAR, std::pair<DWORD, BYTE>>& table, Huffman::Node* pRoot,
								DWORD dwCode, BYTE btDepth)
{
	if (!pRoot->m_pLeft && !pRoot->m_pRight)
	{		
		table[pRoot->m_pKey->m_Symb] = std::make_pair(dwCode, btDepth);
	}
	else
	{
		generateCodes(table, pRoot->m_pRight, (1 << btDepth) | dwCode, btDepth + 1);
		generateCodes(table, pRoot->m_pLeft, dwCode, btDepth + 1);
	}
}

Huffman::~Huffman()
{
	free(m_pRoot);
	delete m_pStat;
	delete m_pSymbList;
}
