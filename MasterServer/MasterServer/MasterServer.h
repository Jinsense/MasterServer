#ifndef _MASTERSERVER_SERVER_MASTER_H_
#define _MASTERSERVER_SERVER_MASTER_H_

#include "Config.h"
#include "Log.h"
#include "CpuUsage.h"
#include "EtherNet_PDH.h"
#include "LanClient.h"
#include "LanServer.h"
#include "BattleMaster.h"
#include "MatchMaster.h"

class CMatchMaster;
class CBattleMaster;

class CMasterServer
{
public:
	CMasterServer();
	~CMasterServer();

	//-----------------------------------------------------------
	// �ܼ� â ���� �Լ�
	//-----------------------------------------------------------
	bool	GetShutDownMode();
	void	SetShutDownMode();
	bool	GetMonitorMode();
	void	SetMonitorMode(bool type);

	//-----------------------------------------------------------
	// ����� ���� �Լ�
	//-----------------------------------------------------------
	BattleServer*	FindBattleServerNo(int ServerNo);
	CLIENT*			FindClientKey(UINT64 ClientKey);
	bool			LanMonitorSendPacket(BYTE DataType);

private:
	static unsigned int WINAPI MonitorPrintThread(LPVOID arg)
	{
		CMasterServer *_pMonitorPrintThread = (CMasterServer *)arg;
		if (NULL == _pMonitorPrintThread)
		{
			wprintf(L"[Server :: MonitorPrintThread] Init Error\n");
			return false;
		}
		_pMonitorPrintThread->MonitorPrintThead_Update();
		return true;
	}

	static unsigned int WINAPI LanMonitoringThread(LPVOID arg)
	{
		CMasterServer *_pLanMonitoringThread = (CMasterServer *)arg;
		if (NULL == _pLanMonitoringThread)
		{
			wprintf(L"[Server :: LanMonitoringThread] Init Error\n");
			return false;
		}
		_pLanMonitoringThread->LanMonitoringThread_Update();
		return true;
	}
	//-----------------------------------------------------------
	// ȭ�� ��� ������ / ����͸� ���� ������
	//-----------------------------------------------------------
	void	MonitorPrintThead_Update();
	void	LanMonitoringThread_Update();

public:
	CBattleMaster *	_pBattleMaster;
	CMatchMaster *	_pMatchMaster;
	CLanClient *	_pMonitor;
	CSystemLog *	_pLog;
	CConfig			_Config;

	//-----------------------------------------------------------
	// ��Ī���� ��ȣ ����
	//-----------------------------------------------------------
	std::map<int, LANSESSION*>	_MatchServerNoMap;
	SRWLOCK		_MatchServerNo_lock;
	//-----------------------------------------------------------
	// Ŭ���̾�Ʈ ����
	//-----------------------------------------------------------
	CMemoryPool<CLIENT> *_ClientPool;
	std::map<UINT64, CLIENT*>	_ClientKeyMap;
	SRWLOCK		_ClientKey_lock;
	//-----------------------------------------------------------
	// ��Ʋ���� ����
	//-----------------------------------------------------------
	long		_BattleServerNo;
	std::map<long, BattleServer*>	_BattleServerMap;
	SRWLOCK		_BattleServer_lock;
	//-----------------------------------------------------------
	// ��Ʋ���� �� ����
	//-----------------------------------------------------------
	std::map<int, BattleRoom*> _WaitRoomMap;
	std::map<int, BattleRoom*> _FullRoomMap;
	SRWLOCK		_WaitRoom_lock;
	SRWLOCK		_FullRoom_lock;
	SRWLOCK		_RoomPlayer_lock;
	//-----------------------------------------------------------
	// ����͸� ����
	//-----------------------------------------------------------
	bool	_bShutDown;
	bool	_bMonitorMode;
	__int64	_BattleRoomEnterFail;			//	��Ʋ���� �� ���� �ϱ� �� ����

	int		_TimeStamp;						//	TimeStamp	
	int		_MasterServer_On;				//	������ ���� ON
	int		_MasterServer_CPU_Process;		//	������ CPU ���� ( ���μ��� )
	int		_MasterServer_CPU_All;			//	������ CPU ���� ( ���� ��ǻ�� ��ü )
	int		_MasterServer_MemoryCommit;		//	������ �޸� ���� Ŀ�� ��뷮 ( Private ) MByte
	int		_MasterServer_PacketPool;		//	������ ��ŶǮ ��뷮
	int		_MasterServer_Match_Connect;	//	������ ��ġ����ŷ ���� ���� ��
	int		_MasterServer_Match_Login;		//	������ ��ġ����ŷ ���� �α��� ��
	int		_MasterServer_Stay_Client;		//	������ ����� Ŭ���̾�Ʈ
	int		_MasterServer_Battle_Connect;	//	������ ��Ʋ ���� ���� ��
	int		_MasterServer_Battle_Login;		//	������ ��Ʋ ���� �α��� ��
	int		_MasterServer_Battle_WaitRoom;	//	������ ��Ʋ ���� ����

	CCpuUsage	_Cpu;
	HANDLE		_hMonitorSendThread;
	HANDLE		_hMonitorPrintThread;

	PDH_HQUERY	_CpuQuery;
	PDH_HCOUNTER	_MemoryAvailableMBytes;
	PDH_HCOUNTER	_MemoryNonpagedBytes;
	PDH_HCOUNTER	_ProcessPrivateBytes;
	PDH_FMT_COUNTERVALUE _CounterVal;
};


#endif // !_MASTERSERVER_SERVER_MASTER_H_

