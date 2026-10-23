#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ===========================================================================
//  弾幕：北斗七星「七星北導（しちせいほくどう）」
//
//  【概要】
//    ボスから発射された7発の弾が北斗七星の形に空中に配置される。
//    杯（ひしゃく）の開口部がプレイヤー（＝北極星）を指すように全体が回転したのち、
//    指極星である天枢・天璇からプレイヤーへ針弾を撃ち込む。
//    最後に杯→柄の順に星が崩れ、全方位リング弾をばら撒く。
//
//  【フェーズ構成】
//    Phase 0: 星描画   7星を順次発射し北斗七星の形に配置  (約3.5秒)
//    Phase 1: 星回転   杯の開口部がプレイヤーを向くよう回転 (約2秒)
//    Phase 2: 北導     天枢・天璇からプレイヤーへ針弾3連射×5波 (約2秒)
//    Phase 3: 星崩し   杯→柄の順に崩壊、各星から全方位リング弾 (約3.5秒)
//    Phase 4: 終了     残弾の消滅を待つ
//
//  【使用素材】
//    大玉(赤) img_enemyShotLargeBall[0]  : 杯4星 (天枢・天璇・天璣・天権)
//    中玉(青) img_enemyShotMediumBall[4] : 柄3星 (玉衡・開陽・搖光)
//    小玉(白) img_enemyShotSmallBall[6]  : 開陽の伴星 / 星崩しリング弾
//    銃弾(黄) img_enemyShotBullet[1]     : 指極星からの針弾
//
//  【効果音】
//    sound_enemyCharge      : Phase 0→1, 1→2 遷移時（予告音）
//    sound_enemyShot_medium  : Phase 0 星発射 / Phase 3 星崩し時
//    sound_enemyShot_light   : Phase 2 針弾発射時
//    sound_enemyShot_heavy  : Phase 3 開始時
//
//  【パラメータ割り当て (sEnemyShotSet)】
//    param_i[0] = 現在のフェーズ (0..4)
//    param_i[1] = フェーズ開始時の count
//    param_d[0] = 北斗七星の中心 x
//    param_d[1] = 北斗七星の中心 y
//    param_d[2] = 縮尺
//    param_d[3] = 現在の回転角度
//    param_d[4..7] = Phase 2 用: 天枢(x,y), 天璇(x,y)
//    param_d[8] = Phase 1 用: 目標回転角度
//
//  【パラメータ割り当て (sEnemyShot)】
//    param_i[0] = 1:移動中  2:到着静止  0:通常弾(直進)  99:崩壊済
//    param_i[1] = 星のインデックス (0..6), 通常弾は -1
//    param_i[2] = 0:本体  1:伴星(アルコル)
//    param_d[0] = 目標 x
//    param_d[1] = 目標 y
// ===========================================================================

// 北斗七星 7星の相対座標 (dipper中心=0,0 基準、scale=1.0時のピクセル)
// 配置: 杯（四角形）が左、柄が右に伸びる。杯の開口部（天枢-天璇側）が -x 方向。
// 順序: 0=天枢(Dubhe), 1=天璇(Merak), 2=天璣(Phecda), 3=天権(Megrez),
//       4=玉衡(Alioth), 5=開陽(Mizar), 6=搖光(Alkaid)
// 崩壊順序も 0→1→2→3→4→5→6 で杯→柄へ。
static const double DIPPER_STARS_REL_X[7] = { -80.0, -80.0, -30.0, -30.0,  20.0,  60.0, 100.0 };
static const double DIPPER_STARS_REL_Y[7] = { -20.0,  20.0,  20.0, -20.0, -20.0, -25.0, -35.0 };

// 開陽(index=5)の伴星アルコルの相対座標（本物の開陽のすぐ脇にある小星を再現）
static const double COMPANION_REL_X = 52.0;
static const double COMPANION_REL_Y = -12.0;

// ---------------------------------------------------------------------------
//  弾幕メイン関数
// ---------------------------------------------------------------------------
static void ShotBigDipper(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // パラメータ読み込み
    int    phase = pEnemyShotSet->param_i[0];
    int    phase_start = pEnemyShotSet->param_i[1];
    int    elapsed = pEnemyShotSet->count - phase_start;
    double cx = pEnemyShotSet->param_d[0];
    double cy = pEnemyShotSet->param_d[1];
    double scale = pEnemyShotSet->param_d[2];
    double rotation = pEnemyShotSet->param_d[3];

    // -------------------------------------------------------------------
    //  Phase 0: 星描画 - 7星を順次発射し北斗七星の形に配置
    // -------------------------------------------------------------------
    if (phase == 0) {
        // 26フレームごとに1星発射 (7星で 7*26 = 182フレーム ≈ 3秒)
        int starIdx = elapsed / 26;
        if (starIdx < 7 && elapsed % 26 == 0) {
            // 星の目標位置を計算（回転を適用）
            double rx = DIPPER_STARS_REL_X[starIdx] * scale;
            double ry = DIPPER_STARS_REL_Y[starIdx] * scale;
            double tx = cx + rx * cos(rotation) - ry * sin(rotation);
            double ty = cy + rx * sin(rotation) + ry * cos(rotation);

            // 星弾を生成（ボス位置から発射）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = atan2(ty - pEnemyShotSet->y, tx - pEnemyShotSet->x);
            pEnemyShot->speed = 4.0;
            pEnemyShot->param_d[0] = tx;
            pEnemyShot->param_d[1] = ty;
            pEnemyShot->param_i[0] = 1;       // 移動中
            pEnemyShot->param_i[1] = starIdx; // 星インデックス
            pEnemyShot->param_i[2] = 0;      // 本体
            // 弾種: 杯(0-3)=大玉(赤), 柄(4-6)=中玉(青)
            if (starIdx <= 3) {
                pEnemyShot->kind = img_enemyShotLargeBall[0];
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[4];
            }
            // 双方向リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 開陽(index=5)の直後に伴星アルコルを添える
            if (starIdx == 5) {
                double crx = COMPANION_REL_X * scale;
                double cry = COMPANION_REL_Y * scale;
                double ctx = cx + crx * cos(rotation) - cry * sin(rotation);
                double cty = cy + crx * sin(rotation) + cry * cos(rotation);

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = atan2(cty - pEnemyShotSet->y, ctx - pEnemyShotSet->x);
                pEnemyShot->speed = 4.0;
                pEnemyShot->param_d[0] = ctx;
                pEnemyShot->param_d[1] = cty;
                pEnemyShot->param_i[0] = 1;   // 移動中
                pEnemyShot->param_i[1] = 5;   // 開陽と同じインデックス（同時に崩壊）
                pEnemyShot->param_i[2] = 1;   // 伴星フラグ
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 小玉(白)
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 全星が到着したら Phase 1 へ
        if (elapsed >= 7 * 26 + 15) {
            bool allArrived = true;
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[0] == 1) { allArrived = false; break; }
                p = p->next;
            }
            if (allArrived) {
                // フェーズ遷移時に目標回転角度を固定（プレイヤー位置は開始時点で固定）
                // 杯の開口部はデフォルトで -x方向(angle=π) を向いているので、
                // angle_to_player - π だけ回転させると開口部がプレイヤーを指す
                double angle_to_player = atan2(player.y - cy, player.x - cx);
                pEnemyShotSet->param_d[8] = angle_to_player - DX_PI;
                pEnemyShotSet->param_i[0] = 1;
                pEnemyShotSet->param_i[1] = pEnemyShotSet->count;
                PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            }
        }
    }
    // -------------------------------------------------------------------
    //  Phase 1: 星回転 - 杯の開口部がプレイヤーを向くよう回転
    // -------------------------------------------------------------------
    else if (phase == 1) {
        // 目標回転角度は Phase 0→1 遷移時に param_d[8] へ設定済み
        double target_rotation = pEnemyShotSet->param_d[8];

        // 残りフレームで割って線形に収束させる（120フレームで確定収束）
        int remaining = 120 - elapsed;
        double diff = target_rotation - rotation;
        while (diff > DX_PI) diff -= DX_PI * 2.0;
        while (diff < -DX_PI) diff += DX_PI * 2.0;
        double rotStep = (remaining > 0) ? (diff / (double)remaining) : 0.0;
        double newRot = rotation + rotStep;
        pEnemyShotSet->param_d[3] = newRot;

        // 到着静止中の星（param_i[0]==2）の位置を新しい回転で更新
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 2) {
                int idx = p->param_i[1];
                double rx, ry;
                if (p->param_i[2] == 1 && idx == 5) {
                    // 開陽の伴星
                    rx = COMPANION_REL_X * scale;
                    ry = COMPANION_REL_Y * scale;
                }
                else {
                    rx = DIPPER_STARS_REL_X[idx] * scale;
                    ry = DIPPER_STARS_REL_Y[idx] * scale;
                }
                p->x = cx + rx * cos(newRot) - ry * sin(newRot);
                p->y = cy + rx * sin(newRot) + ry * cos(newRot);
            }
            p = p->next;
        }

        if (elapsed >= 120) {
            // フェーズ遷移時に指極星（天枢・天璇）の最終位置を記憶
            // 回転完了後の位置を param_d[4..7] へ保存
            double px1 = cx, py1 = cy, px2 = cx, py2 = cy;
            sEnemyShot* pCap = pEnemyShotSet->pEnemyShotHead->next;
            while (pCap != pEnemyShotSet->pEnemyShotHead) {
                if (pCap->param_i[0] == 2) {
                    if (pCap->param_i[1] == 0) { px1 = pCap->x; py1 = pCap->y; } // 天枢
                    if (pCap->param_i[1] == 1) { px2 = pCap->x; py2 = pCap->y; } // 天璇
                }
                pCap = pCap->next;
            }
            pEnemyShotSet->param_d[4] = px1; // 天枢 x
            pEnemyShotSet->param_d[5] = py1; // 天枢 y
            pEnemyShotSet->param_d[6] = px2; // 天璇 x
            pEnemyShotSet->param_d[7] = py2; // 天璇 y
            pEnemyShotSet->param_i[0] = 2;
            pEnemyShotSet->param_i[1] = pEnemyShotSet->count;
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }
    // -------------------------------------------------------------------
    //  Phase 2: 北導 - 天枢・天璇からプレイヤーへ針弾を撃ち込む
    // -------------------------------------------------------------------
    else if (phase == 2) {
        // 指極星（天枢・天璇）の位置は Phase 1→2 遷移時に param_d[4..7] へ設定済み

        // 20フレームごとに両指極星から3連射の針弾を発射（5波）
        if (elapsed % 20 == 0 && elapsed < 100) {
            for (int src = 0; src < 2; src++) {
                double sx = (src == 0) ? pEnemyShotSet->param_d[4] : pEnemyShotSet->param_d[6];
                double sy = (src == 0) ? pEnemyShotSet->param_d[5] : pEnemyShotSet->param_d[7];
                double baseAng = atan2(player.y - sy, player.x - sx);
                // 3連射（中央 + 左右に僅かに広げる）
                for (int k = -1; k <= 1; k++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = sx;
                    pEnemyShot->y = sy;
                    pEnemyShot->muki = baseAng + k * 0.08;
                    pEnemyShot->speed = 6.0;
                    pEnemyShot->kind = img_enemyShotBullet[1]; // 銃弾(黄)
                    pEnemyShot->param_i[0] = 0; // 通常弾（直進）
                    pEnemyShot->param_i[1] = -1;
                    pEnemyShot->param_i[2] = 0;
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        if (elapsed >= 120) {
            pEnemyShotSet->param_i[0] = 3;
            pEnemyShotSet->param_i[1] = pEnemyShotSet->count;
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }
    // -------------------------------------------------------------------
    //  Phase 3: 星崩し - 杯→柄の順に崩壊、全方位リング弾を放射
    // -------------------------------------------------------------------
    else if (phase == 3) {
        // 30フレームごとに1星崩壊 (7星で 7*30 = 210フレーム ≈ 3.5秒)
        int collapseIdx = elapsed / 30-1;
        if (collapseIdx < 7 && elapsed % 30 == 0) {
            // 該当インデックスの静止中の星を探す（伴星含む）
            sEnemyShot* targets[4];
            int nTargets = 0;
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead && nTargets < 4) {
                if (p->param_i[1] == collapseIdx && p->param_i[0] == 2) {
                    targets[nTargets++] = p;
                }
                p = p->next;
            }
            // 各ターゲットからリング弾を放射して崩壊させる
            for (int t = 0; t < nTargets; t++) {
                sEnemyShot* target = targets[t];
                bool isCompanion = (target->param_i[2] == 1);
                // 杯(0-3)は8方向、柄(4-6)は12方向、伴星は6方向
                int numBullets = isCompanion ? 6 : (collapseIdx <= 3 ? 8 : 12);
                numBullets *= 3;
                for (int i = 0; i < numBullets; i++) for (int j = 0; j < 5; j++) {
                    double ang = (DX_PI * 2.0 * (double)i) / (double)numBullets;
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = target->x;
                    pEnemyShot->y = target->y;
                    pEnemyShot->muki = ang;
                    pEnemyShot->speed = (isCompanion ? 2.0 : 2.5)-j*0.3;
                    pEnemyShot->kind = img_enemyShotSmallBall[6]; // 小玉(白)
                    pEnemyShot->param_i[0] = 0; // 通常弾
                    pEnemyShot->param_i[1] = -1;
                    pEnemyShot->param_i[2] = 0;
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
                // 星を画面外へ飛ばし、メインルーチンに消去させる
                target->param_i[0] = 99;
                target->x = -10000.0;
                target->y = -10000.0;
                target->speed = 0.0;
            }
            if (nTargets > 0) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
        }

        if (elapsed >= 7 * 30 + 30) {
            pEnemyShotSet->param_i[0] = 4;
            pEnemyShotSet->param_i[1] = pEnemyShotSet->count;
        }
    }
    // -------------------------------------------------------------------
    //  Phase 4: 終了 - 残弾消滅待機（何もしない）
    // -------------------------------------------------------------------

    // -------------------------------------------------------------------
    //  全弾の移動更新
    // -------------------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 移動中の星弾: 目標位置へ向かう
            double tx = pShot->param_d[0];
            double ty = pShot->param_d[1];
            double dx = tx - pShot->x;
            double dy = ty - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist <= pShot->speed) {
                // 到着
                pShot->x = tx;
                pShot->y = ty;
                pShot->speed = 0.0;
                pShot->param_i[0] = 2;
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else if (pShot->param_i[0] == 0) {
            // 通常弾（針弾・リング弾）: 直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        // param_i[0]==2 (静止星)  : 位置更新しない（Phase 1 で直接更新済）
        // param_i[0]==99(崩壊済)  : 更新しない（画面外に置いたのでメインルーチンが消す）
        pShot = pShot->next;
    }
}

// ===========================================================================
//  敵本体のパターン
// ===========================================================================
void EnemyPat_Tmp()
{
    static int muki;
    static int pattern_count;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        pattern_count = 0;
    }
    else {
        // ゆっくり左右に移動（北斗七星の展開を邪魔しないよう緩やかに）
        enemy.x += 0.5 * (double)muki;
        if (enemy.x < 80.0) { enemy.x = 80.0;  muki = 1; }
        if (enemy.x > 400.0) { enemy.x = 400.0; muki = -1; }
        if (count % 240 == 120) muki *= -1;
    }

    // 北斗七星弾幕を発動
    // 初回は count==90 で発動。以降は900フレーム周期（15秒）で再発動。
    //   Phase0: 約3.5秒 / Phase1: 2秒 / Phase2: 2秒 / Phase3: 約3.5秒 / 残弾消滅: 約2秒
    if (count == 90 || (count > 90 && (count - 90) % 600 == 0)) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBigDipper;
        // 北斗七星の中心・縮尺（固定。ボスの移動に依存しない）
        pEnemyShotSet->param_d[0] = 240.0;  // cx
        pEnemyShotSet->param_d[1] = 110.0+100;  // cy
        pEnemyShotSet->param_d[2] = 1.2;    // scale
        pEnemyShotSet->param_d[3] = 0.0;    // rotation (初期)
        pEnemyShotSet->param_i[0] = 0;     // phase 0
        pEnemyShotSet->param_i[1] = 0;     // phase_start
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = pattern_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
