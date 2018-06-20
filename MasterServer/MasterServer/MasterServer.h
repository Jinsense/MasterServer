#ifndef _MASTERSERVER_SERVER_MASTER_H_
#define _MASTERSERVER_SERVER_MASTER_H_


#include "Config.h"
#include "Log.h"
#include "CpuUsage.h"
#include "EtherNet_PDH.h"
#include "LanClient.h"
#include "MatchMaster.h"
#include "BattleMaster.h"

class CMasterServer
{
public:
	CMasterServer();
	~CMasterServer();

	//-----------------------------------------------------------
	// 콘솔 창 제어 함수
	//-----------------------------------------------------------
	bool	GetShutDownMode();
	bool	SetShutDownMode();
	bool	GetMonitorMode();
	bool	SetMonitorMode(bool type);

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
	CMatchMaster *	_pMatchMaster;
	CBattleMaster *	_pBattleMaster;
	CLanClient *	_pMonitor;
	CSystemLog *	_pLog;
	CConfig			_Config;

protected:
	SRWLOCK			_srwlock;


};


#endif // !_MASTERSERVER_SERVER_MASTER_H_

