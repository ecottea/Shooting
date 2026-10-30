#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// スプラトゥーン風弾幕：ナワバリバトル ～カラフル・インクウォーズ～
// 専用のインク素材は使わず、赤色・青色の敵弾をインクに見立てる。
// count / sEnemyShotSet::count / sEnemyShot::count の更新や、画面外の弾の削除はメインルーチン側で行う。

static void InkWarPlaySound(int sound)
{
    if (CheckSoundMem(sound)) StopSoundMem(sound);
    PlaySoundMem(sound, DX_PLAYTYPE_BACK);
}

static void InkWarAddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind, double turnSpeed = 0.0)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->param_d[0] = turnSpeed;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

static void InkWarMoveStraight(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 広い扇状に小玉を散らし、ところどころに中玉を混ぜてインクの飛沫を表現。
// kind: 0 = 赤、1 = 青
static void InkWarPatternSpray(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int color = (pSet->kind == 0) ? 0 : 4;
        const int bulletCount = 19;
        for (int i = 0; i < bulletCount; ++i) {
            const double offset = -0.78 + 1.56 * i / (bulletCount - 1);
            const int bulletKind = (i % 6 == 0)
                ? img_enemyShotMediumBall[color]
                : img_enemyShotSmallBall[color];
            const double speed = 2.0 + (i % 5) * 0.16;
            InkWarAddShot(pSet, pSet->x, pSet->y,
                pSet->muki + offset, speed, bulletKind);
        }
    }
    InkWarMoveStraight(pSet);
}

// プレイヤー方向へ、細い銃弾と丸い弾を混ぜた狙い撃ちの連射。
static void InkWarPatternAimBurst(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int color = (pSet->kind == 0) ? 0 : 4;
        for (int i = 0; i < 7; ++i) {
            const double offset = (i - 3) * 0.052;
            const int bulletKind = (i % 2 == 0)
                ? img_enemyShotMediumBall[color]
                : img_enemyShotBullet[color];
            const double speed = (i == 3) ? 4.0 : 3.0 + (i % 3) * 0.18;
            InkWarAddShot(pSet, pSet->x, pSet->y,
                pSet->muki + offset, speed, bulletKind);
        }
    }
    InkWarMoveStraight(pSet);
}

// 赤と青の扇を逆方向に少しずつ回転させ、終盤の交差弾幕を作る。
// kind: 0 = 赤（時計回り）、1 = 青（反時計回り）
static void InkWarPatternCrossSweep(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int color = (pSet->kind == 0) ? 0 : 4;
        const double turnSpeed = (pSet->kind == 0) ? -0.0048 : 0.0048;
        const int bulletCount = 15;
        const double colorOffset = (pSet->kind == 0) ? -0.12 : 0.12;

        for (int i = 0; i < bulletCount; ++i) {
            const double offset = -0.58 + 1.16 * i / (bulletCount - 1);
            const int bulletKind = (i % 7 == 0)
                ? img_enemyShotMediumBall[color]
                : img_enemyShotSmallBall[color];
            const double speed = 2.15 + (i % 4) * 0.13;
            InkWarAddShot(pSet, pSet->x, pSet->y,
                pSet->muki + colorOffset + offset, speed, bulletKind, turnSpeed);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->muki += pShot->param_d[0];
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

static void InkWarCreateShotSet(sEnemyShotSet::PatternFunc patternFunc,
    double x, double y, double muki, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = patternFunc;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

static void InkWarFireSpray(int color)
{
    const double x = enemy.x;
    const double y = enemy.y + 10.0;
    const double aim = atan2(player.y - y, player.x - x);
    InkWarCreateShotSet(InkWarPatternSpray, x, y, aim, color);
}

static void InkWarFireAimBurst(int color)
{
    const double x = enemy.x;
    const double y = enemy.y + 10.0;
    const double aim = atan2(player.y - y, player.x - x);
    InkWarCreateShotSet(InkWarPatternAimBurst, x, y, aim, color);
}

static void InkWarFireCrossSweepPair(void)
{
    const double x = enemy.x;
    const double y = enemy.y + 10.0;
    const double aim = atan2(player.y - y, player.x - x);
    InkWarCreateShotSet(InkWarPatternCrossSweep, x, y, aim, 0);
    InkWarCreateShotSet(InkWarPatternCrossSweep, x, y, aim, 1);
}

// 敵本体の弾幕パターン
void EnemyPat_Splatoon_ChatGPT()
{
    static int moveDirection;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 44.0;
        enemy.maxHp = enemy.hp = 200; // 変更しないこと
        moveDirection = 1;
    }
    else {
        const int phase = count % 480;
        const double moveSpeed = (phase >= 300) ? 1.25 : 0.92;
        enemy.x += moveSpeed * moveDirection;
        if (enemy.x < 55.0) {
            enemy.x = 55.0;
            moveDirection = 1;
        }
        else if (enemy.x > 425.0) {
            enemy.x = 425.0;
            moveDirection = -1;
        }
        enemy.y = 44.0 + 8.0 * sin(count * 0.025);
    }

    const int phase = count % 480;

    // 警告音の後、赤と青の回転扇弾幕へ移行。
    if (phase == 286) {
        InkWarPlaySound(sound_enemyCharge);
    }

    // 前半：色を交互に切り替えながら、広い扇状の飛沫を撒く。
    if (phase < 150 && count % 24 == 1) {
        const int color = ((phase / 24) % 2 == 0) ? 0 : 1;
        InkWarFireSpray(color);
        InkWarPlaySound(sound_enemyShot_light);
    }
    // 中盤：プレイヤーを狙う連射で、塗り合いの圧力を強める。
    else if (phase >= 150 && phase < 286 && phase % 30 == 5) {
        const int color = ((phase / 30) % 2 == 0) ? 0 : 1;
        InkWarFireAimBurst(color);
        InkWarPlaySound(sound_enemyShot_medium);
    }
    // 終盤：赤と青の回転扇を同時に放ち、交差する弾幕を展開。
    else if (phase >= 300 && (phase - 300) % 12 == 0) {
        InkWarFireCrossSweepPair();
        if (phase == 300) {
            InkWarPlaySound(sound_enemyShot_extreme);
        }
        else if ((phase - 300) % 48 == 0) {
            InkWarPlaySound(sound_enemyShot_heavy);
        }
    }
}
