// enemyPat_tmp.cpp
// 紅葉をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ======================================================
// 弾幕A：落ち葉
//   大玉・菱形弾（赤/黄）を 12 発、扇状（下向き ±75 度）に発射。
//   最初の 25 フレームは発射方向に直進し、
//   その後は muki を揺れ位相に流用してサイン波で左右に漂いながら落下。
// ======================================================
static void ShotFallingLeaves(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int LEAF_COUNT = 12;
        for (int i = 0; i < LEAF_COUNT; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(1) ? 1 : -1) * (180 + GetRand(20));  // ±10px のばらつき
            pEnemyShot->y = pEnemyShotSet->y;

            // 扇状（真下 = DX_PI/2 を中心に ±75 度）
            double spread = (-75.0 + (double)i / (LEAF_COUNT - 1) * 150.0) * DX_PI / 180.0;
            pEnemyShot->muki = DX_PI / 2.0 + spread;        // 発射角（後でフェーズ2の揺れ位相に流用）
            pEnemyShot->speed = (90 + GetRand(60)) / 100.0;  // 0.90〜1.50

            // 秋色：赤(0) か 黄(1)、形：大玉 か 菱形弾をランダムに選択
            int color = GetRand(1);
            pEnemyShot->kind = (GetRand(1) == 0)
                ? img_enemyShotLargeBall[color]
                : img_enemyShotDiamond[color];
            pEnemyShot->margin = 200;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        int c = pEnemyShot->count;
        if (c < 25) {
            // フェーズ1：発射方向に直進
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        else {
            // フェーズ2：muki を揺れ位相として流用し、左右に漂いながら落下
            //   sin の周期は約 90 フレーム（≒ 1.5 秒）
            double t = (double)(c - 25);
            pEnemyShot->x += 1.5 * sin(t * 0.07 + pEnemyShot->muki) * 0.3;
            pEnemyShot->y += 0.8 * pEnemyShot->speed + t * 0.018;  // 緩やかな加速落下
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// ======================================================
// 弾幕B：紅葉渦
//   中玉（赤・黄を 1 リングごとに交互）を 2 重リング × 18 発で
//   全方向に発射。最初の 40 フレームはゆっくり広がり、
//   その後は muki を揺れ位相に流用して重力落下する。
// ======================================================
static void ShotMomijiWhirl(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int RING_COUNT = 2;
        const int SHOTS_PER_RING = 24;

        for (int ring = 0; ring < RING_COUNT; ring++) {
            for (int i = 0; i < SHOTS_PER_RING; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                // 2 重リングを互い違いに配置（ring ごとに半ピッチずらす）
                double angle = (double)i / SHOTS_PER_RING * 2.0 * DX_PI
                    + (double)ring * DX_PI / SHOTS_PER_RING;
                pEnemyShot->muki = angle;                      // 発射角（後でフェーズ2の揺れ位相に流用）
                pEnemyShot->speed = (40 + ring * 30) / 100.0;  // 内リング 0.40 / 外リング 0.70

                // 赤(0) と 黄(1) を 1 リングごとに交互
                int color = (ring % 2 == 0) ? 0 : 1;
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                pEnemyShot->margin = 200;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        int c = pEnemyShot->count;
        if (c < 40) {
            // フェーズ1：全方向にゆっくり広がる
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki) * 5;
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki) * 5;
        }
        else {
            // フェーズ2：muki を揺れ位相として流用し、重力に引かれて落下
            //   sin の周期は約 125 フレーム（≒ 2.1 秒）
            double t = (double)(c - 40);
            pEnemyShot->x += 1.2 * sin(t * 0.05 + pEnemyShot->muki);
            pEnemyShot->y += 0.6 + t * 0.02;  // 加速度的な落下
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// ======================================================
// 敵本体：EnemyPat_Maple_Claude
//   画面上部 (x=240, y=60) から出現し、ゆっくり左右に往復する。
//   ・35 フレームごとに弾幕A「落ち葉」を発射
//   ・120 フレームごとに弾幕B「紅葉渦」を発射
// ======================================================
void EnemyPat_Maple_Claude()
{
    static int muki;
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.7 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 弾幕A：落ち葉（35 フレームごと）
    if (count % 35 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFallingLeaves;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 弾幕B：紅葉渦（120 フレームごと）
    if (count % 100 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMomijiWhirl;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}