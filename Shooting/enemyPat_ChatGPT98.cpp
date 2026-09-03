// enemyPat_Tmp.cpp
// 紫奥義「弾幕結界」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {
    constexpr double CX = 240.0;
    constexpr double CY_ = 240.0;
    constexpr double OUTER_RADIUS = 240.0;
    constexpr int EXPAND_FRAMES = 120;

    struct Params {
        int k;
        double a;
        double v;
        int t1;
        int t2;
        double th0;
        double l0;
        double l;
        int T;
    };

    // 周期番号ごとに少しずつ性質を変える。
    static Params GetParams(int cycle)
    {
        static const Params table[] = {
            { 3, 0.018, 2.20,  2, 10, 0.62,  55.0, 0.08, 180 },
            { 4, 0.023, 2.35,  3,  9, 0.70,  48.0, 0.10, 192 },
            { 5, 0.028, 2.50,  3,  8, 0.78,  42.0, 0.11, 204 },
            { 6, 0.033, 2.65,  4,  8, 0.86,  36.0, 0.12, 216 },
            { 5, 0.039, 2.80,  4,  7, 0.94,  30.0, 0.13, 228 },
            { 6, 0.046, 2.95,  5,  7, 1.02,  26.0, 0.14, 240 },
        };
        return table[cycle % (int)(sizeof(table) / sizeof(table[0]))];
    }

    static void AddShot(sEnemyShotSet* set, double x, double y,
        double muki, double speed, int kind)
    {
        sEnemyShot* shot = new sEnemyShot;
        shot->x = x;
        shot->y = y;
        shot->muki = muki;
        shot->speed = speed;
        shot->kind = kind;
        shot->param_i[0] = 0; // 鱗弾
        shot->param_i[4] = -1;
        shot->param_i[5] = -1;

        shot->prev = set->pEnemyShotHead->prev;
        shot->next = set->pEnemyShotHead;
        set->pEnemyShotHead->prev->next = shot;
        set->pEnemyShotHead->prev = shot;
    }

    static void ShotDanmakuKekkai(sEnemyShotSet* set)
    {
        int cycle = set->param_i[0];
        int cycleStart = set->param_i[1];
        Params p = GetParams(cycle);
        int local = set->count - cycleStart;
        int cycleLength = EXPAND_FRAMES + p.T;
        sEnemyShot* shot;

        // 周期終了。古い大玉を回収して、新しい周期の大玉を生成する。
        if (local >= cycleLength) {
            cycle++;
            p = GetParams(cycle);
            set->param_i[0] = cycle;
            set->param_i[1] = set->count;
            cycleStart = set->count;
            local = 0;

            // 鱗弾はそのまま飛び続け、大玉だけを新しい結界へ更新する。
            shot = set->pEnemyShotHead->next;
            while (shot != set->pEnemyShotHead) {
                sEnemyShot* next = shot->next;
                if (shot->param_i[0] == 1) {
                    // 再配置は下の通常更新で行う。
                    shot->param_i[4] = -1;
                    shot->param_i[5] = -1;
                }
                shot = next;
            }
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 必要な大玉だけを確保する。param_i[6] がスロット番号で、
        // 0～k-1 が青、k～2k-1 が紫になる。
        bool used[12] = {};
        shot = set->pEnemyShotHead->next;
        while (shot != set->pEnemyShotHead) {
            if (shot->param_i[0] == 1 && shot->param_i[6] >= 0 && shot->param_i[6] < 12)
                used[shot->param_i[6]] = true;
            shot = shot->next;
        }

        for (int slot = 0; slot < p.k * 2; slot++) {
            if (used[slot]) continue;

            shot = new sEnemyShot;
            shot->param_i[0] = 1; // 大玉
            shot->param_i[6] = slot;
            shot->param_i[4] = -1; // 発射フレーム記録
            shot->param_i[5] = -1; // 発射周期記録
            shot->prev = set->pEnemyShotHead->prev;
            shot->next = set->pEnemyShotHead;
            set->pEnemyShotHead->prev->next = shot;
            set->pEnemyShotHead->prev = shot;
        }

        // 現周期の k 個に合わせて大玉を更新する。
        shot = set->pEnemyShotHead->next;
        while (shot != set->pEnemyShotHead) {
            sEnemyShot* next = shot->next;

            if (shot->param_i[0] == 1) {
                int slot = shot->param_i[6];
                int color = slot < p.k ? 0 : 1;
                int index = color == 0 ? slot : slot - p.k;
                shot->param_i[1] = index;
                shot->param_i[2] = color;
                shot->param_i[3] = cycle;

                if (slot >= p.k * 2) {
                    // この周期では余る大玉。メインルーチンの画面外消去に任せる。
                    shot->x = -1000.0;
                    shot->y = -1000.0;
                    shot = next;
                    continue;
                }

                double r = OUTER_RADIUS * (local >= EXPAND_FRAMES
                    ? 1.0
                    : (double)local / EXPAND_FRAMES);
                double base = color * DX_PI / p.k + 2.0 * DX_PI * index / p.k;
                double angle = base + p.a * local;
                shot->x = CX + r * cos(angle);
                shot->y = CY_ + r * sin(angle);
                shot->muki = angle;
                shot->speed = 0.0;
                shot->kind = color == 0 ? img_enemyShotLargeBall[4]
                    : img_enemyShotLargeBall[5];

                // 鱗弾の発射。外向き3発＋内向き1発。
                if (local >= EXPAND_FRAMES && local < EXPAND_FRAMES + p.T && local % 2 == 0) {
                    int fireFrame = local - EXPAND_FRAMES;
                    if (fireFrame % (p.t1 + p.t2) < p.t1) {
                        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                        double offset = (p.T <= 1)
                            ? 0.0
                            : -p.th0 + 2.0 * p.th0 * fireFrame / (p.T - 1);
                        double innerDir = angle + DX_PI + offset;
                        double outerDir = angle + offset;
                        int scaleKind = color == 0 ? img_enemyShotScale[4]
                            : img_enemyShotScale[5];

                        // 同一フレームに何度も発射することを防ぐ。
                        if (shot->param_i[4] != fireFrame) {
                            shot->param_i[4] = fireFrame;

                            AddShot(set, shot->x, shot->y,
                                innerDir, p.v, scaleKind);

                            for (int j = -1; j <= 1; j++) {
                                double spread = j * 0.16;
                                AddShot(set, shot->x, shot->y,
                                    outerDir + spread, p.v * 0.38, scaleKind);

                                sEnemyShot* s = set->pEnemyShotHead->prev;
                                s->param_i[1] = 1; // 外向き
                                s->param_i[2] = color;
                            }

                            // 今追加した内向き弾へ、停止距離などを記録する。
                            sEnemyShot* inner = set->pEnemyShotHead->prev;
                            // 外向き3発の直後なので、4つ前が内向き弾。
                            for (int i = 0; i < 3; i++) inner = inner->prev;
                            inner->param_i[1] = 0; // 内向き
                            inner->param_i[2] = color;
                            inner->param_d[0] = CX;
                            inner->param_d[1] = CY_;
                            inner->param_d[2] = p.l0 + p.l * fireFrame;
                            inner->param_d[3] = innerDir;
                            inner->param_d[4] = p.v;
                            inner->param_i[14] = p.T;
                            inner->param_i[15] = set->count;
                        }
                    }
                }
            }
            shot = next;
        }

        // 鱗弾を更新。
        shot = set->pEnemyShotHead->next;
        while (shot != set->pEnemyShotHead) {
            sEnemyShot* next = shot->next;
            if (shot->param_i[0] == 0) {
                if (shot->param_i[1] == 0) {
                    // 内向き弾：指定距離まで進んだらT終了まで停止。
                    double dx = shot->x - shot->param_d[0];
                    double dy = shot->y - shot->param_d[1];
                    double r = sqrt(dx * dx + dy * dy);
                    if (shot->speed > 0.0 && r > shot->param_d[2]) {
                        shot->x += shot->speed * cos(shot->muki);
                        shot->y += shot->speed * sin(shot->muki);
                        double dx2 = shot->x - shot->param_d[0];
                        double dy2 = shot->y - shot->param_d[1];
                        double r2 = sqrt(dx2 * dx2 + dy2 * dy2);
                        if (r2 <= shot->param_d[2]) {
                            double a = atan2(dy2, dx2);
                            shot->x = shot->param_d[0] + shot->param_d[2] * cos(a);
                            shot->y = shot->param_d[1] + shot->param_d[2] * sin(a);
                            shot->param_i[3] = shot->param_i[2];
                            shot->speed = 0.0;
                            shot->param_d[3] = -1.0; // 停止中
                        }
                    }

                    // T後は再び速さvで内側へ進ませる。
                    if (shot->param_d[3] < 0.0 &&
                        set->count - shot->param_i[15] >= shot->param_i[14]) {
                        shot->speed = shot->param_d[4];
                        shot->param_d[3] = shot->muki;
                    }
                }

                if (shot->param_d[3] >= 0.0) {
                    shot->x += shot->speed * cos(shot->muki);
                    shot->y += shot->speed * sin(shot->muki);
                }
            }
            else {
                // 外向き弾。
                shot->x += shot->speed * cos(shot->muki);
                shot->y += shot->speed * sin(shot->muki);
            }
            shot = next;
        }

    }
}

void EnemyPat_DanmakuKekkai_ChatGPT()
{
    static int initialized;

    if (count == 1) {
        initialized = 0;
        enemy.x = CX;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
    }

    if (!initialized) {
        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = ShotDanmakuKekkai;
        set->x = CX;
        set->y = CY_;
        set->muki = 0.0;
        set->kind = 0;
        set->param_i[0] = 0; // 周期番号
        set->param_i[1] = 0; // 周期開始時のset.count

        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        initialized = 1;
    }
}
