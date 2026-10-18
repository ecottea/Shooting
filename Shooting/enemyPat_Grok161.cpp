#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  銃符「ルナティックガン」 (Gun Sign "Lunatic Gun")
//  東方紺珠伝 1面ボス・清蘭のスペルカード再現
//
//  弾幕構成（調査に基づく）:
//    1. 青い座薬弾（銃弾列 / 全方位鈍速弾）
//       - 画面全体・ボス周辺から展開される固定寄り・鈍速の弾列
//       - 塀を作って自機の移動を制限する
//    2. 赤い座薬弾（自機依存の山形/逆三角形の銃弾塊）
//       - 清蘭本人から逆三角形に並んだ弾を自機周辺へ乱射
//       - 抜けようとした方向を上から蓋をするように圧殺
//
//  使用素材:
//    - 弾種: 銃弾 (img_enemyShotBullet) … 座薬弾に近い細長い形状
//    - 色  : 0=赤（自機狙い塊）、4=青（弾列・全方位）
//    - 音  : sound_enemyShot_medium / heavy
// ============================================================

// ------------------------------------------------------------
//  青い弾列（固定寄り・鈍速の銃弾列）
//  ボス周辺および左右から縦に連結したような弾を飛ばし、
//  画面に「塀」を作る
// ------------------------------------------------------------
static void ShotBlueWall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 左右に複数の縦列を形成する鈍速弾
        // kind: 0=左側寄り, 1=中央寄り, 2=右側寄り など
        const int columnCount = 5;
        const double baseX = pEnemyShotSet->x;
        const double spacing = 70.0;

        for (int col = 0; col < columnCount; col++) {
            double colX = baseX + (col - (columnCount - 1) / 2.0) * spacing;
            // 1列あたり縦に数発
            for (int row = 0; row < 4; row++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = colX + (GetRand(10) - 5);          // わずかな揺らぎ
                pEnemyShot->y = pEnemyShotSet->y + row * 18.0;
                // ほぼ真下方向（わずかに外向き）
                double offsetAngle = (col - (columnCount - 1) / 2.0) * 0.08;
                pEnemyShot->muki = DX_PI / 2.0 + offsetAngle;
                pEnemyShot->speed = 1.4 + GetRand(30) / 100.0;     // 鈍速
                pEnemyShot->kind = img_enemyShotBullet[4];         // 青の銃弾
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 追加で全方位に広がる青弾（渦を巻くような雰囲気を簡易再現）
        const int radialCount = 12;
        for (int i = 0; i < radialCount; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = pEnemyShotSet->muki + (i * 2.0 * DX_PI / radialCount);
            pEnemyShot->x = pEnemyShotSet->x + 20.0 * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + 20.0 * sin(angle);
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 1.1 + GetRand(40) / 100.0;
            pEnemyShot->kind = img_enemyShotBullet[4];             // 青
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  赤い自機狙い銃弾塊（逆三角形 / 山形）
//  自機方向に向かって山形に並んだ弾をまとめて飛ばす
// ------------------------------------------------------------
static void ShotRedClump(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 自機への基準角度
        double baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 逆三角形（山形）に配置：上段1、中段3、下段5 程度
        // 行ごとに横幅を広げ、全て同じ向き（自機狙い）で発射
        const int rows = 3;
        int bulletsPerRow[] = { 1, 3, 5 };
        double rowSpacing = 14.0;   // 進行方向に対する前後オフセット
        double sideSpacing = 12.0;  // 横方向の間隔

        for (int r = 0; r < rows; r++) {
            int n = bulletsPerRow[r];
            for (int i = 0; i < n; i++) {
                pEnemyShot = new sEnemyShot;

                // 横方向のオフセット（中央揃え）
                double sideOffset = (i - (n - 1) / 2.0) * sideSpacing;
                // 進行方向のオフセット（後ろほど遅れて見えるように）
                double forwardOffset = r * rowSpacing;

                // 基準位置からオフセットを回転させて配置
                pEnemyShot->x = pEnemyShotSet->x
                    + forwardOffset * cos(baseMuki)
                    + sideOffset * cos(baseMuki + DX_PI / 2.0);
                pEnemyShot->y = pEnemyShotSet->y
                    + forwardOffset * sin(baseMuki)
                    + sideOffset * sin(baseMuki + DX_PI / 2.0);

                // 全て自機方向（わずかに散らして自然に）
                pEnemyShot->muki = baseMuki + (GetRand(20) - 10) / 180.0 * DX_PI;
                pEnemyShot->speed = 2.8 + GetRand(60) / 100.0;   // 青弾より速め
                pEnemyShot->kind = img_enemyShotBullet[0];       // 赤の銃弾

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  敵本体パターン
//  関数名は指定通り void EnemyPat_LunaticGun_Grok()
// ------------------------------------------------------------
void EnemyPat_LunaticGun_Grok()
{
    static int moveDir;      // 移動方向
    static int phase;        // 弾幕の位相管理用

    // 初期化（count はメインルーチンで毎フレーム +1 される）
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;   // 200から変更しないこと
        moveDir = 1;
        phase = 0;
    }
    else {
        // 緩やかな左右移動（1面ボスらしい動き）
        enemy.x += 0.7 * (double)moveDir;
        if (enemy.x < 100.0) {
            enemy.x = 100.0;
            moveDir = 1;
        }
        else if (enemy.x > 380.0) {
            enemy.x = 380.0;
            moveDir = -1;
        }
        // 一定間隔で反転も入れる
        if (count % 150 == 0) {
            moveDir *= -1;
        }
    }

    // ---------- 青い弾列（塀）の生成 ----------
    // 比較的高頻度で発生させ、画面に弾の壁を作る
    if (count >= 30 && count % 45 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBlueWall;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = DX_PI / 2.0;   // 下方向基準
        pEnemyShotSet->kind = phase++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ---------- 赤い自機狙い山形塊の生成 ----------
    // 少し間隔を空けて撃ち、プレイヤーを圧殺する感じを出す
    if (count >= 60 && count % 70 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRedClump;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        // 発射時点の自機方向を記録（patternFunc内で再計算もするが初期値として）
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = phase++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}