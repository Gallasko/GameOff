#include "fightscene.h"

#include "UI/ttftext.h" 

namespace pg
{
    void FightScene::init()
    {
        fightSys = ecsRef->getSystem<FightSystem>();

        float xEnemyName = 80;

        for (auto enemy : fightSys->enemies)
        {
            makeTTFText(this, xEnemyName, 20, "res/font/Inter/static/Inter_28pt-Light.ttf", enemy.name, 0.4);

            xEnemyName += 100;
        }
    }
}