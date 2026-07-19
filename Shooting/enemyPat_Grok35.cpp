// enemyPat_Tmp.cpp
// 「裂鞭波 (Cracking Whip Wave)」ワインダー弾幕パターン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ワインダー本体処理
static void ShotWinder(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int PHASE = pEnemyShotSet->count;

    if (PHASE == 0) {
        // 初回のみ効果音と初期化
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 自由パラメータ初期化
        pEnemyShotSet->param_i[0] = 0;        // 発射済み弾数カウンタ
        pEnemyShotSet->param_d[0] = 0.0;      // 現在の位相 (sin用)
        pEnemyShotSet->param_d[1] = 65.0;     // 最大振幅
        pEnemyShotSet->param_d[2] = 0.28;     // 角周波数 (素早い鞭のしなり)
    }

    // 毎フレーム少しずつ発射（高速連射）
    if (PHASE % 2 == 0) {
        int fireCount = 2; // 1フレームに2発で密度確保
        for (int i = 0; i < fireCount; i++) {
            if (pEnemyShotSet->param_i[0] > 280) break; // 総弾数制限

            pEnemyShot = new sEnemyShot;

            // ワインダーの核心：発射座標をsin波で大きくずらす
            double phase = pEnemyShotSet->param_d[0];
            double offsetX = sin(phase) * pEnemyShotSet->param_d[1];

            pEnemyShot->x = pEnemyShotSet->x + offsetX;
            pEnemyShot->y = pEnemyShotSet->y;

            // 基本は真下方向、少し外側に広がるように角度を付ける
            double angleOffset = sin(phase * 1.8) * 0.25; // 鞭の先端がしなる表現
            pEnemyShot->muki = DX_PI / 2.0 + angleOffset; // 下方向 (π/2)

            pEnemyShot->speed = 2.8 + (pEnemyShotSet->param_i[0] % 12) * 0.03; // やや加速気味

            // 弾の種類：中玉を中心に使い、色はシアン〜青系で統一感
            pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン

            // リンク挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            pEnemyShotSet->param_i[0]++;
        }
    }

    // 位相更新（ワインダーのうねり）
    pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[2];

    // 振幅の変化（最初に拡大 → 後半に収束）
    if (PHASE < 85) {
        pEnemyShotSet->param_d[1] = 65.0 * (1.0 - pow(1.0 - (PHASE / 85.0), 2.0));
    }
    else {
        pEnemyShotSet->param_d[1] *= 0.985; // 徐々に収束
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }

    // パターン終了判定（約4秒程度で終了）
    if (PHASE > 240) {
        // 残った弾は自然に消えるように任せる
    }
}

// 敵本体パターン
void EnemyPat_Winder_Grok()
{
    static int muki = 1;
    static int shotTimer = 0;

    if (count == 1) {
        // 初期配置（ゲーム画面 480x480）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotTimer = 0;
    }
    else {
        // 緩やかな左右移動
        enemy.x += 1.15 * (double)muki;
        if (enemy.x < 120.0 || enemy.x > 360.0) muki *= -1;
    }

    // 定期的にワインダー発動
    shotTimer++;
    if (shotTimer % 110 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWinder;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y - 18.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 弾リスト初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵ショットセットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}