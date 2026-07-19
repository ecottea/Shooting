// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：虹色渦巻き弾幕（24発の弾を渦巻き状に展開）
static void ShotRainbowSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 24発の弾を渦巻き状に展開
        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 渦巻きの角度（15度間隔）
            double angle = i * DX_PI / 12.0;
            pEnemyShot->muki = pEnemyShotSet->muki + angle;

            // 速度（1.0～2.5）
            pEnemyShot->speed = (100 + GetRand(150)) / 100.0;

            // 虹色（0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白）
            int color = i % 7;
            // 弾種をランダムに選択（小玉、中玉、大玉、菱形弾）
            int type = GetRand(3);
            switch (type) {
            case 0: pEnemyShot->kind = img_enemyShotSmallBall[color]; break;
            case 1: pEnemyShot->kind = img_enemyShotMediumBall[color]; break;
            case 2: pEnemyShot->kind = img_enemyShotLargeBall[color]; break;
            case 3: pEnemyShot->kind = img_enemyShotDiamond[color]; break;
            }

            // リンクドリストに追加
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

// 敵本体のパターン
void EnemyPat_Beautiful_Vibe()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 複雑な8の字移動
        enemy.x = 240.0 + 100.0 * sin(count * 0.02) * cos(count * 0.03);
        enemy.y = 40.0 + 50.0 * sin(count * 0.04);
    }

    // 120フレームごとに弾幕発射
    if (count % 6 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRainbowSpiral;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // リンクドリスト初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵弾幕セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}