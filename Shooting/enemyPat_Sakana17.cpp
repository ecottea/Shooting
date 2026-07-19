// enemyPat_momiji.cpp
// 紅葉をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 紅葉をイメージした弾幕パターン
// ・赤〜オレンジ系の色を中心に、葉が舞い落ちるような軌道
// ・外側に向かう放射状の弾と、少し曲がりながら落ちていく弾を混ぜる
static void ShotMomiji(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 紅葉っぽい効果音（軽め）を再生
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 放射状に 8 方向の弾を出す
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 中心から外へ向かう方向
            pEnemyShot->muki = (i / 8.0) * 2.0 * DX_PI;
            // 基本速度は一定だが、少しランダムで揺らす
            pEnemyShot->speed = 2.0 + (GetRand(20) / 100.0);

            // 紅葉イメージの色：赤(0)・黄(1)・オレンジっぽい白(6) など
            int color = GetRand(2); // 0:赤, 1:黄, 2:白 のいずれか
            switch (color) {
            case 0:
                color = 0; // 赤
                break;
            case 1:
                color = 1; // 黄
                break;
            case 2:
                color = 6; // 白（オレンジ寄りに使う）
                break;
            }

            // 弾の種類は「鱗弾」か「菱形弾」を主に使う（葉っぽい形）
            int type = GetRand(1); // 0:鱗弾, 1:菱形弾
            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotScale[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotDiamond[color];
                break;
            }

            // リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 追加で、少し曲がりながら落ちていく弾を 4 発
        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(40) - 20);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(20) - 10);
            // 下方向に少し傾いた角度
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.5 + (GetRand(20) / 100.0);

            // 色は赤か黄
            int color = GetRand(1); // 0:赤, 1:黄
            // 弾の種類は小玉か中玉（葉っぽい丸み）
            int type = GetRand(1); // 0:小玉, 1:中玉
            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[color];
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
        // 速度に少しランダムな揺らぎを加えて葉っぽい動きに
        double wobble = (GetRand(20) - 10) / 1000.0;
        pEnemyShot->speed += wobble;
        if (pEnemyShot->speed < 0.5) pEnemyShot->speed = 0.5;

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（関数名は指定どおり）
void EnemyPat_Maple_Sakana()
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

    // 紅葉弾幕を一定間隔で発射
    if (count % 8 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMomiji;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        // プレイヤー方向はあまり気にせず、主に下方向・放射状に
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