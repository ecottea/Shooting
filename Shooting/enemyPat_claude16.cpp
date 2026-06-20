// enemyPat_igo.cpp
// 囲碁をモチーフにした弾幕パターン
//
// 副パターン一覧:
//   ShotFuseki   - 布石: 5×5 市松模様の大玉を列ごとに時間差で打ち出す
//   ShotGoishi   - 碁石の波: 19路盤にちなんだ 19 発一斉放射
//   ShotShicho   - シチョウ(梯子): 斜め方向へ放った中玉が 30f ごとに直角旋回
//
// 敵本体:
//   EnemyPat_Go_Claude - 碁打ち師。ゆっくり左右移動しながら
//                  180f サイクルで 3 種を 60f 間隔で順番に繰り出す

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕1: 布石（ふせき）
//   碁盤目状 5×5 を列ごとに 8f 間隔でプレイヤー方向へ打ち出す。
//   市松模様(黒=7, 白=6)の大玉で「石を並べる」ビジュアルを表現。
//   列ごとに速度差を付けることで、斜め飛翔の弾幕壁を形成する。
// ============================================================
static void ShotFuseki(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 8フレームごとに 1 列ずつ発射（col: 0〜4）
    int col = pEnemyShotSet->count / 8;
    if (col < 5 && pEnemyShotSet->count % 8 == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int row = 0; row < 5; row++) {
            pEnemyShot = new sEnemyShot;
            // 格子間隔 40px・中央揃え (col-2, row-2) で展開
            pEnemyShot->x = pEnemyShotSet->x + (col - 2) * 40.0;
            pEnemyShot->y = pEnemyShotSet->y + (row - 2) * 40.0;
            pEnemyShot->muki = pEnemyShotSet->muki;          // プレイヤー方向
            pEnemyShot->speed = 1.4 + row * 0.25;             // 行で速度差
            // 市松模様: (col+row) が偶数なら黒(7)・奇数なら白(6)
            int color = ((col + row) % 2 == 0) ? 7 : 6;
            pEnemyShot->kind = img_enemyShotLargeBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 弾幕2: 碁石の波（ごいし の なみ）
//   19路盤にちなんで 19 発の黒白交互の大玉を全方向に一斉放射。
//   「盤上の石が一気に飛び出す」ビジュアルを表現。
// ============================================================
static void ShotGoishi(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        const int N = 19;  // 19路盤の「路」の数
        for (int i = 0; i < N; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 2.0 * DX_PI * i / N;  // 全方向に均等配置
            pEnemyShot->speed = 2.5;
            // 黒(7)・白(6)を交互に
            int color = (i % 2 == 0) ? 7 : 6;
            pEnemyShot->kind = img_enemyShotLargeBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 弾幕3: シチョウ（梯子）
//   4斜め方向(45/135/225/315度)へ中玉を 3 速度分放ち、
//   30フレームごとに右 90度旋回させる。
//   格子に沿った四角渦巻き軌跡が「シチョウの読み筋」を表現。
//   速度の違いで入れ子の大小四角形が生まれ、視覚的に豊かになる。
// ============================================================
static void ShotShicho(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        // 4方向 × 3速度 = 12 発
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 7; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // 斜め 4 方向: 45°/ 135°/ 225°/ 315°
                pEnemyShot->muki = DX_PI / 4.0 + DX_PI / 2.0 * i;
                // 速度 1.5 / 2.5 / 3.5 → 1辺 45/75/105px の四角形を描く
                pEnemyShot->speed = 1.5 + j * 1.0;
                // 向きで黒白を分ける (i=0,2 が黒 / i=1,3 が白)
                int color = (i % 2 == 0) ? 7 : 6;
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                pEnemyShot->margin = 999;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 全弾移動 + 30f ごとの直角旋回
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // count==0 は除外し、30f 周期で右 90度旋回（格子追跡）
        if (pEnemyShot->count > 0 && pEnemyShot->count % 30 == 0 && pEnemyShot->count <= 150) {
            pEnemyShot->muki += DX_PI / 2.0;
        }
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体: 碁打ち師（ごうちし）
//   ゆっくり左右に動きながら、180f サイクルで 3 種の弾幕を
//   60f 間隔で順に繰り出す:
//     count % 180 ==  10 → 布石（ShotFuseki）
//     count % 180 ==  70 → 碁石の波（ShotGoishi）
//     count % 180 == 130 → シチョウ（ShotShicho）
// ============================================================
void EnemyPat_Go_Claude()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 囲碁らしくゆったり左右移動（150f 周期で折り返し）
        enemy.x += 0.7 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // ------- 布石 (Fuseki) -------
    if (count % 180 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFuseki;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ------- 碁石の波 (Goishi no Nami) -------
    if (count % 180 == 70) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGoishi;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - enemy.y,
            player.x - enemy.x);
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ------- シチョウ (Shicho) -------
    if (count % 180 == 130) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShicho;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - enemy.y,
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