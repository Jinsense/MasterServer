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

