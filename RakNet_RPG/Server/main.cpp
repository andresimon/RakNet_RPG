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
#include <vector>

#include "Common.h"
#include "CharacterClass.h"
#include "WarriorClass.h"
#include "RogueClass.h"
#include "ClericClass.h"

static unsigned int SERVER_PORT = 65000;
static unsigned int const MAX_CONNECTIONS = 3; 

bool isServer = true;
bool isRunning = true;
bool isDefineTurn = true; // Tells the server to start a new player's turn
unsigned int lastPlayerTurn = 0;

RakNet::RakPeerInterface *g_rakPeerInterface = nullptr;
RakNet::SystemAddress g_serverAddress;

std::mutex g_networkState_mutex;
NetworkState g_networkState = NS_Init;

std::map<unsigned long, SPlayer> m_players;
RakNet::RakNetGUID playersIDs[MAX_CONNECTIONS];

unsigned int const	numPlayersRequired  = MAX_CONNECTIONS;
std::string lastPlayerAction = "";

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

	if (g_networkState == NS_WaitingPlayers)
	{
		for (unsigned int x = 1; x <= MAX_CONNECTIONS; x++)
		{
			std::string chosenClass = "WAITING";

			if (RakNet::RakNetGUID::ToUint32(playersIDs[x - 1]) > 0 ) // != NULL)
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
	}
	else if ( g_networkState == NS_GameStarted  )
	{
		std::cout << "Game STARTED" << std::endl;
		std::cout << std::endl;

		for (unsigned int x = 1; x <= MAX_CONNECTIONS; x++)
		{
			SPlayer& player = GetPlayer(playersIDs[x - 1]);

			std::cout << "Player " << player.m_number << " - " << CharacterClass::GetCharacterClassName(player.m_class);
			switch (player.m_status)
			{
				case PS_Disconnected:
					std::cout << " - DISCONNECTED";
					break;
				case PS_Dead:
					std::cout << " - DEAD";
					break;
				default:
					break;
			} 
			std::cout << std::endl;
		}
	}
	else if ( g_networkState == NS_GameOver )
	{
		for (unsigned int x = 1; x <= MAX_CONNECTIONS; x++)
		{
			SPlayer& player = GetPlayer(playersIDs[x - 1]);

			std::cout << "Player " << player.m_number << " - " << CharacterClass::GetCharacterClassName(player.m_class);
			switch (player.m_status)
			{
				case PS_Winner:
					std::cout << " - WINNER";
					break;
				case PS_Disconnected:
					std::cout << " - DISCONNECTED";
					break;
				case PS_Dead:
					std::cout << " - DEAD";
					break;
				default:
					break;
			}
			std::cout << std::endl;
		}

		std::cout << std::endl;
		std::cout << "GAME OVER" << std::endl;
	}
}

std::string GetPlayerStats()
{
	std::stringstream ss;

	ss << std::endl;

	for (unsigned int x = 0; x < MAX_CONNECTIONS; x++)
	{
		if (RakNet::RakNetGUID::ToUint32(playersIDs[x]) >0) //!= NULL)
		{
			SPlayer& player = GetPlayer(playersIDs[x]);
			ss << "Player " << player.m_number << " - " << player.m_character->GetMyCharacterClassName() << " - ";
					
			switch (player.m_status)
			{
				case PS_Connected:
					ss << player.m_character->m_health << " HP";
					break;
				case PS_Winner:
					ss << "WINNER";
					break;
				case PS_Disconnected:
					ss << "DISCONNECTED";
					break;
				case PS_Dead:
					ss << "DEAD";
					break;
				default:
					break;
			}
			ss << std::endl;
		}
	}
		
	return ss.str();
}

bool IsGameOver()
{
	unsigned int validPlayerCount = 0;
	unsigned int winner = 0;

	std::stringstream ss;

	ss << std::endl;

	for (unsigned int x = 0; x < MAX_CONNECTIONS; x++)
	{
		SPlayer& player = GetPlayer(playersIDs[x]);

		if ( player.m_status == PS_Connected )
		{
			validPlayerCount++;
			winner = player.m_number;
		}
	}

	if (validPlayerCount <= 1) // Game Over
	{
		g_networkState_mutex.lock();
		g_networkState = NS_GameOver;
		g_networkState_mutex.unlock();

		SPlayer& player = GetPlayer(playersIDs[winner - 1]);
		player.m_status = PS_Winner;

		ShowHeader();

		// Notify all player that game has finished
		RakNet::BitStream writeBs;
		writeBs.Write((RakNet::MessageID)ID_THEGAME_OVER);
		std::string _status = GetPlayerStats();
		RakNet::RakString status(_status.c_str());
		writeBs.Write(status);
		writeBs.Write(player.m_number);

		assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true));
	}
	return 0;
}

void OnLostConnection(RakNet::Packet* packet)
{
	SPlayer& lostPlayer = GetPlayer(packet->guid);

	if (g_networkState == NS_WaitingPlayers)
	{
		playersIDs[lostPlayer.m_number - 1] = (RakNet::RakNetGUID)0;

		unsigned long keyVal = RakNet::RakNetGUID::ToUint32(packet->guid);
		m_players.erase(keyVal);
	}
	else if (g_networkState == NS_GameStarted)
	{
		lostPlayer.m_status = PS_Disconnected;
		if (lostPlayer.m_number == lastPlayerTurn)
			isDefineTurn = true; // Transfer the turn to another player
	}

	ShowHeader();

	IsGameOver();
}

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

	SPlayer& newPlayer = SPlayer();
	newPlayer.m_number = availableSlot + 1;
	newPlayer.m_status = PS_Connected;
	newPlayer.m_address = packet->systemAddress;

	m_players.insert(std::make_pair(RakNet::RakNetGUID::ToUint32(packet->guid), newPlayer));
	playersIDs[availableSlot] = packet->guid;

	ShowHeader();
}

bool CheckGameStart()
{
	if (g_networkState == NS_WaitingPlayers)
	{
		if (m_players.size() < numPlayersRequired)
			return false;

		for (std::map<unsigned long, SPlayer>::iterator it = m_players.begin(); it != m_players.end(); ++it)
		{
			SPlayer& player = it->second;
			if (player.m_class == NULL)
			{
				return false;
			}
		}

		// Notify all player that game has started
		RakNet::BitStream writeBs;
		writeBs.Write((RakNet::MessageID)ID_THEGAME_START);
		std::string _status = GetPlayerStats();
		RakNet::RakString status(_status.c_str());
		writeBs.Write(status);

		assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true));
		
		g_networkState_mutex.lock();
		g_networkState = NS_GameStarted;
		g_networkState_mutex.unlock();

		ShowHeader();
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

	RakNet::BitStream writeBs;
	writeBs.Write((RakNet::MessageID)ID_PLAYER_READY);
	writeBs.Write(player.m_number);

	//returns 0 when something is wrong
	assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false));

	CheckGameStart();
}

void ResolveAttack(RakNet::Packet *packet)
{
	std::stringstream ss;

	SPlayer& player = GetPlayer(packet->guid);
	unsigned int targetPlayerNum = 0;

	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	bs.Read(targetPlayerNum);

	SPlayer& targetPlayer = GetPlayer(playersIDs[targetPlayerNum - 1]);
	
	if (targetPlayer.m_status != PS_Connected)
		return; // Target player was disconnected

	unsigned int targetDefense = targetPlayer.m_character->GetAttribDefense();
	if (targetPlayer.m_isDefending)
	{
		targetDefense += 5;
		targetPlayer.m_isDefending = false;
	}

	ss << std::endl << "*" << std::endl;
	ss << "Player " << player.m_number << " attacked player " << targetPlayer.m_number << " and ";

	unsigned int toHit = Rolld20();
	if (toHit >= targetDefense)
	{
		targetPlayer.m_character->m_health -= player.m_character->GetAttribDamage();
		ss << "did  " << player.m_character->GetAttribDamage() << " points of damage." << std::endl;

		// Check if target died
		if (targetPlayer.m_character->m_health <= 0)
		{
			targetPlayer.m_character->m_health = 0;
			targetPlayer.m_status = PS_Dead;

			ss << "Player " << targetPlayer.m_number << " died. " << std::endl;
		}
	}
	else
	{
		ss << "missed." << std::endl;
	}

	ss << "*" << std::endl;

	lastPlayerAction = ss.str();

	IsGameOver();
}

void ResolveDefend(RakNet::Packet *packet)
{
	std::stringstream ss;
	unsigned int targetPlayerNum = 0;

	SPlayer& player = GetPlayer(packet->guid);

	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	bs.Read(targetPlayerNum);

	player.m_isDefending = true;

	ss << std::endl << "*" << std::endl;
	ss << "Player " << player.m_number << " is defending. " << std::endl;
	ss << "*" << std::endl;

	lastPlayerAction = ss.str();
}

void ResolveHeal(RakNet::Packet *packet)
{
	std::stringstream ss;
	unsigned int targetPlayerNum = 0;

	SPlayer& player = GetPlayer(packet->guid);

	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	bs.Read(targetPlayerNum);

	unsigned int toHeal = Rolld20();
	int maxHeal = player.m_character->GetAttribHealth() - player.m_character->m_health;

	maxHeal = (toHeal > maxHeal ? maxHeal : toHeal);

	player.m_character->m_health += maxHeal;
	
	ss << std::endl << "*" << std::endl;
	ss << "Player " << player.m_number << " healed " << maxHeal << " points." << std::endl;
	ss << "*" << std::endl;

	lastPlayerAction = ss.str();
}

void ResolveTurn(RakNet::Packet *packet)
{
	SPlayer& player = GetPlayer(packet->guid);

	unsigned char packetIdentifier = GetPacketIdentifier(packet);
	switch (packetIdentifier)
	{
		case ID_PLAYER_ATTACK:
			ResolveAttack(packet);
			break;

		case ID_PLAYER_DEFEND:
			ResolveDefend(packet);
			break;

		case ID_PLAYER_HEAL:
			ResolveHeal(packet);
			break;

		default:
			break;
	}

	isDefineTurn = true;

	ShowHeader();
}

/*
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
*/

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

	//std::thread inputHandler(InputHandler);
	std::thread packetHandler(PacketHandler);

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
				g_networkState_mutex.lock();
				g_networkState = NS_WaitingPlayers;
				g_networkState_mutex.unlock();
					
				std::cout << "RakNet RPG Server started!!!" << std::endl;
				std::cout << "Waitting for " << numPlayersRequired << " players." << std::endl;
			}
		
		}
		else if (g_networkState == NS_WaitingPlayers)
		{
		}
		else if (g_networkState == NS_GameStarted)
		{
			if (isDefineTurn)
			{
				lastPlayerTurn++;
				if (lastPlayerTurn > m_players.size())
					lastPlayerTurn = 1;

				// Get a valid player
				for ( unsigned int x = lastPlayerTurn - 1; x < MAX_CONNECTIONS; x++ )
				{
					SPlayer& player = GetPlayer(playersIDs[x]);
					if (player.m_status == PS_Connected)
					{
						lastPlayerTurn = player.m_number;
						break;
					}
				}

				/* Broadcast player's number turn && last player's action */
				RakNet::BitStream writeBs;
				writeBs.Write((RakNet::MessageID)ID_PLAYER_TURN);
				writeBs.Write(lastPlayerTurn);
				
				std::string _status = GetPlayerStats();
				RakNet::RakString status(_status.c_str());
				writeBs.Write(status);
				RakNet::RakString playerAction(lastPlayerAction.c_str());
				writeBs.Write(playerAction);

				//returns 0 when something is wrong
				assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true));

				/* Player receives a turn start */
				SPlayer& player = GetPlayer(playersIDs[lastPlayerTurn - 1]);
				RakNet::BitStream writeBs2;
				writeBs2.Write((RakNet::MessageID)ID_YOUR_TURN);

				// Get valid targets
				unsigned int numValidTargets = 0;
				std::vector<unsigned int> validTargets;

				for (unsigned int x = 0; x < MAX_CONNECTIONS; x++)
				{
					SPlayer& player = GetPlayer(playersIDs[x]);
					if (player.m_status == PS_Connected)
					{
						if (player.m_number != lastPlayerTurn)
						{
							numValidTargets++;
							validTargets.push_back(player.m_number);
						}
					}
				}

				writeBs2.Write(numValidTargets);
				for (std::vector<unsigned int>::iterator it = validTargets.begin(); it != validTargets.end(); ++it)
					writeBs2.Write(*it);

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