// enemyPat_swimmy.cpp
// スイミーをモチーフにした弾幕：「大きな魚の目になるスイミー」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 魚を形作る赤い小弾の数
static const int NUM_RED_FISH = 40;

// 各赤い小弾の、魚の中心からの相対位置（オフセット）
static double fishOffsetX[NUM_RED_FISH];
static double fishOffsetY[NUM_RED_FISH];
static bool offsetInitialized = false;

// 魚の形状を静的に定義（最初の一度だけ呼ぶ）
static void InitFishOffsets()
{
    if (offsetInitialized) return;
    int idx = 0;

    // 1. 胴体の輪郭（楕円）
    for (int i = 0; i < 24; ++i) {
        double angle = 2.0 * DX_PI * i / 24.0;
        double rx = 42.0;
        double ry = 20.0;
        double fx = rx * cos(angle);
        double fy = ry * sin(angle);
        // 頭の前方中心付近（目になる場所）には赤い弾を置かず、間引く
        if (fx > 30.0 && fabs(fy) < 8.0)
            continue;
        fishOffsetX[idx] = fx;
        fishOffsetY[idx] = fy;
        idx++;
        if (idx >= NUM_RED_FISH) break;
    }

    // 2. 尾びれ
    if (idx < NUM_RED_FISH) {
        double tailBaseX = -42.0;
        for (int i = 0; i < 6; ++i) {
            double t = (i - 2.5) * 0.4; // -1.0 ～ 1.0 程度
            fishOffsetX[idx] = tailBaseX - 12.0;
            fishOffsetY[idx] = t * 18.0;
            idx++;
            if (idx >= NUM_RED_FISH) break;
        }
    }

    // 3. 背びれ
    if (idx < NUM_RED_FISH) {
        for (int i = 0; i < 4; ++i) {
            fishOffsetX[idx] = -10.0 + i * 8.0;
            fishOffsetY[idx] = -22.0 - (i % 2 == 0 ? 5.0 : 9.0);
            idx++;
            if (idx >= NUM_RED_FISH) break;
        }
    }

    // 4. 腹びれ
    if (idx < NUM_RED_FISH) {
        for (int i = 0; i < 4; ++i) {
            fishOffsetX[idx] = -10.0 + i * 8.0;
            fishOffsetY[idx] = 22.0 + (i % 2 == 0 ? 5.0 : 9.0);
            idx++;
            if (idx >= NUM_RED_FISH) break;
        }
    }

    // 5. 余ったぶんは内部を埋めるランダムな点（歯抜けの表現）
    while (idx < NUM_RED_FISH) {
        fishOffsetX[idx] = (GetRand(70) - 35) * 0.7;
        fishOffsetY[idx] = (GetRand(36) - 18) * 0.7;
        idx++;
    }

    offsetInitialized = true;
}

// スイミー弾幕パターン関数
static void SwimmyPattern(sEnemyShotSet* pSet)
{
    // フェーズ管理用パラメータ
    int& phase = pSet->param_i[0]; // 現在のフェーズ
    int& timer = pSet->param_i[1]; // フェーズ内タイマー
    double& baseX = pSet->param_d[0]; // 魚全体の基準 X
    double& baseY = pSet->param_d[1]; // 魚全体の基準 Y

    // 突進用の速度ベクトル
    double& rushVx = pSet->param_d[2];
    double& rushVy = pSet->param_d[3];

    const int PHASE_SCATTER = 0; // 散開（赤い小魚のばらまき）
    const int PHASE_FORM = 1; // 集合（魚の形を作る）
    const int PHASE_CHARGE = 2; // 予告（スイミー登場、後退）
    const int PHASE_RUSH = 3; // 突進
    const int PHASE_DISPERSE = 4; // 離散

    // ==============================
    // 各フェーズの状態遷移と制御
    // ==============================
    if (phase == PHASE_SCATTER) {
        // --- 散開：ばらまき ---
        InitFishOffsets();

        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        baseX = 240.0;
        baseY = 300.0;

        // 赤い小弾を 40 個生成し、ランダムな位置から魚の中心へ集める
        for (int i = 0; i < NUM_RED_FISH; ++i) {
            sEnemyShot* pShot = new sEnemyShot;

            // 初期位置：中心から放射状に離れた場所
            double angle = (2.0 * DX_PI * i) / NUM_RED_FISH
                + (GetRand(60) - 30) / 180.0 * DX_PI;
            double dist = 150.0 + GetRand(80);
            pShot->x = baseX + dist * cos(angle);
            pShot->y = baseY + dist * sin(angle);

            // 中心に向かう初速
            pShot->muki = atan2(baseY - pShot->y, baseX - pShot->x);
            pShot->speed = 1.5 + GetRand(100) / 100.0;

            // 赤い小玉 (色:0 赤)
            pShot->kind = img_enemyShotSmallBall[0];

            // この弾の魚内でのオフセットを記録（微振れあり）
            pShot->param_d[0] = fishOffsetX[i] + (GetRand(10) - 5);
            pShot->param_d[1] = fishOffsetY[i] + (GetRand(10) - 5);

            pShot->margin = 9999;

            // 双方向リストに追加
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        timer = 0;
        phase = PHASE_FORM;
    }
    else if (phase == PHASE_FORM) {
        // --- 集合：魚の形に整列 ---
        bool allClose = true;

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            double targetX = baseX + pShot->param_d[0];
            double targetY = baseY + pShot->param_d[1];
            double dx = targetX - pShot->x;
            double dy = targetY - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist > 1.5) {
                allClose = false;
                // 目標へ近づく速度を再設定
                pShot->muki = atan2(dy, dx);
                pShot->speed = dist * 0.1 + 0.3;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                // 十分近ければ位置を固定
                pShot->x = targetX;
                pShot->y = targetY;
                pShot->speed = 0.0;
            }
            pShot = pShot->next;
        }

        if (allClose || timer > 120) {
            // 全弾を正しい位置にスナップ
            pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                pShot->x = baseX + pShot->param_d[0];
                pShot->y = baseY + pShot->param_d[1];
                pShot->speed = 0.0;
                pShot = pShot->next;
            }
            timer = 0;
            phase = PHASE_CHARGE;
        }
        else {
            timer++;
        }
    }
    else if (phase == PHASE_CHARGE) {
        // --- 予告：スイミー（黒い中弾）登場と後退 ---
        if (timer == 0) {
            // 予告音
            if (CheckSoundMem(sound_enemyCharge))
                StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            // 目に当たる黒い中弾を生成
            sEnemyShot* pEye = new sEnemyShot;
            pEye->x = baseX + 30.0;   // 目のオフセット X
            pEye->y = baseY;
            pEye->muki = 0.0;
            pEye->speed = 0.0;
            pEye->kind = img_enemyShotMediumBall[7]; // 黒 (色:7)
            pEye->param_d[0] = 30.0;  // オフセット X を記憶
            pEye->param_d[1] = 0.0;   // オフセット Y
            pEye->margin = 480.0;

            pEye->prev = pSet->pEnemyShotHead->prev;
            pEye->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pEye;
            pSet->pEnemyShotHead->prev = pEye;
        }

        // 魚全体をゆっくり後退させる（30 フレーム間）
        if (timer < 30) {
            baseX -= 0.4;
            sEnemyShot* pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                pShot->x = baseX + pShot->param_d[0];
                pShot->y = baseY + pShot->param_d[1];
                pShot = pShot->next;
            }
        }

        timer++;
        if (timer > 60) {
            // 突進方向を決定（プレイヤーへ向かう）
            double dx = player.x - baseX;
            double dy = player.y - baseY;
            double len = sqrt(dx * dx + dy * dy);
            if (len > 1.0) {
                rushVx = (dx / len) * 5.5;
                rushVy = (dy / len) * 5.5;
            }
            else {
                rushVx = 0.0;
                rushVy = -5.5;
            }

            timer = 0;
            phase = PHASE_RUSH;

            // 突進音
            if (CheckSoundMem(sound_enemyShot_heavy))
                StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }
    else if (phase == PHASE_RUSH) {
        // --- 突進：魚の形を保ったまま高速移動 ---
        baseX += rushVx;
        baseY += rushVy;

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x = baseX + pShot->param_d[0];
            pShot->y = baseY + pShot->param_d[1];
            pShot = pShot->next;
        }

        timer++;
        if (timer > 40) {
            timer = 0;
            phase = PHASE_DISPERSE;

            if (CheckSoundMem(sound_enemyShot_light))
                StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }
    else if (phase == PHASE_DISPERSE) {
        // --- 離散：赤い小魚とスイミーが散り散りになる ---
        if (timer == 0) {
            sEnemyShot* pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                if (pShot->kind == img_enemyShotMediumBall[7]) {
                    // 黒中弾（スイミー）は画面外へ飛ばす
                    pShot->muki = atan2(pShot->y - 240.0, pShot->x - 240.0)
                        + (GetRand(60) - 30) / 180.0 * DX_PI;
                    pShot->speed = 8.0;
                }
                else {
                    // 赤小弾はランダム方向へ
                    pShot->muki = (GetRand(360) / 180.0) * DX_PI;
                    pShot->speed = 4.0 + GetRand(200) / 100.0;
                }
                pShot = pShot->next;
            }
        }

        // 毎フレーム移動
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }

        timer++;
        if (timer > 90) {
            // 離散が十分進んだら、再び散開フェーズへ戻して新たな魚を作る
            phase = PHASE_SCATTER;
            timer = 0;
            baseX = 240.0;
            baseY = 300.0;
        }
    }
}

// 敵本体パターン
void EnemyPat_Swimmy_DeepSeek()
{
    static int  muki;
    static bool patternSpawned = false;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        patternSpawned = false;
        offsetInitialized = false;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60)
            muki *= -1;
    }

    // スイミー弾幕を一度だけ発動（敵本体が生存中はパターンがループし続ける）
    if (!patternSpawned && count == 100) {
        sEnemyShotSet* pSet = new sEnemyShotSet;

        pSet->count = 0;
        pSet->patternFunc = SwimmyPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // パラメータ初期化
        pSet->param_i[0] = 0; // phase = PHASE_SCATTER
        pSet->param_i[1] = 0; // timer = 0

        // ダミーヘッドの作成
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルの enemyShotSetHead リストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        patternSpawned = true;
    }
}