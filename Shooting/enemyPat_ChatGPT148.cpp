#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 四天王奥義「三歩必殺」
//
// 三回の踏み込みを一つの弾幕サイクルとして再現する。
// 1歩目: 青系の中楕円弾で8本の弾幕の縄を作る。
// 2歩目: 1歩目の間を埋めるようにシアン系の菱形弾を追加する。
// 3歩目: それまでの縄から小玉を大量に崩し飛ばし、最後に大玉も放つ。
//
// count, pEnemyShotSet->count, pEnemyShot->count の更新と、
// 画面外の弾の削除はメインルーチン側で行う仕様に合わせ、ここでは変更しない。

static void AddSanpoShot(
    sEnemyShotSet* pEnemyShotSet,
    double x,
    double y,
    double muki,
    double speed,
    int kind,
    int role)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[0] = role;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static void StopAndPlaySanpoSound(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

static void SpawnSanpoRope(
    sEnemyShotSet* pEnemyShotSet,
    double angle,
    int kind,
    double speed,
    int role)
{
    // 根元から先端まで一直線に並べた弾で「縄」を作る。
    // 速度はわずかに変えて、時間経過で縄が自然に伸びるようにする。
    for (int i = 0; i < 20; i++) {
        const double radius = 18.0 + i * 8.0;
        const double bulletSpeed = speed + i * 0.015;
        const double x = pEnemyShotSet->x + cos(angle) * radius;
        const double y = pEnemyShotSet->y + sin(angle) * radius;
        AddSanpoShot(
            pEnemyShotSet,
            x,
            y,
            angle,
            bulletSpeed,
            kind,
            role);
    }
}

static void ShotSanpoHitsusatsu(sEnemyShotSet* pEnemyShotSet)
{
    const int N = 12;

    // 1歩目：8方向に青い弾の縄を展開。
    if (pEnemyShotSet->count == 0) {
        StopAndPlaySanpoSound(sound_enemyCharge);

        const double phase = pEnemyShotSet->muki;
        const bool alternate = (pEnemyShotSet->param_i[0] != 0);
        const int firstKind = alternate ? img_enemyShotMediumOval[0] : img_enemyShotMediumOval[4];
        for (int i = 0; i < N; i++) {
            const double angle = phase + DX_PI * 2.0 * i / N;
            SpawnSanpoRope(
                pEnemyShotSet,
                angle,
                firstKind,
                1.55,
                1);
        }
    }

    // 2歩目：8本の縄の中間へ、シアン色の菱形弾を追加。
    if (pEnemyShotSet->count == 45) {
        StopAndPlaySanpoSound(sound_enemyShot_heavy);

        const double phase = pEnemyShotSet->muki + DX_PI / N;
        const bool alternate = (pEnemyShotSet->param_i[0] != 0);
        const int secondKind = alternate ? img_enemyShotDiamond[5] : img_enemyShotDiamond[3];
        for (int i = 0; i < N; i++) {
            const double angle = phase + DX_PI * 2.0 * i / N;
            SpawnSanpoRope(
                pEnemyShotSet,
                angle,
                secondKind,
                1.80,
                2);
        }
    }

    // 3歩目：今までの縄を崩すように、各弾の位置から最終波を放つ。
    if (pEnemyShotSet->count == 90) {
        StopAndPlaySanpoSound(sound_enemyShot_extreme);

        const bool alternate = (pEnemyShotSet->param_i[0] != 0);
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;

            if (pShot->param_i[0] == 1 || pShot->param_i[0] == 2) {
                if (!alternate) {
                    // 青系の縄がほどけ、白・青の小玉が周囲へ散る。
                    const double spread = 0.20 + GetRand(50) / 100.0;
                    const double speedA = 2.10 + GetRand(100) / 100.0;
                    const double speedB = 2.10 + GetRand(100) / 100.0;

                    AddSanpoShot(
                        pEnemyShotSet,
                        pShot->x,
                        pShot->y,
                        pShot->muki + spread,
                        speedA,
                        img_enemyShotSmallBall[6],
                        0);
                    AddSanpoShot(
                        pEnemyShotSet,
                        pShot->x,
                        pShot->y,
                        pShot->muki - spread,
                        speedB,
                        img_enemyShotSmallBall[4],
                        0);
                }
                else if (pShot->param_i[0] == 2) {
                    // 赤紫の縄から、青い大玉がまとまって崩れ落ちる。
                    const double angle = pShot->muki + (GetRand(36) - 18) / 100.0;
                    const double speed = 1.90 + GetRand(80) / 100.0;
                    AddSanpoShot(
                        pEnemyShotSet,
                        pShot->x,
                        pShot->y,
                        angle,
                        speed,
                        img_enemyShotLargeBall[4],
                        0);
                }
            }

            pShot = pNext;
        }

        if (alternate) {
            // 最終段をさらに外向きへ押し広げ、画面全体を一気に埋める。
            const double phase = pEnemyShotSet->muki + DX_PI / 16.0;
            for (int i = 0; i < 16; i++) {
                const double angle = phase + DX_PI * 2.0 * i / 16.0;
                AddSanpoShot(
                    pEnemyShotSet,
                    pEnemyShotSet->x,
                    pEnemyShotSet->y,
                    angle,
                    2.25 + (i % 2) * 0.25,
                    img_enemyShotLargeBall[4],
                    0);
            }
        }
    }

    {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_SanpoHissatsu_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 勇儀が舞台上をゆっくり横切るイメージの移動。
        enemy.x += 0.72 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一つのセットの中に「三歩」をまとめ、周期的に繰り返す。
    if (count % 240 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSanpoHitsusatsu;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;

        // 周回ごとに縄の向きを少しずつ回し、同じ避け方の固定化を防ぐ。
        pEnemyShotSet->muki = (shot_count % 16) * DX_PI / 32.0;
        pEnemyShotSet->param_i[0] = shot_count % 2;
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
