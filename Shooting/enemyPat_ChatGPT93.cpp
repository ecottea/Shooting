#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 信号機弾幕
// 赤 : 停止信号。巨大な赤玉を中心に、円形の「停止線」が広がる。
// 黄 : 注意信号。交差する黄色の流れが左右に揺れ、安全地帯が移動する。
// 青 : 進行信号。上から大量の青弾が車列のように流れ、通行帯が蛇行する。
// 信号機本体は大玉・中玉・黒玉で組み立てる。
// ============================================================

static void AddShot(sEnemyShotSet* set, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = set->kind;

    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;
}

// ------------------------------------------------------------
// 信号機本体
// ------------------------------------------------------------
static void ShotTrafficLight(sEnemyShotSet* set)
{
    if (set->count == 0) {
        // 筐体
        const double cx = 240.0;
        const double cy = 92.0;
        const double lx = 240.0;

        // 大玉3個で赤・黄・青のランプを構成
        AddShot(set, lx, cy - 40.0, 0.0, 0.0, img_enemyShotLargeBall[7]);
        AddShot(set, lx, cy, 0.0, 0.0, img_enemyShotLargeBall[7]);
        AddShot(set, lx, cy + 40.0, 0.0, 0.0, img_enemyShotLargeBall[7]);

        // 周囲を中玉で囲って信号機らしい輪郭を作る
        for (int i = 0; i < 8*2; ++i) {
            double a = DX_PI * 2.0 * i / 8.0 / 2;
            AddShot(set,
                cx + 30.0 * cos(a),
                cy + 64.0 * sin(a),
                a, 0.0, img_enemyShotMediumBall[7]);
        }
    }

    // 信号の切り替え。一定時間ごとに赤→黄→青を循環。
    int phase = (set->count / 180) % 3;
    sEnemyShot* p = set->pEnemyShotHead->next;
    int idx = 0;
    while (p != set->pEnemyShotHead) {
        if (idx < 3) {
            int color = 7; // 消灯＝黒
            if (phase == 0 && idx == 0) color = 0;       // 赤
            if (phase == 1 && idx == 1) color = 1;       // 黄
            if (phase == 2 && idx == 2) color = 2;       // 緑
            p->kind = img_enemyShotLargeBall[color];
        }
        ++idx;
        p = p->next;
    }
}

// ------------------------------------------------------------
// 赤信号 : 停止線
// 複数の円環を少しずつずらして発生させ、画面中央の安全地帯を
// 時間とともに狭める。終盤はリングを回転させて出口をずらす。
// ------------------------------------------------------------
static void ShotTrafficRed(sEnemyShotSet* set)
{
    if (set->count == 0) {
        StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    if (set->count < 115 && set->count % 10 == 0) {
        double cx = 240.0 + 62.0 * sin(0.018 * set->count);
        double cy = 275.0;
        double rot = 0.22 * sin(0.025 * set->count);

        for (int i = 0; i < 24; ++i) {
            double a = DX_PI * 2.0 * i / 24.0 + rot;
            AddShot(set, cx, cy, a, 1.65 + 0.12 * (i % 3), img_enemyShotSmallBall[0]);
        }
    }

    sEnemyShot* p = set->pEnemyShotHead->next;
    while (p != set->pEnemyShotHead) {
        // 通常は放射状、後半になるほど外向きに加速して速やかに画面外へ。
        double extra = (set->count > 118) ? 2.5 : 0.0;
        p->x += cos(p->muki) * (p->speed + extra);
        p->y += sin(p->muki) * (p->speed + extra);
        p = p->next;
    }
}

// ------------------------------------------------------------
// 黄信号 : 交差する注意流
// 左右から押し寄せる黄色い流れが正弦波で蛇行し、通れる場所が
// 徐々に反対側へ移動する。白弾を混ぜて車線境界を表現する。
// ------------------------------------------------------------
static void ShotTrafficYellow(sEnemyShotSet* set)
{
    if (set->count == 0) {
        StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (set->count < 135 && set->count % 4 == 0) {
        double s = set->count * 0.045;
        double bend = 0.85 * sin(s);

        for (int side = 0; side < 2; ++side) {
            double y = 110.0 + (set->count % 60) * 4.8;
            double x = (side == 0) ? -12.0 : 492.0;
            double a = (side == 0 ? 0.0 : DX_PI);
            a += 0.42 * sin(0.05 * set->count + side * DX_PI) + bend * 0.08;

            for (int j = 0; j < 4; ++j) {
                double yy = y + j * 13.0;
                AddShot(set, x, yy, a, 1.9 + 0.08 * j, img_enemyShotSmallBall[1]);
            }
        }

        // 交差点の車線境界
        if (set->count % 12 == 0) {
            double x = 240.0 + 92.0 * sin(0.042 * set->count);
            for (int j = 0; j < 7; ++j) {
                double yy = 65.0 + j * 58.0;
                AddShot(set, x, yy, DX_PI * 0.5, 1.35, img_enemyShotDiamond[6]);
            }
        }
    }

    sEnemyShot* p = set->pEnemyShotHead->next;
    while (p != set->pEnemyShotHead) {
        p->x += cos(p->muki) * p->speed;
        p->y += sin(p->muki) * p->speed;
        p = p->next;
    }
}

// ------------------------------------------------------------
// 青信号 : 大量の車列
// 上方から複数車線の青弾が降り、各車線が左右へ蛇行する。
// プレイヤー方向へ向かう白弾を少量混ぜて、車線変更を強制する。
// ------------------------------------------------------------
static void ShotTrafficGreen(sEnemyShotSet* set)
{
    if (set->count == 0) {
        StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    if (set->count < 145 && set->count % 3 == 0) {
        int lane = (set->count / 3) % 9;
        double baseX = 48.0 + lane * 48.0;
        double x = baseX + 28.0 * sin(0.035 * set->count + lane * 0.7);

        // 車列本体
        AddShot(set, x, -10.0, DX_PI * 0.5, 1.55 + 0.08 * (lane % 4), img_enemyShotSmallBall[2]);
        AddShot(set, x + 10.0 * sin(lane), -28.0, DX_PI * 0.5, 1.75, img_enemyShotSmallBall[2]);

        // 一定間隔で車線変更を促す大型弾
        if (lane == 4 && set->count % 15 == 0) {
            double a = atan2(player.y - 42.0, player.x - 240.0);
            AddShot(set, 240.0, 42.0, a, 2.15, img_enemyShotBullet[6]);
            set->pEnemyShotHead->prev->param_i[1] = 1;
        }
    }

    // 終盤は蛇行を強めて「進行方向が急に変わる」区間を作る。
    sEnemyShot* p = set->pEnemyShotHead->next;
    while (p != set->pEnemyShotHead) {
        double wave = 0.0;
        if (set->count > 70)
            wave = 0.26 * sin(0.04 * p->count + 0.015 * set->count);

        if (p->speed < 0.0) {
            // 終了処理用の特殊状態は使わないので通常どおり進行。
        }

        // 小玉の車列は上から下へ。弾ごとに少しだけ蛇行。
        p->muki = DX_PI * 0.5 + wave;
        p->x += cos(p->muki) * p->speed;
        p->y += sin(p->muki) * p->speed;
        p = p->next;
    }
}

// ============================================================
// 敵本体
// ============================================================
void EnemyPat_TrafficLight_ChatGPT()
{
    static int moveDir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 48.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;

        // 信号機本体を一つだけ作る。
        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = ShotTrafficLight;
        set->x = 240.0;
        set->y = 92.0;
        set->muki = 0.0;
        set->kind = 0;
        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;
        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }
    {
        enemy.x += 0.65 * moveDir;
        if (enemy.x < 120.0 || enemy.x > 360.0)
            moveDir *= -1;

        // 画面上部で左右に動くことで、自機の移動先にも圧をかける。
        if (count % 180 == 1) {
            int phase = ((count - 1) / 180) % 3;
            sEnemyShotSet* set = new sEnemyShotSet;
            set->count = 0;
            set->x = enemy.x;
            set->y = enemy.y + 12.0;
            set->muki = atan2(player.y - set->y, player.x - set->x);
            set->kind = phase;

            if (phase == 0) set->patternFunc = ShotTrafficRed;
            if (phase == 1) set->patternFunc = ShotTrafficYellow;
            if (phase == 2) set->patternFunc = ShotTrafficGreen;

            set->pEnemyShotHead = new sEnemyShot;
            set->pEnemyShotHead->prev = set->pEnemyShotHead;
            set->pEnemyShotHead->next = set->pEnemyShotHead;
            set->prev = enemyShotSetHead.prev;
            set->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = set;
            enemyShotSetHead.prev = set;
        }
    }
}
