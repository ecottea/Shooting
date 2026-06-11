// enemyPat_comet.cpp
// 彗星をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include <math.h>

// 彗星の「核」部分（先頭の弾）と「尾」部分（後ろに連なる弾）を表現する
static void ShotComet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);        

        // 彗星の核（先頭の弾）
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 3.0; // 核は速め
        pEnemyShot->kind = img_enemyShotLargeBall[2]; // 緑色の大玉で核を表現

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 彗星の尾（後ろに連なる弾）
        const int tail_count = 8; // 尾の弾数
        const double tail_speed_decay = 0.7; // 後ろほど遅くなる係数
        const double tail_spread_angle = DX_PI / 12.0; // 尾が少し広がる角度

        for (int i = 0; i < tail_count; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 核の方向に少し左右に広がる
            double angle_offset = (i % 2 == 0 ? 1.0 : -1.0) * (i / 2) * tail_spread_angle;
            pEnemyShot->muki = pEnemyShotSet->muki + angle_offset;
            // 後ろほど遅くして尾っぽらしさを出す
            pEnemyShot->speed = 3.0 * pow(tail_speed_decay, i + 1);

            // 尾の弾は小玉・中玉・鱗弾などで色を変える
            int type = GetRand(2); // 0:小玉, 1:中玉, 2:鱗弾
            int color = GetRand(2); // 0:赤, 1:黄, 2:緑 など彗星っぽい色
            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotScale[color];
                break;
            }

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

// 敵本体のパターン（指定どおりの関数名）
void EnemyPat_Comet_Sakana()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 100;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 10フレームごとに彗星弾幕を発射
    if (count % 10 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotComet; // 彗星弾幕関数を指定
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
    }
}