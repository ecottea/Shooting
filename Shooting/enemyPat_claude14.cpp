// enemyPat_poison.cpp
// モチーフ: 毒
//
// ┌──────────────────────────────────────────────────────┐
// │ パターン1  ShotPoisonFang  ── 毒の牙                 │
// │   プレイヤー方向を中心に ±18° で2本の"牙"を放つ。   │
// │   左牙=緑の鱗弾7発、右牙=マゼンタの鱗弾7発。        │
// │   各牙は速さが異なる(i=0:2.0 〜 i=6:4.4)ため、      │
// │   縦に伸びた牙の形になって飛来する。                 │
// │   30フレーム後から減速するため、広がりながら迫る。   │
// │                                                      │
// │ パターン2  ShotPoisonMist  ── 毒霧の輪               │
// │   15フレーム間隔で12発×5輪のリングを順次展開         │
// │   (合計60発)。各リングはDX_PI/12ずつ回転しており、  │
// │   重なって"毒の花"のような見た目になる。             │
// │   弾はサイン波でうねる軌道を描くため回避しにくい。   │
// │                                                      │
// │ 敵本体  EnemyPat_Poison_Claude                                 │
// │   x:360フレーム周期、y:180フレーム周期の             │
// │   リサジュー曲線(2:1)で画面上部を揺れる。           │
// │   毒霧の輪 : 120フレームごと                         │
// │   毒の牙   :  60フレームごと (offset 35)             │
// └──────────────────────────────────────────────────────┘

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>


// ----------------------------------------------------------------
//  パターン1: 毒の牙 (Venom Fang)
//
//  count == 0 のみ: 2本の牙(緑/マゼンタ)を各7発生成
//  毎フレーム    : 30フレーム超過後に速い弾ほど減速させ、
//                  牙の形を保ちながら扇が広がるように見せる
// ----------------------------------------------------------------
static void ShotPoisonFang(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int fang = 0; fang < 2; fang++) {
            // fang=0 : 左牙(緑)  /-18°  fang=1 : 右牙(マゼンタ)  /+18°
            double offset = (fang == 0) ? -DX_PI / 10.0 : DX_PI / 10.0;
            int    color = (fang == 0) ? 2 : 5;   // 2=緑, 5=マゼンタ

            const int N = 7;
            for (int i = 0; i < N; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // 牙の中心軸から ±0.12 rad の範囲に均等に並べる
                pEnemyShot->muki = pEnemyShotSet->muki + offset + (i - N / 2) * 0.04;
                // 速さを変えることで牙が縦に伸びた形になる
                pEnemyShot->speed = 2.0 + i * 0.4;  // i=0: 2.0, i=6: 4.4
                pEnemyShot->kind = img_enemyShotScale[color];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレーム: 移動 + 減速
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 30フレーム後から緩やかに減速 → 牙形状を保ちつつ扇が広がる
        if (pEnemyShot->count > 30 && pEnemyShot->speed > 1.5) {
            pEnemyShot->speed -= 0.03;
        }
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}


// ----------------------------------------------------------------
//  パターン2: 毒霧の輪 (Poison Mist Ring)
//
//  count  0/ 15/ 30/ 45/ 60 のとき1リングずつ展開(計5輪)
//  各リングは DX_PI/12 ずつ回転しており「毒の花」に見える
//  毎フレーム: サイン波で向きをうねらせ回避しにくくする
// ----------------------------------------------------------------
static void ShotPoisonMist(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    const int RINGS = 5;
    const int BULLETS_PER_RING = 12 * 2;
    const int RING_INTERVAL = 15;   // リング展開のフレーム間隔

    // 15フレームごとに1リング展開 (ring 0〜4 まで)
    if (pEnemyShotSet->count % RING_INTERVAL == 0) {
        int ring = pEnemyShotSet->count / RING_INTERVAL;
        if (ring < RINGS) {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            for (int i = 0; i < BULLETS_PER_RING; i++) {
                pEnemyShot = new sEnemyShot;

                // リングごとに DX_PI/12 (=15°) ずつ回転して花びら状に重ねる
                double angle = (2.0 * DX_PI * i / BULLETS_PER_RING)
                    + ring * (DX_PI / BULLETS_PER_RING);

                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = angle;
                // 外側のリングほど速く → 展開後も等間隔に近い距離を保つ
                pEnemyShot->speed = 0.8 + ring * 0.15;  // ring0: 0.80 〜 ring4: 1.40

                // 緑大玉・マゼンタ中玉・緑小玉を3連パターンで配置
                switch (i % 3) {
                case 0: pEnemyShot->kind = img_enemyShotLargeBall[2];  break;  // 緑大玉
                case 1: pEnemyShot->kind = img_enemyShotMediumBall[5]; break;  // マゼンタ中玉
                case 2: pEnemyShot->kind = img_enemyShotSmallBall[2];  break;  // 緑小玉
                }

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレーム: サイン波で向きをうねらせながら移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 周期 ≈ 79フレームのうねり、最大偏角 ≈ ±0.25 rad (≈14°)
        pEnemyShot->muki += sin(pEnemyShot->count * 0.08) * 0.02;
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}


// ----------------------------------------------------------------
//  敵本体: EnemyPat_Poison_Claude
//
//  動き   : x=360F・y=180F の 2:1 リサジュー曲線で上部を揺れる
//            x: 80〜400、y: 40〜80 の範囲に収まる
//  攻撃   : 毒霧の輪 … 120フレームごと (count%120==10)
//            毒の牙   …  60フレームごと (count%60 ==35)
//  ワープ防止: ローカル時間変数 t を用いて sin の位相を 0 から始める
// ----------------------------------------------------------------
void EnemyPat_Poison_Claude()
{
    static double t;   // 動き用ローカル時間 (count を直接 sin に渡すとワープする)

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        t = 0.0;
    }
    else {
        t += 1.0;
        // x: 360フレームで1往復 (80〜400)
        // y: 180フレームで1往復 (40〜80)  ← 2:1 比率でリサジュー軌跡
        enemy.x = 240.0 + 160.0 * sin(t * DX_PI / 180.0);
        enemy.y = 60.0 + 20.0 * sin(t * DX_PI / 90.0);
    }

    // 毒霧の輪: 120フレームごと発射
    if (count % 100 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPoisonMist;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = 0.0;   // 全方位展開のため実質未使用

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 毒の牙: 60フレームごと発射 (霧の合間に2連射のリズム)
    if (count % 50 == 35) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPoisonFang;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - (enemy.y + 10.0),
            player.x - enemy.x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}