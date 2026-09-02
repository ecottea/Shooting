// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 反則弾幕「反則判定・即時失格」
// ・序盤: 通常の狙い撃ち
// ・中盤: 自機の位置を勝手に変更
// ・後半: 自機ショットを勝手に移動
// ・終盤: ボスHPまで不正操作し、最後は弾幕側から強制失格

static void AddShot(sEnemyShotSet* set, double x, double y, double muki,
    double speed, int kind, int mode = 0, double p0 = 0.0, double p1 = 0.0)
{
    sEnemyShot* shot = new sEnemyShot;
    shot->x = x;
    shot->y = y;
    shot->muki = muki;
    shot->speed = speed;
    shot->kind = kind;
    shot->param_i[0] = mode;
    shot->param_d[0] = p0;
    shot->param_d[1] = p1;

    shot->prev = set->pEnemyShotHead->prev;
    shot->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = shot;
    set->pEnemyShotHead->prev = shot;
}

static void InitSet(sEnemyShotSet* set, double x, double y)
{
    set->count = 0;
    set->x = x;
    set->y = y;
    set->pEnemyShotHead = new sEnemyShot;
    set->pEnemyShotHead->prev = set->pEnemyShotHead;
    set->pEnemyShotHead->next = set->pEnemyShotHead;

    set->prev = enemyShotSetHead.prev;
    set->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = set;
    enemyShotSetHead.prev = set;
}

// 主役となる「反則弾」。途中から軌道や速度を勝手に変更する。
static void ShotRuleBreak(sEnemyShotSet* set)
{
    sEnemyShot* shot = set->pEnemyShotHead->next;
    while (shot != set->pEnemyShotHead) {
        int mode = shot->param_i[0];
        int t = shot->count;

        if (mode == 0) {
            shot->x += shot->speed * cos(shot->muki);
            shot->y += shot->speed * sin(shot->muki);
        }
        else if (mode == 1) {
            // 一度まっすぐ飛んだ後、突然自機へ「補正」する反則。
            if (t < 80) {
                shot->x += shot->speed * cos(shot->muki);
                shot->y += shot->speed * sin(shot->muki);
            }
            else {
                double a = atan2(player.y - shot->y, player.x - shot->x);
                shot->x += 3.4 * cos(a);
                shot->y += 3.4 * sin(a);
            }
        }
        else if (mode == 2) {
            // 見かけはリングだが、一定時間後に突然中心へ落ちる。
            double r = shot->param_d[0] - 0.035 * t;
            double a = shot->param_d[1] + 0.035 * t;
            shot->x = enemy.x + r * cos(a);
            shot->y = enemy.y + 0.62 * r * sin(a);
        }

        shot = shot->next;
    }

    if (set->count == 1) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
}

// 敵本体のパターン
void EnemyPat_Violate_ChatGPT()
{
    static int phase;
    static int shotSeed;

    if (count == 1) {
        phase = 0;
        shotSeed = 0;

        enemy.x = 240.0;
        enemy.y = 65.0;
        enemy.maxHp = enemy.hp = 200;

        player.x = 240.0;
        player.y = 420.0;
    }

    // ボスは規則正しく動いていたはずなのに、途中から反則的に位置を変える。
    if (count < 360) {
        enemy.x = 240.0 + 145.0 * sin(count * 0.009);
        enemy.y = 65.0 + 22.0 * sin(count * 0.013);
    }
    else if (count < 720) {
        enemy.x = 90.0 + (GetRand(1) ? 300.0 : 0.0);
        enemy.y = 55.0 + 55.0 * sin(count * 0.03);
    }
    else {
        enemy.x = 240.0 + 170.0 * sin(count * 0.021);
        enemy.y = 70.0 + 35.0 * cos(count * 0.017);
    }

    // ------------------------------------------------------------
    // 序盤: 普通の自機狙い連射
    // ------------------------------------------------------------
    if (count < 300 && count % 12 == 1) {
        sEnemyShotSet* set = new sEnemyShotSet;
        InitSet(set, enemy.x, enemy.y + 12.0);
        set->patternFunc = ShotRuleBreak;

        double base = atan2(player.y - set->y, player.x - set->x);
        for (int i = -2; i <= 2; ++i) {
            AddShot(set, set->x, set->y, base + i * 0.10, 2.4,
                img_enemyShotSmallBall[(shotSeed + i + 8) % 3], 0);
        }
        shotSeed++;
    }

    // ------------------------------------------------------------
    // 第1反則: 「自機狙いなのに避けた場所へ後から曲がる」
    // ------------------------------------------------------------
    if (count >= 240 && count < 600 && count % 18 == 1) {
        sEnemyShotSet* set = new sEnemyShotSet;
        InitSet(set, enemy.x, enemy.y + 15.0);
        set->patternFunc = ShotRuleBreak;

        double base = atan2(player.y - set->y, player.x - set->x);
        for (int i = -3; i <= 3; ++i) {
            AddShot(set, set->x, set->y, base + i * 0.075, 2.1,
                img_enemyShotMediumBall[(shotSeed + i + 12) % 5], 1);
        }
        shotSeed++;
    }

    // ------------------------------------------------------------
    // 第2反則: 画面中央のリングを作った直後、内側へ潰す。
    // ------------------------------------------------------------
    if (count >= 420 && count < 900 && count % 72 == 1) {
        sEnemyShotSet* set = new sEnemyShotSet;
        InitSet(set, enemy.x, enemy.y);
        set->patternFunc = ShotRuleBreak;

        for (int i = 0; i < 24; ++i) {
            double a = DX_PI * 2.0 * i / 24.0;
            AddShot(set, set->x, set->y, a, 0.0,
                img_enemyShotScale[(shotSeed + i) % 4], 2,
                170.0, a);
        }
        shotSeed++;
    }

    // ------------------------------------------------------------
    // 第3反則: 自機そのものを弾幕側が勝手に移動させる。
    // ------------------------------------------------------------
    if (count == 540 || count == 720 || count == 900) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (count >= 540 && count < 600) {
        double a = (count - 540) * DX_PI / 59.0;
        player.x = 240.0 + 115.0 * cos(a);
        player.y = 300.0 + 75.0 * sin(a);
    }
    else if (count >= 720 && count < 780) {
        // 逃げようとしても、画面端へ強制ワープ。
        player.x = (count < 750) ? 42.0 : 438.0;
        player.y = 400.0 - 110.0 * sin((count - 720) * DX_PI / 59.0);
    }
    else if (count >= 900 && count < 1020) {
        player.x = 240.0;
        player.y = 250.0;
    }

    // ------------------------------------------------------------
    // 第4反則: 自機ショットを勝手にボスへ吸着させる。
    // ------------------------------------------------------------
    if (count >= 810 && count < 1060) {
        sPlayerShot* shot = playerShotHead.next;
        while (shot != &playerShotHead) {
            double dx = enemy.x - shot->x;
            double dy = enemy.y - shot->y;
            double d = sqrt(dx * dx + dy * dy);

            if (d > 1.0) {
                // 本来のショット操作を無視し、ボスへワープ気味に吸着。
                shot->x += dx * 0.28;
                shot->y += dy * 0.28;
            }
            shot = shot->next;
        }
    }

    // ------------------------------------------------------------
    // 第5反則: ボスHPを不正操作。「倒したはず」が元に戻る。
    // ------------------------------------------------------------
    if (count >= 1020 && count < 1140) {
        enemy.hp = enemy.maxHp;
        if (count % 15 == 0) {
            sEnemyShotSet* set = new sEnemyShotSet;
            InitSet(set, enemy.x, enemy.y);
            set->patternFunc = ShotRuleBreak;

            double base = atan2(player.y - set->y, player.x - set->x);
            for (int i = 0; i < 18; ++i) {
                double a = base + DX_PI * 2.0 * i / 18.0;
                AddShot(set, set->x, set->y, a, 2.7,
                    img_enemyShotDiamond[(shotSeed + i) % 6], 0);
            }
            shotSeed++;
        }
    }

    // ------------------------------------------------------------
    // 最終反則: 「反則」の字形を弾で作り、最後に強制失格。
    // ------------------------------------------------------------
    if (count == 1140) {
        // 反
        const int p0[][2] = {
            {-3, -3},{-2,-3},{-1,-3},{0,-3},{1,-3},{2,-3},{3,-3},
            {0,-2},{-3,-1},{-2,-1},{-1,-1},{0,-1},{1,-1},{2,-1},{3,-1},
            {0,0},{-1,1},{-2,2},{-3,3},{0,2},{1,3},{2,3}
        };
        // 則
        const int p1[][2] = {
            {-6,-3},{-5,-3},{-4,-3},{-3,-3},{-2,-3},{-1,-3},{0,-3},{1,-3},{2,-3},{3,-3},
            {-6,-2},{-3,-2},{3,-2},{-6,-1},{-3,-1},{3,-1},
            {-6,0},{-5,0},{-4,0},{-3,0},{-2,0},{-1,0},{0,0},{1,0},{2,0},{3,0},
            {-4,1},{0,1},{3,1},{-4,2},{0,2},{3,2},{-4,3},{-3,3},{-2,3},{-1,3},{0,3},{1,3},{2,3},{3,3}
        };

        sEnemyShotSet* set = new sEnemyShotSet;
        InitSet(set, 240.0, 185.0);
        set->patternFunc = ShotRuleBreak;

        for (int i = 0; i < (int)(sizeof(p0) / sizeof(p0[0])); ++i) {
            AddShot(set, 0, 0, 0, 0,
                img_enemyShotMediumOval[i % 6], 2,
                0.0, 0.0);
            sEnemyShot* s = set->pEnemyShotHead->prev;
            s->param_i[0] = 0;
            s->param_d[0] = 0;
            s->x = 145.0 + p0[i][0] * 8.0;
            s->y = 185.0 + p0[i][1] * 8.0;
            s->speed = 0.0;
        }
        for (int i = 0; i < (int)(sizeof(p1) / sizeof(p1[0])); ++i) {
            AddShot(set, 0, 0, 0, 0,
                img_enemyShotMediumOval[(i + 2) % 6], 0);
            sEnemyShot* s = set->pEnemyShotHead->prev;
            s->x = 315.0 + p1[i][0] * 8.0;
            s->y = 185.0 + p1[i][1] * 8.0;
            s->speed = 0.0;
        }
    }

    if (count >= 1180 && count % 4 == 0) {
        sEnemyShotSet* set = new sEnemyShotSet;
        InitSet(set, 240.0, 185.0);
        set->patternFunc = ShotRuleBreak;

        double a = DX_PI * 2.0 * (count / 4) / 30.0;
        for (int i = 0; i < 30; ++i) {
            double aa = a + DX_PI * 2.0 * i / 30.0;
            AddShot(set, 240.0, 185.0, aa, 1.8 + 0.05 * (i % 5),
                img_enemyShotLargeBall[(i + count) % 7], 0);
        }
    }

    // 最後は「プレイヤーの腕とは無関係に」失格させる。
    if (count >= 1260) {
        player.x = 240.0;
        player.y = 240.0;
        enemy.hp = enemy.maxHp;
    }
}
