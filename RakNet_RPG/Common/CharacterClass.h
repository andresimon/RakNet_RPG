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
		unsigned int m_number;
		EPlayerAttribQuality m_healthQuality;
		unsigned int m_health;
		EPlayerAttribQuality m_damageQuality;
		EPlayerAttribQuality m_defenseQuality;


	public:
		CharacterClass();
		~CharacterClass();

		//virtual void LoadContent();
		//virtual void UnloadContent();
		//virtual void Update();
		//virtual void Draw(sf::RenderWindow &window);

		static std::string GetCharacterClassName(const EPlayerClass className);
		virtual std::string GetMyCharacterClassName();
		unsigned int GetAttribHealth(const EPlayerAttribQuality quality);
		unsigned int GetAttribDamage(const EPlayerAttribQuality quality);
		unsigned int GetAttribDefense(const EPlayerAttribQuality quality);
};

CharacterClass::CharacterClass() {}
CharacterClass::~CharacterClass() {}

inline unsigned int CharacterClass::GetAttribDamage(const EPlayerAttribQuality quality)
{
	unsigned int result = 0;

	switch (quality)
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

inline unsigned int CharacterClass::GetAttribHealth(const EPlayerAttribQuality quality)
{
	unsigned int result = 0;

	switch (quality)
	{
		case Low:
			result = 50;
			break;
		case Medium:
			result = 75;
			break;
		case High:
			result = 100;
			break;
		default:
			break;
	}
	return result;
}

inline unsigned int CharacterClass::GetAttribDefense(const EPlayerAttribQuality quality)
{
	unsigned int result = 0;

	switch (quality)
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
}