// enemyPat_mapleLeaf.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：紅葉モチーフ（3層の葉を広げ、揺れながら落下する弾幕）
static void ShotMapleLeafComplex(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // --- 1層目：中心の大玉（茎）
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 1.0;
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // --- 2層目：6発の中玉（葉の輪郭）
        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 60度間隔で均等配置（±30°、±90°、±150°）
            double angle = pEnemyShotSet->muki + (i < 3 ? -1 : 1) * ((i % 3 + 1) * DX_PI / 6);
            angle += (GetRand(10) - 5) * DX_PI / 180.0; // 微妙なばらつき
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 1.8 + GetRand(40) / 100.0;
            // 色：赤→黄→オレンジ（マゼンタ）
            int color = (i < 2) ? 0 : (i < 4) ? 1 : 5;
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // --- 3層目：12発の小玉（葉の脈と細部）
        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 30度間隔で配置（±15°、±45°、±75°、±105°、±135°、±165°）
            double angle = pEnemyShotSet->muki + (i < 6 ? -1 : 1) * ((i % 6 + 1) * DX_PI / 12);
            angle += (GetRand(15) - 7) * DX_PI / 180.0; // ばらつき
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.2 + GetRand(60) / 100.0;
            // 色：ランダム（赤、黄、オレンジ、緑）
            int color = GetRand(4);
            if (color >= 3) color = 2; // 緑
            else if (color == 2) color = 5; // オレンジ
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 弾の移動（揺れる動きを加味）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 揺れの振幅を弾のサイズで変化
        double sway = (pEnemyShot->kind == img_enemyShotLargeBall[0] ? 0.2 :
            pEnemyShot->kind >= img_enemyShotMediumBall[0] && pEnemyShot->kind <= img_enemyShotMediumBall[6] ? 0.4 : 0.6);
        double swayX = sway * sin(pEnemyShotSet->count * 0.1 + pEnemyShot->x * 0.02);
        double swayY = sway * 0.3 * cos(pEnemyShotSet->count * 0.15 + pEnemyShot->y * 0.02);

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki) + swayX;
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki) + swayY;

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Maple_Vibe()
{
    static int movePhase = 0; // 0:出現, 1:移動, 2:退避
    static int muki = 1;
    static int shotInterval = 60;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = -60.0;
        enemy.maxHp = enemy.hp = 200;
        movePhase = 0;
        muki = 1;
        shotInterval = 60;
    }
    else {
        switch (movePhase) {
        case 0: // 出現フェーズ（画面上部から降下）
            enemy.y += 1.5;
            if (enemy.y >= 60.0) movePhase = 1;
            break;
        case 1: // 移動フェーズ（8の字運動）
            enemy.x = 240.0 + 120.0 * sin(count * 0.02);
            enemy.y = 100.0 + 80.0 * sin(count * 0.03);
            // HPが半分以下になったら退避フェーズへ
            if (enemy.hp <= enemy.maxHp / 2) movePhase = 2;
            break;
        case 2: // 退避フェーズ（画面下部へ退避）
            enemy.y += (400 - 20) / 600.0;
            if (enemy.y >= 480.0) {
                enemy.x = 240.0;
                enemy.y = -60.0;
                enemy.hp = enemy.maxHp;
                movePhase = 0;
            }
            break;
        }
    }

    // 弾幕発射（フェーズ1の時のみ、間隔を徐々に短く）
    if (movePhase == 1 && count % shotInterval == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMapleLeafComplex;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 間隔を短く（最小20フレーム）
        if (shotInterval > 20) shotInterval -= 2;
    }
}