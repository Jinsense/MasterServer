#ifndef _MASTER_LIB_LANSERVER_H_
#define _MASTER_LIB_LANSERVER_H_

#include <map>
#include <list>

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winmm.lib")

#include "Packet.h"
#include "RingBuffer.h"
#include "MemoryPool.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"


#define		LAN_WSABUF_NUMBER		100
#define		LAN_QUEUE_SIZE			10000
#define		LAN_HEADER_SIZE			2
#define		LAN_MONITOR_NUM			42
#define		LAN_SERVER_NUM			10

#define		SET_INDEX(Index, SessionKey)		Index = Index << 48; SessionKey = Index | SessionKey;
#define		GET_INDEX(Index, SessionKey)		Index = SessionKey >> 48;

typedef struct st_MONITOR
{
	BYTE Type;
	int Value;
	int TimeStamp;
	int Min;
	int Max;
	float Avr;
	bool Recv;
	st_MONITOR()
	{
		Type = NULL, Value = NULL, TimeStamp = NULL, Min = 0xffffffff, Max = 0, Avr = NULL, Recv = false;
	}
}MONITOR;

typedef struct st_ServerInfo
{
	bool Login;
	char IP[20];
	USHORT Port;
	st_ServerInfo()
	{
		Login = false, ZeroMemory(&IP, sizeof(IP)), Port = NULL;
	}
}SERVERINFO;

typedef struct st_CLIENTINFO
{
	UINT64 ClientKey;
	UINT64 AccountNo;
	WORD BattleServerNo;
	int BattleRoomNo;
	int MatchServerNo;

}CLIENT;

typedef struct st_RELEASE_COMPARE
{
	__int64	iIOCount;
	__int64	iReleaseFlag;

	st_RELEASE_COMPARE() :
		iIOCount(0),
		iReleaseFlag(false) {}
}LANCOMPARE;

typedef struct st_Client
{
	int					ServerNo;
	char				IP[20];
	unsigned short		Port;
	bool				bLoginPacketCheck;	//	로그인 패킷을 통해 로그인이 성공했는지 여부
	bool				bLoginFlag;			//	세션 배열에서 해당 세션이 로그인 상태인지 여부
	bool				bRelease;
	int					Index;
	long				lIOCount;
	long				lSendFlag;
	long				lSendCount;
	unsigned __int64	iClientID;
	SOCKET				sock;
	OVERLAPPED			SendOver;
	OVERLAPPED			RecvOver;
	CRingBuffer			RecvQ;
	CRingBuffer			PacketQ;
	CLockFreeQueue<CPacket*> SendQ;
	st_Client() :
		RecvQ(LAN_QUEUE_SIZE),
		PacketQ(LAN_QUEUE_SIZE),
		lIOCount(0),
		lSendFlag(true),
		bLoginPacketCheck(false)
		{}
}LANSESSION;

typedef struct st_RoomPlayerInfo
{
	UINT64 AccountNo;
	UINT64 ClientKey;
	CLIENT * pClient;
}RoomPlayerInfo;

typedef struct st_BattleRoom
{
	long BattleServerNo;
	int RoomNo;
	int MaxUser;
	long CurUser;
	char EnterToken[32];
	std::list<RoomPlayerInfo> RoomPlayer;
}BattleRoom;

typedef struct st_BattleServer
{
	WCHAR	ServerIP[16];
	WORD	Port;
	char	ConnectToken[32];
	char	MasterToken[32];

	WCHAR	ChatServerIP[16];
	WORD	ChatServerPort;
	WORD	ServerNo;
}BattleServer;

#endif 