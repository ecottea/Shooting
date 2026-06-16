// enemyPat_Sun.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --------------------------------------------------------
// 太陽弾幕 1：コロナ（螺旋状に広がる光）
// --------------------------------------------------------
static void ShotSunSpiral(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 軽い発射音で連続発射の雰囲気を出す
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 12方向へ円状に発射
        for (int i = 0; i < 12; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // セット自体の向き（時間で回転）を基準に、12方向へ等間隔に配置
            pEnemyShot->muki = pEnemyShotSet->muki + (DX_PI * 2.0 / 12.0) * i;

            // 初速は遅めに設定し、後で加速させる
            pEnemyShot->speed = 1.0;

            // 色を赤(0)と黄(1)で交互に変えて太陽の燃え盛る色を表現
            int color = (i % 2 == 0) ? 0 : 1;
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            // リストの末尾に接続
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理（毎フレーム）
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 放射状に徐々に加速させる（最大スピードの制限付き）
        if (pEnemyShot->speed < 4.0) {
            pEnemyShot->speed += 0.02;
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// --------------------------------------------------------
// 太陽弾幕 2：太陽フレア（自機狙いの荒々しい爆発）
// --------------------------------------------------------
static void ShotSolarFlare(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 重い音で爆発を表現
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 8発の大玉をばらまく
        for (int i = 0; i < 8; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // pEnemyShotSet->muki には自機への角度が入っている想定
            // 扇状に散らばるよう、-30度〜+30度のブレを加える (GetRand(60)は0〜60)
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(60) - 30) / 180.0 * DX_PI;

            // 速度にもばらつきを持たせる (2.00 〜 4.50)
            pEnemyShot->speed = (200 + GetRand(250)) / 100.0;

            // フレアなので大玉の赤(0)を使用
            pEnemyShot->kind = img_enemyShotLargeBall[0];

            // リストの末尾に接続
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理（毎フレーム）
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// --------------------------------------------------------
// 敵本体のパターン（太陽の振る舞い）
// --------------------------------------------------------
void EnemyPat_Sun_Gemini()
{
    // 初期化
    if (count == 1) {
        enemy.x = 240.0; // 画面中央
        enemy.y = 80.0;  // 空に浮かぶ太陽のように上部に配置
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // ゆったりと上下に浮遊させる（サイン波を使用）
        enemy.y = 80.0 + 10.0 * sin(count / 60.0 * DX_PI);
    }

    // 1. コロナ弾幕の生成（10フレームに1回）
    if (count % 7 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSunSpiral;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        // 螺旋を描くように、基準となる角度を時間で回転させる（毎フレーム1.5度回転）
        pSet->muki = (count * 1.5) / 180.0 * DX_PI;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 2. 太陽フレア弾幕の生成（90フレームに1回）
    if (count % 90 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotSolarFlare;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        // 自機を狙う角度を計算
        pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}