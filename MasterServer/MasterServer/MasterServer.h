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
	// �ܼ� â ���� �Լ�
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
	// ȭ�� ��� ������ / ����͸� ���� ������
	//-----------------------------------------------------------
	void	MonitorPrintThead_Update();
	void	LanMonitoringThread_Update();

public:
	CMatchMaster *	_pMatchMaster;
	CBattleMaster *	_pBattleMaster;
	CLanClient *	_pMonitor;
	CSystemLog *	_pLog;
	CConfig			_Config;

	//-----------------------------------------------------------
	// ��Ī���� ��ȣ ����
	//-----------------------------------------------------------
	std::map<int, LANSESSION*>	_MatchServerNoMap;
	SRWLOCK		_MatchServerNo_lock;
	//-----------------------------------------------------------
	// Ŭ���̾�Ʈ Ű ����
	//-----------------------------------------------------------
	std::map<UINT64, UINT64>	_ClientKeyMap;
	SRWLOCK		_ClientKey_lock;
	//-----------------------------------------------------------
	// ��Ʋ���� ����
	//-----------------------------------------------------------
	std::map<int, BattleServer*>	_BattleServerMap;
	SRWLOCK		_BattleServer_lock;
	//-----------------------------------------------------------
	// ��Ʋ���� �� ����
	//-----------------------------------------------------------
	std::list<BattleRoom*>		_RoomList;
	SRWLOCK		_Room_lock;

protected:
	SRWLOCK			_srwlock;

	//-----------------------------------------------------------
	// ����͸� ��� ����
	//-----------------------------------------------------------
	
};


#endif // !_MASTERSERVER_SERVER_MASTER_H_

