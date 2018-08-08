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
	// 콘솔 창 제어 함수
	//-----------------------------------------------------------
	bool	GetShutDownMode();
	void	SetShutDownMode();
	bool	GetMonitorMode();
	void	SetMonitorMode(bool type);

	//-----------------------------------------------------------
	// 사용자 정의 함수
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
	// 화면 출력 스레드 / 모니터링 전송 스레드
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
	// 매칭서버 번호 관리
	//-----------------------------------------------------------
	std::map<int, LANSESSION*>	_MatchServerNoMap;
	SRWLOCK		_MatchServerNo_lock;
	//-----------------------------------------------------------
	// 클라이언트 관리
	//-----------------------------------------------------------
	CMemoryPool<CLIENT> *_ClientPool;
	std::map<UINT64, CLIENT*>	_ClientKeyMap;
	SRWLOCK		_ClientKey_lock;
	//-----------------------------------------------------------
	// 배틀서버 관리
	//-----------------------------------------------------------
	long		_BattleServerNo;
	std::map<long, BattleServer*>	_BattleServerMap;
	SRWLOCK		_BattleServer_lock;
	//-----------------------------------------------------------
	// 배틀서버 방 관리
	//-----------------------------------------------------------
	std::map<int, BattleRoom*> _WaitRoomMap;
	std::map<int, BattleRoom*> _FullRoomMap;
	SRWLOCK		_WaitRoom_lock;
	SRWLOCK		_FullRoom_lock;
	SRWLOCK		_RoomPlayer_lock;
	//-----------------------------------------------------------
	// 모니터링 변수
	//-----------------------------------------------------------
	bool	_bShutDown;
	bool	_bMonitorMode;
	__int64	_BattleRoomEnterFail;			//	배틀서버 방 입장 하기 전 끊김

	int		_TimeStamp;						//	TimeStamp	
	int		_MasterServer_On;				//	마스터 서버 ON
	int		_MasterServer_CPU_Process;		//	마스터 CPU 사용률 ( 프로세스 )
	int		_MasterServer_CPU_All;			//	마스터 CPU 사용률 ( 서버 컴퓨터 전체 )
	int		_MasterServer_MemoryCommit;		//	마스터 메모리 유저 커밋 사용량 ( Private ) MByte
	int		_MasterServer_PacketPool;		//	마스터 패킷풀 사용량
	int		_MasterServer_Match_Connect;	//	마스터 매치메이킹 서버 연결 수
	int		_MasterServer_Match_Login;		//	마스터 매치메이킹 서버 로그인 수
	int		_MasterServer_Stay_Client;		//	마스터 대기자 클라이언트
	int		_MasterServer_Battle_Connect;	//	마스터 배틀 서버 연결 수
	int		_MasterServer_Battle_Login;		//	마스터 배틀 서버 로그인 후
	int		_MasterServer_Battle_WaitRoom;	//	마스터 배틀 서버 대기방

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

