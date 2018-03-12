#pragma once

#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"

#include "CharacterClass.h"
#include "WarriorClass.h"

enum NetworkState
{
	NS_Init = 0,
	NS_PendingStart,
	NS_Started,
	NS_Pending,
	NS_Lobby,
	NS_WaitingPlayers,
	NS_GameStarted,
};

enum PlayerStatus
{
	PS_Connected = 0,
	PS_Playing,
	PS_Dead,
	PS_Disconnected
};

enum
{
	ID_THEGAME_LOBBY_READY = ID_USER_PACKET_ENUM,
	ID_PLAYER_READY,
	ID_THEGAME_START,
	ID_PLAYER_TURN,
	ID_YOUR_TURN,
	ID_PLAYER_ATTACK,
	ID_PLAYER_DEFEND,
	ID_PLAYER_HEAL,
};

struct SPlayer
{
	unsigned int m_number;
	RakNet::SystemAddress m_address;

	//unsigned int m_health;
	EPlayerClass m_class;
	PlayerStatus m_status;
	CharacterClass* m_character;


	//function to send a packet with name/health/class etc
	void SendName(RakNet::SystemAddress systemAddress, bool isBroadcast)
	{
		RakNet::BitStream writeBs;
		writeBs.Write((RakNet::MessageID)ID_PLAYER_READY);
		//RakNet::RakString name(m_name.c_str());
		//writeBs.Write(name);

		//returns 0 when something is wrong
//		assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemAddress, isBroadcast));
	}
};
void cls() //windows h function to replace screen with nulls
{
	DWORD n;
	DWORD size;
	COORD coord = { 0 };
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(h, &csbi);
	size = csbi.dwSize.X * csbi.dwSize.Y;
	FillConsoleOutputCharacter(h, TEXT(' '), size, coord, &n);
	GetConsoleScreenBufferInfo(h, &csbi);
	FillConsoleOutputAttribute(h, csbi.wAttributes, size, coord, &n);
	SetConsoleCursorPosition(h, coord);
}