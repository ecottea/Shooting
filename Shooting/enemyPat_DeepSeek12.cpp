// enemyPat_sun.cpp
// 太陽をモチーフにした弾幕パターン
// 
// 【利用できる素材一覧】
//   ◆ 画像 (img_enemyShot...)
//      - img_enemyShotSmallBall[color]   (小玉)
//      - img_enemyShotMediumBall[color]  (中玉)
//      - img_enemyShotLargeBall[color]   (大玉)
//      - img_enemyShotBullet[color]      (銃弾)
//      - img_enemyShotScale[color]       (鱗弾)
//      - img_enemyShotDiamond[color]     (菱形弾)
//      color は 0～7 (赤,黄,緑,シアン,青,マゼンタ,白,黒)
//   ◆ 効果音
//      - sound_enemyShot_light
//      - sound_enemyShot_medium
//      - sound_enemyShot_heavy
//      - sound_enemyShot_extreme
//   ◆ 関数
//      - GetRand(x)       0～x の整数を返す (x+1 通り)
//      - PlaySoundMem()
//      - atan2(), cos(), sin()
//   ◆ 定数
//      - DX_PI
//   ◆ グローバル変数
//      - enemy, player, enemyShotSetHead, count

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//------------------------------------------------
// パターン1: 太陽リング
//   敵の周囲に放射状の弾を放つ
//------------------------------------------------
static void SunRingPattern(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int   num = 16 * 3;   // リングの弾数
        const double baseAngle = pEnemyShotSet->muki; // 敵からプレイヤー方向

        for (int i = 0; i < num; ++i) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // リング状に等間隔で配置、さらにベース角度を加えて回転させる
            pEnemyShot->muki = baseAngle + (2.0 * DX_PI * i / num);
            pEnemyShot->speed = 2.5;  // ゆっくりめ

            // 色は太陽らしく黄～オレンジ系 (color=1(黄), 0(赤) など)
            int color = (i % 3 == 0) ? 0 : 1;        // 0:赤 1:黄
            int kind = (i % 4 == 0) ? img_enemyShotMediumBall[color]
                : img_enemyShotSmallBall[color];
            pEnemyShot->kind = kind;

            // 双方向リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存の弾の移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

//------------------------------------------------
// パターン2: 太陽フレア (プレイヤーに向けた扇状弾)
//------------------------------------------------
static void SunFlarePattern(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int   num = 12 * 3;
        const double centerAngle = pEnemyShotSet->muki;  // プレイヤー方向
        const double spread = (40.0 / 180.0) * DX_PI; // ±40度の拡がり

        for (int i = 0; i < num; ++i) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 扇状にばらつかせる
            double offset = spread * (2.0 * i / (num - 1) - 1.0); // -spread ～ +spread
            pEnemyShot->muki = centerAngle + offset;
            pEnemyShot->speed = (180.0 + GetRand(80)) / 100.0; // 1.8～2.6

            // 色は黄～オレンジ、たまに白
            int r = GetRand(3);
            int color = (r == 0) ? 0 : 1;   // 0:赤 1:黄
            if (r == 2) color = 6;          // 白
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

//------------------------------------------------
// 敵本体パターン
//------------------------------------------------
void EnemyPat_Sun_DeepSeek()
{
    static int muki;
    static int lastFlareCount;  // 前回フレアを発射した count を記録（必要に応じて）

    // 初回処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        lastFlareCount = 0;
    }
    else {
        // ゆっくり左右に動く
        enemy.x += 0.8 * (double)muki;
        if (count % 160 == 80) {
            muki *= -1;
        }
        // はみ出し防止（念のため）
        if (enemy.x < 80.0) {
            enemy.x = 80.0;
            muki = 1;
        }
        else if (enemy.x > 400.0) {
            enemy.x = 400.0;
            muki = -1;
        }
    }

    // リング弾幕 (一定間隔)
    if (count % 18 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = SunRingPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        // プレイヤー方向（リングの回転基準）
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // フレア弾幕 (一定間隔)
    if (count % 65 == 0 && count > 60) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = SunFlarePattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}