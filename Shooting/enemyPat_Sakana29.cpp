// enemyPat_Tmp.cpp
// シャボン玉をモチーフにした反射・分裂弾幕の実装例

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  使用する素材の一覧（enemyPat_sampleForAI.cpp より）
// ============================================================
// 弾の種類（画像）:
//   img_enemyShotSmallBall[i]   : 小玉
//   img_enemyShotMediumBall[i]  : 中玉
//   img_enemyShotLargeBall[i]   : 大玉
//   img_enemyShotBullet[i]      : 銃弾
//   img_enemyShotScale[i]       : 鱗弾
//   img_enemyShotDiamond[i]     : 菱形弾
//
// 弾の色（i=0..8）:
//   0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒, 8:橙
//
// 効果音:
//   sound_enemyShot_light   : 軽い弾
//   sound_enemyShot_medium  : 中程度の弾
//   sound_enemyShot_heavy   : 重い弾
//   sound_enemyShot_extreme : 極端に重い弾

// ============================================================
//  弾幕パターン：シャボン玉反射・分裂弾幕
// ============================================================
static void ShotBubbleReflectSplit(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;

    // 初回のみセットアップ（反射回数などの初期化）
    if (pEnemyShotSet->count == 0) {
        // 効果音: 中程度の弾音を再生
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 弾の初期設定
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            // 反射回数を param_i[0] に保持（初期値 0）
            pEnemyShot->param_i[0] = 0;
            pEnemyShot = pEnemyShot->next;
        }
    }

    // 弾の移動と反射・分裂処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 移動
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 画面端での反射
        // 画面サイズ: 480x480 と仮定
        if (pEnemyShot->x <= 0.0 || pEnemyShot->x >= 480.0 ||
            pEnemyShot->y <= 0.0 || pEnemyShot->y >= 480.0)
        {
            // 反射回数を増やす
            pEnemyShot->param_i[0]++;

            // 反射時の向き変更（単純に反転）
            if (pEnemyShot->x <= 0.0 || pEnemyShot->x >= 480.0) {
                pEnemyShot->muki = DX_PI - pEnemyShot->muki; // 左右反転
            }
            else {
                pEnemyShot->muki = -pEnemyShot->muki;       // 上下反転
            }

            // 画面外に出ないように位置を補正
            if (pEnemyShot->x < 0.0) pEnemyShot->x = 0.0;
            if (pEnemyShot->x > 480.0) pEnemyShot->x = 480.0;
            if (pEnemyShot->y < 0.0) pEnemyShot->y = 0.0;
            if (pEnemyShot->y > 480.0) pEnemyShot->y = 480.0;

            // 反射回数が N 回に達したら分裂
            const int N = 3; // 3回反射で分裂
            if (pEnemyShot->param_i[0] >= N) {
                // 分裂用の新しい弾を生成
                sEnemyShot* pNewShot = new sEnemyShot;
                *pNewShot = *pEnemyShot; // 既存の弾をコピー

                // 向きをランダムに少しずらす（±30度程度）
                double angleOffset = (GetRand(60) - 30) / 180.0 * DX_PI;
                pNewShot->muki += angleOffset;

                // 反射回数をリセット
                pNewShot->param_i[0] = 0;

                // リストに挿入
                pNewShot->prev = pEnemyShot->prev;
                pNewShot->next = pEnemyShot;
                pEnemyShot->prev->next = pNewShot;
                pEnemyShot->prev = pNewShot;

                // 元の弾も反射回数をリセット
                pEnemyShot->param_i[0] = 0;
            }
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体のパターン関数（指定された関数名）
// ============================================================
void EnemyPat_SoapBubbles_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵の左右移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で弾幕セットを生成
    if (count % 40 == 0) { // 30フレームごとに発射
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBubbleReflectSplit;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        // 弾リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾を数発生成
        const int BUBBLE_COUNT = 5; // 一度に出すシャボン玉の数
        for (int i = 0; i < BUBBLE_COUNT; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // プレイヤー方向を基準に少しばらつかせる
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = (100 + GetRand(100)) / 100.0; // 1.0～2.0 程度

            // 弾の種類と色: 中玉（シャボン玉感）＋青系の色
            // kind は色インデックス付きの画像ハンドル
            // 例: img_enemyShotMediumBall[4] で「中玉・青」
            pEnemyShot->kind = img_enemyShotMediumBall[4]; // 4:青

            // リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 弾幕セットをグローバルリストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}