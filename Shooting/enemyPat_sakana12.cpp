// enemyPat_sun.cpp
// 太陽をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 太陽モチーフの弾幕パターン
static void ShotSun(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 発射音（軽めの音）
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 自機方向の基準角度
        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 放射光線：自機方向を中心に左右に広がる弾
        int rayCount = 7; // 光線の本数
        for (int i = 0; i < rayCount; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 角度：自機方向 ± 左右に少しずつ広がる
            double angleOffset = (i - rayCount / 2) * (DX_PI / 12.0); // ± 15度ずつ
            pEnemyShot->muki = baseAngle + angleOffset;

            // 速度：やや速め
            pEnemyShot->speed = (300 + GetRand(100)) / 100.0;

            // 種類：銃弾 or 小玉（光線っぽく）
            int type = GetRand(1); // 0 or 1
            int color;
            if (GetRand(1) == 0) {
                color = 1; // 黄
            }
            else {
                color = 0; // 赤
            }
            if (type == 0) {
                pEnemyShot->kind = img_enemyShotBullet[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[color];
            }

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // コロナ：敵の周囲を回る弾（円状に配置）
        int coronaCount = 12; // コロナ弾の数
        double radius = 40.0; // 敵からの半径
        for (int i = 0; i < coronaCount; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = (DX_PI * 2.0 * i) / coronaCount;

            // 初期位置：敵の周囲の円上
            pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);

            // 進行方向：円の接線方向（太陽の周りを回るイメージ）
            pEnemyShot->muki = angle + DX_PI / 2.0;

            // 速度：ゆっくり回る
            pEnemyShot->speed = (100 + GetRand(50)) / 100.0;

            // 種類：鱗弾 or 菱形弾（コロナっぽく）
            int type = GetRand(1); // 0 or 1
            int color;
            if (GetRand(2) == 0) {
                color = 1; // 黄
            }
            else {
                color = 6; // 白
            }
            if (type == 0) {
                pEnemyShot->kind = img_enemyShotScale[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotDiamond[color];
            }

            // リストに追加
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
void EnemyPat_Sun_Sakana()
{
    static int muki;
    if (count == 1) {
        // 初期配置
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定フレームごとに弾幕セットを生成
    if (count % 4 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSun; // 太陽モチーフ弾幕を登録
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 弾リストのダミーヘッド初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾幕セットをグローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}