#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 雪の結晶弾幕：六方向の結晶を形成してから散開させる。
namespace {
    constexpr int SNOWFLAKE_SHOT_INTERVAL = 100;
    constexpr int SNOWFLAKE_FORMATION_FRAMES = 36;
    constexpr int SNOWFLAKE_ARM_COUNT = 6;
    constexpr int SNOWFLAKE_TRUNK_COUNT = 10;
    constexpr double SNOWFLAKE_TRUNK_STEP = 8.2;
    constexpr double SNOWFLAKE_BRANCH_STEP = 5.8;

    enum SnowflakeShotRole {
        SNOWFLAKE_TRUNK = 0,
        SNOWFLAKE_BRANCH = 1,
        SNOWFLAKE_TWIG = 2,
        SNOWFLAKE_TARGET = 3
    };

    // count類はメインルーチンが管理するため、この関数では増加させない。
    static void AddSnowflakeShot(
        sEnemyShotSet* pEnemyShotSet,
        double x,
        double y,
        double moveAngle,
        int image,
        int role,
        int armIndex)
    {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = x;
        pShot->y = y;
        pShot->muki = moveAngle;
        pShot->speed = 0.0; // 結晶の形を見せている間は静止させる。
        pShot->kind = image;
        pShot->param_i[0] = role;
        pShot->param_i[1] = armIndex;

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    static void PlaySnowflakeSound(int soundHandle)
    {
        if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
        PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
    }

    static void BuildSnowflake(sEnemyShotSet* pEnemyShotSet)
    {
        const double baseX = pEnemyShotSet->x;
        const double baseY = pEnemyShotSet->y;
        const double baseAngle = pEnemyShotSet->muki;
        const double branchAnchors[] = {
            SNOWFLAKE_TRUNK_STEP * 3.0,
            SNOWFLAKE_TRUNK_STEP * 6.0,
            SNOWFLAKE_TRUNK_STEP * 8.0
        };

        for (int arm = 0; arm < SNOWFLAKE_ARM_COUNT; ++arm) {
            const double armAngle = baseAngle + arm * (DX_PI / 3.0);
            const double armX = cos(armAngle);
            const double armY = sin(armAngle);

            // 六方向の幹。先端の弾は、散開時にプレイヤーを狙う。
            for (int i = 1; i <= SNOWFLAKE_TRUNK_COUNT; ++i) {
                const double r = SNOWFLAKE_TRUNK_STEP * i;
                const double x = baseX + armX * r;
                const double y = baseY + armY * r;
                const bool isTip = (i == SNOWFLAKE_TRUNK_COUNT);
                const int image = isTip
                    ? img_enemyShotMediumBall[6] // 白い中玉：狙い弾の目印
                    : img_enemyShotMediumBall[3]; // シアンの中玉：結晶の幹
                AddSnowflakeShot(
                    pEnemyShotSet, x, y, armAngle, image,
                    isTip ? SNOWFLAKE_TARGET : SNOWFLAKE_TRUNK, arm);
            }

            // 幹の三か所から、左右に伸びる枝を作る。
            for (int anchorIndex = 0; anchorIndex < 3; ++anchorIndex) {
                const double anchorR = branchAnchors[anchorIndex];
                const double anchorX = baseX + armX * anchorR;
                const double anchorY = baseY + armY * anchorR;

                for (int side = -1; side <= 1; side += 2) {
                    const double branchAngle = armAngle + side * (DX_PI / 4.0);
                    const double branchX = cos(branchAngle);
                    const double branchY = sin(branchAngle);

                    for (int j = 1; j <= 4; ++j) {
                        const double d = SNOWFLAKE_BRANCH_STEP * j;
                        AddSnowflakeShot(
                            pEnemyShotSet,
                            anchorX + branchX * d,
                            anchorY + branchY * d,
                            armAngle,
                            img_enemyShotSmallBall[6], // 白い小玉：枝
                            SNOWFLAKE_BRANCH,
                            arm);
                    }

                    // 枝の途中にも短い小枝を付け、雪の結晶らしい細かさを出す。
                    const double twigBaseX = anchorX + branchX * (SNOWFLAKE_BRANCH_STEP * 2.0);
                    const double twigBaseY = anchorY + branchY * (SNOWFLAKE_BRANCH_STEP * 2.0);
                    for (int twigSide = -1; twigSide <= 1; twigSide += 2) {
                        const double twigAngle = branchAngle + twigSide * (DX_PI / 4.0);
                        const double twigX = cos(twigAngle);
                        const double twigY = sin(twigAngle);
                        for (int k = 1; k <= 2; ++k) {
                            const double d = SNOWFLAKE_BRANCH_STEP * k;
                            AddSnowflakeShot(
                                pEnemyShotSet,
                                twigBaseX + twigX * d,
                                twigBaseY + twigY * d,
                                armAngle,
                                img_enemyShotSmallBall[4], // 青い小玉：細枝
                                SNOWFLAKE_TWIG,
                                arm);
                        }
                    }
                }
            }
        }
    }

    static void ShotSnowflake(sEnemyShotSet* pEnemyShotSet)
    {
        if (pEnemyShotSet->count == 0) {
            PlaySnowflakeSound(sound_enemyCharge);
            BuildSnowflake(pEnemyShotSet);
        }

        // 結晶を見せ終えた瞬間、幹・枝・細枝が六方向へ散開する。
        if (pEnemyShotSet->count == SNOWFLAKE_FORMATION_FRAMES) {
            PlaySnowflakeSound(sound_enemyShot_extreme);

            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                switch (pShot->param_i[0]) {
                case SNOWFLAKE_TRUNK:
                    pShot->speed = 3.0;
                    break;
                case SNOWFLAKE_BRANCH:
                    pShot->speed = 2.5;
                    break;
                case SNOWFLAKE_TWIG:
                    pShot->speed = 2.1;
                    break;
                case SNOWFLAKE_TARGET:
                    // 各結晶の先端だけは、散開開始時のプレイヤー位置を狙う。
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->speed = 4.0;
                    break;
                }
                pShot = pShot->next;
            }
        }

        // 弾の移動と画面外削除は、既存のメインルーチンの仕様に合わせる。
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
}

// 敵本体の弾幕パターン
void EnemyPat_Snowflake_ChatGPT()
{
    static double baseAngle;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200; // 変更しないこと
        baseAngle = -DX_PI / 2.0;
    }
    else {
        // ボスは画面上部をゆっくり左右に移動する。
        enemy.x = 240.0 + 50.0 * sin(count * 0.009);
        enemy.y = 55.0;
    }

    if (count % SNOWFLAKE_SHOT_INTERVAL == 1) for (int i = -1; i <= 1; i++) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSnowflake;
        pEnemyShotSet->x = enemy.x + i * 200.0;
        pEnemyShotSet->y = enemy.y + 42.0 + abs(i) * 100.0;
        pEnemyShotSet->muki = baseAngle;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 次の結晶は少し回転させ、模様が徐々に変化するようにする。
        baseAngle += DX_PI / 16.0;
        if (baseAngle >= DX_PI * 2.0) baseAngle -= DX_PI * 2.0;
    }
}
