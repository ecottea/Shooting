// enemyPat_Tmp.cpp
// ドップラー効果をモチーフにした弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ボスの移動パラメータ
// ============================================================
static double bossAngle = 0.0;       // 楕円軌道上の角度
static int    bossDirection = 1;     // 回転方向 (1: 正回転, -1: 逆回転)
static const double ellipseA = 160.0; // 長半径
static const double ellipseB = 100.0; // 短半径
static const double angleSpeed = 0.02; // 角速度 (rad/frame)

// ============================================================
//  弾幕パターン関数：ドップラー・ウェーブ
// ============================================================
static void ShotDoppler(sEnemyShotSet* pEnemyShotSet)
{
    // 初回フレーム：前回位置を現在位置で初期化
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_d[0] = enemy.x;
        pEnemyShotSet->param_d[1] = enemy.y;
    }

    // エミッタ位置をボスに追従
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // ボスの速度ベクトルを計算（前フレームとの差分）
    double vx = enemy.x - pEnemyShotSet->param_d[0];
    double vy = enemy.y - pEnemyShotSet->param_d[1];

    // 前回位置を更新
    pEnemyShotSet->param_d[0] = enemy.x;
    pEnemyShotSet->param_d[1] = enemy.y;

    // 30フレームごとに円形弾幕を放射（初回は速度が安定してから）
    if (pEnemyShotSet->count > 1 && pEnemyShotSet->count % 15 == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 16方向に弾を発射
        for (int i = 0; i < 16; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            double baseAngle = i * (2.0 * DX_PI / 16.0);
            double baseSpeed = 2.0; // 基本速度

            // ボス速度の影響を加味した弾の速度ベクトル
            double factor = 0.8; // 影響度
            double bx = baseSpeed * cos(baseAngle) + vx * factor;
            double by = baseSpeed * sin(baseAngle) + vy * factor;

            pShot->muki = atan2(by, bx);
            pShot->speed = sqrt(bx * bx + by * by);

            pShot->x = enemy.x;
            pShot->y = enemy.y;

            // 基本方向とボス速度の内積でドップラー領域を判定
            double velLen = sqrt(vx * vx + vy * vy);
            double dot = 0.0;
            if (velLen > 0.001) {
                double nx = vx / velLen;
                double ny = vy / velLen;
                dot = cos(baseAngle) * nx + sin(baseAngle) * ny;
            }

            // 色の選択：前方＝青、後方＝赤、側方＝白
            int colorIndex;
            if (dot > 0.3) {
                colorIndex = 4; // 青
            }
            else if (dot < -0.3) {
                colorIndex = 0; // 赤
            }
            else {
                colorIndex = 6; // 白
            }

            // 小玉弾を使用（色は配列のインデックスで指定）
            pShot->kind = img_enemyShotSmallBall[colorIndex];
            pShot->margin = 120;

            // 弾リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 既存の弾を移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_Doppler_DeepSeek()
{
    static int shotSetCreated = 0; // 弾幕セット作成フラグ

    if (count == 1) {
        // 初期位置（楕円の開始点）
        enemy.x = player.x + ellipseA * cos(bossAngle);
        enemy.y = player.y + ellipseB * sin(bossAngle);
        enemy.maxHp = enemy.hp = 30 * 60;
        bossAngle = 0.0;
        bossDirection = 1;
        shotSetCreated = 0;
    }
    else {
        // 120フレームごとに回転方向を反転
        if (count % 240 == 0) {
            bossDirection *= -1;
        }

        // 角度を進める
        bossAngle += bossDirection * angleSpeed;

        // プレイヤーを中心とした楕円軌道で移動
        enemy.x = player.x + ellipseA * cos(bossAngle);
        enemy.y = player.y + ellipseB * sin(bossAngle);
    }
    enemy.hp--;

    // 最初の1回だけドップラー弾幕セットを作成
    if (!shotSetCreated) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDoppler;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0; // 未使用
        pEnemyShotSet->kind = 0;

        // 弾リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        shotSetCreated = 1;
    }
}