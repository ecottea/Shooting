// enemyPat_shotgunBurst.cpp
// ショットガンをモチーフにした弾幕「バックドラフト・バースト」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  前方散弾パターン：ショットガンの扇状拡散
// ============================================================
static void ShotShotgunBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 重い発射音
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // プレイヤー方向を中心に扇状（±30°）に7〜9発
        double baseMuki = pEnemyShotSet->muki;
        int numShots = 7 + GetRand(2); // GetRand(2) → 0〜2

        for (int i = 0; i < numShots; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // ±30°の範囲でランダムに散らす
            // GetRand(60) → 0〜60、-30 → -30〜+30
            double spread = (GetRand(60) - 30) / 180.0 * DX_PI;
            pEnemyShot->muki = baseMuki + spread;

            // 速度：2.0〜4.0
            pEnemyShot->speed = (200 + GetRand(200)) / 100.0;

            // 銃弾（Bullet）を使用、色はオレンジ(8)または黄色(1)
            int color = (GetRand(1) == 0) ? 8 : 1;
            pEnemyShot->kind = img_enemyShotBullet[color];

            // 双方向リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動（直線）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  後方弾殻パターン：反動で撒き散らされる弾殻
// ============================================================
static void ShotShellScatter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 後方方向（敵の反対側）
        double backMuki = pEnemyShotSet->muki + DX_PI;
        int numShells = 5 + GetRand(3); // 5〜8個

        for (int i = 0; i < numShells; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 後方±45°に撒く
            double spread = (GetRand(90) - 45) / 180.0 * DX_PI;
            pEnemyShot->muki = backMuki + spread;

            // 弾殻は遅い：0.8〜2.0
            pEnemyShot->speed = (80 + GetRand(120)) / 100.0;

            // 小玉（SmallBall）を使用、色は黄色(1)
            pEnemyShot->kind = img_enemyShotSmallBall[1];

            // 重力・跳ね返り用パラメータ
            pEnemyShot->param_d[0] = pEnemyShot->speed * sin(pEnemyShot->muki); // y方向初速度
            pEnemyShot->param_d[1] = pEnemyShot->speed * cos(pEnemyShot->muki); // x方向速度
            pEnemyShot->param_i[0] = 0; // 跳ね回数

            // 双方向リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾殻の移動（重力＋跳ね返り）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 重力加速度（毎フレーム y方向速度に加算）
        pEnemyShot->param_d[0] += 0.08;

        // 位置更新
        pEnemyShot->x += pEnemyShot->param_d[1];
        pEnemyShot->y += pEnemyShot->param_d[0];

        // 画面下端（y≈460）で跳ね返る
        if (pEnemyShot->y > 460.0 && pEnemyShot->param_d[0] > 0) {
            pEnemyShot->param_d[0] *= -0.5; // 跳ね返り（減衰）
            pEnemyShot->param_i[0]++;        // 跳ね回数カウント

            // 3回跳ねたらほぼ止まる
            if (pEnemyShot->param_i[0] >= 3) {
                pEnemyShot->param_d[0] = 0.0;
                pEnemyShot->param_d[1] *= 0.3;
            }
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体パターン：バックドラフト・バースト
// ============================================================
void EnemyPat_Shotgun_Kimi()
{
    static int phase;        // 0:初期化, 1:予兆, 2:射撃, 3:連射待機, 4:大待機
    static int phaseCount;   // フェーズ内カウンタ
    static int muki;         // 敵の左右移動方向
    static int shotCount;    // 連射回数カウンタ
    static double targetMuki; // プレイヤーへの照準方向

    // --- 初期化 ---
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
        phaseCount = 0;
        shotCount = 0;
        targetMuki = 0.0;
        return;
    }

    // --- フェーズ管理 ---
    switch (phase) {

        // ----------------------------------------
    case 0: // 初期化完了 → 予兆へ
        // ----------------------------------------
        phase = 1;
        phaseCount = 0;
        break;

        // ----------------------------------------
    case 1: // 予兆：プレイヤー方向を照準、小刻みに動く
        // ----------------------------------------
        phaseCount++;

        // 小刻みな左右移動
        enemy.x += 0.6 * muki;
        if (enemy.x < 80.0) { enemy.x = 80.0;  muki = 1; }
        if (enemy.x > 400.0) { enemy.x = 400.0; muki = -1; }
        if (count % 90 == 0) muki *= -1;

        // プレイヤー方向を計算
        targetMuki = atan2(player.y - enemy.y, player.x - enemy.x);

        // 30フレーム（0.5秒）の予兆後に射撃
        if (phaseCount >= 30) {
            phase = 2;
            phaseCount = 0;
        }
        break;

        // ----------------------------------------
    case 2: // 射撃：前方散弾＋後方弾殻＋反動スライド
        // ----------------------------------------
    {
        // ---- 前方散弾セット生成 ----
        sEnemyShotSet* pSetFront = new sEnemyShotSet;
        pSetFront->count = 0;
        pSetFront->patternFunc = ShotShotgunBurst;
        pSetFront->x = enemy.x;
        pSetFront->y = enemy.y + 10.0;
        pSetFront->muki = targetMuki;
        pSetFront->kind = shotCount;

        pSetFront->pEnemyShotHead = new sEnemyShot;
        pSetFront->pEnemyShotHead->prev = pSetFront->pEnemyShotHead;
        pSetFront->pEnemyShotHead->next = pSetFront->pEnemyShotHead;

        pSetFront->prev = enemyShotSetHead.prev;
        pSetFront->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetFront;
        enemyShotSetHead.prev = pSetFront;

        // ---- 後方弾殻セット生成 ----
        sEnemyShotSet* pSetBack = new sEnemyShotSet;
        pSetBack->count = 0;
        pSetBack->patternFunc = ShotShellScatter;
        pSetBack->x = enemy.x;
        pSetBack->y = enemy.y + 10.0;
        pSetBack->muki = targetMuki;
        pSetBack->kind = shotCount + 100;

        pSetBack->pEnemyShotHead = new sEnemyShot;
        pSetBack->pEnemyShotHead->prev = pSetBack->pEnemyShotHead;
        pSetBack->pEnemyShotHead->next = pSetBack->pEnemyShotHead;

        pSetBack->prev = enemyShotSetHead.prev;
        pSetBack->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetBack;
        enemyShotSetHead.prev = pSetBack;

        // ---- 反動で後方へスライド ----
        // プレイヤー方向の逆方向に15ピクセル移動
        double recoilX = -10.0 * cos(targetMuki);
        double recoilY = -10.0 * sin(targetMuki);
        enemy.x += recoilX;
        enemy.y += recoilY;

        // 画面外に出ないように制限
        if (enemy.x < 40.0)  enemy.x = 40.0;
        if (enemy.x > 440.0) enemy.x = 440.0;
        if (enemy.y < 20.0)  enemy.y = 20.0;
        if (enemy.y > 440.0) enemy.y = 440.0;

        shotCount++;
        phase = 3;
        phaseCount = 0;
    }
    break;

    // ----------------------------------------
    case 3: // 連射待機：20フレーム
        // ----------------------------------------
        phaseCount++;

        // 小刻みな移動
        enemy.x += 0.3 * muki;
        if (enemy.x < 80.0) { enemy.x = 80.0;  muki = 1; }
        if (enemy.x > 400.0) { enemy.x = 400.0; muki = -1; }

        // 20フレーム後、次の射撃または大待機へ
        if (phaseCount >= 10) {
            if (shotCount < 3) {
                // 3回連続射撃
                phase = 1;
                phaseCount = 0;
            }
            else {
                // 3回終了 → 大待機
                phase = 4;
                phaseCount = 0;
                shotCount = 0;
            }
        }
        break;

        // ----------------------------------------
    case 4: // 大待機：120フレーム（2秒）後に再開
        // ----------------------------------------
        phaseCount++;

        // ゆっくり左右移動
        enemy.x += 0.5 * muki;
        if (enemy.x < 80.0) { enemy.x = 80.0;  muki = 1; }
        if (enemy.x > 400.0) { enemy.x = 400.0; muki = -1; }
        if (count % 120 == 60) muki *= -1;

        if (phaseCount >= 60) {
            phase = 1;
            phaseCount = 0;
        }
        break;

        // ----------------------------------------
    default:
        // ----------------------------------------
        phase = 1;
        phaseCount = 0;
        break;
    }
}