#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "BitStream.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <mutex>
#include <conio.h>
#include <sstream>

#include "Common.h"
#include "CharacterClass.h"
#include "WarriorClass.h"
#include "RogueClass.h"
#include "ClericClass.h"

static unsigned int SERVER_PORT = 65000;
static unsigned int const MAX_CONNECTIONS = 2; //4

bool isServer = true;
bool isRunning = true;
bool isDefineTurn = true; // Tells the server to start a new player's turn

RakNet::RakPeerInterface *g_rakPeerInterface = nullptr;
RakNet::SystemAddress g_serverAddress;

std::mutex g_networkState_mutex;
NetworkState g_networkState = NS_Init;

std::map<unsigned long, SPlayer> m_players;
RakNet::RakNetGUID playersIDs[MAX_CONNECTIONS];

unsigned int const		numPlayersRequired  = MAX_CONNECTIONS;
static unsigned int		numPlayersConnected = 0;

SPlayer& GetPlayer(RakNet::RakNetGUID raknetId)
{
	unsigned long guid = RakNet::RakNetGUID::ToUint32(raknetId);
	std::map<unsigned long, SPlayer>::iterator it = m_players.find(guid);
	assert(it != m_players.end());
	return it->second;
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

void ShowHeader()
{
	cls();

	std::cout << "RakNet RPG Server" << std::endl;
	std::cout << std::endl;


//	if (g_networkState == NS_WaitingPlayers)
	//{
		for (unsigned int x = 1; x <= MAX_CONNECTIONS; x++)
		{
			std::string chosenClass = "WAITING";

			if (RakNet::RakNetGUID::ToUint32(playersIDs[x - 1]) != NULL)
			{
				SPlayer& player = GetPlayer(playersIDs[x - 1]);
				if (player.m_class != NULL)
					chosenClass = CharacterClass::GetCharacterClassName(player.m_class);

				std::cout << "Player " << player.m_number << " connected, Class: " << chosenClass << std::endl;
			}
			else
			{
				std::cout << "Player " << x << " WAITING..." << std::endl;
			}
		}
	//}
}

std::string GetPlayerStats()
{
	std::stringstream ss;

	ss << std::endl;

		//player 1 - warrior - 100 hp
		//player 2 - Cleric - 50 hp
		//player 3 - disconnected

		for (unsigned int x = 0; x < MAX_CONNECTIONS; x++)
		{
			if (RakNet::RakNetGUID::ToUint32(playersIDs[x]) != NULL)
			{
				SPlayer& player = GetPlayer(playersIDs[x]);
				ss << "Player " << player.m_number << " - " << player.m_character->GetMyCharacterClassName() << " - " << player.m_character->m_health << " HP" << std::endl;
			}
		}
		
	return ss.str();
}


void OnLostConnection(RakNet::Packet* packet)
{
	std::cout << "Lost connection on Server" << std::endl;

	SPlayer& lostPlayer = GetPlayer(packet->guid);
	playersIDs[lostPlayer.m_number - 1] = (RakNet::RakNetGUID)0;
	//numPlayersConnected--;
	lostPlayer.SendName(RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	unsigned long keyVal = RakNet::RakNetGUID::ToUint32(packet->guid);
	m_players.erase(keyVal);
	ShowHeader();
}

//server
void OnIncomingConnection(RakNet::Packet* packet)
{
	unsigned int availableSlot = 0;
	for (unsigned int x = 0; x <= numPlayersRequired - 1; x++)
	{
		if (RakNet::RakNetGUID::ToUint32(playersIDs[x]) == 0)
		{
			availableSlot = x;
			break;
		}
	}

	//numPlayersConnected++;

	SPlayer& newPlayer = SPlayer();
	newPlayer.m_number = availableSlot + 1;
	newPlayer.m_status = PS_Connected;
	newPlayer.m_address = packet->systemAddress;

	m_players.insert(std::make_pair(RakNet::RakNetGUID::ToUint32(packet->guid), newPlayer));
	playersIDs[availableSlot] = packet->guid;

	SPlayer& connectedPlayer = GetPlayer(packet->guid);
	//ShowWaittingPlayers();
	ShowHeader();
	//std::cout << "Player " << numPlayersConnected << " connected. Class: " << CharacterClass::GetCharacterClassName(connectedPlayer.m_class) << std::endl;
}



//this is on the client side
void DisplayPlayerReady(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString userName;
	bs.Read(userName);

	std::cout << userName.C_String() << " has joined" << std::endl;
}

bool CheckGameStart()
{
	bool isOk = true;

	if (g_networkState == NS_WaitingPlayers)
	{
		if (m_players.size() < numPlayersRequired)
			return false;

		for (std::map<unsigned long, SPlayer>::iterator it = m_players.begin(); it != m_players.end(); ++it)
		{
			SPlayer& player = it->second;
			if (player.m_class == NULL)
			{
				//break;
				return false;
			}
		}

		// Notify all player that game has started
		RakNet::BitStream writeBs;
		writeBs.Write((RakNet::MessageID)ID_THEGAME_START);
		std::string _status = GetPlayerStats();
		RakNet::RakString status(_status.c_str());
		writeBs.Write(status);

		//assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, true));
		assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true));
		
		g_networkState_mutex.lock();
		g_networkState = NS_GameStarted;
		g_networkState_mutex.unlock();

	}
}

void OnLobbyReady(RakNet::Packet* packet)
{
	EPlayerClass playerClass;

	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	bs.Read(playerClass);

	unsigned long guid = RakNet::RakNetGUID::ToUint32(packet->guid);
	SPlayer& player = GetPlayer(packet->guid);
	player.m_class = playerClass;

	// create the character class object for that player
	CharacterClass* character;

	switch (playerClass)
	{
		case Warrior:
			character = new WarriorClass(1);
			player.m_character = character;
			break;

		case Rogue:
			character = new RogueClass(1);
			player.m_character = character;
			break;

		case Cleric:
			character = new ClericClass(1);
			player.m_character = character;
			break;

		default:
			break;
	}

	ShowHeader();

	//notify all other connected players that this plyer has joined the game
	for (std::map<unsigned long, SPlayer>::iterator it = m_players.begin(); it != m_players.end(); ++it)
	{
		//skip over the player who just joined
		if (guid == it->first)
		{
			continue;
		}

		SPlayer& player = it->second;
		player.SendName(packet->systemAddress, false);
		/*RakNet::BitStream writeBs;
		writeBs.Write((RakNet::MessageID)ID_PLAYER_READY);
		RakNet::RakString name(player.m_name.c_str());
		writeBs.Write(name);

		//returns 0 when something is wrong
		assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false));*/
	}

	RakNet::BitStream writeBs;
	writeBs.Write((RakNet::MessageID)ID_PLAYER_READY);
//	RakNet::RakString name(player.m_name.c_str());
	//writeBs.Write(name);
	writeBs.Write(player.m_number);

	//returns 0 when something is wrong
	assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false));

	/*RakNet::BitStream writeBs;
	writeBs.Write((RakNet::MessageID)ID_PLAYER_READY);
	RakNet::RakString name(player.m_name.c_str());
	writeBs.Write(name);

	//returns 0 when something is wrong
	assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false));*/
	//player.SendName(packet->systemAddress, true);
	/*RakNet::BitStream writeBs;
	writeBs.Write((RakNet::MessageID)ID_PLAYER_READY);
	RakNet::RakString name(player.m_name.c_str());
	writeBs.Write(name);
	assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, true));*/

}

void ResolveTurn(RakNet::Packet *packet)
{
	SPlayer& player = GetPlayer(packet->guid);

	unsigned char packetIdentifier = GetPacketIdentifier(packet);
	switch (packetIdentifier)
	{
		case ID_PLAYER_ATTACK:
			//ResolveTurn(packet);
			std::cout << "Player " << player.m_number << "Attacked" << std::endl;
			break;

		case ID_PLAYER_DEFEND:
			//ResolveTurn(packet);
			std::cout << "Player " << player.m_number << "Defend" << std::endl;
			break;

		case ID_PLAYER_HEAL:
			//ResolveTurn(packet);
			std::cout << "Player " << player.m_number << "heal" << std::endl;
			break;

		default:
			break;
	}

	isDefineTurn = true;
}

void InputHandler()
{
	while (isRunning)
	{
		char userInput[255];
		if (g_networkState == NS_Init)
		{
			g_networkState_mutex.lock();
			g_networkState = NS_PendingStart;
			g_networkState_mutex.unlock();

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

bool HandleLowLevelPackets(RakNet::Packet* packet)
{
	bool isHandled = true;
	// We got a packet, get the identifier with our handy function
	unsigned char packetIdentifier = GetPacketIdentifier(packet);

	// Check if this is a network message packet
	switch (packetIdentifier)
	{
	case ID_DISCONNECTION_NOTIFICATION:
		// Connection lost normally
		printf("ID_DISCONNECTION_NOTIFICATION\n");
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
		break;
	case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_CONNECTION_LOST\n");
		break;
	case ID_NEW_INCOMING_CONNECTION:
		//client connecting to server
		OnIncomingConnection(packet);
		//printf("ID_NEW_INCOMING_CONNECTION\n");
		break;
	case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
		OnIncomingConnection(packet);
		//printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
		break;
	case ID_CONNECTION_ATTEMPT_FAILED:
		printf("Connection attempt failed\n");
		break;
	case ID_NO_FREE_INCOMING_CONNECTIONS:
		// Sorry, the server is full.  I don't do anything here but
		// A real app should tell the user
		printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
		break;

	case ID_CONNECTION_LOST:
		// Couldn't deliver a reliable packet - i.e. the other system was abnormally
		// terminated
		printf("ID_CONNECTION_LOST\n");
		OnLostConnection(packet);
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
					case ID_THEGAME_LOBBY_READY:
						OnLobbyReady(packet);
						break;

					case ID_PLAYER_READY:
						DisplayPlayerReady(packet);
						break;

					case ID_PLAYER_ATTACK:
					case ID_PLAYER_DEFEND:
					case ID_PLAYER_HEAL:
						ResolveTurn(packet);
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

	unsigned int lastPlayerTurn = 0;

	while (isRunning)
	{
		if (g_networkState == NS_Init)
		{
			g_networkState_mutex.lock();
			g_networkState = NS_PendingStart;
			g_networkState_mutex.unlock();

		}
		else if (g_networkState == NS_PendingStart)
		{
			RakNet::SocketDescriptor socketDescriptors[1];
			socketDescriptors[0].port = SERVER_PORT;
			socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4

			bool isSuccess = g_rakPeerInterface->Startup(MAX_CONNECTIONS, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
			assert(isSuccess);

			if (isSuccess)
			{
				g_rakPeerInterface->SetMaximumIncomingConnections(MAX_CONNECTIONS);
				/*std::cout << "server started" << std::endl;
				g_networkState_mutex.lock();
				g_networkState = NS_Started;
				g_networkState_mutex.unlock();
				*/
				g_networkState_mutex.lock();
				g_networkState = NS_WaitingPlayers;
				g_networkState_mutex.unlock();
					
				std::cout << "RakNet RPG Server started!!!" << std::endl;
				std::cout << "Waitting for " << numPlayersRequired << " players." << std::endl;
			}
		
		}
		else if (g_networkState == NS_WaitingPlayers)
		{
			CheckGameStart();
		}
		else if (g_networkState == NS_GameStarted)
		{
			if (isDefineTurn)
			{
				lastPlayerTurn++;
				if (lastPlayerTurn > m_players.size())
					lastPlayerTurn = 1;

				// Get a valid player
				for ( unsigned int x = lastPlayerTurn - 1; x < playersIDs->size(); x++ )
				{
					SPlayer& player = GetPlayer(playersIDs[x]);
					if (player.m_status == PS_Connected)
					{
						lastPlayerTurn = player.m_number;
						break;
					}
				}

				// Broadcast player's number turn
				RakNet::BitStream writeBs;
				writeBs.Write((RakNet::MessageID)ID_PLAYER_TURN);
				writeBs.Write(lastPlayerTurn);
				std::string _status = GetPlayerStats();
				RakNet::RakString status(_status.c_str());
				writeBs.Write(status);
				//returns 0 when something is wrong
				assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true));

				// Player receives a turn start
				SPlayer& player = GetPlayer(playersIDs[lastPlayerTurn - 1]);
				RakNet::BitStream writeBs2;
				writeBs2.Write((RakNet::MessageID)ID_YOUR_TURN);
				RakNet::RakString name("123");
				writeBs2.Write(name);
				//returns 0 when something is wrong
				assert(g_rakPeerInterface->Send(&writeBs2, HIGH_PRIORITY, RELIABLE_ORDERED, 0, player.m_address, false));

				isDefineTurn = false;
			}
		}
	}

	//std::cout << "press q and then return to exit" << std::endl;
	//std::cin >> userInput;

	//inputHandler.join();
	packetHandler.join();
	return 0;
}