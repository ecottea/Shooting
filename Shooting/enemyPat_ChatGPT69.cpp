// enemyPat_tmp.cpp
// スイカ連鎖フィーバー

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 果物の段階
// 0:さくらんぼ 1:いちご 2:ぶどう 3:みかん 4:りんご 5:メロン 6:スイカ
// ------------------------------------------------------------
enum {
    FRUIT_CHERRY = 0,
    FRUIT_STRAWBERRY,
    FRUIT_GRAPE,
    FRUIT_ORANGE,
    FRUIT_APPLE,
    FRUIT_MELON,
    FRUIT_WATERMELON,
    FRUIT_MAX
};

static const double FRUIT_RADIUS[FRUIT_MAX] = {
    8.0, 12.0, 16.0, 22.0, 30.0, 42.0, 56.0
};

// 色は img_enemyShotLargeBall / MediumBall / SmallBall のインデックス
static const int FRUIT_COLOR[FRUIT_MAX] = {
    0, // 赤
    8, // 橙
    5, // マゼンタ
    1, // 黄
    0, // 赤
    2, // 緑
    2  // 緑
};

static void ShotStraight(sEnemyShotSet* pEnemyShotSet)
{   
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}
static sEnemyShotSet* setStraight;

// ------------------------------------------------------------
// 見た目生成
// 大きさを自在に変えられないため、小・中・大玉を組み合わせる。
// ------------------------------------------------------------
static void SpawnFruitVisual(sEnemyShotSet* set, double x, double y, int stage)
{
    const int col = FRUIT_COLOR[stage];

    auto addShot = [&](double ox, double oy, int kind) {
        sEnemyShot* s = new sEnemyShot;
        s->x = x + ox;
        s->y = y + oy;
        s->speed = 0.0;
        s->muki = 0.0;
        s->kind = kind;

        s->prev = set->pEnemyShotHead->prev;
        s->next = set->pEnemyShotHead;
        set->pEnemyShotHead->prev->next = s;
        set->pEnemyShotHead->prev = s;
    };

    switch (stage) {
    case FRUIT_CHERRY:
        addShot(0, 0, img_enemyShotSmallBall[col]);
        break;

    case FRUIT_STRAWBERRY:
        addShot(0, 0, img_enemyShotMediumBall[col]);
        break;

    case FRUIT_GRAPE:
        addShot(-6, -4, img_enemyShotSmallBall[col]);
        addShot(6, -4, img_enemyShotSmallBall[col]);
        addShot(0, 6, img_enemyShotSmallBall[col]);
        break;

    case FRUIT_ORANGE:
        addShot(0, 0, img_enemyShotLargeBall[col]);
        break;

    case FRUIT_APPLE:
        addShot(-8, 0, img_enemyShotLargeBall[col]);
        addShot(8, 0, img_enemyShotLargeBall[col]);
        addShot(0, -8, img_enemyShotMediumBall[col]);
        break;

    case FRUIT_MELON:
        addShot(-10, -10, img_enemyShotLargeBall[col]);
        addShot(10, -10, img_enemyShotLargeBall[col]);
        addShot(-10, 10, img_enemyShotLargeBall[col]);
        addShot(10, 10, img_enemyShotLargeBall[col]);
        addShot(0, 0, img_enemyShotMediumBall[col]);
        break;

    case FRUIT_WATERMELON:
        addShot(-16, -16, img_enemyShotLargeBall[col]);
        addShot(16, -16, img_enemyShotLargeBall[col]);
        addShot(-16, 16, img_enemyShotLargeBall[col]);
        addShot(16, 16, img_enemyShotLargeBall[col]);
        addShot(0, -24, img_enemyShotLargeBall[col]);
        addShot(0, 24, img_enemyShotLargeBall[col]);
        addShot(-24, 0, img_enemyShotLargeBall[col]);
        addShot(24, 0, img_enemyShotLargeBall[col]);
        addShot(0, 0, img_enemyShotMediumBall[1]); // 黄で縞っぽく
        break;
    }
}

// ------------------------------------------------------------
// 合体衝撃波
// ------------------------------------------------------------
static void SpawnShockwave(sEnemyShotSet* set, double x, double y, int stage)
{
    if (CheckSoundMem(sound_enemyShot_medium))
        StopSoundMem(sound_enemyShot_medium);
    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

    const int num = 8 + stage * 3;
    const double speed = 1.6 + stage * 0.22;

    for (int i = 0; i < num; ++i) {
        sEnemyShot* s = new sEnemyShot;
        s->x = x;
        s->y = y;
        s->muki = DX_PI * 2.0 * i / num;
        s->speed = speed;
        s->kind = img_enemyShotDiamond[(stage + i) % 6];

        s->prev = set->pEnemyShotHead->prev;
        s->next = set->pEnemyShotHead;
        set->pEnemyShotHead->prev->next = s;
        set->pEnemyShotHead->prev = s;
    }
}

// ------------------------------------------------------------
// スイカ爆発
// ------------------------------------------------------------
static void SpawnWatermelonBurst(sEnemyShotSet* set, double x, double y)
{
    if (CheckSoundMem(sound_enemyShot_extreme))
        StopSoundMem(sound_enemyShot_extreme);
    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

    const int num = 72;

    for (int i = 0; i < num; ++i) {
        sEnemyShot* s = new sEnemyShot;
        s->x = x;
        s->y = y;
        s->muki = DX_PI * 2.0 * i / num;
        s->speed = 3.8 + (i % 4) * 0.28;
        s->kind = img_enemyShotBullet[(i % 2 == 0) ? 7 : 0]; // 黒と赤で種っぽく

        s->prev = set->pEnemyShotHead->prev;
        s->next = set->pEnemyShotHead;
        set->pEnemyShotHead->prev->next = s;
        set->pEnemyShotHead->prev = s;
    }
}

// ------------------------------------------------------------
// 果物弾幕メイン
// ------------------------------------------------------------
static void ShotSuikaChain(sEnemyShotSet* set)
{
    // 初回生成
    if (set->count == 0) {
        SpawnFruitVisual(set, set->x, set->y, set->kind);
        return;
    }

    // スイカの転がり後爆発
    if (set->kind == FRUIT_WATERMELON && set->param_i[1] && set->count > 180) {
        SpawnWatermelonBurst(setStraight, set->x, set->y);

        sEnemyShot* p = set->pEnemyShotHead->next;
        while (p != set->pEnemyShotHead) {
            p->x = -9999;
            p = p->next;
        }
        set->x = -9999;
        set->y = -9999;
        return;
    }

    // --------------------------------------------------------
    // 重力 + 転がり
    // --------------------------------------------------------
    set->param_d[1] += 0.055; // gravity
    if (set->param_d[1] > 2.2) set->param_d[1] = 2.2;

    set->y += set->param_d[1];
    set->x += set->param_d[0];

    const double r = FRUIT_RADIUS[set->kind];

    // 左右の壁で軽く跳ねる
    if (set->x < r) {
        set->x = r;
        set->param_d[0] = fabs(set->param_d[0]) * 0.85;
    }
    if (set->x > 480.0 - r) {
        set->x = 480.0 - r;
        set->param_d[0] = -fabs(set->param_d[0]) * 0.85;
    }

    // 床で跳ねる
    if (set->y > 470.0 - r) {
        set->y = 470.0 - r;
        set->param_d[1] *= -0.45;
        set->param_d[0] *= 0.96;
    }

    // --------------------------------------------------------
    // 見た目を追従
    // --------------------------------------------------------
    sEnemyShot* shot = set->pEnemyShotHead->next;
    while (shot != set->pEnemyShotHead) {
        shot->x += (set->x - shot->x) * 0.28;
        shot->y += (set->y - shot->y) * 0.28;
        shot = shot->next;
    }

    // --------------------------------------------------------
    // 合体判定（同じ段階のみ）
    // --------------------------------------------------------
    if (set->param_i[7]) return; // 既に合体予約済み

    sEnemyShotSet* other = enemyShotSetHead.next;
    while (other != &enemyShotSetHead) {
        if (other != set &&
            other->patternFunc == ShotSuikaChain &&
            other->kind == set->kind &&
            !other->param_i[7]) {

            const double dx = other->x - set->x;
            const double dy = other->y - set->y;
            const double rr = FRUIT_RADIUS[set->kind] + FRUIT_RADIUS[other->kind];

            if (dx * dx + dy * dy < rr * rr) {
                const double mx = (set->x + other->x) * 0.5;
                const double my = (set->y + other->y) * 0.5;

                set->param_i[7] = 1;
                other->param_i[7] = 1;

                SpawnShockwave(setStraight, mx, my, set->kind);

                // 元の果物を消す
                auto eraseVisual = [](sEnemyShotSet* s) {
                    sEnemyShot* p = s->pEnemyShotHead->next;
                    while (p != s->pEnemyShotHead) {
                        p->x = -9999;
                        p = p->next;
                    }
                    s->x = -9999;
                    s->y = -9999;
                };

                eraseVisual(set);
                eraseVisual(other);

                // 次段階生成
                const int nextStage = set->kind + 1;
                if (nextStage < FRUIT_MAX) {
                    sEnemyShotSet* n = new sEnemyShotSet;
                    n->x = mx;
                    n->y = my;
                    n->kind = nextStage;
                    n->patternFunc = ShotSuikaChain;
                    n->count = 0;

                    // 合体時の勢い
                    n->param_d[0] = (GetRand(200) - 100) / 120.0;
                    n->param_d[1] = -1.2;

                    if (nextStage == FRUIT_WATERMELON) {
                        n->param_i[1] = 1; // 爆発フラグ
                        n->param_d[0] *= 0.5;
                    }

                    n->pEnemyShotHead = new sEnemyShot;
                    n->pEnemyShotHead->prev = n->pEnemyShotHead;
                    n->pEnemyShotHead->next = n->pEnemyShotHead;

                    n->prev = enemyShotSetHead.prev;
                    n->next = &enemyShotSetHead;
                    enemyShotSetHead.prev->next = n;
                    enemyShotSetHead.prev = n;
                }
                return;
            }
        }
        other = other->next;
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_SuikaGame_ChatGPT()
{
    static int moveDir;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 56.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;

        setStraight = new sEnemyShotSet;
        setStraight->patternFunc = ShotStraight;
        setStraight->alive = 99999;

        setStraight->pEnemyShotHead = new sEnemyShot;
        setStraight->pEnemyShotHead->prev = setStraight->pEnemyShotHead;
        setStraight->pEnemyShotHead->next = setStraight->pEnemyShotHead;

        setStraight->prev = enemyShotSetHead.prev;
        setStraight->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = setStraight;
        enemyShotSetHead.prev = setStraight;
    }
    else {
        // 上部をゆっくり左右移動
        enemy.x += moveDir * 1.15;
        if (enemy.x < 72.0) {
            enemy.x = 72.0;
            moveDir = 1;
        }
        if (enemy.x > 408.0) {
            enemy.x = 408.0;
            moveDir = -1;
        }
    }

    // --------------------------------------------------------
    // 果物を落とす
    // --------------------------------------------------------
    if (count % 25 == 1) {
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = ShotSuikaChain;

        // ボス位置から少しばらけて落とす
        set->x = enemy.x + GetRand(160) - 80;
        if (set->x < 32.0) set->x = 32.0;
        if (set->x > 448.0) set->x = 448.0;

        set->y = enemy.y + 14.0;

        // 小さい果物が多めに出現
        const int r = GetRand(99);
        if (r < 50)      set->kind = FRUIT_CHERRY;
        else if (r < 78) set->kind = FRUIT_STRAWBERRY;
        else if (r < 92) set->kind = FRUIT_GRAPE;
        else              set->kind = FRUIT_ORANGE;

        // 初速度（左右に少し流れる）
        set->param_d[0] = (GetRand(200) - 100) / 180.0;
        set->param_d[1] = 0.0;

        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }

    // --------------------------------------------------------
    // フィーバー予告：周期的に高速落下を混ぜる
    // --------------------------------------------------------
    if (count % 300 == 240) {
        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (count % 300 >= 240 && count % 20 == 0) {
        for (int i = 0; i < 3; ++i) {
            sEnemyShotSet* set = new sEnemyShotSet;
            set->count = 0;
            set->patternFunc = ShotSuikaChain;
            set->kind = FRUIT_GRAPE + (i % 2);

            set->x = 120.0 + i * 120.0 + GetRand(40) - 20;
            set->y = enemy.y + 8.0;

            set->param_d[0] = (GetRand(200) - 100) / 120.0;
            set->param_d[1] = 5.0; // 速めに落下

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
