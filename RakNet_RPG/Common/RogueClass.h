#pragma once

#include "CharacterClass.h"

// Health: Low, Damage: High, Defense: High 
class RogueClass : public CharacterClass
{
	public:
		RogueClass(unsigned int const playerNum);
		~RogueClass();

	std::string GetMyCharacterClassName() override;
	static EPlayerAction DoTurn();

	private:

};

inline RogueClass::RogueClass(unsigned int const playerNum)
{
	m_number = playerNum;
	m_class = Rogue;

	m_healthQuality = Low;
	m_health = GetAttribHealth(m_healthQuality);

	m_damageQuality = High;
	m_defenseQuality = High;
}

RogueClass::~RogueClass()
{
}

inline std::string RogueClass::GetMyCharacterClassName()
{
	return "Rogue";
}

inline EPlayerAction RogueClass::DoTurn()
{
	EPlayerAction result;

	std::cout << std::endl;
	std::cout << "Select the action below: " << std::endl;
	std::cout << "<1> - Attack " << std::endl;
	std::cout << "<2> - Defend " << std::endl;
	std::cout << std::endl;

	char option = ' ';
	unsigned int iOption = 0;

	while (iOption < 1 || iOption > 2)
	{
		iOption = 0;
		option = _getch();

		iOption = option - '0';
	}

	switch (iOption)
	{
	case 1:
		result = Attack;
		break;
	case 2:
		result = Defend;
		break;
	default:
		break;
	}

	return result;
}

