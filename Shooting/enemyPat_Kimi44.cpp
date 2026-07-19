// enemyPat_arrow.cpp
// 矢印モチーフ弾幕：「方向指示の罠」
// 大玉の矢印マークが回転して方向を示し、その方向に弾を発射する

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数定義
// ============================================================
// 矢印マーク（大玉）の動作フェーズ
#define ARROW_PHASE_APPEAR    0   // 出現・予告（矢印マーク出現、回転開始）
#define ARROW_PHASE_AIM       1   // 照準（プレイヤー方向に向くまで回転）
#define ARROW_PHASE_CHARGE    2   // チャージ（発射前の予告、脈動）
#define ARROW_PHASE_FIRE      3   // 発射（弾を射出、矢印マークは消滅）

// 各フェーズのフレーム数
#define ARROW_APPEAR_FRAMES   30   // 出現フェーズの長さ
#define ARROW_AIM_FRAMES      60   // 照準フェーズの長さ（回転時間）
#define ARROW_CHARGE_FRAMES   40   // チャージフェーズの長さ（予告時間）
#define ARROW_FIRE_FRAMES     10   // 発射フェーズの長さ

// 発射する弾の数
#define ARROW_SHOT_COUNT      12   // 矢印の先端から発射する弾の数

// 弾速（ピクセル/フレーム）
// 画面480pxで、上から下まで約80フレームで到達する想定
#define ARROW_SHOT_SPEED_MIN  5.5  // 端の弾の最低速度
#define ARROW_SHOT_SPEED_MAX  7.5  // 中心弾の最高速度

// ============================================================
//  矢印弾幕パターン：方向指示の罠
// ============================================================
static void ShotArrowTrap(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    sEnemyShot* pArrowMark = nullptr;  // 矢印マーク（大玉）

    // --------------------------------------------------------
    //  フェーズ判定（count はメインルーチンがインクリメント）
    // --------------------------------------------------------
    int phase;
    int phaseTime = pEnemyShotSet->count;

    if (phaseTime < ARROW_APPEAR_FRAMES) {
        phase = ARROW_PHASE_APPEAR;
    }
    else if (phaseTime < ARROW_APPEAR_FRAMES + ARROW_AIM_FRAMES) {
        phase = ARROW_PHASE_AIM;
    }
    else if (phaseTime < ARROW_APPEAR_FRAMES + ARROW_AIM_FRAMES + ARROW_CHARGE_FRAMES) {
        phase = ARROW_PHASE_CHARGE;
    }
    else if (phaseTime < ARROW_APPEAR_FRAMES + ARROW_AIM_FRAMES + ARROW_CHARGE_FRAMES + ARROW_FIRE_FRAMES) {
        phase = ARROW_PHASE_FIRE;
    }
    else {
        // 全フェーズ終了、ShotSet はメインルーチンで解放される
        //return;
        phase = ARROW_PHASE_FIRE;
    }

    // --------------------------------------------------------
    //  初回フレーム：矢印マーク（大玉）を生成
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        // 予告音（チャージ音）
        //if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        //PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 目標角度を計算（プレイヤー方向）
        double targetAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 矢印マーク（大玉）を生成
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;  // 初期角度 = ShotSetのmuki
        pEnemyShot->speed = 0.0;  // 矢印マーク自体は移動しない

        // 色は赤（0）または青（4）で、矢印の方向性を強調
        int color = pEnemyShotSet->kind % 2 == 0 ? 0 : 4;  // 赤 or 青
        pEnemyShot->kind = img_enemyShotLargeBall[color];

        pEnemyShot->param_d[0] = pEnemyShotSet->muki;  // 現在角度
        pEnemyShot->param_d[1] = targetAngle;          // 目標角度
        pEnemyShot->param_d[2] = pEnemyShotSet->x;     // 発射起点X
        pEnemyShot->param_d[3] = pEnemyShotSet->y;     // 発射起点Y
        pEnemyShot->param_i[0] = 0;  // 未発射

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // ShotSet にも目標角度を保存
        pEnemyShotSet->param_d[0] = targetAngle;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->x;  // 発射起点X
        pEnemyShotSet->param_d[2] = pEnemyShotSet->y;  // 発射起点Y
        pEnemyShotSet->param_i[0] = 0;  // 未発射フラグ
    }

    // --------------------------------------------------------
    //  矢印マークを検索（先頭の弾が矢印マーク）
    // --------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    if (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pArrowMark = pEnemyShot;
    }

    // --------------------------------------------------------
    //  フェーズ別処理
    // --------------------------------------------------------
    switch (phase) {

        // ==================== 出現フェーズ ====================
    case ARROW_PHASE_APPEAR:
        if (pArrowMark != nullptr) {
            double current = pArrowMark->param_d[0];
            double target = pArrowMark->param_d[1];

            // 最短回転方向を計算
            double diff = target - current;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;

            // ゆっくり回転（イーズイン）
            double t = (double)pEnemyShotSet->count / (double)ARROW_APPEAR_FRAMES;
            pArrowMark->param_d[0] = current + diff * t * t;
            pArrowMark->muki = pArrowMark->param_d[0];

            // 脈動演出
            double pulse = 1.0 + 0.1 * sin(pEnemyShotSet->count * 0.3);
            pArrowMark->x = pArrowMark->param_d[2] + cos(pArrowMark->muki) * 2.0 * pulse;
            pArrowMark->y = pArrowMark->param_d[3] + sin(pArrowMark->muki) * 2.0 * pulse;
        }
        break;

        // ==================== 照準フェーズ ====================
    case ARROW_PHASE_AIM:
        if (pArrowMark != nullptr) {
            double current = pArrowMark->param_d[0];
            double target = pArrowMark->param_d[1];

            double diff = target - current;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;

            double aimProgress = (double)(pEnemyShotSet->count - ARROW_APPEAR_FRAMES)
                / (double)ARROW_AIM_FRAMES;

            if (aimProgress < 0.7) {
                pArrowMark->param_d[0] += diff * 0.03;
            }
            else {
                pArrowMark->param_d[0] = target;
            }

            pArrowMark->muki = pArrowMark->param_d[0];

            // 矢印マークを少し前方にずらして「指す」感じを演出
            double offset = 5.0;
            pArrowMark->x = pArrowMark->param_d[2] + cos(pArrowMark->muki) * offset;
            pArrowMark->y = pArrowMark->param_d[3] + sin(pArrowMark->muki) * offset;
        }
        break;

        // ==================== チャージフェーズ ====================
    case ARROW_PHASE_CHARGE:
        if (pArrowMark != nullptr) {
            int chargeTime = pEnemyShotSet->count - ARROW_APPEAR_FRAMES - ARROW_AIM_FRAMES;
            double pulse = 1.0 + 0.2 * sin(chargeTime * 0.5);

            pArrowMark->x = pArrowMark->param_d[2] + cos(pArrowMark->muki) * 3.0 * pulse;
            pArrowMark->y = pArrowMark->param_d[3] + sin(pArrowMark->muki) * 3.0 * pulse;

            // 色を点滅させる演出（赤(0)と白(6)を交互に）
            int flashColor = (chargeTime / 5) % 2 == 0 ? 0 : 6;
            pArrowMark->kind = img_enemyShotLargeBall[flashColor];
        }
        break;

        // ==================== 発射フェーズ ====================
    case ARROW_PHASE_FIRE:
        if (pEnemyShotSet->param_i[0] == 0) {
            // 初回のみ発射処理
            pEnemyShotSet->param_i[0] = 1;  // 発射済みフラグ

            // 発射音
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 矢印マークを消去
            if (pArrowMark != nullptr) {
                pArrowMark->prev->next = pArrowMark->next;
                pArrowMark->next->prev = pArrowMark->prev;
                delete pArrowMark;
                pArrowMark = nullptr;
            }

            // 発射起点（矢印マークの位置）
            double fireX = pEnemyShotSet->param_d[1];
            double fireY = pEnemyShotSet->param_d[2];
            double fireAngle = pEnemyShotSet->param_d[0];

            // 矢印の先端位置（全弾共通の発射位置）
            double tipOffset = 15.0;
            double tipX = fireX + cos(fireAngle) * tipOffset;
            double tipY = fireY + sin(fireAngle) * tipOffset;

            // 矢印の先端方向に弾を発射
            // 全弾を同じ位置（矢印先端）から、扇状の角度で発射
            for (int i = 0; i < ARROW_SHOT_COUNT; i++) {
                pEnemyShot = new sEnemyShot;

                // 扇状の角度分布（中心をfireAngle、±30度）
                double spread = (DX_PI / 6.0) * ((double)i / (ARROW_SHOT_COUNT - 1) - 0.5) * 2.0;
                pEnemyShot->muki = fireAngle + spread;

                // 速度：中心が最速、端に行くほど遅くなるが最低値を保証
                double centerDist = fabs((double)i / (ARROW_SHOT_COUNT - 1) - 0.5) * 2.0; // 0=中心, 1=端
                pEnemyShot->speed = ARROW_SHOT_SPEED_MAX - centerDist * (ARROW_SHOT_SPEED_MAX - ARROW_SHOT_SPEED_MIN);

                // 全弾を同じ位置（矢印先端）から発射
                pEnemyShot->x = tipX;
                pEnemyShot->y = tipY;

                // 弾の種類：銃弾（矢印っぽい細長い形状）
                // 色は赤(0)から順番に変化（黒(7)は避ける）
                int color = (pEnemyShotSet->kind + i) % 7;  // 0〜6
                pEnemyShot->kind = img_enemyShotBullet[color];

                // リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // 追加：中心に中玉を1発（矢印の芯）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = tipX;
            pEnemyShot->y = tipY;
            pEnemyShot->muki = fireAngle;
            pEnemyShot->speed = ARROW_SHOT_SPEED_MAX + 0.5;
            pEnemyShot->kind = img_enemyShotMediumBall[0];  // 赤の中玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        break;
    }

    // --------------------------------------------------------
    //  全弾の移動処理（矢印マーク以外）
    // --------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 矢印マークは speed=0 で移動しない
        // 発射済みの弾は speed > 0 で移動
        if (pEnemyShot->speed > 0.0) {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }
}


// ============================================================
//  敵本体のパターン：矢印弾幕「方向指示の罠」
// ============================================================
void EnemyPat_Arrow_Kimi()
{
    static int muki;
    static int shot_count;

    // --------------------------------------------------------
    //  初期化
    // --------------------------------------------------------
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }

    // --------------------------------------------------------
    //  敵の移動（左右にゆっくり移動 + 上下に揺れる）
    // --------------------------------------------------------
    enemy.x += 0.8 * (double)muki;
    if (enemy.x < 80.0) {
        enemy.x = 80.0;
        muki = 1;
    }
    if (enemy.x > 400.0) {
        enemy.x = 400.0;
        muki = -1;
    }

    enemy.y = 60.0 + sin(count * 0.02) * 10.0;

    // --------------------------------------------------------
    //  弾幕発射（一定間隔で矢印弾幕を生成）
    // --------------------------------------------------------
    // 80フレームごとに1セットの矢印弾幕を発生
    // 複数の矢印を時間差で出現させ、交差する弾幕を形成
    if (count % 80 == 0) {
        for (int setIndex = 0; setIndex < 3; setIndex++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = -setIndex * 20;  // 時間差（20フレームずつ遅らせる）
            pEnemyShotSet->patternFunc = ShotArrowTrap;

            // 敵の周囲に扇状に配置
            double baseAngle = -DX_PI / 2.0 + (DX_PI / 4.0) * (setIndex - 1);
            double offsetDist = 30.0;
            pEnemyShotSet->x = enemy.x + cos(baseAngle) * offsetDist;
            pEnemyShotSet->y = enemy.y + sin(baseAngle) * offsetDist;

            // 各矢印の基本方向（下方向中心に少しずつずらす）
            pEnemyShotSet->muki = DX_PI / 2.0 + (DX_PI / 8.0) * (setIndex - 1);

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

    // 120フレームごとにプレイヤー狙いの矢印を1発
    if (count % 120 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotArrowTrap;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
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