#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ガブリエルのラッパをモチーフにした立体感のある弾幕
// 外側（手前）は大きな弾で疎、内側（奥）は小さな弾で密・高速
// 半径を 1/(depth) 的に減少させ、奥行きのある漏斗状のトンネルを形成する
static void ShotGabrielHorn(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 重い弾幕なので heavy を使用
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 12フレームごとに1層のリングを生成（最大12層でラッパ形状を完成）
    // ring=0 が最も外側（手前・大・遅）、ringが大きくなるほど内側（奥・小・速・密）
    if (pEnemyShotSet->count % 18 == 0) {
        int ring = pEnemyShotSet->count / 18;
        const int N = 15;
        if (ring < N) {
            // ガブリエルのラッパ風：半径を 1/(1 + k*scale) で減少
            double radius = 170.0 / (1.0 + ring * 0.28) * 1.3;

            // 内側ほど弾数を増やして密度を上げる（表面積無限のイメージ）
            int num = 14 + ring * 2;

            // 奥ほど速くして遠近感を強調
            double speed = 1.6 + ring * 0.22;

            // わずかな螺旋回転で立体感を付加
            double twist = ring * 0.18;

            // 色はシアン（3）を基調に、奥は少し青寄りに変化させても良いが統一で視認性優先
            int color = 3; // シアン

            // リングの深さに応じて弾種を切り替え（大→中→小→針状）
            // 使える素材: LargeBall, MediumBall, SmallBall, MediumOval, Diamond, Bullet, Scale
            for (int i = 0; i < num; i++) {
                double ang = pEnemyShotSet->muki + twist + (2.0 * DX_PI * i) / num;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + radius * cos(ang);
                pEnemyShot->y = pEnemyShotSet->y + radius * sin(ang);

                // ほぼ同じ方向に飛ばして「トンネルを覗き込む」立体感を出す
                // わずかに外側に開く成分を加えてラッパの広がりを表現
                double open = 0.04 * (N - 1 - ring); // 外側ほど少し開く
                pEnemyShot->muki = pEnemyShotSet->muki + open * sin(ang - pEnemyShotSet->muki);
                pEnemyShot->speed = speed;

                // 弾種選択（既存素材のみ）
                if (ring <= 1) {
                    // 最外層：大玉で「手前の大きな開口部」
                    pEnemyShot->kind = img_enemyShotLargeBall[color];
                }
                else if (ring <= 4) {
                    // 中間：中玉 or 中楕円
                    if (ring % 2 == 0)
                        pEnemyShot->kind = img_enemyShotMediumBall[color];
                    else
                        pEnemyShot->kind = img_enemyShotMediumOval[color];
                }
                else if (ring <= 8) {
                    // 内側：小玉・鱗弾で密度アップ
                    if (ring % 2 == 0)
                        pEnemyShot->kind = img_enemyShotSmallBall[color];
                    else
                        pEnemyShot->kind = img_enemyShotScale[color];
                }
                else {
                    // 最奥：菱形・銃弾で鋭く収束する先端
                    if (ring % 2 == 0)
                        pEnemyShot->kind = img_enemyShotDiamond[color];
                    else
                        pEnemyShot->kind = img_enemyShotBullet[color];
                }

                // リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 全弾の移動（メインルーチンでcount++と画面外消去が行われる）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_GabrielsHorn_Grok()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // 緩やかな左右移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 80.0 || enemy.x > 400.0) muki *= -1;
        if (count % 180 == 90) muki *= -1;
    }

    // 約2秒に1回、新しいラッパ弾幕セットを生成
    if (count % 300 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGabrielHorn;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        // プレイヤー方向を基準にトンネルを向ける
        pEnemyShotSet->muki = DX_PI / 2;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}