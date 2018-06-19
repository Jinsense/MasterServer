#include <conio.h>

#include "Config.h"
#include "MasterServer.h"

int main()
{
	SYSTEM_INFO SysInfo;
	CMasterServer Server;

	GetSystemInfo(&SysInfo);

	//-----------------------------------------------------------
	// Config ���� �ҷ�����
	//-----------------------------------------------------------
	if (false == Server._Config.Set())
	{
		wprintf(L"[MasterServer :: Main]	Config Error\n");
		return false;
	}

	//-----------------------------------------------------------
	// ��Ī���� �����ͼ��� Start
	//-----------------------------------------------------------
	if (false == Server._pMatchMaster->ServerStart(Server._Config.MATCH_BIND_IP, Server._Config.MATCH_BIND_PORT, Server._Config.WORKER_THREAD, true, Server._Config.CLIENT_MAX))
	{
		wprintf(L"[MasterServer :: Main]	MatchMaster Server Start Error\n");
		return false;
	}
	//-----------------------------------------------------------
	// ��Ʋ���� �����ͼ��� Start
	//-----------------------------------------------------------
	if (false == Server._pBattleMaster->ServerStart(Server._Config.BATTLE_BIND_IP, Server._Config.BATTLE_BIND_PORT, Server._Config.WORKER_THREAD, true, Server._Config.CLIENT_MAX))
	{
		wprintf(L"[MasterServer :: Main]	BattleMaster Server Start Error\n");
		return false;
	}
	//-----------------------------------------------------------
	// ����͸� ���� Connect
	//-----------------------------------------------------------
	if (false == Server._pMonitor->Connect(Server._Config.MONITOR_IP, Server._Config.MONITOR_PORT, true, LANCLIENT_WORKERTHREAD))
	{
		wprintf(L"[MatchServer :: Main] Monitoring Connect Error\n");
		return false;
	}
	//-----------------------------------------------------------
	// �ܼ� â ����
	//-----------------------------------------------------------
	while (!Server.GetShutDownMode())
	{
		char ch = _getch();
		switch (ch)
		{
		case 'q': case 'Q':
		{
			break;
		}
		case 'm': case 'M':
		{
			if (false == Server.GetMonitorMode())
			{
				Server.SetMonitorMode(true);
				wprintf(L"[MasterServer :: Main] MonitorMode Start\n");
			}
			else
			{
				Server.SetMonitorMode(false);
				wprintf(L"[MasterServer :: Main] MonitorMode Start\n");
			}
		}
		default:
			break;
		}
	}
	return 0;
}
