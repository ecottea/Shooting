// enemyPat_Tmp.cpp
// 潮吹きモチーフ弾幕パターン実装
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// 潮吹きモチーフ：激潮噴出パターン
// =============================================
static void ShotSquirt(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int phase = pEnemyShotSet->count;

    // 噴射リズム：3連発 → 少し間隔
    if (phase  == 0 ) {
        // 効果音（勢いのある噴射音）
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double baseAngle = pEnemyShotSet->muki; // 基本はプレイヤー方向

        // メイン噴流（太くて勢いのある中心部）
        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 8.0; // 下半身寄りから発射イメージ

            double offset = (GetRand(80) - 40) / 180.0 * DX_PI * 0.15; // 狭い扇
            pEnemyShot->muki = baseAngle + offset;
            pEnemyShot->speed = 4.2 + GetRand(80) / 100.0; // 高速

            // 液体っぽい中玉・短レーザー風
            if (i % 3 == 0) {
                pEnemyShot->kind = img_enemyShotMediumOval[4]; // 水色系
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白
            }

            // 連結リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 飛沫（左右への広がる小弾）
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 8.0;

            double offset = (GetRand(240) - 120) / 180.0 * DX_PI * 0.45; // 広めの扇
            pEnemyShot->muki = baseAngle + offset;
            pEnemyShot->speed = 2.8 + GetRand(120) / 100.0; // やや遅め

            pEnemyShot->kind = img_enemyShotSmallBall[GetRand(5)]; // 小玉でしぶき表現

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 少し重力っぽい落下（潮が弧を描く表現）
        pShot->speed *= 0.995; // 徐々に減速
        pShot->muki += 0.003;  // わずかにカーブ

        pShot = pShot->next;
    }
}

// =============================================
// 敵本体パターン：潮吹きモチーフ
// =============================================
void EnemyPat_Spout_Grok()
{
    static int muki = 1;
    static int shot_count = 0;

    if (count == 1) {
        // 初期配置（ゲーム画面 480x480）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右往復移動
        enemy.x += 1.15 * (double)muki;
        if (count % 110 == 55) muki *= -1;
    }

    // 定期的に噴射セットを生成（リズム重視）
    if (count % 28 == 1 || count % 28 == 9 || count % 28 == 17) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSquirt;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0; // 下方向に少しずらして「下半身」イメージ
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 連結リスト登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}