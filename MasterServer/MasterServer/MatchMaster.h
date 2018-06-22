#ifndef _MASTERSERVER_LIB_MATCHMASTER_H_
#define _MASTERSERVER_LIB_MATCHMASTER_H_

#include "LanServer.h"
#include "MasterServer.h"

class CMasterServer;

class CMatchMaster
{
public:
	CMatchMaster();
	~CMatchMaster();

	void				Disconnect(unsigned __int64 iClientID);
	/*virtual void		OnClientJoin(st_SessionInfo *pInfo) = 0;
	virtual void		OnClientLeave(unsigned __int64 iClientID) = 0;
	virtual void		OnConnectionRequest(WCHAR * pClientIP, int iPort) = 0;
	virtual void		OnError(int iErrorCode, WCHAR *pError) = 0;*/
	unsigned __int64	GetClientCount();

	bool				Set(CMasterServer *pMasterServer);
	bool				ServerStart(WCHAR *pOpenIP, int iPort, int iMaxWorkerThread,
		bool bNodelay, int iMaxSession);
	bool				ServerStop();
	bool				SendPacket(unsigned __int64 iClientID, CPacket *pPacket);
	bool				GetShutDownMode() { return _bShutdown; }
	bool				GetWhiteIPMode() { return _bWhiteIPMode; }
	bool				SetShutDownMode(bool bFlag);
	bool				SetWhiteIPMode(bool bFlag);

	LANSESSION*			SessionAcquireLock(unsigned __int64 iClientID);
	void				SessionAcquireFree(LANSESSION *pSession);

private:
	bool				ServerInit();
	bool				ClientShutdown(LANSESSION *pSession);
	bool				ClientRelease(LANSESSION *pSession);

	static unsigned int WINAPI WorkerThread(LPVOID arg)
	{
		CMatchMaster *_pWorkerThread = (CMatchMaster *)arg;
		if (_pWorkerThread == NULL)
		{
			wprintf(L"[Server :: WorkerThread]	Init Error\n");
			return false;
		}
		_pWorkerThread->WorkerThread_Update();
		return true;
	}

	static unsigned int WINAPI AcceptThread(LPVOID arg)
	{
		CMatchMaster *_pAcceptThread = (CMatchMaster*)arg;
		if (_pAcceptThread == NULL)
		{
			wprintf(L"[Server :: AcceptThread]	Init Error\n");
			return false;
		}
		_pAcceptThread->AcceptThread_Update();
		return true;
	}

	void				PutIndex(unsigned __int64 iIndex);
	void				WorkerThread_Update();
	void				AcceptThread_Update();
	void				StartRecvPost(LANSESSION *pSession);
	void				RecvPost(LANSESSION *pSession);
	void				SendPost(LANSESSION *pSession);
	void				CompleteRecv(LANSESSION *pSession, DWORD dwTransfered);
	void				CompleteSend(LANSESSION *pSession, DWORD dwTransfered);
	bool				OnRecv(LANSESSION *pSession, CPacket *pPacket);
	unsigned __int64*	GetIndex();

public:
	unsigned __int64		_iAcceptTPS;
	unsigned __int64		_iAcceptTotal;
	unsigned __int64		_iRecvPacketTPS;
	unsigned __int64		_iSendPacketTPS;
	unsigned __int64		_iConnectClient;

	SERVERINFO				_ServerInfo[LAN_SERVER_NUM];

private:
	CLockFreeStack<UINT64*>	_SessionStack;
	LANCOMPARE				*_pIOCompare;
	LANSESSION				*_pSessionArray;
	SOCKET					_listensock;

	HANDLE					_hIOCP;
	HANDLE					_hWorkerThread[100];
	HANDLE					_hAcceptThread;
	HANDLE					_hMonitorThread;
	HANDLE					_hAllthread[200];

	bool					_bWhiteIPMode;
	bool					_bShutdown;

	unsigned __int64		_iAllThreadCnt;
	unsigned __int64		*_pIndex;
	unsigned __int64		_iClientIDCnt;

	CMasterServer			*_pMaster;
};



#endif

