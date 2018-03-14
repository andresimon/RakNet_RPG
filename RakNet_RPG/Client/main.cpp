#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "BitStream.h"

#include <iostream>
#include <conio.h> // to use _kbhit() and _getch()
#include <thread>
#include <chrono>
#include <map>
#include <mutex>
#include <vector>
#include <algorithm> // to use find()

#include "Common.h"
#include "CharacterClass.h"
#include "WarriorClass.h"
#include "RogueClass.h"
#include "ClericClass.h"

static unsigned int SERVER_PORT = 65000;
static unsigned int CLIENT_PORT = 65001;

bool isServer = false;
bool isRunning = true;

RakNet::RakPeerInterface *g_rakPeerInterface = nullptr;
RakNet::SystemAddress g_serverAddress;

std::mutex g_networkState_mutex;
NetworkState g_networkState = NS_Init;

std::map<unsigned long, SPlayer> m_players;
EPlayerClass playerClass;
unsigned int playerNumber;

SPlayer& GetPlayer(RakNet::RakNetGUID raknetId)
{
	unsigned long guid = RakNet::RakNetGUID::ToUint32(raknetId);
	std::map<unsigned long, SPlayer>::iterator it = m_players.find(guid);
	assert(it != m_players.end());
	return it->second;
}

void OnLostConnection(RakNet::Packet* packet)
{
	cls();

	std::cout << "Disconnected from RakNet RPG Server." << std::endl;
}

void ShowHeader(RakNet::Packet* packet)
{ 
	cls();

	std::cout << "RakNet RPG" << std::endl;
	std::cout << "You are connected at RakNet RPG Server on " << packet->systemAddress.ToString(true) << std::endl;

	if ( playerNumber > 0 )
		std::cout << "You are player " << playerNumber << " - " << CharacterClass::GetCharacterClassName(playerClass) << std::endl;

	if (g_networkState == NS_WaitingPlayers)
	{
		std::cout << std::endl;
		std::cout << "Waiting for other players to join " << std::endl;
		std::cout << std::endl;
	}

}

void ShowPlayersTurn(RakNet::Packet* packet)
{
	unsigned int playerNum;
	std::string lastPlayerAction;

	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	bs.Read(playerNum);
	RakNet::RakString status;
	bs.Read(status);
	RakNet::RakString _lastPlayerAction;
	bs.Read(_lastPlayerAction);
	lastPlayerAction = _lastPlayerAction.C_String();

	ShowHeader(packet);

	std::cout << std::endl;

	if (!lastPlayerAction.empty())
		std::cout << lastPlayerAction ;

	std::cout << status.C_String() << std::endl;

	std::cout << "PLAYER " << playerNum << " TURN" << std::endl;
}

EPlayerClass SelectClass()
{
	EPlayerClass selectedClass;

	std::cout << std::endl;
	std::cout << "Select the class you wish to use, based on the information below: " << std::endl;
	std::cout << "<1> - Warrior -> Health: High, Damage: Medium, Defense: Medium " << std::endl;
	std::cout << "<2> - Rogue   -> Health: Low, Damage : High, Defense : High " << std::endl;
	std::cout << "<3> - Cleric  -> Health: Medium, Damage : Low, Defense : Medium *CAN HEAL* " << std::endl;
	std::cout << std::endl;

	char option = ' ';
	unsigned int iOption = 0;

	while ( iOption < 1 || iOption > 3 )
	{
		iOption = 0;
		option = _getch();

		iOption = option - '0';
	}

	switch (iOption)
	{
		case 1:
			selectedClass = EPlayerClass::Warrior;
			playerClass = EPlayerClass::Warrior;
			break;
		case 2:
			selectedClass = EPlayerClass::Rogue;
			playerClass = EPlayerClass::Rogue;

			break;
		case 3:
			selectedClass = EPlayerClass::Cleric;
			playerClass = EPlayerClass::Cleric;
			break;
		default:
			playerClass = EPlayerClass::EPlayerClass_ERROR;
			break;
	}

	return selectedClass;
}

void OnConnectionAccepted(RakNet::Packet* packet)
{
	cls();
	ShowHeader(packet);

	g_networkState_mutex.lock();
	g_networkState = NS_Lobby;
	g_networkState_mutex.unlock();
	g_serverAddress = packet->systemAddress;
}

void SetPlayerReady(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	bs.Read(playerNumber);

	ShowHeader(packet);
}

void OnGameStarted(RakNet::Packet* packet)
{
	g_networkState_mutex.lock();
	g_networkState = NS_GameStarted;
	g_networkState_mutex.unlock();
}

void OnGameOver(RakNet::Packet* packet)
{
	unsigned int winner;
	std:

	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString status;
	bs.Read(status);
	bs.Read(winner);

	ShowHeader(packet);

	std::cout << status.C_String() << std::endl;

	std::cout << std::endl;
	std::cout << "GAME OVER" << std::endl;
	std::cout << std::endl;
	std::cout << "Player: " << winner << " has Won." << std::endl;

	g_networkState_mutex.lock();
	g_networkState = NS_GameOver;
	g_networkState_mutex.unlock();

}

unsigned char GetPacketIdentifier(RakNet::Packet *packet)
{
	if (packet == nullptr)
		return 255;

	if ((unsigned char)packet->data[0] == ID_TIMESTAMP)
	{
		RakAssert(packet->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
		return (unsigned char)packet->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
	}
	else
		return (unsigned char)packet->data[0];
}

void InputHandler()
{
	while (isRunning)
	{
		EPlayerClass playerClass;

		char userInput[255];

		if (g_networkState == NS_Lobby)
		{
			assert(strcmp(userInput, "quit"));

			playerClass = SelectClass();
			if (playerClass == EPlayerClass_ERROR)
			{
				g_networkState_mutex.lock();
				g_networkState = NS_Pending;
				g_networkState_mutex.unlock();
			}
			else
			{
				RakNet::BitStream bs;
				bs.Write((RakNet::MessageID)ID_THEGAME_LOBBY_READY);
				bs.Write(playerClass);

				//returns 0 when something is wrong
				assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));

				g_networkState_mutex.lock();
				g_networkState = NS_WaitingPlayers;
				g_networkState_mutex.unlock();
			}
		}
		else if (g_networkState == NS_Pending)
		{
			static bool doOnce = false;
			if (!doOnce)
				std::cout << "pending..." << std::endl;

			doOnce = true;
		}
		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
}

unsigned int ChooseTarget(RakNet::Packet* packet)
{
	unsigned int numValidTargets = 0;
	std::vector<unsigned int> validTargets;
	
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	bs.Read(numValidTargets);

	// Get valid targets
	for (unsigned int x = 0; x < numValidTargets; x++)
	{
		unsigned int playerNum;
		bs.Read(playerNum);
		validTargets.push_back(playerNum);
	}

	std::cout << "Choose the target player to attack: ";
	for (std::vector<unsigned int>::iterator it = validTargets.begin(); it != validTargets.end(); ++it)
		std::cout << *it << " ";
	std::cout << std::endl;

	char option = ' ';
	unsigned int iOption = 0;

	while (true)
	{
		iOption = 0;
		option = _getch();
		iOption = option - '0';

		if ( std::find(validTargets.begin(), validTargets.end(), iOption) != validTargets.end() )
			break;
	}

	return iOption;
}

void OnPlayerTurn(RakNet::Packet* packet)
{
	EPlayerAction action;
	RakNet::MessageID messageID;

	unsigned int targetPlayer = 0;

	std::cout << std::endl << "ITS YOUR TURN NOW" << std::endl;

	switch (playerClass)
	{
		case Warrior:
			action = WarriorClass::DoTurn();
			break;

		case Rogue:
			action = RogueClass::DoTurn();
			break;

		case Cleric:
			action = ClericClass::DoTurn();
			break;

		case EPlayerClass_ERROR:
			break;
		default:
			break;
	}

	switch (action)
	{
		case Attack:
			targetPlayer = ChooseTarget(packet);
			messageID = ID_PLAYER_ATTACK;
			break;

		case Defend:
			messageID = ID_PLAYER_DEFEND;
			break;

		case Heal:
			messageID = ID_PLAYER_HEAL;
			break;

		default:
			break;
	}

	RakNet::BitStream writeBs;
	writeBs.Write((RakNet::MessageID)messageID);
	writeBs.Write(targetPlayer);

	//returns 0 when something is wrong
	assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
};

bool HandleLowLevelPackets(RakNet::Packet* packet)
{
	bool isHandled = true;
	// We got a packet, get the identifier with our handy function
	unsigned char packetIdentifier = GetPacketIdentifier(packet);
	char g;
	// Check if this is a network message packet
	switch (packetIdentifier)
	{
	case ID_DISCONNECTION_NOTIFICATION:
		// Connection lost normally
		printf("ID_DISCONNECTION_NOTIFICATION\n");
		 g = _getch();
		break;
	case ID_ALREADY_CONNECTED:
		// Connection lost normally
		printf("ID_ALREADY_CONNECTED with guid %" PRINTF_64_BIT_MODIFIER "u\n", packet->guid);
		break;
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
		break;
	case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server telling the clients of another client disconnecting gracefully.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n");
		g = _getch();

		break;
	case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_CONNECTION_LOST\n");
		g = _getch();

		break;
	case ID_CONNECTION_ATTEMPT_FAILED:
		printf("Connection attempt failed\n");
		break;
	case ID_NO_FREE_INCOMING_CONNECTIONS:
		// Sorry, the server is full.  I don't do anything here but
		// A real app should tell the user
		printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
		break;
		//@
	case ID_CONNECTION_LOST:
		// Couldn't deliver a reliable packet - i.e. the other system was abnormally terminated
		printf("ID_CONNECTION_LOST\n");
		OnLostConnection(packet);
		break;

	case ID_CONNECTION_REQUEST_ACCEPTED:
		// This tells the client they have connected
		//printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", packet->systemAddress.ToString(true), packet->guid.ToString());
		//printf("My external address is %s\n", g_rakPeerInterface->GetExternalID(packet->systemAddress).ToString(true));
		OnConnectionAccepted(packet);
		break;

	case ID_CONNECTED_PING:
	case ID_UNCONNECTED_PING:
		printf("Ping from %s\n", packet->systemAddress.ToString(true));
		break;
	default:
		isHandled = false;
		break;
	}
	return isHandled;
}

void PacketHandler()
{
	while (isRunning)
	{
		for (RakNet::Packet* packet = g_rakPeerInterface->Receive(); packet != nullptr; g_rakPeerInterface->DeallocatePacket(packet), packet = g_rakPeerInterface->Receive())
		{
			if (!HandleLowLevelPackets(packet))
			{
				//our game specific packets
				unsigned char packetIdentifier = GetPacketIdentifier(packet);
				switch (packetIdentifier)
				{
					case ID_PLAYER_READY:
						SetPlayerReady(packet);
						break;

					case ID_THEGAME_START:
						OnGameStarted(packet);
						break;

					case ID_PLAYER_TURN:
						ShowPlayersTurn(packet);
						break;

					case ID_YOUR_TURN:
						OnPlayerTurn(packet);
						break;

					case ID_THEGAME_OVER:
						OnGameOver(packet);
						break;

					default:
						break;
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
}


int main()
{
	g_rakPeerInterface = RakNet::RakPeerInterface::GetInstance();

	std::thread inputHandler(InputHandler);
	std::thread packetHandler(PacketHandler);

	while (isRunning)
	{
		if (g_networkState == NS_Init)
		{
			g_networkState_mutex.lock();
			g_networkState = NS_PendingStart;
			g_networkState_mutex.unlock();

		}else
		if (g_networkState == NS_PendingStart)
		{
			//client
			
				RakNet::SocketDescriptor socketDescriptor(CLIENT_PORT, 0);
				socketDescriptor.socketFamily = AF_INET;

				while (RakNet::IRNS2_Berkley::IsPortInUse(socketDescriptor.port, socketDescriptor.hostAddress, socketDescriptor.socketFamily, SOCK_DGRAM) == true)
					socketDescriptor.port++;

				RakNet::StartupResult result = g_rakPeerInterface->Startup(8, &socketDescriptor, 1);
				assert(result == RakNet::RAKNET_STARTED);

				g_networkState_mutex.lock();
				g_networkState = NS_Started;
				g_networkState_mutex.unlock();

				g_rakPeerInterface->SetOccasionalPing(true);
				//"127.0.0.1" = local host = your machines address
				RakNet::ConnectionAttemptResult car = g_rakPeerInterface->Connect("127.0.0.1", SERVER_PORT, nullptr, 0);
				RakAssert(car == RakNet::CONNECTION_ATTEMPT_STARTED);
				std::cout << "client attempted connection..." << std::endl;

		}

	}

	//std::cout << "press q and then return to exit" << std::endl;
	//std::cin >> userInput;

	inputHandler.join();
	packetHandler.join();
	return 0;
}