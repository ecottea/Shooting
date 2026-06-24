// enemyPat_volcano.cpp
// 火山をモチーフにした弾幕パターン（1パターン）を実装したファイル

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 火山弾幕パターン：溶岩流＋噴石＋火山灰
static void ShotVolcano(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 効果音：火山の噴火をイメージして重めの音を選ぶ
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 溶岩流：赤〜橙の玉弾を下方向に流す
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(80) - 40); // 少し左右に広がる
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI; // 下方向に少し揺らす
            pEnemyShot->speed = (150 + GetRand(100)) / 100.0; // ゆっくり〜中速

            int color = GetRand(1) == 0 ? 0 : 8; // 赤 or 橙
            pEnemyShot->kind = img_enemyShotLargeBall[color]; // 大玉で溶岩の塊を表現

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 噴石：黒・白の弾をランダム方向に勢いよく発射
        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (GetRand(360) / 180.0 * DX_PI); // 全方向ランダム
            pEnemyShot->speed = (300 + GetRand(200)) / 100.0; // 速め

            int color = GetRand(1) == 0 ? 6 : 7; // 白 or 黒
            int type = GetRand(2); // 0:小玉, 1:中玉, 2:銃弾
            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotBullet[color];
                break;
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 火山灰：小さな弾を扇状に多数発射
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI; // 下方向を中心に扇状
            pEnemyShot->speed = (100 + GetRand(150)) / 100.0; // ゆっくり〜中速

            int color = GetRand(3); // 0:赤, 1:黄, 2:緑 など灰っぽくない色も混ぜる
            pEnemyShot->kind = img_enemyShotSmallBall[color]; // 小玉で灰の粒を表現

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（火山ボス）
void EnemyPat_Volcano_Sakana()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 火山弾幕の発射間隔（例：15フレームごと）
    if (count % 20 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotVolcano;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // ShotVolcano 内で方向を決めるのでここでは使わない

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}