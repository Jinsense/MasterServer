#include <WinSock2.h>
#include <windows.h>
#include <wchar.h>
#include <Ws2tcpip.h>
#include <process.h>
#include <time.h>
#include <iostream>

#include "BattleMaster.h"

using namespace std;

CBattleMaster::CBattleMaster()
{
	_pIOCompare = nullptr;
	_pSessionArray = nullptr;
	_listensock = INVALID_SOCKET;

	_bWhiteIPMode = false;
	_bShutdown = false;
	_iAllThreadCnt = 0;
	_pIndex = nullptr;
	_iClientIDCnt = 1;
	_iAcceptTPS = 0;
	_iAcceptTotal = 0;
	_iRecvPacketTPS = 0;
	_iSendPacketTPS = 0;
	_iConnectClient = 0;

}

CBattleMaster::~CBattleMaster()
{

}

bool CBattleMaster::Set(CMasterServer *pMasterServer)
{
	_pMaster = pMasterServer;
	return true;
}

bool CBattleMaster::ServerStart(WCHAR *pOpenIP, int iPort, int iMaxWorkerThread,
	bool bNodelay, int iMaxSession)
{
	wprintf(L"[Server :: Server_Start]	Start\n");

	int _iRetval = 0;
	setlocale(LC_ALL, "Korean");

	CPacket::MemoryPoolInit();

	_pIOCompare = (LANCOMPARE*)_aligned_malloc(sizeof(LANCOMPARE), 16);
	_pIOCompare->iIOCount = 0;
	_pIOCompare->iReleaseFlag = false;

	_pSessionArray = new LANSESSION[iMaxSession];

	for (int i = 0; i < iMaxSession; i++)
	{
		_pSessionArray[i].lIOCount = 0;
		_pSessionArray[i].sock = INVALID_SOCKET;
		_pSessionArray[i].lSendCount = 0;
		_pSessionArray[i].iClientID = NULL;
		_pSessionArray[i].lSendFlag = true;
		_pSessionArray[i].bLoginFlag = false;
		_pSessionArray[i].bRelease = false;
	}

	_pIndex = new unsigned __int64[iMaxSession];
	for (unsigned __int64 i = 0; i < iMaxSession; i++)
	{
		_pIndex[i] = i;
		_SessionStack.Push(&_pIndex[i]);
	}

	if ((_iRetval = ServerInit()) == false)
	{
		return false;
	}

	struct sockaddr_in _server_addr;
	ZeroMemory(&_server_addr, sizeof(_server_addr));
	_server_addr.sin_family = AF_INET;
	InetPton(AF_INET, (PCWSTR)pOpenIP, &_server_addr.sin_addr);
	_server_addr.sin_port = htons(iPort);

	_iRetval = ::bind(_listensock, (sockaddr *)&_server_addr, sizeof(_server_addr));
	if (_iRetval == SOCKET_ERROR)
	{
		return false;
	}

	_iRetval = listen(_listensock, SOMAXCONN);
	if (_iRetval == SOCKET_ERROR)
	{
		return false;
	}

	_hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, &AcceptThread,
		(LPVOID)this, 0, NULL);
	_hAllthread[_iAllThreadCnt++] = _hAcceptThread;
	wprintf(L"[Server :: Server_Start]	AcceptThread Create\n");

	for (int i = 0; i < iMaxWorkerThread; i++)
	{
		_hWorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, &WorkerThread,
			(LPVOID)this, 0, NULL);
		_hAllthread[_iAllThreadCnt++] = _hWorkerThread[i];
	}
	wprintf(L"[Server :: Server_Start]	WorkerThread Create\n");
	wprintf(L"[Server :: Server_Start]	Complete\n");
	return true;
}

bool CBattleMaster::ServerStop()
{
	wprintf(L"[Server :: Server_Stop]	Start\n");

	_bWhiteIPMode = true;
	_aligned_free(_pIOCompare);
	delete _pSessionArray;
	delete _pIndex;

	wprintf(L"([Server :: Server_Stop]	Complete\n");
	return true;
}

void CBattleMaster::Disconnect(unsigned __int64 iClientID)
{
	unsigned __int64  _iIndex = iClientID >> 48;
	LANSESSION *_pSession = &_pSessionArray[_iIndex];

	InterlockedIncrement(&_pSession->lIOCount);

	if (true == _pSession->bRelease || iClientID != _pSession->iClientID)
	{
		if (0 == InterlockedDecrement(&_pSession->lIOCount))
			ClientRelease(_pSession);
		return;
	}
	shutdown(_pSession->sock, SD_BOTH);

	if (0 == InterlockedDecrement(&_pSession->lIOCount))
		ClientRelease(_pSession);

	return;
}

unsigned __int64 CBattleMaster::GetClientCount()
{
	return _iConnectClient;
}

bool CBattleMaster::SendPacket(unsigned __int64 iClientID, CPacket *pPacket)
{
	unsigned __int64 ID = iClientID;
	unsigned __int64 _iIndex = GET_INDEX(_iIndex, ID);

	if (1 == InterlockedIncrement(&_pSessionArray[_iIndex].lIOCount))
	{
		if (0 == InterlockedDecrement(&_pSessionArray[_iIndex].lIOCount))
		{
			ClientRelease(&_pSessionArray[_iIndex]);
		}
		return false;
	}

	if (true == _pSessionArray[_iIndex].bRelease)
	{
		if (0 == InterlockedDecrement(&_pSessionArray[_iIndex].lIOCount))
		{
			ClientRelease(&_pSessionArray[_iIndex]);
		}
		return false;
	}

	if (_pSessionArray[_iIndex].iClientID == iClientID)
	{
		if (_pSessionArray[_iIndex].bLoginFlag != true)
			return false;

		_iSendPacketTPS++;
		pPacket->AddRef();


		pPacket->SetHeader_CustomShort(pPacket->GetDataSize());
		_pSessionArray[_iIndex].SendQ.Enqueue(pPacket);

		if (0 == InterlockedDecrement(&_pSessionArray[_iIndex].lIOCount))
		{
			ClientRelease(&_pSessionArray[_iIndex]);
			return false;
		}
		SendPost(&_pSessionArray[_iIndex]);
		return true;
	}

	if (0 == InterlockedDecrement(&_pSessionArray[_iIndex].lIOCount))
		ClientRelease(&_pSessionArray[_iIndex]);
	return false;
}

bool CBattleMaster::ServerInit()
{
	WSADATA _Data;
	int _iRetval = WSAStartup(MAKEWORD(2, 2), &_Data);
	if (0 != _iRetval)
	{
		wprintf(L"[Server :: Server_Init]	WSAStartup Error\n");
		return false;
	}

	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	if (_hIOCP == NULL)
	{
		wprintf(L"[Server :: Server_Init]	IOCP Init Error\n");
		return false;
	}

	_listensock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if (_listensock == INVALID_SOCKET)
	{
		wprintf(L"[Server :: Server_Init]	Listen Socket Init Error\n");
		return false;
	}
	wprintf(L"[Server :: Server_Init]		Complete\n");
	return true;
}

bool CBattleMaster::ClientShutdown(LANSESSION *pSession)
{
	int _iRetval;
	_iRetval = shutdown(pSession->sock, SD_BOTH);
	if (false == _iRetval)
	{
		return false;
	}
	return true;
}

bool CBattleMaster::ClientRelease(LANSESSION *pSession)
{
	LANCOMPARE _CheckFlag;
	_CheckFlag.iIOCount = pSession->lIOCount;
	_CheckFlag.iReleaseFlag = pSession->bRelease;

	if (FALSE == InterlockedCompareExchange128((LONG64*)_pIOCompare, _CheckFlag.iIOCount,
		_CheckFlag.iReleaseFlag, (LONG64*)&_CheckFlag))
		return FALSE;

	pSession->bRelease = TRUE;
	closesocket(pSession->sock);

	while (0 < pSession->SendQ.GetUseCount())
	{
		CPacket *_pPacket;
		pSession->SendQ.Dequeue(_pPacket);
		_pPacket->Free();
	}

	while (0 < pSession->PacketQ.GetUseSize())
	{
		CPacket *_pPacket;
		pSession->PacketQ.Peek((char*)&_pPacket, sizeof(CPacket*));
		_pPacket->Free();
		pSession->PacketQ.Dequeue(sizeof(CPacket*));
	}

	pSession->bLoginFlag = false;

	unsigned __int64 iClientID = pSession->iClientID;
	unsigned __int64 iIndex = GET_INDEX(iIndex, iClientID);

	ZeroMemory(&_ServerInfo[pSession->Index].IP, sizeof(_ServerInfo[pSession->Index].IP));
	_ServerInfo[pSession->Index].Port = NULL;
	_ServerInfo[pSession->Index].Login = false;

	InterlockedDecrement(&_iConnectClient);
	if (true == pSession->bLoginPacketCheck)
	{
		pSession->bLoginPacketCheck = false;
		InterlockedDecrement(&_iLoginClient);
	}

	BattleServer * pBattleServer = FindBattleServerNo(pSession->ServerNo);
	AcquireSRWLockExclusive(&_pMaster->_BattleServer_lock);
	_pMaster->_BattleServerMap.erase(pSession->ServerNo);
	ReleaseSRWLockExclusive(&_pMaster->_BattleServer_lock);
	
	std::list<BattleRoom*>::iterator iter;
	AcquireSRWLockExclusive(&_pMaster->_Room_lock);
	for (iter = _pMaster->_RoomList.begin(); iter != _pMaster->_RoomList.end();)
	{
		if (pSession->ServerNo == (*iter)->BattleServerNo)
		{
			BattleRoom * pRoom = *iter;
			iter = _pMaster->_RoomList.erase(iter);
			delete pRoom;
		}
		else
			iter++;
	}
	ReleaseSRWLockExclusive(&_pMaster->_Room_lock);

	PutIndex(iIndex);

	if (nullptr != pBattleServer)
		delete pBattleServer;
	return true;
}

void CBattleMaster::WorkerThread_Update()
{
	DWORD _dwRetval;

	while (1)
	{
		OVERLAPPED * _pOver = NULL;
		LANSESSION * _pSession = NULL;
		DWORD _dwTrans = 0;

		_dwRetval = GetQueuedCompletionStatus(_hIOCP, &_dwTrans, (PULONG_PTR)&_pSession,
			(LPWSAOVERLAPPED*)&_pOver, INFINITE);
		if (nullptr == _pOver)
		{
			if (nullptr == _pSession && 0 == _dwTrans)
			{
				PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);
			}
		}

		if (0 == _dwTrans)
		{
			shutdown(_pSession->sock, SD_BOTH);
		}
		else if (_pOver == &_pSession->RecvOver)
		{
			CompleteRecv(_pSession, _dwTrans);
		}
		else if (_pOver == &_pSession->SendOver)
		{
			CompleteSend(_pSession, _dwTrans);
		}

		if (0 >= (_dwRetval = InterlockedDecrement(&_pSession->lIOCount)))
		{
			if (0 == _dwRetval)
				ClientRelease(_pSession);
		}
	}
}

void CBattleMaster::AcceptThread_Update()
{
	DWORD _dwRetval = 0;
	SOCKADDR_IN _ClientAddr;

	while (1)
	{
		int addrSize = sizeof(_ClientAddr);
		SOCKET clientSock = accept(_listensock, (SOCKADDR*)&_ClientAddr, &addrSize);
		if (INVALID_SOCKET == clientSock)
			break;

		InterlockedIncrement(&_iAcceptTPS);
		InterlockedIncrement(&_iAcceptTotal);

		//		OnConnectionRequest((WCHAR*)&_ClientAddr.sin_addr, _ClientAddr.sin_port);

		unsigned __int64 * _iSessionNum = GetIndex();
		if (_iSessionNum == nullptr)
		{
			closesocket(clientSock);
			continue;
		}

		if (_pSessionArray[*_iSessionNum].bLoginFlag == TRUE)
		{
			ClientRelease(&_pSessionArray[*_iSessionNum]);
			continue;
		}

		unsigned __int64 iIndex = *_iSessionNum;
		_pSessionArray[*_iSessionNum].Index = *_iSessionNum;
		_pSessionArray[*_iSessionNum].iClientID = _iClientIDCnt++;
		SET_INDEX(iIndex, _pSessionArray[*_iSessionNum].iClientID);
		_pSessionArray[*_iSessionNum].sock = clientSock;
		_pSessionArray[*_iSessionNum].RecvQ.Clear();
		_pSessionArray[*_iSessionNum].PacketQ.Clear();
		_pSessionArray[*_iSessionNum].lSendFlag = TRUE;
		_pSessionArray[*_iSessionNum].lSendCount = 0;
		InterlockedIncrement(&_iConnectClient);
		_pSessionArray[*_iSessionNum].bLoginFlag = TRUE;

		inet_ntop(AF_INET, &_ClientAddr.sin_addr, _pSessionArray[*_iSessionNum].IP, sizeof(_pSessionArray[*_iSessionNum].IP));
		_pSessionArray[*_iSessionNum].Port = _ClientAddr.sin_port;

		_pSessionArray[*_iSessionNum].bRelease = FALSE;

		InterlockedIncrement(&_pSessionArray[*_iSessionNum].lIOCount);
		CreateIoCompletionPort((HANDLE)clientSock, _hIOCP,
			(ULONG_PTR)&_pSessionArray[*_iSessionNum], 0);
		//		OnClientJoin(&pSessionArray[*_iSessionNum].Info);
		StartRecvPost(&_pSessionArray[*_iSessionNum]);
	}
}

LANSESSION* CBattleMaster::SessionAcquireLock(unsigned __int64 iClientID)
{
	unsigned __int64 _iIndex = iClientID >> 48;
	LANSESSION *_pSession = &_pSessionArray[_iIndex];
	InterlockedIncrement(&_pSession->lIOCount);

	return _pSession;
}

void CBattleMaster::SessionAcquireFree(LANSESSION *pSession)
{
	DWORD _dwRetval;
	if (0 >= (_dwRetval = InterlockedDecrement(&pSession->lIOCount)))
	{
		if (0 == _dwRetval)
			ClientRelease(pSession);
	}
}

void CBattleMaster::StartRecvPost(LANSESSION *pSession)
{
	DWORD _dwRetval = 0;
	DWORD _dwFlags = 0;
	ZeroMemory(&pSession->RecvOver, sizeof(pSession->RecvOver));

	WSABUF _Buf[2];
	DWORD _dwFreeSize = pSession->RecvQ.GetFreeSize();
	DWORD _dwNotBrokenPushSize = pSession->RecvQ.GetNotBrokenPushSize();
	if (0 == _dwFreeSize && 0 == _dwNotBrokenPushSize)
	{
		if (0 >= (_dwRetval = InterlockedDecrement(&pSession->lIOCount)))
		{
			if (0 == _dwRetval)
				ClientRelease(pSession);
		}
		else
			shutdown(pSession->sock, SD_BOTH);
		return;
	}
	int _iNumOfBuf = (_dwNotBrokenPushSize < _dwFreeSize) ? 2 : 1;

	_Buf[0].buf = pSession->RecvQ.GetWriteBufferPtr();
	_Buf[0].len = _dwNotBrokenPushSize;

	if (2 == _iNumOfBuf)
	{
		_Buf[1].buf = pSession->RecvQ.GetBufferPtr();
		_Buf[1].len = _dwFreeSize - _dwNotBrokenPushSize;
	}
	if (SOCKET_ERROR == WSARecv(pSession->sock, _Buf, _iNumOfBuf,
		NULL, &_dwFlags, &pSession->RecvOver, NULL))
	{
		int _iLastError = WSAGetLastError();
		if (ERROR_IO_PENDING != _iLastError)
		{
			if (0 >= (_dwRetval = InterlockedDecrement(&pSession->lIOCount)))
			{
				if (0 == _dwRetval)
					ClientRelease(pSession);
			}
			else
				shutdown(pSession->sock, SD_BOTH);
		}
	}
}

void CBattleMaster::RecvPost(LANSESSION *pSession)
{
	InterlockedIncrement(&pSession->lIOCount);

	DWORD _dwRetval = 0;
	DWORD _dwFlags = 0;
	ZeroMemory(&pSession->RecvOver, sizeof(pSession->RecvOver));

	WSABUF _Buf[2];
	DWORD _dwFreeSize = pSession->RecvQ.GetFreeSize();
	DWORD _dwNotBrokenPushSize = pSession->RecvQ.GetNotBrokenPushSize();
	if (0 == _dwFreeSize && 0 == _dwNotBrokenPushSize)
	{
		if (0 >= (_dwRetval = InterlockedDecrement(&pSession->lIOCount)))
		{
			if (0 == _dwRetval)
				ClientRelease(pSession);
		}
		else
			shutdown(pSession->sock, SD_BOTH);
		return;
	}

	int _iNumOfBuf = (_dwNotBrokenPushSize < _dwFreeSize) ? 2 : 1;

	_Buf[0].buf = pSession->RecvQ.GetWriteBufferPtr();
	_Buf[0].len = _dwNotBrokenPushSize;

	if (2 == _iNumOfBuf)
	{
		_Buf[1].buf = pSession->RecvQ.GetBufferPtr();
		_Buf[1].len = _dwFreeSize - _dwNotBrokenPushSize;
	}

	if (SOCKET_ERROR == WSARecv(pSession->sock, _Buf, _iNumOfBuf,
		NULL, &_dwFlags, &pSession->RecvOver, NULL))
	{
		int _iLastError = WSAGetLastError();
		if (ERROR_IO_PENDING != _iLastError)
		{
			if (0 >= (_dwRetval = InterlockedDecrement(&pSession->lIOCount)))
			{
				if (0 == _dwRetval)
					ClientRelease(pSession);
			}
			else
				shutdown(pSession->sock, SD_BOTH);
		}
	}
}

void CBattleMaster::SendPost(LANSESSION *pSession)
{
	DWORD _dwRetval;
	do
	{
		if (false == InterlockedCompareExchange(&pSession->lSendFlag, false, true))
			return;

		if (0 == pSession->SendQ.GetUseCount())
		{
			InterlockedExchange(&pSession->lSendFlag, true);
			continue;
		}
		ZeroMemory(&pSession->SendOver, sizeof(pSession->SendOver));

		WSABUF _Buf[LAN_WSABUF_NUMBER];
		CPacket *_pPacket;
		long _lBufNum = 0;
		int _iUseSize = (pSession->SendQ.GetUseCount());
		if (_iUseSize > LAN_WSABUF_NUMBER)
		{
			_lBufNum = LAN_WSABUF_NUMBER;
			pSession->lSendCount = LAN_WSABUF_NUMBER;
			for (int i = 0; i < LAN_WSABUF_NUMBER; i++)
			{
				pSession->SendQ.Dequeue(_pPacket);
				pSession->PacketQ.Enqueue((char*)&_pPacket, sizeof(CPacket*));
				_Buf[i].buf = _pPacket->GetReadPtr();
				_Buf[i].len = _pPacket->GetPacketSize_CustomHeader(static_cast<int>(CPacket::en_PACKETDEFINE::SHORT_HEADER_SIZE));
			}
		}
		else
		{
			_lBufNum = _iUseSize;
			pSession->lSendCount = _iUseSize;
			for (int i = 0; i < _iUseSize; i++)
			{
				pSession->SendQ.Dequeue(_pPacket);
				pSession->PacketQ.Enqueue((char*)&_pPacket, sizeof(CPacket*));
				_Buf[i].buf = _pPacket->GetReadPtr();
				_Buf[i].len = _pPacket->GetPacketSize_CustomHeader(static_cast<int>(CPacket::en_PACKETDEFINE::SHORT_HEADER_SIZE));
			}
		}
		InterlockedIncrement(&pSession->lIOCount);
		ZeroMemory(&pSession->SendOver, sizeof(pSession->SendOver));
		if (SOCKET_ERROR == WSASend(pSession->sock, _Buf, _lBufNum,
			NULL, 0, &pSession->SendOver, NULL))
		{
			int _LastError = WSAGetLastError();
			if (ERROR_IO_PENDING != _LastError)
			{
				if (0 >= (_dwRetval = InterlockedDecrement(&pSession->lIOCount)))
				{
					if (0 == _dwRetval)
						ClientRelease(pSession);
				}
				else
					shutdown(pSession->sock, SD_BOTH);
			}
		}
	} while (0 != pSession->SendQ.GetUseCount());
}

void CBattleMaster::CompleteRecv(LANSESSION *pSession, DWORD dwTransfered)
{
	pSession->RecvQ.Enqueue(dwTransfered);
	WORD _wPayloadSize = 0;

	while (LAN_HEADER_SIZE == pSession->RecvQ.Peek((char*)&_wPayloadSize, LAN_HEADER_SIZE))
	{
		CPacket *_pPacket = CPacket::Alloc();
		if (pSession->RecvQ.GetUseSize() < LAN_HEADER_SIZE + _wPayloadSize)
		{
			_pPacket->Free();
			break;
		}

		pSession->RecvQ.Dequeue(LAN_HEADER_SIZE);

		if (_pPacket->GetFreeSize() < _wPayloadSize)
		{
			shutdown(pSession->sock, SD_BOTH);
			_pPacket->Free();
			return;
		}
		pSession->RecvQ.Dequeue(_pPacket->GetWritePtr(), _wPayloadSize);
		_pPacket->PushData(_wPayloadSize + sizeof(CPacket::st_PACKET_HEADER));
		_pPacket->PopData(sizeof(CPacket::st_PACKET_HEADER));

		if (false == OnRecv(pSession, _pPacket))
		{
			_pPacket->Free();
			return;
		}
		_pPacket->Free();
	}
	RecvPost(pSession);
}

void CBattleMaster::CompleteSend(LANSESSION *pSession, DWORD dwTransfered)
{
	CPacket *_pPacket[LAN_WSABUF_NUMBER];
	int Num = pSession->lSendCount;

	pSession->PacketQ.Peek((char*)&_pPacket, sizeof(CPacket*) * Num);
	for (int i = 0; i < pSession->lSendCount; i++)
	{
		_pPacket[i]->Free();
		pSession->PacketQ.Dequeue(sizeof(CPacket*));
	}
	pSession->lSendCount -= Num;

	InterlockedExchange(&pSession->lSendFlag, true);

	SendPost(pSession);
}

bool CBattleMaster::OnRecv(LANSESSION *pSession, CPacket *pPacket)
{
	//-------------------------------------------------------------
	//	모니터링 측정 변수
	//-------------------------------------------------------------
	_iRecvPacketTPS++;

	WORD Type;
	*pPacket >> Type;
	//-------------------------------------------------------------
	//	컨텐츠 처리 - 배틀서버 켜짐 알림
	//	Type	- en_PACKET_BAT_MAS_REQ_SERVER_ON
	//	WCHAR	- ServerIP[16]
	//	WORD	- Port
	//	char	- ConnectToken[32] ( 배틀서버 발행 )
	//	char	- MasterToken[32] ( 사전에 지정된 값 )
	//	WCHAR	- ChatServerIP[16]
	//	WORD	- ChatServerPort
	//-------------------------------------------------------------
	if (Type == en_PACKET_BAT_MAS_REQ_SERVER_ON)
	{
		BattleServer * pBattleServer = new BattleServer;
		pPacket->PopData((char*)&pBattleServer->ServerIP, sizeof(pBattleServer->ServerIP));
		*pPacket >> pBattleServer->Port;
		pPacket->PopData((char*)&pBattleServer->ConnectToken, sizeof(pBattleServer->ConnectToken));
		pPacket->PopData((char*)&pBattleServer->MasterToken, sizeof(pBattleServer->MasterToken));
		pPacket->PopData((char*)&pBattleServer->ChatServerIP, sizeof(pBattleServer->ChatServerIP));
		*pPacket >> pBattleServer->ChatServerPort;


		//	마스터 토큰 검사
		if (0 != strncmp(_pMaster->_Config.MASTERTOKEN, pBattleServer->MasterToken, sizeof(_pMaster->_Config.MASTERTOKEN)))
		{
			//	마스터 토큰이 다를 경우 로그 남기고 끊음
			_pMaster->_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"MasterToken Not Same [MatchServerNo : %s]"), pBattleServer->ServerIP);
			Disconnect(pSession->iClientID);
			delete pBattleServer;
			return true;
		}
		//	배틀서버 번호 할당
		pBattleServer->ServerNo = InterlockedIncrement(&_pMaster->_BattleServerNo);
		pSession->ServerNo = pBattleServer->ServerNo;
		//	배틀서버 맵에 추가
		AcquireSRWLockExclusive(&_pMaster->_BattleServer_lock);
		_pMaster->_BattleServerMap.insert(make_pair(pBattleServer->ServerNo, pBattleServer));
		ReleaseSRWLockExclusive(&_pMaster->_BattleServer_lock);
		//-------------------------------------------------------------
		//	배틀서버에게 서버 켜짐 응답
		//	Type - en_PACKET_BAT_MAS_RES_SERVER_ON
		//	int  - BattleServerNo ( 마스터 서버가 부여 )
		//-------------------------------------------------------------
		InterlockedIncrement(&_iLoginClient);
		pSession->bLoginPacketCheck = true;
		CPacket *newPacket = CPacket::Alloc();
		Type = en_PACKET_BAT_MAS_RES_SERVER_ON;
		*newPacket << Type << pBattleServer->ServerNo;
		SendPacket(pSession->iClientID, newPacket);
		newPacket->Free();
		return true;
	}
	//-------------------------------------------------------------
	//	컨텐츠 처리 - 배틀서버의 연결토큰 재발행
	//	Type	- en_PACKET_BAT_MAS_REQ_CONNECT_TOKEN
	//	char	- ConnectToken[32]
	//	UINT	- ReqSequence
	//-------------------------------------------------------------
	else if (Type == en_PACKET_BAT_MAS_REQ_CONNECT_TOKEN)
	{
		UINT ReqSequence;
		BattleServer * pBattleServer = FindBattleServerNo(pSession->ServerNo);
		if (nullptr == pBattleServer)
		{
			_pMaster->_pLog->Log(const_cast<WCHAR*>(L"Error"), LOG_SYSTEM, const_cast<WCHAR*>(L"BattleServerNo Not Exist [BattleServerNo : %d]"), pSession->ServerNo);
			Disconnect(pSession->iClientID);
			return true;
		}
		pPacket->PopData((char*)&pBattleServer->ConnectToken, sizeof(pBattleServer->ConnectToken));
		*pPacket >> ReqSequence;
		ReqSequence++;
		//-------------------------------------------------------------
		//	배틀서버에게 재발행 수신 확인
		//	Type - en_PACKET_BAT_MAS_RES_CONNECT_TOKEN
		//-------------------------------------------------------------
		Type = en_PACKET_BAT_MAS_RES_CONNECT_TOKEN;
		CPacket *newPacket = CPacket::Alloc();
		*newPacket << Type << ReqSequence;
		SendPacket(pSession->iClientID, newPacket);
		newPacket->Free();
		return true;
	}
	//-------------------------------------------------------------
	//	컨텐츠 처리 - 배틀서버의 신규 대기 방 생성 알림
	//	Type	- en_PACKET_BAT_MAS_REQ_CREATED_ROOM
	//	int		- BattleServerNo
	//	int		- RoomNo
	//	int		- MaxUser
	//	char	- EnterToken[32]
	//	UINT	- ReqSequence
	//-------------------------------------------------------------
	else if (Type == en_PACKET_BAT_MAS_REQ_CREATED_ROOM)
	{
		BattleRoom *pBattleRoom = new BattleRoom;
		UINT ReqSequence;
		pBattleRoom->CurUser = 0;
		pBattleRoom->RoomPlayer.clear();
		*pPacket >> pBattleRoom->BattleServerNo >> pBattleRoom->RoomNo >> pBattleRoom->MaxUser;
		pPacket->PopData((char*)&pBattleRoom->EnterToken, sizeof(pBattleRoom->EnterToken));
		*pPacket >> ReqSequence;

		AcquireSRWLockExclusive(&_pMaster->_Room_lock);
		_pMaster->_RoomList.push_back(pBattleRoom);
		ReleaseSRWLockExclusive(&_pMaster->_Room_lock);
		ReqSequence++;
		//-------------------------------------------------------------
		//	배틀서버에게 신규 대기 방 생성 수신 응답
		//	Type	- en_PACKET_BAT_MAS_RES_CREATED_ROOM
		//	int		- RoomNo
		//	UINT	- ReqSequence
		//-------------------------------------------------------------
		Type = en_PACKET_BAT_MAS_RES_CREATED_ROOM;
		CPacket *newPacket = CPacket::Alloc();
		*newPacket << Type << pBattleRoom->RoomNo << ReqSequence;
		SendPacket(pSession->iClientID, newPacket);
		newPacket->Free();
		return true;
	}
	//-------------------------------------------------------------
	//	컨텐츠 처리 - 방 닫힘 알림
	//	Type	- en_PACKET_BAT_MAS_REQ_CLOSED_ROOM
	//	int		- RoomNo
	//	UINT	- ReqSequence
	//-------------------------------------------------------------
	else if (Type == en_PACKET_BAT_MAS_REQ_CLOSED_ROOM)
	{
		int RoomNo;
		UINT ReqSequence;
		*pPacket >> RoomNo >> ReqSequence;
		AcquireSRWLockExclusive(&_pMaster->_Room_lock);
		for (auto i = _pMaster->_RoomList.begin(); i != _pMaster->_RoomList.end(); i++)
		{
			if ((*i)->RoomNo == RoomNo && (*i)->BattleServerNo == pSession->ServerNo)
			{
				_pMaster->_RoomList.erase(i);
				delete *i;
				break;
			}
			continue;
		}		
		ReleaseSRWLockExclusive(&_pMaster->_Room_lock);
		ReqSequence++;
		//-------------------------------------------------------------
		//	배틀서버에게 방 닫힘 수신확인
		//	Type - en_PACKET_BAT_MAS_RES_CLOSED_ROOM
		//-------------------------------------------------------------
		Type = en_PACKET_BAT_MAS_RES_CLOSED_ROOM;
		CPacket *newPacket = CPacket::Alloc();
		*newPacket << Type << RoomNo << ReqSequence;
		SendPacket(pSession->iClientID, newPacket);
		newPacket->Free();
		return true;
	}
	//-------------------------------------------------------------
	//	컨텐츠 처리 - 방에서 유저가 나감, 1명 나갈때마다 전송
	//	Type	- en_PACKET_BAT_MAS_REQ_LEFT_USER
	//	int		- RoomNo
	//	INT64	- AccountNo
	//	UINT	- ReqSequence
	//-------------------------------------------------------------
	else if (Type == en_PACKET_BAT_MAS_REQ_LEFT_USER)
	{
		int RoomNo;
		INT64 AccountNo;
		UINT ReqSequence;
		*pPacket >> RoomNo >> AccountNo >> ReqSequence;
		AcquireSRWLockExclusive(&_pMaster->_Room_lock);
		for (auto i = _pMaster->_RoomList.begin(); i != _pMaster->_RoomList.end(); i++)
		{
			if ((*i)->RoomNo == RoomNo && (*i)->BattleServerNo == pSession->ServerNo)
			{
				//	내부에 AccountNo를 비교하여 실제 방에 아직 존재하는 유저인지 확인 후 차감 할 것
				//	MatchMaster를 통해 2번 차감 될 수 있음
				for (auto iter = (*i)->RoomPlayer.begin(); iter != (*i)->RoomPlayer.end(); iter++)
				{
					if (iter->AccountNo == AccountNo)
					{
						(*i)->RoomPlayer.erase(iter);
						(*i)->CurUser--;
						break;
					}
					continue;
				}
				break;
			}
			continue;
		}
		ReleaseSRWLockExclusive(&_pMaster->_Room_lock);
		ReqSequence++;
		//-------------------------------------------------------------
		//	배틀서버에게 방 유저 나감 수신 확인
		//	Type - en_PACKET_BAT_MAS_RES_LEFT_USER
		//-------------------------------------------------------------
		Type = en_PACKET_BAT_MAS_REQ_LEFT_USER;
		CPacket *newPacket = CPacket::Alloc();
		*newPacket << Type << RoomNo << ReqSequence;
		SendPacket(pSession->iClientID, newPacket);
		newPacket->Free();
		return true;
	}
	return true;
}

bool CBattleMaster::SetShutDownMode(bool bFlag)
{
	_bShutdown = bFlag;
	PostQueuedCompletionStatus(_hIOCP, 0, 0, 0);
	WaitForMultipleObjects(_iAllThreadCnt, _hAllthread, true, INFINITE);

	ServerStop();

	return _bShutdown;
}

bool CBattleMaster::SetWhiteIPMode(bool bFlag)
{
	_bWhiteIPMode = bFlag;
	return _bWhiteIPMode;
}

unsigned __int64* CBattleMaster::GetIndex()
{
	unsigned __int64 *_iIndex = nullptr;
	_SessionStack.Pop(_iIndex);
	return _iIndex;
}

void CBattleMaster::PutIndex(unsigned __int64 iIndex)
{
	_SessionStack.Push(&_pIndex[iIndex]);
}

BattleServer* CBattleMaster::FindBattleServerNo(int ServerNo)
{
	BattleServer *pBattleServer = nullptr;
	AcquireSRWLockExclusive(&_pMaster->_BattleServer_lock);
	pBattleServer = _pMaster->_BattleServerMap.find(ServerNo)->second;
	ReleaseSRWLockExclusive(&_pMaster->_BattleServer_lock);
	return pBattleServer;
}