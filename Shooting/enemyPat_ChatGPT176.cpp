#include "DxLib.h"
#include "gv.h"
#include <cmath>

// enemyPat_sampleForAI.cpp に記載されている素材を使用する。
// 色番号: 0=赤、1=黄、2=緑、3=シアン、4=青、5=マゼンタ、6=白、7=黒、8=橙
extern int img_enemyShotSmallBall[9];
extern int img_enemyShotMediumBall[9];
extern int img_enemyShotBullet[9];
extern int sound_enemyShot_light;
extern int sound_enemyShot_medium;

namespace {
    constexpr double SPARKLER_PI = 3.14159265358979323846;
    constexpr int SPARKLER_CORE = 1;
    constexpr int SPARKLER_CORE_INNER = 2;

    // 弾を弾幕セットのリスト末尾に追加する。
    // role=0 は飛び散る火花、role=1/2 は火玉本体と中心の光。
    static sEnemyShot* AddSparklerShot(
        sEnemyShotSet* pSet,
        double x,
        double y,
        double angle,
        double speed,
        int kind,
        int role)
    {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = x;
        pShot->y = y;
        pShot->muki = angle;
        pShot->speed = speed;
        pShot->kind = kind;
        pShot->param_i[0] = role;

        if (role == 0) {
            // 速度をX/Y成分で保持し、わずかな重力で火花が落ちるようにする。
            pShot->param_d[0] = speed * std::cos(angle);
            pShot->param_d[1] = speed * std::sin(angle);
            pShot->param_d[2] = 0.012 + GetRand(18) / 1000.0;
        }

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
        return pShot;
    }

    // 線香花火の火玉と火花の制御。
    static void ShotSparkler(sEnemyShotSet* pSet)
    {
        const int t = pSet->count;

        if (t == 0) {
            // 小さな橙色の火玉と、その中心にある白い光。
            AddSparklerShot(
                pSet, pSet->x, pSet->y, 0.0, 0.0,
                img_enemyShotSmallBall[8], SPARKLER_CORE);
            AddSparklerShot(
                pSet, pSet->x, pSet->y, 0.0, 0.0,
                img_enemyShotSmallBall[6], SPARKLER_CORE_INNER);

            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 火玉は小さく揺れ、燃え進むにつれて少しずつ大きくなる。
        const double swayX = std::sin(t * 0.13) * 3.0 + std::sin(t * 0.31) * 1.0;
        const double swayY = std::sin(t * 0.19) * 2.0;
        double dropY = 0.0;
        if (t >= 92+30) {
            const double fallTime = static_cast<double>(t - 92-30);
            dropY = fallTime * 3.8/3 + fallTime * fallTime * 0.035/3;
        }
        const double coreX = pSet->x + swayX;
        const double coreY = pSet->y + swayY + dropY;

        // 燃え始め: ときおり小さな火花が控えめに散る。
        if (t < 32*2 && t % 5 == 2) {
            for (int i = 0; i < 2; ++i) {
                const double angle = GetRand(6283) / 1000.0;
                const double speed = 0.9 + GetRand(160) / 100.0;
                const int color = (i == 0) ? 1 : 8;
                const bool streak = (GetRand(2) == 0);
                const int kind = streak
                    ? img_enemyShotBullet[color]
                    : img_enemyShotSmallBall[color];
                AddSparklerShot(pSet, coreX, coreY, angle, speed, kind, 0);
            }
        }

        // 火花が増える中盤: 黄色・橙色・白色の火花を不規則に放つ。
        if (t >= 32*2 && t < 90*2 && t % 4 == 0) {
            const int sparkCount = 4 + GetRand(2); // 4～6個
            for (int i = 0; i < sparkCount; ++i) {
                const double angle = GetRand(6283) / 1000.0;
                const double speed = 1.2 + GetRand(250) / 100.0;
                const int colorRoll = GetRand(4);
                const int color = (colorRoll == 0) ? 6 : ((colorRoll <= 2) ? 1 : 8);
                const bool streak = (GetRand(3) == 0);
                const int kind = streak
                    ? img_enemyShotBullet[color]
                    : img_enemyShotSmallBall[color];
                AddSparklerShot(pSet, coreX, coreY, angle, speed, kind, 0);
            }
        }

        // 最後の大きな火花。放射状に散る弾と細い銃弾を重ねて閃きを表現する。
        if (t == 92*2) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            const int burstCount = 72;
            for (int i = 0; i < burstCount; ++i) {
                const double baseAngle = SPARKLER_PI * 2.0 * i / burstCount;
                const double jitter = (GetRand(30) - 15) / 100.0;
                const double angle = baseAngle + jitter;
                const double speed = 2.0 + GetRand(330) / 100.0;
                const int color = (i % 3 == 0) ? 6 : ((i % 3 == 1) ? 1 : 8);
                const bool streak = (i % 3 == 0) || (GetRand(3) == 0);
                const int kind = streak
                    ? img_enemyShotBullet[color]
                    : img_enemyShotSmallBall[color];
                AddSparklerShot(pSet, coreX, coreY, angle, speed, kind, 0);
            }

            // 火玉の近くに、ゆっくり流れる余韻の火花も少し残す。
            for (int i = 0; i < 18; ++i) {
                const double angle = SPARKLER_PI * 2.0 * i / 18.0
                    + (GetRand(20) - 10) / 100.0;
                const double speed = 0.7 + GetRand(90) / 100.0;
                const int color = (i % 2 == 0) ? 6 : 8;
                AddSparklerShot(
                    pSet, coreX, coreY, angle, speed,
                    img_enemyShotSmallBall[color], 0);
            }
        }

        // 全弾を更新する。火花には弱い重力を加え、銃弾の向きも進行方向に合わせる。
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == SPARKLER_CORE) {
                pShot->x = coreX;
                pShot->y = coreY;

                if (t < 30*2) {
                    pShot->kind = img_enemyShotSmallBall[8];
                }
                else if (t < 84*2) {
                    pShot->kind = ((t / 6) % 2 == 0)
                        ? img_enemyShotMediumBall[1]
                        : img_enemyShotMediumBall[8];
                }
                else {
                    // 最後は火玉が小さくなり、落ちていく。
                    pShot->kind = img_enemyShotSmallBall[8];
                }
            }
            else if (pShot->param_i[0] == SPARKLER_CORE_INNER) {
                pShot->x = coreX;
                pShot->y = coreY;
                pShot->kind = (t % 6 < 3)
                    ? img_enemyShotSmallBall[6]
                    : img_enemyShotSmallBall[1];
            }
            else {
                pShot->param_d[1] += pShot->param_d[2];
                pShot->x += pShot->param_d[0];
                pShot->y += pShot->param_d[1];
                pShot->muki = std::atan2(pShot->param_d[1], pShot->param_d[0]);
                pShot->speed = std::sqrt(
                    pShot->param_d[0] * pShot->param_d[0]
                    + pShot->param_d[1] * pShot->param_d[1]);
            }

            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Sparkler_ChatGPT()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 48.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 線香花火を見せやすいよう、ボスは画面上部をゆっくり移動する。
        enemy.x = 240.0 + std::sin(count * 0.006) * 125.0;
        enemy.y = 48.0 + std::sin(count * 0.013) * 7.0;
    }

    // 火玉が燃え尽きる周期に合わせて、次の線香花火を点火する。
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSparkler;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = std::atan2(
            player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
