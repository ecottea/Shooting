#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 三節棍モチーフ弾幕「三節連撃波」
// 中玉(赤)3つを節本体、小玉(黄)を鎖、短レーザー(青)を打撃衝撃波として表現
// ============================================================

// 節間距離（ピクセル）
static const double SECTION_LEN = 48.0;
// 鎖の小玉数（各節間）
static const int CHAIN_NUM = 8;

// 三節棍パターン本体
static void ShotSanSetsu(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ---------- 初期化（count == 0）----------
    if (pEnemyShotSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // ショットセットのパラメータ初期化
        // param_d[0] : 現在の第1関節角度（ラジアン、初期0＝真下）
        // param_d[1] : 現在の第2関節角度
        // param_d[2] : ベースX（出現位置）
        // param_d[3] : ベースY
        // param_d[4] : 突進用の目標角度（後で設定）
        // param_i[0] : フェーズ (0:下降 1:第1折り 2:第2折り 3:突進 4:終了)
        // param_i[1] : フェーズ内カウンタ
        pEnemyShotSet->param_d[0] = 0.0;          // joint1 angle
        pEnemyShotSet->param_d[1] = 0.0;          // joint2 angle
        pEnemyShotSet->param_d[2] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[3] = pEnemyShotSet->y;
        pEnemyShotSet->param_d[4] = 0.0;
        pEnemyShotSet->param_i[0] = 0;            // phase
        pEnemyShotSet->param_i[1] = 0;            // phase timer

        // --- 3つの節本体（中玉・赤）を生成 ---
        // param_i[0] = 節番号 (0=先端側, 1=中間, 2=根元側)
        // param_i[1] = 1 なら節本体
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + i * SECTION_LEN;
            pEnemyShot->muki = DX_PI / 2.0;       // 下向き
            pEnemyShot->speed = 0.0;              // 位置を直接制御するので0
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤の中玉
            pEnemyShot->param_i[0] = i;           // 節番号
            pEnemyShot->param_i[1] = 1;           // 節フラグ
            pEnemyShot->param_d[0] = 0.0;
            pEnemyShot->param_d[1] = 0.0;
            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // --- 鎖の小玉（黄）を各節間に生成 ---
        // param_i[0] = 所属する節間 (0=0-1間, 1=1-2間)
        // param_i[1] = 0 なら鎖
        // param_i[2] = 節間内のインデックス (0〜CHAIN_NUM-1)
        for (int seg = 0; seg < 2; seg++) {
            for (int c = 0; c < CHAIN_NUM; c++) {
                pEnemyShot = new sEnemyShot;
                double t = (c + 1.0) / (CHAIN_NUM + 1.0); // 0〜1の補間位置
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y + (seg + t) * SECTION_LEN;
                pEnemyShot->muki = DX_PI / 2.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄の小玉
                pEnemyShot->param_i[0] = seg;
                pEnemyShot->param_i[1] = 0;       // 鎖フラグ
                pEnemyShot->param_i[2] = c;
                pEnemyShot->margin = 240;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        return;
    }

    // ---------- 毎フレーム処理 ----------
    int phase = pEnemyShotSet->param_i[0];
    int phaseTimer = pEnemyShotSet->param_i[1];
    double joint1 = pEnemyShotSet->param_d[0];
    double joint2 = pEnemyShotSet->param_d[1];
    double baseX = pEnemyShotSet->param_d[2];
    double baseY = pEnemyShotSet->param_d[3];

    // フェーズ進行
    phaseTimer++;
    pEnemyShotSet->param_i[1] = phaseTimer;

    // フェーズ遷移
    // 0:下降(40f) → 1:第1折り(50f) → 2:第2折り(50f) → 3:突進(60f) → 4:終了
    if (phase == 0 && phaseTimer >= 40) {
        phase = 1; phaseTimer = 0;
        pEnemyShotSet->param_i[0] = phase;
        pEnemyShotSet->param_i[1] = phaseTimer;
        // 折り曲げ開始時に軽い効果音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    else if (phase == 1 && phaseTimer >= 50) {
        phase = 2; phaseTimer = 0;
        pEnemyShotSet->param_i[0] = phase;
        pEnemyShotSet->param_i[1] = phaseTimer;
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    else if (phase == 2 && phaseTimer >= 50) {
        phase = 3; phaseTimer = 0;
        pEnemyShotSet->param_i[0] = phase;
        pEnemyShotSet->param_i[1] = phaseTimer;
        // 突進開始：プレイヤー方向を記録
        // 現在の先端位置からプレイヤーへの角度
        double tipX = baseX;
        double tipY = baseY;
        // 簡易的に現在の先端を計算（後で正確に更新）
        pEnemyShotSet->param_d[4] = atan2(player.y - (baseY + 2.0 * SECTION_LEN), player.x - baseX);
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    else if (phase == 3 && phaseTimer >= 60) {
        phase = 4; phaseTimer = 0;
        pEnemyShotSet->param_i[0] = phase;
        pEnemyShotSet->param_i[1] = phaseTimer;
    }

    // 関節角度の更新
    if (phase == 0) {
        // 下降：角度固定、ベースを下へ
        baseY += 1.8;
        pEnemyShotSet->param_d[3] = baseY;
    }
    else if (phase == 1) {
        // 第1関節を右へ90度曲げる（0 → +π/2）
        double t = phaseTimer / 50.0;
        // ease-out
        t = 1.0 - (1.0 - t) * (1.0 - t);
        joint1 = t * (DX_PI / 2.0);
        pEnemyShotSet->param_d[0] = joint1;
        // 折り目に衝撃波レーザーを1回だけ出す
        if (phaseTimer == 25) {
            // 第1関節位置に短レーザーを放射
            double jx = baseX;
            double jy = baseY + SECTION_LEN;
            for (int k = 0; k < 5; k++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = jx;
                pEnemyShot->y = jy;
                pEnemyShot->muki = joint1 + (k - 2) * 0.35;
                pEnemyShot->speed = 4.5 + GetRand(10) / 10.0;
                pEnemyShot->kind = img_enemyShotLaser[4]; // 青の短レーザー
                pEnemyShot->param_i[1] = 2; // 衝撃波フラグ
                pEnemyShot->margin = 80;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    else if (phase == 2) {
        // 第2関節を左へ90度曲げる（0 → -π/2）
        double t = phaseTimer / 50.0;
        t = 1.0 - (1.0 - t) * (1.0 - t);
        joint2 = -t * (DX_PI / 2.0);
        pEnemyShotSet->param_d[1] = joint2;
        if (phaseTimer == 25) {
            // 第2関節位置に衝撃波
            // 第1関節の先の位置を計算
            double j1x = baseX + SECTION_LEN * cos(DX_PI / 2.0 + joint1);
            double j1y = baseY + SECTION_LEN * sin(DX_PI / 2.0 + joint1);
            for (int k = 0; k < 5; k++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = j1x;
                pEnemyShot->y = j1y;
                pEnemyShot->muki = joint1 + joint2 + (k - 2) * 0.35;
                pEnemyShot->speed = 4.5 + GetRand(10) / 10.0;
                pEnemyShot->kind = img_enemyShotLaser[4];
                pEnemyShot->param_i[1] = 2;
                pEnemyShot->margin = 80;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    else if (phase == 3) {
        // 突進：全体をプレイヤー方向へ加速
        double chargeAng = pEnemyShotSet->param_d[4];
        double spd = 3.0 + phaseTimer * 0.12;
        baseX += spd * cos(chargeAng);
        baseY += spd * sin(chargeAng);
        pEnemyShotSet->param_d[2] = baseX;
        pEnemyShotSet->param_d[3] = baseY;
        // 角度は維持したまま平行移動
    }
    // phase 4 は何もしない（弾は画面外で消える）

    // ---------- 全弾の位置更新 ----------
    // まず3つの節の目標位置を計算
    // 根元（節2）は base
    double secX[3], secY[3];
    secX[2] = baseX;
    secY[2] = baseY;

    // 節1 = 根元から joint1 方向
    double ang1 = DX_PI / 2.0 + joint1; // 下向きを基準に加算
    secX[1] = secX[2] + SECTION_LEN * cos(ang1);
    secY[1] = secY[2] + SECTION_LEN * sin(ang1);

    // 節0（先端）= 節1から joint1+joint2 方向
    double ang0 = ang1 + joint2;
    secX[0] = secX[1] + SECTION_LEN * cos(ang0);
    secY[0] = secY[1] + SECTION_LEN * sin(ang0);

    // 各弾を更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[1] == 1) {
            // 節本体：直接位置をセット
            int idx = pShot->param_i[0];
            pShot->x = secX[idx];
            pShot->y = secY[idx];
            pShot->speed = 0.0;
        }
        else if (pShot->param_i[1] == 0) {
            // 鎖：所属節間の2点を線形補間
            int seg = pShot->param_i[0];
            int c = pShot->param_i[2];
            double t = (c + 1.0) / (CHAIN_NUM + 1.0);
            int a = 2 - seg;     // 開始節 (seg0 → 節2と節1, seg1 → 節1と節0)
            int b = 1 - seg;
            pShot->x = secX[a] + (secX[b] - secX[a]) * t;
            pShot->y = secY[a] + (secY[b] - secY[a]) * t;
            pShot->speed = 0.0;
        }
        else if (pShot->param_i[1] == 2) {
            // 衝撃波レーザー：通常の速度移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        // それ以外は何もしない

        // 突進フェーズ終了後、鎖と節を少し散らす
        if (phase == 4) {
            // まだ速度が0の状態なら、散らす速度を一度だけ与える
            if (pShot->param_i[1] != 2) {
                pShot->param_i[1] = 2;

                // 中心からの方向に少しランダム性を持たせて外側へ飛ばす
                double cx = enemy.x;
                double cy = enemy.y;
                double dx = pShot->x - cx;
                double dy = pShot->y - cy;
                double len = sqrt(dx * dx + dy * dy);
                if (len < 1.0) len = 1.0;

                // 外側方向 + 小さなランダム角度
                double baseAng = atan2(-dy, -dx);
                double randAng = (GetRand(60) - 30) / 180.0 * DX_PI;
                pShot->muki = baseAng + randAng;
                pShot->speed = 1.2 + GetRand(15) / 10.0;  // 1.2〜2.7
            }
            // 以降は通常の速度移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_ThreeSectionStaff_Grok()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.7 * (double)muki;
        if (count % 150 == 75) muki *= -1;
        // 画面端で反転
        if (enemy.x < 80.0) { enemy.x = 80.0; muki = 1; }
        if (enemy.x > 400.0) { enemy.x = 400.0; muki = -1; }
    }

    // 一定間隔で三節棍を放出
    // 最初の予告的な間隔を少し長めに
    if (count >= 60 && (count - 60) % 180 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSanSetsu;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y - 12.0;
        pEnemyShotSet->muki = DX_PI / 2.0; // 下向き開始
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