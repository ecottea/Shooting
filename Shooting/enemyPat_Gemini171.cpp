#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：フラクタル・バーンズリーのシダ
static void ShotBarnsleyFern(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音の再生
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double base_x = pEnemyShotSet->x;
        double base_y = pEnemyShotSet->y;

        // バーンズリーのシダを構成する点群の生成（アフィン変換の反復）
        double fx = 0.0, fy = 0.0;
        int total_dots = 130*2; // 弾の総数

        // 収束のための初期ウォームアップ（描画しない）
        for (int i = 0; i < 20; i++) {
            int r = GetRand(99); // 0〜99の100種類
            double nx, ny;
            if (r == 0) { // 1%
                nx = 0.0;
                ny = 0.16 * fy;
            }
            else if (r < 86) { // 85% (1〜85)
                nx = 0.85 * fx + 0.04 * fy;
                ny = -0.04 * fx + 0.85 * fy + 1.6;
            }
            else if (r < 93) { // 7% (86〜92)
                nx = 0.2 * fx - 0.26 * fy;
                ny = 0.23 * fx + 0.22 * fy + 1.6;
            }
            else { // 7% (93〜99)
                nx = -0.15 * fx + 0.28 * fy;
                ny = 0.26 * fx + 0.24 * fy + 0.44;
            }
            fx = nx;
            fy = ny;
        }

        // 実際の弾生成ループ（シダの形状を計算して配置）
        for (int i = 0; i < total_dots; i++) {
            int r = GetRand(99);
            double nx, ny;
            if (r == 0) {
                nx = 0.0;
                ny = 0.16 * fy;
            }
            else if (r < 86) {
                nx = 0.85 * fx + 0.04 * fy;
                ny = -0.04 * fx + 0.85 * fy + 1.6;
            }
            else if (r < 93) {
                nx = 0.2 * fx - 0.26 * fy;
                ny = 0.23 * fx + 0.22 * fy + 1.6;
            }
            else {
                nx = -0.15 * fx + 0.28 * fy;
                ny = 0.26 * fx + 0.24 * fy + 0.44;
            }
            fx = nx;
            fy = ny;

            pEnemyShot = new sEnemyShot;

            // スケーリングと座標変換（シダの標準サイズをゲーム画面に調整）
            double scale = 13.0*2;
            double lx = fx * scale;
            double ly = (fy * scale) - 65.0;

            // ボスの位置を基準に配置
            pEnemyShot->x = base_x + lx;
            pEnemyShot->y = base_y + 20.0 + ly;

            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0; // 初期状態は静止してフラクタル図形を描く

            // 素材の選択: 緑色（色コード:2）の小玉・鱗弾・中玉を組み合わせて質感を表現
            if (i % 6 == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[2]; // 緑の中玉
            }
            else if (i % 2 == 0) {
                pEnemyShot->kind = img_enemyShotScale[2];     // 緑の鱗弾
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[2];  // 緑の小玉
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の挙動制御
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    // 150フレーム経過後、シダの形から全方位へ美しく拡散する演出
    if (pEnemyShotSet->count == 150) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            double dx = pShot->x - pEnemyShotSet->x;
            double dy = pShot->y - pEnemyShotSet->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 0.001) {
                pShot->muki = atan2(dy, dx);
            }
            else {
                pShot->muki = GetRand(100) / 50.0 * DX_PI;
            }
            pShot->speed = 1.0 + GetRand(100) / 100.0 * 1.5;
            pShot = pShot->next;
        }
    }
    else if (pEnemyShotSet->count > 150) {
        // 拡散中の移動
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
    else {
        // 最初の150フレームは静止して、幾何学的なシダのシルエットを見せる
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_BarnsleyFern_Gemini()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        enemy.x += 0.8 * (double)muki;
        if (enemy.x < 120.0 || enemy.x > 360.0) {
            muki *= -1;
        }
    }

    // 300フレーム毎にバーンズリーのシダ弾幕を展開
    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBarnsleyFern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}