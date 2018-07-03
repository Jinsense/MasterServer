#include "MasterServer.h"

CMasterServer::CMasterServer()
{
	_pBattleMaster = new CBattleMaster;
	_pBattleMaster->Set(this);
	_pMatchMaster = new CMatchMaster;
	_pMatchMaster->Set(this);
	_pMonitor = new CLanClient;
	_pMonitor->Constructor(this);
	_pLog = _pLog->GetInstance();

	InitializeSRWLock(&_MatchServerNo_lock);
	InitializeSRWLock(&_ClientKey_lock);
	InitializeSRWLock(&_BattleServer_lock);
	InitializeSRWLock(&_Room_lock);
	
	_ClientPool = new CMemoryPool<CLIENT>();

	_bShutDown = false;
	_bMonitorMode = true;
	_BattleRoomEnterFail = NULL;

	_BattleServerNo = 0;
	_TimeStamp = NULL;
	_MasterServer_On = 1;
	_MasterServer_CPU_Process = NULL;
	_MasterServer_CPU_All = NULL;
	_MasterServer_MemoryCommit = NULL;
	_MasterServer_PacketPool = NULL;
	_MasterServer_Match_Connect = NULL;
	_MasterServer_Match_Login = NULL;
	_MasterServer_Stay_Client = NULL;
	_MasterServer_Battle_Connect = NULL;
	_MasterServer_Battle_Login = NULL;
	_MasterServer_Battle_WaitRoom = NULL;

	_hMonitorSendThread = (HANDLE)_beginthreadex(NULL, 0, &LanMonitoringThread, (LPVOID)this, 0, NULL);
	_hMonitorPrintThread = (HANDLE)_beginthreadex(NULL, 0, &MonitorPrintThread, (LPVOID)this, 0, NULL);
}

CMasterServer::~CMasterServer()
{
	delete _ClientPool;
	delete _pBattleMaster;
	delete _pMatchMaster;
	delete _pMonitor;

}

bool CMasterServer::GetShutDownMode()
{
	return _bShutDown;
}

void CMasterServer::SetShutDownMode()
{
	_bShutDown = true;
	return;
}

bool CMasterServer::GetMonitorMode()
{
	return _bMonitorMode;
}

void CMasterServer::SetMonitorMode(bool type)
{
	_bMonitorMode = type;
	return;
}

BattleServer* CMasterServer::FindBattleServerNo(int ServerNo)
{
	AcquireSRWLockExclusive(&_BattleServer_lock);
	BattleServer* pBattleServer = _BattleServerMap.find(ServerNo)->second;
	ReleaseSRWLockExclusive(&_BattleServer_lock);
	return pBattleServer;
}

CLIENT* CMasterServer::FindClientKey(int ClientKey)
{
	AcquireSRWLockExclusive(&_ClientKey_lock);
	CLIENT* pClient = _ClientKeyMap.find(ClientKey)->second;
	ReleaseSRWLockExclusive(&_BattleServer_lock);
	return pClient;
}

void CMasterServer::MonitorPrintThead_Update()
{
	//	모니터링 항목 출력 스레드
	wprintf(L"\n\n");
	struct tm *t = new struct tm;
	time_t timer;
	timer = time(NULL);
	localtime_s(t, &timer);

	int year = t->tm_year + 1900;
	int month = t->tm_mon + 1;
	int day = t->tm_mday;
	int hour = t->tm_hour;
	int min = t->tm_min;
	int sec = t->tm_sec;

	while (1)
	{
		Sleep(1000);
		_Cpu.UpdateCpuTime();

		timer = time(NULL);
		localtime_s(t, &timer);

		if (true == _bMonitorMode)
		{
			wprintf(L"	[ServerStart : %d/%d/%d %d:%d:%d]\n\n", year, month, day, hour, min, sec);
			wprintf(L"	[%d/%d/%d %d:%d:%d]\n\n", t->tm_year + 1900, t->tm_mon + 1,
				t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
			wprintf(L"	마스터 CPU 사용률 (프로세스)	:	%d\n", _MasterServer_CPU_Process);
			wprintf(L"	마스터 CPU 사용률 ( 전체 ) 	:	%d\n", _MasterServer_CPU_All);
			wprintf(L"	마스터 메모리 유저 커밋 사용량	:	%d\n", _MasterServer_MemoryCommit);
			wprintf(L"	마스터 패킷풀 사용량		:	%d\n\n", _MasterServer_PacketPool);
			wprintf(L"	매치메이킹 서버 연결 수		:	%d\n", _MasterServer_Match_Connect);
			wprintf(L"	매치메이킹 서버 로그인 수	:	%d\n", _MasterServer_Match_Login);
			wprintf(L"	대기자 클라이언트		:	%d\n\n", _MasterServer_Stay_Client);
			wprintf(L"	배틀 서버 연결 수		:	%d\n", _MasterServer_Battle_Connect);
			wprintf(L"	배틀 서버 로그인 수		:	%d\n", _MasterServer_Battle_Login);
			wprintf(L"	배틀 서버 대기방		:	%d\n\n", _MasterServer_Battle_WaitRoom);
		}
	}
	delete t;
	
	return;
}

void CMasterServer::LanMonitoringThread_Update()
{
	//-------------------------------------------------------------
	//	모니터링 서버에 전송할 항목 업데이트 후 전송
	//-------------------------------------------------------------
	while (1)
	{
		Sleep(1000);
		if (false == _pMonitor->IsConnect())
		{
			//	연결 상태가 아닐 경우 재접속 시도
			_pMonitor->Connect(_Config.MONITOR_IP, _Config.MONITOR_PORT, true, LANCLIENT_WORKERTHREAD);
			continue;
		}
		PdhCollectQueryData(_CpuQuery);
		PdhGetFormattedCounterValue(_ProcessPrivateBytes, PDH_FMT_DOUBLE, NULL, &_CounterVal);
		_MasterServer_MemoryCommit = (int)_CounterVal.doubleValue / (1024 * 1024);
		//-------------------------------------------------------------
		//	모니터링 서버에 전송할 패킷 생성 후 전송
		//-------------------------------------------------------------
		//	마스터 서버 ON 전송
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_SERVER_ON);
		//	마스터 CPU 사용률 전송 ( 프로세스 )
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_CPU);
		//	마스터 CPU 사용률 전송 ( 서버 컴퓨터 전체 )
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_CPU_SERVER);
		//	마스터 메모리 유저 커밋 사용량 ( Private ) MByte
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_MEMORY_COMMIT);
		//	마스터 패킷풀 사용량
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_PACKET_POOL);
		//	마스터 매치메이킹 서버 연결 수
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_MATCH_CONNECT);
		//	마스터 매치메이킹 서버 로그인 수
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_MATCH_LOGIN);
		//	마스터 대기자 클라이언트
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_STAY_CLIENT);
		//	마스터 배틀 서버 연결 수
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_BATTLE_CONNECT);
		//	마스터 배틀 서버 로그인 수
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_BATTLE_LOGIN);
		//	마스터 배틀 서버 대기방
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_BATTLE_STANDBY_ROOM);
	}

	return;
}

bool CMasterServer::LanMonitorSendPacket(BYTE DataType)
{
	struct tm *pTime = new struct tm;
	time_t Now;
	localtime_s(pTime, &Now);
	_TimeStamp = time(NULL);
	WORD Type = en_PACKET_SS_MONITOR_DATA_UPDATE;

	switch (DataType)
	{
		//	마스터 서버 ON 전송
	case dfMONITOR_DATA_TYPE_MASTER_SERVER_ON:
	{
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_On << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	마스터 CPU 사용률 전송 ( 프로세스 )
	case dfMONITOR_DATA_TYPE_MASTER_CPU:
	{
		_MasterServer_CPU_Process = _Cpu.ProcessTotal();
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_CPU_Process << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	마스터 CPU 사용률 전송 ( 서버 컴퓨터 전체 )
	case dfMONITOR_DATA_TYPE_MASTER_CPU_SERVER:
	{
		_MasterServer_CPU_All = _Cpu.ProcessTotal();
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_CPU_All << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	마스터 메모리 유저 커밋 사용량 ( Private ) MByte
	case dfMONITOR_DATA_TYPE_MASTER_MEMORY_COMMIT:
	{
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_MemoryCommit << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	마스터 패킷풀 사용량
	case dfMONITOR_DATA_TYPE_MASTER_PACKET_POOL:
	{
		_MasterServer_PacketPool = CPacket::m_pMemoryPool->_UseCount;
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_PacketPool << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	마스터 매치메이킹 서버 연결 수
	case dfMONITOR_DATA_TYPE_MASTER_MATCH_CONNECT:
	{
		_MasterServer_Match_Connect = _pMatchMaster->_iConnectClient;
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_Match_Connect << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	마스터 매치메이킹 서버 로그인 수
	case dfMONITOR_DATA_TYPE_MASTER_MATCH_LOGIN:
	{
		_MasterServer_Match_Login = _pMatchMaster->_iLoginClient;
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_Match_Login << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	마스터 대기자 클라이언트
	case dfMONITOR_DATA_TYPE_MASTER_STAY_CLIENT:
	{
		_MasterServer_Stay_Client = _pMatchMaster->GetClientCount();
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_Stay_Client << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	마스터 배틀 서버 연결 수
	case dfMONITOR_DATA_TYPE_MASTER_BATTLE_CONNECT:
	{

	}
	break;
		//	마스터 배틀 서버 로그인 수
	case dfMONITOR_DATA_TYPE_MASTER_BATTLE_LOGIN:
	{

	}
	break;
		//	마스터 배틀 서버 대기방
	case dfMONITOR_DATA_TYPE_MASTER_BATTLE_STANDBY_ROOM:
	{

	}
	break;
	}


	return true;
}