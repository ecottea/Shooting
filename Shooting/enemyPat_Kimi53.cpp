// enemyPat_absoluteApex.cpp
// 弾幕「Absolute Apex」- 大文字「A」をモチーフにした弾幕

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 定数定義
// ============================================================
static constexpr double PI = DX_PI;
static constexpr double DEG2RAD = PI / 180.0;

// ゲーム画面サイズ
static constexpr int SCREEN_W = 480;
static constexpr int SCREEN_H = 480;

// ============================================================
// 弾の種類・色のエイリアス
// ============================================================
// 弾の種類: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、
//           鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
// 弾の色: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙

// 色を指定して弾画像インデックスを取得
static int GetShotKind(int baseKind, int color)
{
    // baseKind は 0〜7（小玉〜短レーザー）
    // color は 0〜8
    return 4 + baseKind * 9 + color;
}

// ============================================================
// 第1段階：頂点の収束（Apex Convergence）
// 画面左右端上部から頂点に向かって収束する細いレーザー
// ============================================================
static void ShotApexConvergence(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 頂点位置（画面中央上部）
        double apexX = SCREEN_W / 2.0;
        double apexY = 60.0;

        // 左右から頂点へ向かうレーザー（各5本、計10本）
        for (int i = 0; i < 30; i++) {
            pEnemyShot = new sEnemyShot;

            // 左右交互に配置
            bool isLeft = (i % 2 == 0);
            double startX = isLeft ? 10.0 + (i / 2) * 40.0 : SCREEN_W - 10.0 - (i / 2) * 40.0;
            double startY = 20.0 + (i / 2) * 15.0;

            pEnemyShot->x = startX;
            pEnemyShot->y = startY;

            // 頂点へ向かう角度
            pEnemyShot->muki = atan2(apexY - startY, apexX - startX);
            pEnemyShot->speed = 3.0 + (i % 3) * 0.5;

            // 短レーザー、色は橙→赤のグラデーション風
            int color = (i < 5) ? 8 : 0;  // 橙 or 赤
            pEnemyShot->kind = GetShotKind(7, color);  // 短レーザー

            // パラメータ: 頂点通過後の動作制御用
            pEnemyShot->param_i[0] = 0;  // 頂点通過フラグ
            pEnemyShot->param_d[0] = apexX;  // 目標頂点X
            pEnemyShot->param_d[1] = apexY;  // 目標頂点Y
            pEnemyShot->margin = 40;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 頂点付近を通過したら、少し滞留後に下方向へ反射
        double dx = pShot->x - pShot->param_d[0];
        double dy = pShot->y - pShot->param_d[1];
        if (!pShot->param_i[0] && dx * dx + dy * dy < 400.0) {
            pShot->param_i[0] = 1;
            pShot->muki = PI / 2.0 + (GetRand(60) - 30) * DEG2RAD;  // 下方向に広がる
            pShot->speed = 2.0;
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 第2段階：両脚の展開（Leg Expansion）
// V字型に広がる弾幕の両脚。呼吸するように開閉する
// ============================================================
static void ShotLegExpansion(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 射撃音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 脚の基本パラメータ
        pEnemyShotSet->param_d[0] = SCREEN_W / 2.0;  // 頂点X
        pEnemyShotSet->param_d[1] = 80.0;             // 頂点Y
        pEnemyShotSet->param_d[2] = 5.0 * DEG2RAD; // 脚の基本開き角度
        pEnemyShotSet->param_i[0] = 0;               // 発射済み弾数
    }

    // 毎フレーム、脚の先端から弾を発射（連続的に流れる弾の列）
    if (pEnemyShotSet->count % 3 == 0 && pEnemyShotSet->param_i[0] < 120) {
        // 呼吸する開閉（sin波）
        double breathe = sin(pEnemyShotSet->count * 0.05) * 0.3 + 1.0;  // 0.7〜1.3
        double baseAngle = pEnemyShotSet->param_d[2] * breathe;

        // 左脚
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->param_d[0];
        pEnemyShot->y = pEnemyShotSet->param_d[1];
        pEnemyShot->muki = PI / 2.0 + baseAngle;  // 左下方向
        pEnemyShot->speed = 2.5;
        pEnemyShot->kind = GetShotKind(0, 0);  // 小玉・赤
        pEnemyShot->param_i[0] = 0;  // 脚の識別（0=左脚）

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 右脚
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->param_d[0];
        pEnemyShot->y = pEnemyShotSet->param_d[1];
        pEnemyShot->muki = PI / 2.0 - baseAngle;  // 右下方向
        pEnemyShot->speed = 2.5;
        pEnemyShot->kind = GetShotKind(0, 8);  // 小玉・橙
        pEnemyShot->param_i[0] = 1;  // 脚の識別（1=右脚）

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        pEnemyShotSet->param_i[0] += 2;
    }

    // 弾の移動処理（脚の開閉に追従して角度を微調整）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 弾が下に進むにつれて、脚の開きを維持するため角度を少し広げる
        if (pShot->count < 30) {
            double openRate = pShot->count * 0.0003;
            if (pShot->param_i[0] == 0) {
                pShot->muki += openRate;  // 左脚はさらに左へ
            }
            else {
                pShot->muki -= openRate;  // 右脚はさらに右へ
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 第3段階：横棒の切断（Crossbar Intersection）
// 水平方向の弾幕の帯。厚みが周期的に変化
// ============================================================
static void ShotCrossbar(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 射撃音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_d[0] = 220.0;  // 横棒のY座標
        pEnemyShotSet->param_i[0] = 0;       // 発射済みフラグ
    }

    // 横棒は1回だけ大量に発射
    if (pEnemyShotSet->count == 1 && !pEnemyShotSet->param_i[0]) {
        pEnemyShotSet->param_i[0] = 1;

        // 横棒の厚み変化を表現（3段階の行）
        for (int row = 0; row < 3; row++) {
            // 各行のYオフセット（中央を基準）
            double yOffset = (row - 1) * 12.0;
            double rowY = pEnemyShotSet->param_d[0] + yOffset;

            // 行ごとに色を変える（中央が最も濃い）
            int color;
            if (row == 1) color = 0;      // 中央: 赤
            else if (row == 0) color = 8; // 上: 橙
            else color = 1;                // 下: 黄

            for (int i = 0; i < 24; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = 20.0 + i * 19.0;  // 画面横断
                pEnemyShot->y = rowY;
                pEnemyShot->muki = row % 2 ? DX_PI : 0.0;  // 右方向
                pEnemyShot->speed = 1.5 + row * 0.3;  // 行ごとに速度変化
                pEnemyShot->kind = GetShotKind(3, color);  // 銃弾
                pEnemyShot->param_i[0] = row;  // 行識別
                pEnemyShot->param_d[0] = rowY; // 基準Y

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の移動処理（微振動）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 上下微振動（横棒が"生きている"感じ）
        pShot->y += sin(pShot->count * 0.1 + pShot->param_i[0]) * 0.3;

        pShot = pShot->next;
    }
}

// ============================================================
// 最終段階：完全形「A」の顕現
// 3要素が同時発動。安全地帯は「A」の負の空間
// ============================================================
static void ShotCompleteA(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 強力な射撃音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // ===== Aの輪郭を描く弾 =====
        double apexX = SCREEN_W / 2.0;
        double apexY = 60.0;
        double legAngle = 22.0 * DEG2RAD;
        int numLegShots = 20;

        // 左脚の輪郭
        for (int i = 0; i < numLegShots; i++) {
            double t = (double)i / (numLegShots - 1);
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = apexX + (t * SCREEN_W * 0.4) * cos(PI / 2.0 + legAngle);
            pEnemyShot->y = apexY + (t * SCREEN_H * 0.65);
            pEnemyShot->muki = PI / 2.0 + legAngle + 0.05;
            pEnemyShot->speed = 1.8;
            pEnemyShot->kind = GetShotKind(1, 0);  // 中玉・赤
            pEnemyShot->param_i[0] = 0;  // 輪郭識別

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 右脚の輪郭
        for (int i = 0; i < numLegShots; i++) {
            double t = (double)i / (numLegShots - 1);
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = apexX + (t * SCREEN_W * 0.4) * cos(PI / 2.0 - legAngle);
            pEnemyShot->y = apexY + (t * SCREEN_H * 0.65);
            pEnemyShot->muki = PI / 2.0 - legAngle - 0.05;
            pEnemyShot->speed = 1.8;
            pEnemyShot->kind = GetShotKind(1, 8);  // 中玉・橙
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 横棒の輪郭
        int numCrossShots = 20;
        double crossY = 220.0;
        for (int i = 0; i < numCrossShots; i++) {
            double t = (double)i / (numCrossShots - 1);
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = apexX - 80.0 + t * 160.0;
            pEnemyShot->y = crossY;
            pEnemyShot->muki = DX_PI / 2;
            pEnemyShot->speed = 1.2;
            pEnemyShot->kind = GetShotKind(1, 1);  // 中玉・黄
            pEnemyShot->param_i[0] = 1;  // 横棒識別

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ===== Aの内部を埋める弾（安全地帯を作るため、中央は空ける） =====
        // 内部の"埋め弾"はAの形に沿って配置し、中央の三角形エリアは避ける
        for (int ring = 0; ring < 45; ring++) {
            double innerT = (ring + 1) * 0.05;
            int shotsInRing = 12 + ring * 4;

            for (int i = 0; i < shotsInRing; i++) {
                double angle = (double)i / shotsInRing * 2.0 * PI;
                double legSpread = legAngle * (0.5 + innerT * 0.5);

                // Aの内部の範囲に限定（中央の三角形は除外）
                double shotAngle = angle;
                if (shotAngle > PI / 2.0 - legSpread && shotAngle < PI / 2.0 + legSpread) {
                    // 脚の間の角度ならスキップ（安全地帯）
                    continue;
                }

                pEnemyShot = new sEnemyShot;
                double dist = 30.0 + ring * 25.0;
                pEnemyShot->x = apexX + dist * cos(shotAngle);
                pEnemyShot->y = apexY + dist * sin(shotAngle) * 0.7;
                pEnemyShot->muki = shotAngle;
                pEnemyShot->speed = 1.0 + ring * 0.2;
                pEnemyShot->kind = GetShotKind(0, 5);  // 小玉・マゼンタ
                pEnemyShot->param_i[0] = 2;  // 内部弾識別
                pEnemyShot->margin = 480;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 輪郭弾はゆっくり下へ流れる
        if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
            pShot->y += 0.3;
        }
        // 内部弾は少し拡散
        else if (pShot->param_i[0] == 2) {
            pShot->muki += sin(pShot->count * 0.02) * 0.01;
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン「Absolute Apex」
// ============================================================
void EnemyPat_Daimonji_Kimi()
{
    static int muki;
    static int shot_count;
    static int phase;        // 現在の段階
    static int phaseTimer;   // 段階内タイマー

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
        phase = 0;
        phaseTimer = 0;
    }
    else {
        // 敵の左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 段階管理
    phaseTimer++;

    // 段階遷移
    switch (phase) {
    case 0:  // 第1段階：頂点の収束（0〜180フレーム）
        if (phaseTimer == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotApexConvergence;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = 0.0;
            pSet->kind = shot_count++;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        if (phaseTimer >= 180) {
            phase = 1;
            phaseTimer = 0;
        }
        break;

    case 1:  // 第2段階：両脚の展開（180〜420フレーム）
        if (phaseTimer == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotLegExpansion;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = 0.0;
            pSet->kind = shot_count++;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        if (phaseTimer >= 120) {
            phase = 2;
            phaseTimer = 0;
        }
        break;

    case 2:  // 第3段階：横棒の切断（420〜660フレーム）
        if (phaseTimer == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotCrossbar;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = 0.0;
            pSet->kind = shot_count++;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        if (phaseTimer >= 120) {
            phase = 3;
            phaseTimer = 0;
        }
        break;

    case 3:  // 最終段階：完全形「A」の顕現（660〜900フレーム）
        if (phaseTimer == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotCompleteA;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = 0.0;
            pSet->kind = shot_count++;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        if (phaseTimer >= 300) {
            phase = 0;  // ループ
            phaseTimer = 0;
        }
        break;
    }
}