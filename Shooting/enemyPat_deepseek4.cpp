// enemyPat_wafu.cpp
// 和風弾幕：扇のように拡がる花吹雪
#include "DxLib.h"
#include "gv.h"
#include <math.h>

static void ShotWafu(sEnemyShotSet* pEnemyShotSet) {
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音再生（中玉用の少し重い音）
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int n = 100;                           // 12方向に扇状
        double baseAngle = pEnemyShotSet->muki; // 自機を狙った角度を基準

        printf("%d", pEnemyShotSet->kind);

        for (int i = 0; i < n; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double angle = baseAngle + (pEnemyShotSet->kind ? 1 : -1) * (2.0 * 3.14159265 * i / n);
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.0 + (rand() % 100) / 200.0; // 2.0～2.5

            // 紅（赤：0）と桃（マゼンタ：4）を交互に、時々入れ替え
            int color = (i % 2 == 0) ? 0 : 5;
            if (rand() % 3 == 0) color = (color == 0) ? 5 : 0;

            // 偶数番：鱗弾（花びら風）  奇数番：菱形弾（鋭い風情）
            if (i % 2 == 0) {
                pEnemyShot->kind = img_enemyShotScale[color];
            } else {
                pEnemyShot->kind = img_enemyShotDiamond[color];
            }

            // 連結リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動（拡がりながら回転）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 直線的に飛ばすが、向き(muki)を回転させることでグラフィックが回る
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot->muki += (pEnemyShotSet->kind ? 1 : -1) * 0.018;   // 花びらが舞うようにクルクル回転
        pEnemyShot->speed *= 1.005; // 徐々に加速して外側へ拡がる

        pEnemyShot = pEnemyShot->next;
    }
}

void EnemyPat_Japanese_DeepSeek() {
    static int muki = 1;
    static int frameCount = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = 120;
        enemy.hp = enemy.maxHp;
        muki = 1;
        frameCount = 0;
    } else {
        // ゆったり左右に揺れる動き
        enemy.x += 0.5 * muki;
        frameCount++;
        if (frameCount % 180 == 0) muki *= -1; // 3秒ごとに反転
    }

    // 60フレームおきに扇状の弾幕セットを生成
    if (count % 60 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWafu;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
                                     player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = (count / 60) & 1;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}