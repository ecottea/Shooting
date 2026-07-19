// enemyPat_Tmp.cpp
// 砂時計をモチーフにした弾幕パターン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 砂時計風弾幕：上部から砂のように広がりながら落ち、中央の首で狭まり、下部で広がる流れを表現
// 複数のShotSetを使ってレイヤー化し、視覚的に砂時計の形を浮かび上がらせる
static void ShotHourglass(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音（軽め）
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numBullets = 10 + (pEnemyShotSet->count / 60) % 8; // 徐々に増加
        double baseAngle = pEnemyShotSet->muki;
        double spread = DX_PI / 3.0; // 広がり角

        for (int i = 0; i < numBullets; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 砂時計の形を意識した角度分布：上部広がり→中央狭まりをシミュレート
            double offset = (i - numBullets / 2.0) / (numBullets / 2.0) * spread;
            // 中央付近で角度を狭くする補正
            double neckFactor = 1.0 - 0.6 * fabs(sin(pEnemyShotSet->count * 0.05));
            pEnemyShot->muki = baseAngle + offset * neckFactor;

            pEnemyShot->speed = 1.8 + GetRand(80) / 100.0; // 1.8〜2.6程度

            // 暖色系中心（赤・黄・橙）を優先
            int color; int c = GetRand(5);
            if (c == 0)      color = 0;  // 赤
            else if (c == 1) color = 1;  // 黄
            else if (c == 2) color = 8;  // 橙
            else if (c == 3) color = 5;  // マゼンタ（暖かみあり）
            else             color = 0;  // 赤（多めに）

            // 弾の種類：主に小玉と鱗弾で砂っぽく
            int type = GetRand(4);
            switch (type) {
            case 0:
            case 1:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotScale[color];
                break;
            default:
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                break;
            }

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動 + 砂時計らしい加速度変化（中央通過時に加速/減速）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pEnemyShotSet->count * 0.03;
        // 中央付近で速度変化（砂が落ちるような加速）
        double accel = 1.0 + 0.4 * sin(t + pEnemyShot->muki * 5.0);
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki) * accel;
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki) * accel * 1.1; // 下方向に強調
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Hourglass_Grok()
{
    static int muki = 1;
    static int phase = 0;

    if (count == 1) {
        // 初期位置（上部中央）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
    }
    else {
        // ゆっくり左右往復 + 微妙な上下動き
        enemy.x += 1.2 * (double)muki;
        enemy.y = 60.0 + 25.0 * sin(count * 0.02);

        if (count % 140 == 70) muki *= -1;

        // フェーズ変化で攻撃のバリエーション
        if (count % 240 == 0) phase = (phase + 1) % 3;
    }

    // 定期的に弾幕発射
    if (count % 8 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHourglass;
        pEnemyShotSet->x = enemy.x + GetRand(30) - 15;
        pEnemyShotSet->y = enemy.y + 20.0;

        // プレイヤー方向を基本に、砂時計らしい多方向展開
        double aim = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->muki = aim;

        // 首の部分を意識した微調整
        if (phase == 1) pEnemyShotSet->muki += (GetRand(60) - 30) / 180.0 * DX_PI * 0.6;
        if (phase == 2) pEnemyShotSet->muki += DX_PI * 0.15 * sin(count * 0.1); // 揺らぎ

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リンク
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 時々追加のリング状展開（砂時計の広がりを強調）
    if (count % 90 == 45 && phase != 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHourglass;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = DX_PI / 2.0; // 下向き基調

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}