// enemyPat_kinkakuji.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 金閣寺の一枚天井：天井弾（黄色い弾が左右から横一列に降り注ぐ）
// ============================================================
static void ShotKinkakuji_Ceiling(sEnemyShotSet* pEnemyShotSet)
{
    // 60フレームごとに横一列の弾を生成
    if (pEnemyShotSet->count % 60 == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 敵の左右に、10ドット間隔で横一列に並べる
        for (int i = 0; i < 20; i++) {
            double x_offset = 20.0 + i * 10.0;

            // 左側の弾
            sEnemyShot* pShotL = new sEnemyShot;
            pShotL->x = pEnemyShotSet->x - x_offset;
            pShotL->y = pEnemyShotSet->y;
            pShotL->muki = DX_PI / 2.0; // 下向き
            pShotL->speed = 0.8;        // ゆっくり移動
            pShotL->kind = img_enemyShotSmallBall[1]; // 1:黄

            // リストに追加
            pShotL->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShotL->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShotL;
            pEnemyShotSet->pEnemyShotHead->prev = pShotL;

            // 右側の弾
            sEnemyShot* pShotR = new sEnemyShot;
            pShotR->x = pEnemyShotSet->x + x_offset;
            pShotR->y = pEnemyShotSet->y;
            pShotR->muki = DX_PI / 2.0;
            pShotR->speed = 0.8;
            pShotR->kind = img_enemyShotSmallBall[1]; // 1:黄

            // リストに追加
            pShotR->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShotR->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShotR;
            pEnemyShotSet->pEnemyShotHead->prev = pShotR;
        }
    }

    // 弾の移動 (インクリメントや画面外消去はメインルーチン任せ)
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 金閣寺の一枚天井：回転弾幕（4方向にばら撒き、時間とともに回転）
// ============================================================
static void ShotKinkakuji_Rotate(sEnemyShotSet* pEnemyShotSet)
{
    // 段階の決定 (900フレーム = 約15秒ごとに段階変化)
    int phase = (count / 300) % 4; // ※短く

    // 段階ごとのパラメータ
    // 色: 2:緑, 3:シアン, 4:青, 5:マゼンタ
    int colors[4] = { 2, 3, 4, 5 };
    double speeds[4] = { 2.5, 3.5, 1.5, 2.5 };       // 中速度、高速、やや低速、中速度
    double rot_dirs[4] = { 1.0, -1.0, 1.0, -1.0 };   // 1.0:時計回り, -1.0:反時計回り
    double rot_speeds[4] = { 0.02, 0.03, 0.015, 0.025 };

    int color = colors[phase];
    double speed = speeds[phase];
    double rot_dir = rot_dirs[phase];
    double rot_speed = rot_speeds[phase];

    // 基本角度の更新 (param_d[0] に保存)
    pEnemyShotSet->param_d[0] += rot_speed * rot_dir;

    // 10フレームごとに弾を発射
    if (pEnemyShotSet->count % 10 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double base_angle = pEnemyShotSet->param_d[0];

        // 4方向 (0, 90, 180, 270度) に、それぞれ3発ずつ (-15度, 0度, +15度) ばらまく
        for (int dir = 0; dir < 4; dir++) {
            double dir_angle = base_angle + dir * (DX_PI / 2.0);
            for (int spread = -1; spread <= 1; spread++) {
                double angle = dir_angle + spread * (15.0 / 180.0 * DX_PI);

                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = angle;
                pShot->speed = speed;
                pShot->kind = img_enemyShotSmallBall[color];

                // リストに追加
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 弾の移動
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Kinkakuji_Qwen()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;

        // 天井弾用 sEnemyShotSet の生成
        sEnemyShotSet* pSetCeiling = new sEnemyShotSet;
        pSetCeiling->count = 0;
        pSetCeiling->patternFunc = ShotKinkakuji_Ceiling;
        pSetCeiling->x = enemy.x;
        pSetCeiling->y = enemy.y;
        pSetCeiling->pEnemyShotHead = new sEnemyShot;
        pSetCeiling->pEnemyShotHead->prev = pSetCeiling->pEnemyShotHead;
        pSetCeiling->pEnemyShotHead->next = pSetCeiling->pEnemyShotHead;

        pSetCeiling->prev = enemyShotSetHead.prev;
        pSetCeiling->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetCeiling;
        enemyShotSetHead.prev = pSetCeiling;

        // 回転弾幕用 sEnemyShotSet の生成
        sEnemyShotSet* pSetRotate = new sEnemyShotSet;
        pSetRotate->count = 0;
        pSetRotate->patternFunc = ShotKinkakuji_Rotate;
        pSetRotate->x = enemy.x;
        pSetRotate->y = enemy.y;
        pSetRotate->param_d[0] = 0.0; // 基本角度の初期値
        pSetRotate->pEnemyShotHead = new sEnemyShot;
        pSetRotate->pEnemyShotHead->prev = pSetRotate->pEnemyShotHead;
        pSetRotate->pEnemyShotHead->next = pSetRotate->pEnemyShotHead;

        pSetRotate->prev = enemyShotSetHead.prev;
        pSetRotate->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetRotate;
        enemyShotSetHead.prev = pSetRotate;
    }
    else {
        // 敵を少し左右に動かす
        enemy.x += 0.5 * cos(count * 0.02);

        // sEnemyShotSet の位置を敵に追従させる
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet = pSet->next;
        }
    }
}