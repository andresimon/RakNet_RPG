#pragma once

#include <string>

enum EPlayerClass
{
	Warrior = 1,
	Rogue,
	Cleric,
	EPlayerClass_ERROR,
};

enum EPlayerAttribQuality
{
	Low = 0,
	Medium,
	High
};

enum EPlayerAction
{
	Attack = 0,
	Defend,
	Heal
};

class CharacterClass 
{
	public:
		EPlayerClass m_class;
		EPlayerAttribQuality m_healthQuality;
		int m_health;
		EPlayerAttribQuality m_damageQuality;
		EPlayerAttribQuality m_defenseQuality;

	public:
		CharacterClass();
		~CharacterClass();

		static std::string GetCharacterClassName(const EPlayerClass className);

		virtual std::string GetMyCharacterClassName();
		unsigned int GetAttribHealth();
		unsigned int GetAttribDamage();
		unsigned int GetAttribDefense();
};

CharacterClass::CharacterClass() {}
CharacterClass::~CharacterClass() {}

inline unsigned int CharacterClass::GetAttribDamage()
{
	unsigned int result = 0;

	switch (m_damageQuality)
	{
		case Low:
			result = 5;
			break;
		case Medium:
			result = 10;
			break;
		case High:
			result = 15;
			break;
		default:
			break;
	}
	return result;
}

inline std::string CharacterClass::GetCharacterClassName(const EPlayerClass className)
{
	std::string result;

	switch (className)
	{
		case Warrior:
			result = "Warrior";
			break;
		case Rogue:
			result = "Rogue";
			break;
		case Cleric:
			result = "Cleric";
			break;
		default:
			result = "Character class not found";
			break;
	}
	return result; 
}

inline std::string CharacterClass::GetMyCharacterClassName()
{
	return "Base Class";
}

inline unsigned int CharacterClass::GetAttribHealth()
{
	unsigned int result = 0;

	switch (m_healthQuality)
	{
		case Low:
			result = 40;
			break;
		case Medium:
			result = 60;
			break;
		case High:
			result = 80;
			break;
		default:
			break;
	}
	return result;
}

inline unsigned int CharacterClass::GetAttribDefense()
{
	unsigned int result = 0;

	switch (m_defenseQuality)
	{
		case Low:
			result = 5;
			break;
		case Medium:
			result = 10;
			break;
		case High:
			result = 15;
			break;
		default:
			break;
	}
	return result;
}