// 弾幕：平安京の悪夢
// enemyPat_sampleForAI.cpp / gv.h / imgSoundLoad.h の構成に合わせた実装。
// 弾・弾セットの count 加算、および画面外弾の消去はメインルーチンに任せる。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <algorithm>

namespace {

    // ゲーム画面は 480x480。弾幕は画面外 60px まで広げて配置する。
    constexpr double FIELD_MIN = -60.0;
    constexpr double FIELD_MAX = 540.0;
    constexpr double FIELD_SPAN = FIELD_MAX - FIELD_MIN;
    constexpr double GRID_W = 60.0;       // 横方向の間隔 W は全ラウンドで固定
    constexpr int    WAIT_FRAMES = 60;    // 1 秒（60 fps 想定）
    constexpr int    PATTERN_INTERVAL = 360;

    // sEnemyShotSet をリストへ登録する。count はここでは加算しない。
    static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func)
    {
        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = func;
        set->x = enemy.x;
        set->y = enemy.y;
        set->muki = 0.0;
        set->kind = 0;

        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
        return set;
    }

    static void AppendShot(sEnemyShotSet* set, sEnemyShot* shot)
    {
        sEnemyShot* head = set->pEnemyShotHead;
        shot->prev = head->prev;
        shot->next = head;
        head->prev->next = shot;
        head->prev = shot;
    }

    // この弾セットに属する弾をすべて消す。count の操作は行わない。
    static void ClearShots(sEnemyShotSet* set)
    {
        sEnemyShot* head = set->pEnemyShotHead;
        sEnemyShot* shot = head->next;
        while (shot != head) {
            sEnemyShot* next = shot->next;
            shot->prev->next = shot->next;
            shot->next->prev = shot->prev;
            delete shot;
            shot = next;
        }
        head->next = head;
        head->prev = head;
    }

    static void PlaySoundRestart(int sound)
    {
        if (CheckSoundMem(sound)) StopSoundMem(sound);
        PlaySoundMem(sound, DX_PLAYTYPE_BACK);
    }

    static double LaserAngle(bool transposed)
    {
        // 下向きレーザーを x/y 転置すると右向きレーザーになる。
        return transposed ? 0.0 : DX_PI / 2.0;
    }

    // 0 から 1 まで、最初は加速し、中央で最大速度になり、最後は減速する進行率。
    static double EaseAccelerateDecelerate(int elapsed, int T)
    {
        if (elapsed <= 0) return 0.0;
        if (elapsed >= 2 * T) return 1.0;

        if (elapsed <= T) {
            const double u = static_cast<double>(elapsed) / T;
            return 0.5 * u * u;
        }

        const double u = static_cast<double>(elapsed - T) / T;
        return 0.5 + u - 0.5 * u * u;
    }

    // 1 回分の基本パターン設定。第 3 フェーズでは通常形と転置形で共有する。
    struct GridConfig {
        int N = 0;
        int T = 0;
        int rowCount = 0;
        int colCount = 0;
        double H = 0.0;
        int P[8]{};
        int R[8]{};
        int D[8]{};
    };

    static GridConfig MakeGridConfig()
    {
        GridConfig cfg;
        cfg.N = 3 + GetRand(5);             // 3～8
        // 600 の約数から選び、-60 から 540 までを間隔 H でちょうど埋める。
        // いずれも W=60 より狭い間隔。
        const int hCandidates[] = { 10, 12, 15, 20, 24, 25, 30 };
        cfg.H = static_cast<double>(hCandidates[GetRand(6)]);
        cfg.T = 30 + GetRand(25);            // 30～55 フレーム
        cfg.colCount = static_cast<int>(FIELD_SPAN / GRID_W) + 1;
        cfg.rowCount = static_cast<int>(FIELD_SPAN / cfg.H) + 1;

        for (int k = 0; k < cfg.N; ++k) cfg.P[k] = k;
        // Fisher–Yates で 0..N-1 のランダム順列を作る。
        for (int k = cfg.N - 1; k > 0; --k) {
            const int swapIndex = GetRand(k);
            std::swap(cfg.P[k], cfg.P[swapIndex]);
        }
        for (int k = 0; k < cfg.N; ++k) {
            cfg.R[k] = GetRand(1);
            cfg.D[k] = GetRand(1);
        }
        return cfg;
    }

    // 基本パターン（転置時は位置とレーザーの向きを x/y 交換する）。
    // param_i[4] は転置フラグ、param_i[5..12] は P、param_i[13..14] は R/D のビットマスク。
    static void PatternHeianGrid(sEnemyShotSet* set)
    {
        const int t = set->count;
        const bool transposed = (set->param_i[4] != 0);

        if (t == 0) {
            const int N = set->param_i[0];
            const double H = set->param_d[0];
            const double W = set->param_d[1];
            const int rowCount = set->param_i[2];
            const int colCount = set->param_i[3];
            const int rMask = set->param_i[13];
            const int dMask = set->param_i[14];

            int P[8]{};
            int R[8]{};
            int D[8]{};
            for (int k = 0; k < N; ++k) {
                P[k] = set->param_i[5 + k];
                R[k] = (rMask >> k) & 1;
                D[k] = (dMask >> k) & 1;
            }

            for (int i = 0; i < colCount; ++i) {
                for (int j = 0; j < rowCount; ++j) {
                    const int r = j % N;
                    const int Q = (P[r] - r + N) % N;
                    const int i2 = i + (R[r] ? 1 : -1);
                    const int j2 = D[r] ? (j + Q) : (j + Q - N);

                    const double sourceX = transposed
                        ? FIELD_MIN + j * H
                        : FIELD_MIN + i * W;
                    const double sourceY = transposed
                        ? FIELD_MIN + i * W
                        : FIELD_MIN + j * H;
                    const double targetX = transposed
                        ? FIELD_MIN + j2 * H
                        : FIELD_MIN + i2 * W;
                    const double targetY = transposed
                        ? FIELD_MIN + i2 * W
                        : FIELD_MIN + j2 * H;

                    sEnemyShot* shot = new sEnemyShot;
                    shot->x = sourceX;
                    shot->y = sourceY;
                    shot->muki = LaserAngle(transposed);
                    shot->speed = 0.0;
                    shot->kind = img_enemyShotLaser[0]; // 赤レーザー
                    shot->param_i[0] = i;
                    shot->param_i[1] = j;
                    shot->param_d[0] = sourceX;
                    shot->param_d[1] = sourceY;
                    shot->param_d[2] = targetX;
                    shot->param_d[3] = targetY;
                    AppendShot(set, shot);
                }
            }
            return; // 出現フレームでは初期位置に静止させる。
        }

        const int T = set->param_i[1];
        const int moveStart = WAIT_FRAMES;
        const int moveEnd = moveStart + 2 * T;
        const int eraseTime = moveEnd + WAIT_FRAMES;

        if (t >= eraseTime) {
            // 到着して 1 秒待った後、格子レーザーを一斉に消す。
            ClearShots(set);
            // 次のパターン開始までは空のまま待機する。
            return;
        }

        sEnemyShot* shot = set->pEnemyShotHead->next;
        while (shot != set->pEnemyShotHead) {
            if (t < moveStart) {
                shot->x = shot->param_d[0];
                shot->y = shot->param_d[1];
                shot->kind = img_enemyShotLaser[0];
                shot->muki = LaserAngle(transposed);
                shot->speed = 0.0;
            }
            else if (t < moveEnd) {
                const double progress = EaseAccelerateDecelerate(t - moveStart, T);
                const double dx = shot->param_d[2] - shot->param_d[0];
                const double dy = shot->param_d[3] - shot->param_d[1];
                shot->x = shot->param_d[0] + dx * progress;
                shot->y = shot->param_d[1] + dy * progress;
                shot->kind = img_enemyShotMediumBall[0]; // 移動中は赤い中玉
                shot->muki = atan2(dy, dx);
                shot->speed = 0.0;
            }
            else {
                shot->x = shot->param_d[2];
                shot->y = shot->param_d[3];
                shot->kind = img_enemyShotLaser[0];
                shot->muki = LaserAngle(transposed);
                shot->speed = 0.0;
            }
            shot = shot->next;
        }
    }

    static sEnemyShotSet* CreateGridPattern(bool transposed, const GridConfig& cfg)
    {
        sEnemyShotSet* set = CreateShotSet(PatternHeianGrid);
        set->param_i[0] = cfg.N;
        set->param_i[1] = cfg.T;
        set->param_i[2] = cfg.rowCount;
        set->param_i[3] = cfg.colCount;
        set->param_i[4] = transposed ? 1 : 0;
        set->param_d[0] = cfg.H;
        set->param_d[1] = GRID_W;

        int rMask = 0;
        int dMask = 0;
        for (int k = 0; k < cfg.N; ++k) {
            set->param_i[5 + k] = cfg.P[k];
            rMask |= (cfg.R[k] << k);
            dMask |= (cfg.D[k] << k);
        }
        set->param_i[13] = rMask;
        set->param_i[14] = dMask;
        return set;
    }

    // 大玉の列。axis=0 は上下方向、axis=1 は左右方向。
    // param_i[0]=axis、param_i[1]=端の選択（0:上/左、1:下/右）。
    static void PatternBigShotLine(sEnemyShotSet* set)
    {
        const int axis = set->param_i[0];
        if (set->count == 0) {
            set->param_i[1] = GetRand(1);
            PlaySoundRestart(sound_enemyShot_heavy);
        }

        // 先に既存弾を動かし、今フレームに生成する弾は端から出現させる。
        sEnemyShot* shot = set->pEnemyShotHead->next;
        while (shot != set->pEnemyShotHead) {
            shot->x += shot->speed * cos(shot->muki);
            shot->y += shot->speed * sin(shot->muki);
            shot = shot->next;
        }

        if (set->count <= 84 && set->count % 14 == 0) {
            const int side = set->param_i[1];
            sEnemyShot* bullet = new sEnemyShot;
            bullet->kind = img_enemyShotLargeBall[0]; // 赤い大玉
            bullet->speed = 3.0;

            if (axis == 0) {
                // 自機と同じ x 座標。上端から下向き、または下端から上向き。
                bullet->x = player.x;
                if (side == 0) {
                    bullet->y = 0.0;
                    bullet->muki = DX_PI / 2.0;
                }
                else {
                    bullet->y = 480.0;
                    bullet->muki = -DX_PI / 2.0;
                }
            }
            else {
                // 自機と同じ y 座標。左端から右向き、または右端から左向き。
                bullet->y = player.y;
                if (side == 0) {
                    bullet->x = 0.0;
                    bullet->muki = 0.0;
                }
                else {
                    bullet->x = 480.0;
                    bullet->muki = DX_PI;
                }
            }
            AppendShot(set, bullet);
        }
    }

    static void CreateBigShotLine(int axis)
    {
        sEnemyShotSet* set = CreateShotSet(PatternBigShotLine);
        set->param_i[0] = axis;
    }

    static int GetHpPhase()
    {
        if (enemy.maxHp <= 0) return 1;
        // HP がちょうど 2/3 の場合は第 1 フェーズ、1/3 は第 3 フェーズ。
        if (enemy.hp * 3 >= enemy.maxHp * 2) return 1;
        if (enemy.hp * 3 > enemy.maxHp) return 2;
        return 3;
    }

} // namespace

// 敵本体の弾幕パターン
void EnemyPat_Tmp()
{
    static int moveDirection = 1;
    static int activePhase = 0;
    static int nextPatternFrame = 1;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // サンプル仕様に合わせ、maxHp は 200
        moveDirection = 1;
        activePhase = 0;
        nextPatternFrame = 60;
    }
    else {
        enemy.x += 0.98 * static_cast<double>(moveDirection);
        if (count % 120 == 60) moveDirection *= -1;
    }

    const int phase = GetHpPhase();
    if (phase != activePhase) {
        activePhase = phase;
        nextPatternFrame = count + 60; // フェーズ移行後、すぐに新しいパターンを開始
    }

    if (count >= nextPatternFrame) {
        const GridConfig cfg = MakeGridConfig();
        if (phase == 1) {
            PlaySoundRestart(sound_enemyCharge);
            CreateGridPattern(false, cfg);
            CreateBigShotLine(0); // 上下方向の大玉列
        }
        else if (phase == 2) {
            PlaySoundRestart(sound_enemyCharge);
            CreateGridPattern(true, cfg); // x/y 転置
            CreateBigShotLine(1);         // 左右方向の大玉列
        }
        else {
            PlaySoundRestart(sound_enemyCharge);
            CreateGridPattern(false, cfg);
            CreateGridPattern(true, cfg); // 同じ設定の基本形と転置形を同時に発生
        }
        nextPatternFrame = count + PATTERN_INTERVAL;
    }
}
