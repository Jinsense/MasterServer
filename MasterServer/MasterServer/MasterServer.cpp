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
//	InitializeSRWLock(&_WaitRoom_lock);
//	InitializeSRWLock(&_FullRoom_lock);
	InitializeSRWLock(&_RoomPlayer_lock);
	
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
	bool Find = false;
	std::map<long, BattleServer*>::iterator iter;
	AcquireSRWLockExclusive(&_BattleServer_lock);
	iter = _BattleServerMap.find(ServerNo);
	if (iter == _BattleServerMap.end())
		Find = false;
	else
		Find = true;
	ReleaseSRWLockExclusive(&_BattleServer_lock);

	if (false == Find)
		return nullptr;
	else
		return (*iter).second;
}

CLIENT* CMasterServer::FindClientKey(UINT64 ClientKey)
{
	bool Find = false;
	std::map<UINT64, CLIENT*>::iterator iter;
	AcquireSRWLockExclusive(&_ClientKey_lock);
	iter = _ClientKeyMap.find(ClientKey);
	if (iter == _ClientKeyMap.end())
		Find = false;
	else
		Find = true;
	ReleaseSRWLockExclusive(&_ClientKey_lock);

	if (false == Find)
		return nullptr;
	else
		return (*iter).second;
}

void CMasterServer::MonitorPrintThead_Update()
{
	//	����͸� �׸� ��� ������
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
			wprintf(L"	Master CPU (Process)		:	%d\n", _MasterServer_CPU_Process);
			wprintf(L"	Master CPU ( ALL )		:	%d\n", _MasterServer_CPU_All);
			wprintf(L"	Master Memory UserCommit	:	%d\n", _MasterServer_MemoryCommit);
			wprintf(L"	Master PacketPool AllocCount	:	%d\n", CPacket::m_pMemoryPool->GetAllocCount());
			wprintf(L"	Master PacketPool UseCount	:	%d\n", CPacket::m_pMemoryPool->_UseCount);
			wprintf(L"	Master ClientPool AllocCount	:	%d\n", _ClientPool->GetAllocCount());
			wprintf(L"	Master ClientPool UseCount	:	%d\n", _ClientPool->GetUseCount());
			wprintf(L"	ClientKeyMap Size		:	%d\n\n", _ClientKeyMap.size());
			wprintf(L"	MatchMaking Server Connect	:	%d\n", _pMatchMaster->_iConnectClient);
			wprintf(L"	MatchMaking Server Login	:	%d\n", _pMatchMaster->_iLoginClient);
			wprintf(L"	MatchMaking Wait Client		:	%d\n\n", _MasterServer_Stay_Client);
			wprintf(L"	Battle Server Connect		:	%d\n", _pBattleMaster->_iConnectClient);
			wprintf(L"	Battle Server Login		:	%d\n", _pBattleMaster->_iLoginClient);
			wprintf(L"	Battle Server WaitRoomMap	:	%d\n", _WaitRoomMap.size());
			wprintf(L"	Battle Server FullRoomMap	:	%d\n\n", _FullRoomMap.size());
		}
	}
	delete t;
	
	return;
}

void CMasterServer::LanMonitoringThread_Update()
{
	//-------------------------------------------------------------
	//	����͸� ������ ������ �׸� ������Ʈ �� ����
	//-------------------------------------------------------------
	while (1)
	{
		Sleep(1000);
		PdhCollectQueryData(_CpuQuery);
		PdhGetFormattedCounterValue(_ProcessPrivateBytes, PDH_FMT_DOUBLE, NULL, &_CounterVal);
		_MasterServer_MemoryCommit = (int)_CounterVal.doubleValue / (1024 * 1024);
		if (false == _pMonitor->IsConnect())
		{
			//	���� ���°� �ƴ� ��� ������ �õ�
			_pMonitor->Connect(_Config.MONITOR_IP, _Config.MONITOR_PORT, true, LANCLIENT_WORKERTHREAD);
			continue;
		}
		//-------------------------------------------------------------
		//	����͸� ������ ������ ��Ŷ ���� �� ����
		//-------------------------------------------------------------
		//	������ ���� ON ����
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_SERVER_ON);
		//	������ CPU ���� ���� ( ���μ��� )
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_CPU);
		//	������ CPU ���� ���� ( ���� ��ǻ�� ��ü )
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_CPU_SERVER);
		//	������ �޸� ���� Ŀ�� ��뷮 ( Private ) MByte
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_MEMORY_COMMIT);
		//	������ ��ŶǮ ��뷮
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_PACKET_POOL);
		//	������ ��ġ����ŷ ���� ���� ��
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_MATCH_CONNECT);
		//	������ ��ġ����ŷ ���� �α��� ��
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_MATCH_LOGIN);
		//	������ ����� Ŭ���̾�Ʈ
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_STAY_CLIENT);
		//	������ ��Ʋ ���� ���� ��
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_BATTLE_CONNECT);
		//	������ ��Ʋ ���� �α��� ��
		LanMonitorSendPacket(dfMONITOR_DATA_TYPE_MASTER_BATTLE_LOGIN);
		//	������ ��Ʋ ���� ����
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
		//	������ ���� ON ����
	case dfMONITOR_DATA_TYPE_MASTER_SERVER_ON:
	{
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_On << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	������ CPU ���� ���� ( ���μ��� )
	case dfMONITOR_DATA_TYPE_MASTER_CPU:
	{
		_MasterServer_CPU_Process = _Cpu.ProcessTotal();
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_CPU_Process << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	������ CPU ���� ���� ( ���� ��ǻ�� ��ü )
	case dfMONITOR_DATA_TYPE_MASTER_CPU_SERVER:
	{
		_MasterServer_CPU_All = _Cpu.ProcessTotal();
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_CPU_All << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	������ �޸� ���� Ŀ�� ��뷮 ( Private ) MByte
	case dfMONITOR_DATA_TYPE_MASTER_MEMORY_COMMIT:
	{
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_MemoryCommit << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	������ ��ŶǮ ��뷮
	case dfMONITOR_DATA_TYPE_MASTER_PACKET_POOL:
	{
		_MasterServer_PacketPool = CPacket::m_pMemoryPool->_UseCount;
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_PacketPool << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	������ ��ġ����ŷ ���� ���� ��
	case dfMONITOR_DATA_TYPE_MASTER_MATCH_CONNECT:
	{
		_MasterServer_Match_Connect = _pMatchMaster->_iConnectClient;
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_Match_Connect << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	������ ��ġ����ŷ ���� �α��� ��
	case dfMONITOR_DATA_TYPE_MASTER_MATCH_LOGIN:
	{
		_MasterServer_Match_Login = _pMatchMaster->_iLoginClient;
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_Match_Login << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	������ ����� Ŭ���̾�Ʈ
	case dfMONITOR_DATA_TYPE_MASTER_STAY_CLIENT:
	{
		_MasterServer_Stay_Client = _pMatchMaster->GetClientCount();
		CPacket *pPacket = CPacket::Alloc();
		*pPacket << Type << DataType << _MasterServer_Stay_Client << _TimeStamp;
		_pMonitor->SendPacket(pPacket);
		pPacket->Free();
	}
	break;
		//	������ ��Ʋ ���� ���� ��
	case dfMONITOR_DATA_TYPE_MASTER_BATTLE_CONNECT:
	{

	}
	break;
		//	������ ��Ʋ ���� �α��� ��
	case dfMONITOR_DATA_TYPE_MASTER_BATTLE_LOGIN:
	{

	}
	break;
		//	������ ��Ʋ ���� ����
	case dfMONITOR_DATA_TYPE_MASTER_BATTLE_STANDBY_ROOM:
	{

	}
	break;
	}


	return true;
}