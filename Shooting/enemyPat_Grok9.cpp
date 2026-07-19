// enemyPat_Tmp.cpp
// 吹雪をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotBlizzard(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int phase = pEnemyShotSet->count;

    // 定期的に新しい雪片を生成
    if (phase % 30 == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int num = 6 + (phase / 30) / 10;  // 徐々に密度アップ

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(180) - 90;  // 横に広くばら撒く
            pEnemyShot->y = pEnemyShotSet->y + GetRand(30) - 15;

            // 吹雪らしい動き：下方向主体＋横風＋ゆらぎ
            double baseAngle = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI; // ほぼ下向き
            pEnemyShot->muki = baseAngle;
            pEnemyShot->speed = (80 + GetRand(120)) / 100.0;  // 比較的ゆったり

            // 風の影響（横成分は後で count で更新）
            pEnemyShot->count = GetRand(40) - 20;  // 個別風力オフセット

            // 雪らしい見た目（白・シアン中心の小玉・鱗弾・菱形）
            int type = GetRand(2);
            int color = (GetRand(2) == 0) ? 6 : (GetRand(1) ? 3 : 4); // 6:白, 3:シアン, 4:青

            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotScale[color];
                pEnemyShot->speed *= 0.9;
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotDiamond[color];
                pEnemyShot->speed *= 1.15;
                break;
            }

            // 連結
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾更新（吹雪特有の動き）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本落下
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);

        // 風による横揺らぎ（時間で変化）
        double wind = sin(phase * 0.08 + pEnemyShot->count) * 1.8 * 0.1;
        pEnemyShot->x += wind;

        // ゆるやかな回転風味（角度微調整）
        pEnemyShot->muki += 0.008 * sin(phase * 0.01);

        pEnemyShot = pEnemyShot->next;
    }

    pEnemyShotSet->count++;
}

// 敵本体のパターン
void EnemyPat_Blizzard_Grok()
{
    static int muki = 1;
    static int shotPhase = 0;

    if (count == 1) {
        // 初期配置（ゲーム画面 480x480）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotPhase = 0;
    }
    else {
        // ゆったり左右移動
        enemy.x += 1.15 * (double)muki;
        if (count % 110 == 55) muki *= -1;

        // たまに高度を少し上下させる
        if (count % 180 == 90) {
            //enemy.y = 55.0 + GetRand(25);
        }
    }

    // 吹雪生成（徐々に激しく）
    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBlizzard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;
        pEnemyShotSet->muki = DX_PI / 2.0;  // 下方向基準

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    shotPhase++;
}