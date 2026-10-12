#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：幾何学「ルーローの定幅ローテーション」
static void ShotRouxTriangle(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 効果音の再生
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 1. ルーローの三角形の輪郭を形作る小玉を多数生成 (60個)
        int numParticles = 60;
        for (int i = 0; i < numParticles; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->param_d[0] = (double)i / (double)numParticles; // 位相 (0.0 ～ 1.0)
            pEnemyShot->param_i[0] = 0; // 輪郭フラグ
            pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアンの小玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 2. 3つの頂点（角）を強調する中玉を3つ生成
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->param_d[0] = (double)i / 3.0; // 頂点位相
            pEnemyShot->param_i[0] = 1; // 頂点フラグ
            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄の中玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 3. 一定間隔で中心から低速のインナー弾を放射状に発射 (45フレームごと)
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 45 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        double baseAngle = (double)pEnemyShotSet->count * 0.05;
        for (int i = 0; i < 5*5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + (double)i * (2.0 * DX_PI / 5.0/5);
            pEnemyShot->speed = 1.5;
            pEnemyShot->kind = img_enemyShotDiamond[2]; // 緑の菱形弾
            pEnemyShot->param_i[0] = 2; // 通常移動弾フラグ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 4. 弾の移動・位置更新ループ（カウント等はメインルーチンで管理されるため、ここでは座標の再計算のみ行います）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    double time = (double)pEnemyShotSet->count;
    double scale = 40.0 + fmod(time * 0.4, 70.0); // 緩やかに拡大・収縮を繰り返す
    double baseAngle = time * 0.015;               // ゆっくり回転

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 2) {
            // インナー弾：通常の等速直線運動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // ルーローの三角形の輪郭を形成する弾
            double t = pShot->param_d[0];
            double theta = t * 2.0 * DX_PI + baseAngle;
            double x_offset = 0.0;
            double y_offset = 0.0;

            if (pShot->param_i[0] == 1) {
                // 頂点（角）の位置計算
                int vertexIdx = (int)(t * 3.0 + 0.5) % 3;
                double vAngle = baseAngle + (double)vertexIdx * (2.0 * DX_PI / 3.0);
                x_offset = scale * cos(vAngle);
                y_offset = scale * sin(vAngle);
            }
            else {
                // 輪郭（円弧部分）の数式表現
                double r = scale * (0.866 + 0.134 * cos(3.0 * (theta - baseAngle)));
                x_offset = r * cos(theta);
                y_offset = r * sin(theta);
            }

            // ボス位置を基準にした絶対座標を毎フレーム更新
            pShot->x = pEnemyShotSet->x + x_offset;
            pShot->y = pEnemyShotSet->y + y_offset;
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_ReuleauxTriangle_Gemini()
{
    static int muki;
    static int shot_count;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // ボスが左右に移動
        enemy.x += 0.8 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 一定周期でルーローの三角形弾幕セットを生成 (300フレームごと)
    if (count % 300 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRouxTriangle;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}