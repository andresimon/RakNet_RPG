#pragma once

#include "CharacterClass.h"

// Health: High, Damage: Medium, Defense: Medium 
class WarriorClass : public CharacterClass
{
	public:
		WarriorClass(unsigned int const playerNum);
		~WarriorClass();

		std::string GetMyCharacterClassName() override;
		static EPlayerAction DoTurn();

	private:

};

inline WarriorClass::WarriorClass(unsigned int const playerNum)
{
	m_number = playerNum;
	m_class = Warrior;

	m_healthQuality = High;
	m_health = GetAttribHealth(m_healthQuality);

	m_damageQuality  = Medium;
	m_defenseQuality = Medium;
}

WarriorClass::~WarriorClass()
{
}

inline std::string WarriorClass::GetMyCharacterClassName()
{
	return "Warrior";
}

inline EPlayerAction WarriorClass::DoTurn()
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
