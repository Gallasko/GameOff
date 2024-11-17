#pragma once

#include "commons.h"

namespace pg
{
    struct CharacterStat
    {
        int health = 0;

        int physicalAttack = 0;
        int magicalAttack = 0;

        int physicalDefense = 0;
        int magicalDefense = 0;

        int speed = 0;

        // In percentage
        int critChance = 0;
        int critDamage = 0;
        int evasionRate = 0;

        int elementalRes[NbElements] = {0};
    };

    struct BaseCharacterStat : public CharacterStat
    {
        BaseCharacterStat()
        {
            health = 100;
            physicalAttack = 10;
            magicalAttack = 10;

            physicalDefense = 1;
            magicalDefense = 1;
            
            speed = 100;

            critChance = 5;
            critDamage = 150;
            evasionRate = 5;
        }
    };
}