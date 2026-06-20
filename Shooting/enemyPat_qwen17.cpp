// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：紅葉の舞い散り
static void ShotMomiji(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 一定フレームごとに弾を生成
    if (pEnemyShotSet->count % 10 == 0 && pEnemyShotSet->count <= 180) {
        // 1秒に1回程度（60フレームごと）効果音を鳴らす
        if (pEnemyShotSet->count % 60 == 0) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 1度に3発ずつ生成
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            // 生成位置を少しずらして自然なばらつきを出す
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(40) - 20);
            pEnemyShot->y = pEnemyShotSet->y;

            // 下方向(DX_PI/2)を中心に、左右45度ずつの範囲で散らばる
            double base_muki = DX_PI / 2.0;
            pEnemyShot->muki = base_muki + (GetRand(90) - 45) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.5 + GetRand(100) / 100.0 + 5.0;

            // 色の分布: 赤(0) 50%, 黄(1) 40%, 緑(2) 10%
            int c = GetRand(9);
            int color = 0;
            if (c < 5) color = 0;
            else if (c < 9) color = 1;
            else color = 2;

            // 鱗弾を紅葉の葉に見立てる
            pEnemyShot->kind = img_enemyShotScale[color];

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動と挙動更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 蛇行しながら落下 (mukiが画像の回転角度も兼ねる)
        pEnemyShot->muki += sin(pEnemyShot->count * 0.15) * 0.05;

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 徐々に速度を落とし、空気抵抗のように減速させる
        pEnemyShot->speed *= 0.98;
        if (pEnemyShot->speed < 0.5) pEnemyShot->speed = 0.5;
        // 最終的に下方向へ落ちるように角度を補正
        pEnemyShot->muki += (DX_PI / 2.0 - pEnemyShot->muki) * 0.02;

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Maple_Qwen()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.0 * (double)muki;
        // 画面端で反転 (180フレーム周期)
        if (count % 180 == 90) muki *= -1;
    }

    // 弾幕セットの生成 (2.5秒に1回程度)
    if (count % 150 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMomiji;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = DX_PI / 2.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}