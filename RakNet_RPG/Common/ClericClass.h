#pragma once

#include "CharacterClass.h"

// Health: Medium, Damage: Low, Defense: Medium *CAN HEAL*
class ClericClass : public CharacterClass
{
	public:
		ClericClass(unsigned int const playerNum);
		~ClericClass();

		std::string GetMyCharacterClassName() override;
		static EPlayerAction DoTurn();

	private:

};

inline ClericClass::ClericClass(unsigned int const playerNum)
{
	m_number = playerNum;
	m_class = Cleric;

	m_healthQuality = Medium;
	m_health = GetAttribHealth(m_healthQuality);

	m_damageQuality = Low;
	m_defenseQuality = Medium;
}

ClericClass::~ClericClass()
{
}

inline std::string ClericClass::GetMyCharacterClassName()
{
	return "Cleric";
}

inline EPlayerAction ClericClass::DoTurn()
{
	EPlayerAction result;

	std::cout << std::endl;
	std::cout << "Select the action below: " << std::endl;
	std::cout << "<1> - Attack " << std::endl;
	std::cout << "<2> - Defend " << std::endl;
	std::cout << "<3> - Heal " << std::endl;
	std::cout << std::endl;

	char option = ' ';
	unsigned int iOption = 0;

	while (iOption < 1 || iOption > 3)
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
	case 3:
		result = Heal;
		break;
	default:
		break;
	}

	return result;
}

