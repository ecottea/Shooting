#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 二重スリット実験モチーフ弾幕
// 使用素材:
//   壁(スリット形成): 中玉 (img_enemyShotMediumBall) シアン(3)
//   粒子弾:           小玉 (img_enemyShotSmallBall) 黄(1) / 橙(8)
//   干渉縞:           菱形弾 (img_enemyShotDiamond) マゼンタ(5) / 緑(2)
//   効果音:           sound_enemyShot_medium, sound_enemyShot_light, sound_enemyCharge
// ============================================================

// スリット位置の定数 (画面幅480)
static const double SLIT_Y = 110.0;   // 壁の中心Y
static const double SLIT_LEFT_X = 190.0;   // 左スリット中心
static const double SLIT_RIGHT_X = 290.0;   // 右スリット中心
static const double SLIT_WIDTH = 28.0;    // 各スリットの幅
static const double WALL_TOP = 70.0;
static const double WALL_BOTTOM = 150.0;

// ------------------------------------------------------------
// スリット壁形成パターン (速度0の中玉で左右の壁を作る)
// ------------------------------------------------------------
static void ShotSlitWalls(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 左壁: x = 20 ～ (左スリット左端)
        // 右壁: (右スリット右端) ～ 460
        // 上下に中玉を並べて壁を形成。スリット部分は意図的に空ける。
        const double leftWallEnd = SLIT_LEFT_X - SLIT_WIDTH * 0.5;
        const double rightWallStart = SLIT_RIGHT_X + SLIT_WIDTH * 0.5;

        // 縦方向の間隔
        const double dy = 14.0;
        // 横方向の間隔
        const double dx = 16.0;

        // 左壁
        for (double y = WALL_TOP; y <= WALL_BOTTOM; y += dy) {
            for (double x = 20.0; x <= leftWallEnd; x += dx) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = x;
                pEnemyShot->y = y;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;               // 静止
                pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン中玉
                pEnemyShot->margin = 40.0;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        // 右壁
        for (double y = WALL_TOP; y <= WALL_BOTTOM; y += dy) {
            for (double x = rightWallStart; x <= 460.0; x += dx) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = x;
                pEnemyShot->y = y;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotMediumBall[3];
                pEnemyShot->margin = 40.0;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        // 中央の細い仕切り (2スリットを分離する薄い壁)
        for (double y = WALL_TOP; y <= WALL_BOTTOM; y += dy) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 240.0;
            pEnemyShot->y = y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[3];
            pEnemyShot->margin = 40.0;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 速度0なので移動処理不要。画面外削除はメインに任せる。
}

// ------------------------------------------------------------
// 粒子射出パターン (スリットに向かう小玉)
// 通過後に横方向の揺れを加えて干渉の芽を作る
// ------------------------------------------------------------
static void ShotParticles(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 左右のスリットへ交互に粒子を射出
        const int num = 5;
        for (int i = 0; i < num; i++) {
            // どちら側のスリットを狙うか
            int side = (pEnemyShotSet->kind + i) % 2;
            double targetX = (side == 0) ? SLIT_LEFT_X : SLIT_RIGHT_X;
            // スリット幅内で少し散らす
            targetX += (GetRand(20) - 10);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(30) - 15);
            pEnemyShot->y = pEnemyShotSet->y;
            // 初期方向はターゲットへ
            pEnemyShot->muki = atan2(SLIT_Y - pEnemyShot->y, targetX - pEnemyShot->x);
            pEnemyShot->speed = 3.2 + GetRand(80) / 100.0;

            // 色を交互に (粒子感)
            if (i % 2 == 0)
                pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄
            else
                pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙

            // param_d[0] : 通過後の横揺れ位相
            pEnemyShot->param_d[0] = GetRand(360) / 180.0 * DX_PI;
            // param_d[1] : 揺れ振幅
            pEnemyShot->param_d[1] = 0.8 + GetRand(60) / 100.0;
            // param_i[0] : 通過フラグ (0=未通過, 1=通過後)
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // スリットを通過したか判定
        if (pShot->param_i[0] == 0 && pShot->y >= SLIT_Y) {
            pShot->param_i[0] = 1;
            // 通過直後に少し減速して波的挙動へ
            pShot->speed *= 0.85;
        }

        if (pShot->param_i[0] == 0) {
            // 通過前: 素直に直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 通過後: 基本方向を保ちつつ横方向に正弦波揺れを加える
            // これにより左右の粒子流が重なり合い、密度の濃淡が生まれやすくなる
            pShot->param_d[0] += 0.12; // 位相進行
            double lateral = pShot->param_d[1] * sin(pShot->param_d[0]);
            pShot->x += pShot->speed * cos(pShot->muki) + lateral * 0.6/2;
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 干渉縞パターン (スリット通過後の領域に濃淡の縦帯を生成)
// 菱形弾を使って明るい帯(密集)と暗い帯(疎)を交互に作る
// ------------------------------------------------------------
static void ShotInterference(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 干渉縞の中心を画面中央付近に配置
        // 明るい帯の間隔を約40px前後に設定
        const double fringeSpacing = 42.0*2;
        const int numFringes = 7; // 中央含む左右3本ずつ
        const double baseY = 180.0 + (pEnemyShotSet->kind % 3) * 15.0-60;

        for (int f = -numFringes / 2; f <= numFringes / 2; f++) {
            double cx = 240.0 + f * fringeSpacing;
            // 明るい帯は密度高め、端の帯は少し弱く
            int density = (abs(f) <= 1) ? 6 : 4;
            int color = (abs(f) % 2 == 0) ? 5 : 2; // マゼンタ / 緑 交互

            for (int i = 0; i < density; i++) {
                pEnemyShot = new sEnemyShot;
                // 帯の中で少し散らす
                pEnemyShot->x = cx + (GetRand(18) - 9);
                pEnemyShot->y = baseY + (GetRand(30) - 15);
                // ほぼ真下へ、わずかに外側へ開く
                double open = f * 0.04;
                pEnemyShot->muki = DX_PI * 0.5 + open + (GetRand(20) - 10) / 180.0 * DX_PI/2;
                pEnemyShot->speed = 1.8 + GetRand(60) / 100.0;
                pEnemyShot->kind = img_enemyShotDiamond[color];
                // 帯の中でゆっくり左右に揺れるパラメータ
                pEnemyShot->param_d[0] = GetRand(360) / 180.0 * DX_PI;
                pEnemyShot->param_d[1] = 0.4 + GetRand(40) / 100.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 移動 + 緩やかな横揺れ (干渉縞が波打つ感じ)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->param_d[0] += 0.08;
        double lateral = pShot->param_d[1] * sin(pShot->param_d[0]);
        pShot->x += pShot->speed * cos(pShot->muki) + lateral * 0.35;
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_DoubleSlit_Grok()
{
    static int moveDir;
    static int particleCount;
    static int interferenceCount;
    static bool wallsCreated;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 固定
        moveDir = 1;
        particleCount = 0;
        interferenceCount = 0;
        wallsCreated = false;
    }
    else {
        // 緩やかな左右移動
        enemy.x += 0.7 * (double)moveDir;
        if (enemy.x < 160.0) {
            enemy.x = 160.0;
            moveDir = 1;
        }
        else if (enemy.x > 320.0) {
            enemy.x = 320.0;
            moveDir = -1;
        }
    }

    // ----- スリット壁を一度だけ生成 -----
    if (!wallsCreated && count == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSlitWalls;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
        wallsCreated = true;
    }

    // ----- 粒子弾を定期的に射出 (スリットが形成された後) -----
    if (count > 60 && count % 12 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotParticles;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = DX_PI * 0.5; // 下向き基準
        pEnemyShotSet->kind = particleCount++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ----- 干渉縞を定期的に生成 (粒子が十分通過した後) -----
    if (count > 140 && count % 45 == 5) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotInterference;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 200.0;
        pEnemyShotSet->muki = DX_PI * 0.5;
        pEnemyShotSet->kind = interferenceCount++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}