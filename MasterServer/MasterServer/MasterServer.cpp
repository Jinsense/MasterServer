#include "MasterServer.h"

CMasterServer::CMasterServer()
{

}

CMasterServer::~CMasterServer()
{

}

bool CMasterServer::GetShutDownMode()
{

	return true;
}

bool CMasterServer::SetShutDownMode()
{

	return true;
}

bool CMasterServer::GetMonitorMode()
{

	return true;
}

bool CMasterServer::SetMonitorMode(bool type)
{

	return true;
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

	return;
}

void CMasterServer::LanMonitoringThread_Update()
{

	return;
}