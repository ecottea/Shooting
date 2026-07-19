// enemyPat_Tmp.cpp
// 扇風機モチーフ弾幕パターン
// 特徴: 左右に首を振りながら扇状に「風」を吹き出すような拡散弾幕＋回転する羽根をイメージした曲がり弾
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 扇風機風：扇状拡散＋回転羽根弾
static void ShotFanBlade(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 軽めの風をイメージした効果音
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int fanCount = 14;
        const double baseAngle = pEnemyShotSet->muki;
        const double spread = DX_PI * 0.48; // 広い扇状（約55度くらい）

        // メインの風弾（青系中心の小玉・鱗弾で涼しげな風を表現）
        for (int i = 0; i < fanCount; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 10.0;

            double angleOffset = spread * (2.0 * i / (fanCount - 1) - 1.0);
            pEnemyShot->muki = baseAngle + angleOffset + (GetRand(16) - 8) / 180.0 * DX_PI * 0.4;

            pEnemyShot->speed = (140 + GetRand(110)) / 100.0; // 風らしいやや遅めの速度

            // 自分で選んだ組み合わせ：青・シアン系の小玉と鱗弾をメインに
            int sel = GetRand(5);
            if (sel <= 2) {
                pEnemyShot->kind = img_enemyShotSmallBall[4];  // 青
            }
            else if (sel == 3) {
                pEnemyShot->kind = img_enemyShotSmallBall[3];  // シアン
            }
            else {
                pEnemyShot->kind = img_enemyShotScale[4];      // 青鱗弾
            }
            
            // リンク挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 回転羽根をイメージした追加弾（中玉・菱形弾で少し重みを持たせる）
        if (pEnemyShotSet->kind % 4 == 0) {
            for (int k = 0; k < 5; k++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y + 10.0;
                pEnemyShot->muki = baseAngle + DX_PI * 2.0 * k / 5.0;
                pEnemyShot->speed = 2.4;
                pEnemyShot->kind = img_enemyShotMediumBall[2]; // 緑系中玉
                pEnemyShot->param_i[0] = 1; // 回転羽根フラグ

                pEnemyShot->margin = 200;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレーム移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == 1) {
            // 羽根弾は徐々に曲がる（回転感）
            pEnemyShot->muki += 0.022;
            pEnemyShot->speed *= 0.995; // 少し減速して弧を描く
        }
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        if (pEnemyShot->count >= 1800) pEnemyShot->x = 9999;

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体パターン：扇風機
void EnemyPat_ElectricFan_Grok()
{
    static int muki = 1;
    static int shotTimer = 0;
    static double swayPhase = 0.0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotTimer = 0;
        swayPhase = 0.0;
    }
    else {
        // 扇風機らしいゆったりした左右移動＋首振り風の上下微動
        enemy.x += 0.95 * (double)muki;
        swayPhase += 0.085;
        enemy.y = 55.0 + sin(swayPhase) * 22.0;

        if (count % 105 == 50) muki *= -1;
    }

    // 扇状弾幕の連射（首振りタイミングに同期）
    shotTimer++;
    if (shotTimer % 6 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFanBlade;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;

        // プレイヤー方向をベースに、扇風機の首振りで角度を振る
        double target = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->muki = target + sin(count * 0.13) * 0.52;

        pEnemyShotSet->kind = shotTimer;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}