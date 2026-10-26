#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// マスゲーム・フォーメーション
// 既存弾（小玉・中玉・短レーザー・銃弾など）を組み合わせて
// 集団が同期して絵を作るような弾幕を表現
// ============================================================
static void ShotMassGame(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int COLS = 12+4;
    const int ROWS = 7+3;
    const double GRID_SPACING_X = 32.0;
    const double GRID_SPACING_Y = 26.0;
    const double BASE_CENTER_X = 240.0;
    const double BASE_CENTER_Y = 150.0;

    // -------------------------------------------------------
    // 初期化（count == 0 のときのみ弾を生成）
    // -------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 画面上部にグリッド状に小玉を配置（人が並んでいるイメージ）
        double startX = BASE_CENTER_X - (COLS - 1) * GRID_SPACING_X * 0.5;
        double startY = -80.0; // 画面外上から

        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = startX + c * GRID_SPACING_X + (GetRand(8) - 4);
                pEnemyShot->y = startY + r * GRID_SPACING_Y + (GetRand(6) - 3);
                pEnemyShot->muki = DX_PI * 0.5; // 下向き
                pEnemyShot->speed = 0.0;
                // 初期は青系の小玉
                pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青
                // グリッド座標と状態を保存
                pEnemyShot->param_i[0] = c;          // 列
                pEnemyShot->param_i[1] = r;          // 行
                pEnemyShot->param_i[2] = 0;          // 0:フォーメーション制御中  1:攻撃モード
                pEnemyShot->param_d[0] = 0.0;        // 予備
                pEnemyShot->param_d[1] = 0.0;

                // リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        // セット自体にグリッド情報を保存
        pEnemyShotSet->param_i[0] = COLS;
        pEnemyShotSet->param_i[1] = ROWS;
        pEnemyShotSet->param_i[2] = 0; // フェーズ補助
    }

    // -------------------------------------------------------
    // 毎フレーム更新
    // -------------------------------------------------------
    int phase = pEnemyShotSet->count;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int c = pShot->param_i[0];
        int r = pShot->param_i[1];
        int state = pShot->param_i[2];

        // ===== フェーズ1: 整列・カードフリップ（0〜90） =====
        if (phase < 90) {
            // 目標グリッド位置へ滑らかに移動
            double targetX = BASE_CENTER_X - (COLS - 1) * GRID_SPACING_X * 0.5 + c * GRID_SPACING_X;
            double targetY = 60.0 + r * GRID_SPACING_Y;
            pShot->x += (targetX - pShot->x) * 0.09;
            pShot->y += (targetY - pShot->y) * 0.09;

            // 同期した色変化（マスゲームのカードフリップ風）
            int colorIndex = ((phase / 8) + c + r * 2) % 9;
            // 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
            pShot->kind = img_enemyShotSmallBall[colorIndex];
        }
        // ===== フェーズ2: 楕円フォーメーション形成＋輪郭レーザー化（90〜210） =====
        else if (phase < 210) {
            double t = (phase - 90) / 120.0; // 0.0〜1.0
            double angle = (c + r * 0.7) / (double)COLS * 2.0 * DX_PI + phase * 0.018;
            double radiusX = 40.0 + 110.0 * t;
            double radiusY = 30.0 + 70.0 * t;
            double centerY = BASE_CENTER_Y + 25.0 * sin(phase * 0.04);

            double tx = BASE_CENTER_X + radiusX * cos(angle);
            double ty = centerY + radiusY * sin(angle);

            pShot->x += (tx - pShot->x) * 0.11;
            pShot->y += (ty - pShot->y) * 0.11;

            // 輪郭部分を短レーザーに変化させて強調
            bool isEdge = (c == 0 || c == COLS - 1 || r == 0 || r == ROWS - 1);
            if (isEdge && phase > 130) {
                int laserColor = ((phase / 12) + c) % 9;
                pShot->kind = img_enemyShotLaser[laserColor];
                // レーザーは接線方向を向かせる
                pShot->muki = angle + DX_PI * 0.5;
            }
            else {
                // 内側は中玉で存在感を出す（色はゆっくり変化）
                int colorIndex = ((phase / 15) + c * 3 + r) % 9;
                pShot->kind = img_enemyShotMediumBall[colorIndex];
            }
        }
        // ===== フェーズ3: 波打ち攻撃（210〜360） =====
        else if (phase < 360) {
            // 全体が波打つように上下移動
            double wave = 18.0 * sin(phase * 0.09 + c * 0.45 + r * 0.2);
            double baseY = BASE_CENTER_Y + 40.0 + (phase - 210) * 0.35; // 徐々に下降
            double targetY = baseY + r * 18.0 + wave;
            double targetX = BASE_CENTER_X - (COLS - 1) * 28.0 * 0.5 + c * 28.0
                + 12.0 * sin(phase * 0.05 + r * 0.3);

            if (state == 0) {
                pShot->x += (targetX - pShot->x) * 0.08;
                pShot->y += (targetY - pShot->y) * 0.08;

                // 下段の弾を一定間隔で攻撃モードに切り替え
                if (r >= 4 && (phase - 210) % 18 == 0 && GetRand(4) <= 1) {
                    pShot->param_i[2] = 1; // 攻撃モードへ
                    pShot->speed = 2.8 + GetRand(120) / 100.0;
                    // 自機狙い＋少しばらつき
                    double aim = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->muki = aim + (GetRand(50) - 25) / 180.0 * DX_PI;
                    // 攻撃弾は銃弾や鱗弾に変更
                    if (GetRand(1) == 0) {
                        pShot->kind = img_enemyShotBullet[0]; // 赤の銃弾
                    }
                    else {
                        pShot->kind = img_enemyShotScale[5]; // マゼンタの鱗弾
                    }
                }
                else {
                    // まだフォーメーション内の弾は色を変え続ける
                    int colorIndex = ((phase / 10) + c + r) % 9;
                    if (c == 0 || c == COLS - 1 || r == 0) {
                        pShot->kind = img_enemyShotLaser[colorIndex];
                    }
                    else {
                        pShot->kind = img_enemyShotSmallBall[colorIndex];
                    }
                }
            }
            else {
                // 攻撃モードの弾は速度で移動
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }

            // 追加で新しい弾を放出（フォーメーションから撃ち出すイメージ）
            if ((phase - 210) % 25 == 5 && r == ROWS - 1 && c % 3 == 0 && pShot->kind != img_enemyShotDiamond[1]) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = pShot->x;
                pNew->y = pShot->y;
                pNew->speed = 3.2 + GetRand(80) / 100.0;
                double aim = atan2(player.y - pNew->y, player.x - pNew->x);
                pNew->muki = aim + (GetRand(30) - 15) / 180.0 * DX_PI;
                pNew->kind = img_enemyShotDiamond[1]; // 黄の菱形弾
                pNew->param_i[0] = c;
                pNew->param_i[1] = r;
                pNew->param_i[2] = 1; // 最初から攻撃モード

                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;

                // 発射音（重ならないように軽く）
                if (c == 0 || c == 6) {
                    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                }
            }
        }
        // ===== フェーズ4: 崩壊・収束攻撃（360〜） =====
        else {
            if (state == 0) {
                // まだフォーメーションに残っている弾を攻撃モードへ
                pShot->param_i[2] = 1;
                pShot->speed = 2.0 + GetRand(150) / 100.0;
                // 自機方向へ収束
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x)
                    + (GetRand(40) - 20) / 180.0 * DX_PI;
                // 最終弾は中玉や大玉で圧をかける
                int finalColor = GetRand(8);
                if (GetRand(3) == 0) {
                    pShot->kind = img_enemyShotMediumBall[finalColor];
                }
                else {
                    pShot->kind = img_enemyShotSmallBall[finalColor];
                }
            }
            // 速度で移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }

    // 崩壊時に一度だけ強めの音を鳴らす
    if (phase == 360) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_MassGame_Grok()
{
    static int moveDir = 1;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        moveDir = 1;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.6 * (double)moveDir;
        if (enemy.x < 120.0 || enemy.x > 360.0) {
            moveDir *= -1;
        }
    }

    // マスゲーム弾幕を1回だけ生成（長時間持続する大技）
    if (count % 300 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMassGame;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}