#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：ガブリエルのラッパ
//  同心円リングを連続生成し、弾種・色・半径で疑似遠近法を表現。
//  各リングは一定時間その場で"呼吸"するように揺れたのち、
//  半径方向(muki)へ加速しながら吹き出す。最後にラッパの先端が
//  一斉放射バーストを吹き鳴らす。
// ============================================================
namespace HornPat {
    const int    NUM_RINGS = 16;    // リング総数
    const int    SPAWN_INTERVAL = 6;     // リング生成間隔(フレーム)
    const int    SHOTS_PER_RING = 30;    // 1リングあたりの弾数
    const double R_MAX = 210.0; // 最大リング半径(ring 0 = 手前)
    const double R_DECAY = 0.32;  // 半径減衰係数(奥に行くほど半径が縮む)
    const double TWIST_STEP = 14.0 * DX_PI / 180.0; // リングごとのねじれ角
    const int    HOLD_FRAMES = 100;   // 静止(呼吸)保持フレーム数
    const double BREATHE_AMP = 6.0;   // 呼吸振幅
    const double BREATHE_FREQ = 0.05;  // 呼吸周波数
    const double ACCEL = 0.045; // 保持後、放出フェーズでの加速度
}

// 弾幕：ラッパの壁面を成すリング一層分
static void ShotHornRing(sEnemyShotSet* pEnemyShotSet)
{
    using namespace HornPat;
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int ringIndex = pEnemyShotSet->kind; // リング番号(0=手前・大、奥に行くほど番号増加)
        double t = (double)ringIndex / (double)(NUM_RINGS - 1); // 0(手前)〜1(奥)
        double baseR = R_MAX / (1.0 + ringIndex * R_DECAY);     // ガブリエルの角笛 1/x カーブの離散近似
        double twist = ringIndex * TWIST_STEP;                  // らせん状のねじれで筒の立体感を強調

        // 色：手前は暖色、奥に行くほど寒色にして奥行きを表現（赤→橙→黄→緑→シアン→青）
        int colorOrder[6] = { 0, 8, 1, 2, 3, 4 };
        int colorIdx = colorOrder[(int)(t * 5.999)];

        // 弾種：手前=大玉、中間=中玉、奥=小玉 で疑似遠近法を表現
        int shapeGroup;
        if (t < 0.35)      shapeGroup = 2; // 大玉
        else if (t < 0.70) shapeGroup = 1; // 中玉
        else               shapeGroup = 0; // 小玉

        for (int i = 0; i < SHOTS_PER_RING; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = twist + (2.0 * DX_PI * i) / SHOTS_PER_RING;

            pEnemyShot->x = pEnemyShotSet->x + baseR * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + baseR * sin(angle);
            pEnemyShot->muki = angle; // 壁面の法線(半径)方向を向かせる。速度方向ではなく形状の向き
            pEnemyShot->speed = 0.0;  // 保持フェーズは静止から始まる

            // ring0(ベル最外周)は6発に1発を短レーザーにしてベルの縁のきらめきを表現
            // 奇数リングは4発に1発を鱗弾にして角笛表面の筋(テクスチャ)を表現
            bool useLaser = (ringIndex == 0 && (i % 6 == 0));
            bool useScale = (!useLaser && (ringIndex % 2 == 1) && (i % 4 == 0));

            if (useLaser) {
                pEnemyShot->kind = img_enemyShotLaser[colorIdx];
            }
            else if (useScale) {
                pEnemyShot->kind = img_enemyShotScale[colorIdx];
            }
            else {
                switch (shapeGroup) {
                case 0: pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];  break;
                case 1: pEnemyShot->kind = img_enemyShotMediumBall[colorIdx]; break;
                case 2: pEnemyShot->kind = img_enemyShotLargeBall[colorIdx];  break;
                }
            }

            // param_d: [0]=中心x [1]=中心y [2]=角度 [3]=基準半径 [4]=放出フェーズでの現在速度
            pEnemyShot->param_d[0] = pEnemyShotSet->x;
            pEnemyShot->param_d[1] = pEnemyShotSet->y;
            pEnemyShot->param_d[2] = angle;
            pEnemyShot->param_d[3] = baseR;
            pEnemyShot->param_d[4] = 0.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double cx = pShot->param_d[0];
        double cy = pShot->param_d[1];
        double angle = pShot->param_d[2];
        double baseR = pShot->param_d[3];

        if (pShot->count < HOLD_FRAMES) {
            // 保持フェーズ：半径を正弦波でわずかに揺らし、角笛の壁面が呼吸するように見せる
            double r = baseR + BREATHE_AMP * sin(pShot->count * BREATHE_FREQ + angle);
            pShot->x = cx + r * cos(angle);
            pShot->y = cy + r * sin(angle);
        }
        else {
            // 放出フェーズ：muki(半径方向)へ加速しながら吹き出す
            pShot->param_d[4] += HornPat::ACCEL;
            pShot->speed = pShot->param_d[4];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 弾幕：ラッパの先端(収束点)が吹き鳴らす一斉放射バースト
static void ShotHornBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const int BURST_NUM = 40*3;
        for (int i = 0; i < BURST_NUM; i++) {
            pEnemyShot = new sEnemyShot;

            // GetRand(x) は 0〜x の x+1 種類を返すので、揺らぎ幅を100分割してから中心を引く
            double jitter = (GetRand(100) - 50) / 100.0 * (DX_PI / BURST_NUM);
            double angle = (2.0 * DX_PI * i) / BURST_NUM + jitter;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 3.0 + GetRand(300) / 100.0;

            // 赤/橙のみで統一感を出しつつ、5発に1発を銃弾にして緩急をつける
            int colorIdx = (GetRand(1) == 0) ? 0 : 8;
            if (i % 5 == 0) {
                pEnemyShot->kind = img_enemyShotBullet[colorIdx];
                pEnemyShot->speed += 2.0;
            }
            else {
                pEnemyShot->kind = img_enemyShotDiamond[colorIdx];
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン：ガブリエルのラッパ
void EnemyPat_GabrielsHorn_Claude()
{
    using namespace HornPat;
    static int ringIndex;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 200;
        ringIndex = 0;
    }

    // NUM_RINGS個のリングを SPAWN_INTERVAL フレームごとに順番に発生させる
    if (ringIndex < NUM_RINGS && count % SPAWN_INTERVAL == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHornRing;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->kind = ringIndex; // リング番号を格納

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        ringIndex++;

        // 最終リングを撒いた直後、ラッパの先端(=収束点)が吹き鳴らすバーストを追加発生
        if (ringIndex == NUM_RINGS) {
            sEnemyShotSet* pBurstSet = new sEnemyShotSet;
            pBurstSet->count = 0;
            pBurstSet->patternFunc = ShotHornBurst;
            pBurstSet->x = enemy.x;
            pBurstSet->y = enemy.y;

            pBurstSet->pEnemyShotHead = new sEnemyShot;
            pBurstSet->pEnemyShotHead->prev = pBurstSet->pEnemyShotHead;
            pBurstSet->pEnemyShotHead->next = pBurstSet->pEnemyShotHead;

            pBurstSet->prev = enemyShotSetHead.prev;
            pBurstSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pBurstSet;
            enemyShotSetHead.prev = pBurstSet;
        }
    }

    if (count % 300 == 0) {
        ringIndex = 0;
    }
}