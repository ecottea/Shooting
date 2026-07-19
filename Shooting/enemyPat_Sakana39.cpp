// enemyPat_InvaderFormation.cpp
// インベーダー風フォーメーション弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：インベーダー風の横一列弾幕
static void ShotInvaderFormation(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ弾を生成
    if (pEnemyShotSet->count == 0) {
        // 効果音：中くらいの弾発射音を使う
        if (!CheckSoundMem(sound_enemyShot_medium))
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 弾幕の列数（インベーダーっぽさを出すため多め）
        const int COLUMN_COUNT = 11;

        for (int i = 0; i < COLUMN_COUNT; i++) {
            pEnemyShot = new sEnemyShot;

            // 横一列に並べる
            double offsetX = (i - (COLUMN_COUNT - 1) * 0.5) * 30.0; // 30ピクセル間隔
            pEnemyShot->x = pEnemyShotSet->x + offsetX;
            pEnemyShot->y = pEnemyShotSet->y;

            // 下方向にまっすぐ降りる
            pEnemyShot->muki = DX_PI / 2.0; // 90度（下向き）
            pEnemyShot->speed = 2.0;        // 速度はお好みで調整

            // 弾の種類：中玉（img_enemyShotMediumBall）
            // 弾の色：インベーダーっぽい緑・赤・青などを選ぶ
            // kind を 0〜7 で変えると色が変わる（0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒, 8:橙）
            int colorIndex = (i % 3) + 2; // 2:緑, 3:シアン, 4:青 のループ
            if (colorIndex > 4) colorIndex = 2; // 2〜4の範囲に収める
            pEnemyShot->kind = img_enemyShotMediumBall[colorIndex];

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Invader_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // インベーダーっぽく左右に揺れる
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で弾幕セットを生成
    if (count % 27 == 0) { // 60フレームごとに発射
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotInvaderFormation;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0; // 敵の少し下から発射
        pEnemyShotSet->muki = 0.0;         // 向きは弾幕側で設定するのでここでは0でOK
        pEnemyShotSet->kind = shot_count++;

        // 弾用のダミーヘッドを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾幕セットをグローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}