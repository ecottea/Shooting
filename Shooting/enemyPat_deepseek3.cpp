// enemyPat_DeepSeek3.cpp
// パターン：レインボースパイラル（多重渦巻き）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

// 弾幕：スパイラル状に拡散するカラフル弾
static void ShotRainbowSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 効果音（中弾用）
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int arms = 6;               // 渦の本数
        const int bulletsPerArm = 12;     // 1本あたりの弾数
        const double baseAngle = pEnemyShotSet->muki;  // 自機方向を基準

        for (int a = 0; a < arms; ++a) {
            double armAngle = baseAngle + (2.0 * PI / arms) * a;
            for (int b = 0; b < bulletsPerArm; ++b) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = armAngle;
                pEnemyShot->speed = 1.8 + b * 0.35;   // 外側ほど速い

                // 弾の種類と色をバリエーション豊かに
                int kindType = a % 3;         // 0:小玉, 1:中玉, 2:銃弾
                int color = (a + b) % 6;      // 0～4の色を巡回
                switch (kindType) {
                case 0: pEnemyShot->kind = img_enemyShotSmallBall[color];  break;
                case 1: pEnemyShot->kind = img_enemyShotMediumBall[color]; break;
                case 2: pEnemyShot->kind = img_enemyShotScale[color];      break;
                }

                // 双方向リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレームの移動・回転
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 直進
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 方向を少しずつ回転 → スパイラル軌道
        if (pEnemyShotSet->count <= 240) {
            pEnemyShot->muki += 0.03;
        }

        // 30フレーム以降は徐々に減速（花火のような余韻）
        if (pEnemyShotSet->count > 30) {
            pEnemyShot->speed *= 0.985;
            if (pEnemyShot->speed < 1.0) pEnemyShot->speed = 1.0;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Beautiful_DeepSeek()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // ゆるやかな横移動（コサインカーブ）
    enemy.x += 0.4 * cos(count * 0.05);

    // 40フレームごとに弾幕セットを生成
    if (count % 20 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRainbowSpiral;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        // 自機の方向を基準角度にする（渦の中心がプレイヤーに向かう）
        pEnemyShotSet->muki = count;

        // ダミーヘッダの作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵弾管理リストに接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}