#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：無限喇叭「ガブリエル」
//   既存の小弾・中弾・大弾だけで、立体的に見えるラッパを表現する。
//   - 楕円圧縮で立体感
//   - 奥側＝小弾（シアン）／手前側＝中弾（白）／口の縁＝大弾（赤）
//   - 保持中はゆっくり回転、咆哮で外向きの法線方向へ加速
// ============================================================
static void ShotGabriel(sEnemyShotSet* pEnemyShotSet)
{
    const double PI2 = 2.0 * DX_PI;
    static int    HOLD_FRAMES = 72;    // 1.2秒 @60fps
    const int    RINGS = 10;    // ラッパの輪の数
    const double R_MOUTH = 100.0; // 口の半径
    const double X_MAX = 5.0;   // 奥行き係数の最大値
    const double GAP = 25.0;  // 弾の間隔（弧長）
    const double FLATTEN = 0.35;  // 縦方向の圧縮率（立体感）
    const double Y_MOUTH_OFF = 160.0; // 口のyオフセット
    const double ROT_SPEED = 0.015; // 保持中の回転速度

    // ---- スポーン ----
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        HOLD_FRAMES = 72 + (GetRand(20) - 10);

        const double cx = pEnemyShotSet->x;
        const double y_mouth = pEnemyShotSet->y + Y_MOUTH_OFF;
        const double y_tail = pEnemyShotSet->y;

        // 奥（尾）から手前（口）の順に追加し、
        // 手前のリングほど後から描画＝上に重なるようにする
        for (int i = RINGS - 1; i >= 0; i--) {
            double t = (double)i / (double)(RINGS - 1);
            double xv = 1.0 + (X_MAX - 1.0) * t;
            double R = R_MOUTH / xv;
            double cy = y_mouth + (y_tail - y_mouth) * t;

            int N = (int)(PI2 * R / GAP);
            if (N < 6) N = 6;

            double phi = i * 0.37; // リングごとに位相をずらす

            // 奥側(sin<0)を先、手前側(sin>=0)を後に追加
            for (int pass = 0; pass < 2; pass++) {
                for (int j = 0; j < N; j++) {
                    double theta = phi + PI2 * (double)j / (double)N;
                    bool isFront = (sin(theta) >= 0.0);
                    if (pass == 0 && isFront) continue;
                    if (pass == 1 && !isFront) continue;

                    double px = cx + R * cos(theta);
                    double py = cy + FLATTEN * R * sin(theta);

                    sEnemyShot* p = new sEnemyShot;
                    p->x = px;
                    p->y = py;
                    p->muki = 0.0;
                    p->speed = 0.0;
                    p->count = 0;

                    // 種別：口の縁は大弾、奥側は小弾、手前側は中弾
                    if (i == 0) {
                        p->kind = img_enemyShotLargeBall[0];   // 赤
                    }
                    else if (!isFront) {
                        p->kind = img_enemyShotSmallBall[3];   // シアン
                    }
                    else {
                        p->kind = img_enemyShotMediumBall[6];  // 白
                    }

                    // パラメータ保存
                    p->param_d[0] = cx;      // 中心x
                    p->param_d[1] = cy;      // 中心y
                    p->param_d[2] = R;       // 半径
                    p->param_d[3] = theta;   // 基準角度
                    p->param_i[0] = i;       // リング番号
                    p->param_i[1] = 0;       // 状態: 0=保持, 1=咆哮後

                    // リスト挿入（末尾＝head の手前）
                    p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    p->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = p;
                    pEnemyShotSet->pEnemyShotHead->prev = p;
                }
            }
        }
    }

    // ---- 咆哮：口からの大弾放射バースト ----
    if (pEnemyShotSet->count == HOLD_FRAMES) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const double mx = pEnemyShotSet->x;
        const double my = pEnemyShotSet->y + Y_MOUTH_OFF;

        for (int k = 0; k < 24; k++) {
            double ang = PI2 * (double)k / 24.0;
            sEnemyShot* p = new sEnemyShot;
            p->x = mx;
            p->y = my;
            p->muki = ang;
            p->speed = 3.0;
            p->count = 0;
            p->kind = img_enemyShotLargeBall[8]; // 橙
            p->param_i[1] = 1;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // ---- 咆哮2波目：中弾の半角ずらし ----
    if (pEnemyShotSet->count == HOLD_FRAMES + 10) {
        const double mx = pEnemyShotSet->x;
        const double my = pEnemyShotSet->y + Y_MOUTH_OFF;

        for (int k = 0; k < 24; k++) {
            double ang = PI2 * ((double)k + 0.5) / 24.0;
            sEnemyShot* p = new sEnemyShot;
            p->x = mx;
            p->y = my;
            p->muki = ang;
            p->speed = 2.4;
            p->count = 0;
            p->kind = img_enemyShotMediumBall[1]; // 黄
            p->param_i[1] = 1;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // ---- 全弾更新 ----
    const int c = pEnemyShotSet->count;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;

        if (pShot->param_i[1] == 0) {
            const int    i = pShot->param_i[0];
            const double theta = pShot->param_d[3] + c * ROT_SPEED;
            const double R = pShot->param_d[2];
            const double cx = pShot->param_d[0];
            const double cy = pShot->param_d[1];

            // 回転に合わせて弾種を更新し、奥/手前の見た目を保つ
            if (i == 0) {
                pShot->kind = img_enemyShotLargeBall[0];
            }
            else if (sin(theta) < 0.0) {
                pShot->kind = img_enemyShotSmallBall[3];
            }
            else {
                pShot->kind = img_enemyShotMediumBall[6];
            }

            if (c < HOLD_FRAMES) {
                // 保持：楕円上を回転
                pShot->x = cx + R * cos(theta);
                pShot->y = cy + FLATTEN * R * sin(theta);
            }
            else {
                // 咆哮へ移行：外向きの法線方向へ加速
                double dirx = cos(theta);
                double diry = FLATTEN * sin(theta);
                double len = sqrt(dirx * dirx + diry * diry);
                if (len < 1e-6) len = 1.0;
                pShot->muki = atan2(diry / len, dirx / len);

                double s = sin(theta);
                if (i == 0) {
                    pShot->speed = 2.2;
                }
                else if (s >= 0.0) {
                    // 手前側は速く膨らむ
                    pShot->speed = 2.5 + s * 1.5;
                }
                else {
                    // 奥側はゆっくり
                    pShot->speed = 0.8 + (1.0 + s) * 0.6;
                }

                pShot->param_i[1] = 1;
            }
        }

        if (pShot->param_i[1] == 1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = next;
    }
}

// ============================================================
// 敵本体：無限喇叭「ガブリエル」
// ============================================================
void EnemyPat_GabrielsHorn_DeepSeek()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 140.0) muki = 1;
        if (enemy.x > 340.0) muki = -1;
    }

    // 5秒ごとにラッパを1回発射
    if (count % 200 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGabriel;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
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