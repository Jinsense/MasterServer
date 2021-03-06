#ifndef _MASTERSERVER_LIB_PACKET_H_
#define _MASTERSERVER_LIB_PACKET_H_

#include "Config.h"
#include "Dump.h"
#include "MemoryPool.h"

//	extern CConfig _Config;

class CPacket
{
public:
	enum class en_PACKETDEFINE
	{
		PUSH_ERR = 0,
		POP_ERR = 1,
		SHORT_HEADER_SIZE = 2,
		HEADER_SIZE = 5,
		PAYLOAD_SIZE = 1024,
		BUFFER_SIZE = HEADER_SIZE + PAYLOAD_SIZE,

		//		PACKET_CODE = 119,
		//		PACKET_KEY1 = 50,
		//		PACKET_KEY2 = 132,
	};

#pragma pack(push, 1)   
	struct st_PACKET_HEADER
	{
		BYTE	byCode;
		WORD	shLen;
		BYTE	RandKey;
		BYTE	CheckSum;
		st_PACKET_HEADER() :
			byCode(_byCode), RandKey(rand() % 255), CheckSum(NULL), shLen(NULL) {}
	};
#pragma pack(pop)

	struct st_ERR_INFO
	{
		int iErrType;
		int iRequestSize;
		int iCurrentSize;

		st_ERR_INFO(int _iErrType, int _iRequestSize, int _iCurrentSize)
		{
			iErrType = _iErrType;
			iRequestSize = _iRequestSize;
			iCurrentSize = _iCurrentSize;
		}
	};

public:
	CPacket();
	~CPacket();

	static CPacket*	Alloc();
	static void		MemoryPoolInit();
	static void		Init(BYTE byCode, BYTE byPacketKey1, BYTE byPacketKey2);

	//	static __int64	GetUsePool() { return m_pMemoryPool->_UseCount; }
	static __int64	GetAllocPool() { return m_pMemoryPool->GetAllocCount(); }

	void	AddRef();
	void	Free();
	void	Clear();
	void	PushData(WCHAR* pSrc, int iSize);
	void	PopData(WCHAR* pDest, int iSize);
	void	PushData(char *pSrc, int iSize);
	void	PopData(char *pDest, int iSize);
	void	PushData(int iSize);
	void	PopData(int iSize);
	void	SetHeader(char * pHeader);
	void	SetHeader_CustomHeader(char *pHeader, int iCustomHeaderSize);
	void	SetHeader_CustomShort(unsigned short shHeader);
	char*	GetBufferPtr() { return _chBuffer; }
	char*	GetWritePtr() { return _pWritePos; }
	char*	GetReadPtr() { return _pReadPos; }
	int		GetBufferSize() { return _iBufferSize; }
	int		GetDataSize() { return _iDataSize; }
	int		GetPacketSize()
	{
		return static_cast<int>(en_PACKETDEFINE::HEADER_SIZE) + _iDataSize;
	}
	int		GetPacketSize_CustomHeader(int iCustomeHeaderSize)
	{
		return iCustomeHeaderSize + _iDataSize;
	}
	int		GetFreeSize()
	{
		return static_cast<int>(en_PACKETDEFINE::PAYLOAD_SIZE) - _iDataSize;
	}
	void	EnCode();
	bool	DeCode(st_PACKET_HEADER * pInHeader);

	__int64 GetRefCount() { return _iRefCount; }
public:
	CPacket & operator=(CPacket &Packet);

	CPacket& operator<<(char Value);
	CPacket& operator<<(unsigned char Value);
	CPacket& operator<<(short Value);
	CPacket& operator<<(unsigned short Value);
	CPacket& operator<<(int Value);
	CPacket& operator<<(unsigned int Value);
	CPacket& operator<<(long Value);
	CPacket& operator<<(unsigned long Value);
	CPacket& operator<<(float Value);
	CPacket& operator<<(__int64 Value);
	CPacket& operator<<(UINT64 Value);
	CPacket& operator<<(double Value);

	CPacket& operator >> (char& Value);
	CPacket& operator >> (unsigned char& Value);
	CPacket& operator >> (short& Value);
	CPacket& operator >> (unsigned short& Value);
	CPacket& operator >> (int& Value);
	CPacket& operator >> (unsigned int& Value);
	CPacket& operator >> (long& Value);
	CPacket& operator >> (unsigned long& Value);
	CPacket& operator >> (float& Value);
	CPacket& operator >> (__int64& Value);
	CPacket& operator >> (UINT64& Value);
	CPacket& operator >> (double& Value);

public:
	static BYTE	_byCode;
	static BYTE	_byPacketKey1;
	static BYTE	_byPacketKey2;
	//	static		CMemoryPool<CPacket> *m_pMemoryPool;
	static		CMemoryPoolTLS<CPacket> *m_pMemoryPool;
	st_PACKET_HEADER	m_header;

	static long	_UseCount;
private:

	char	_chBuffer[static_cast<int>(en_PACKETDEFINE::BUFFER_SIZE)];
	char	*_pEndPos;
	char	*_pWritePos;
	char	*_pReadPos;
	int		_iBufferSize;
	int		_iDataSize;
	__int64	_iRefCount;
	long	_lHeaderSetFlag;

};

#endif