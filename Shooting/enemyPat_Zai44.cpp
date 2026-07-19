// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ホーミング・イリュージョン（擬態する羅針盤）
// 直進しているのに、描画される向きだけが常に自機を指し続ける弾幕
static void ShotHomingIllusion(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 連発音に合わせて軽い音を採用
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 発射時の自機方向（ベース角度）
        double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        int shot_num = 5; // 1セットの弾数
        double angle_step = DX_PI / 8.0; // 5度刻みの扇状

        for (int i = 0; i < shot_num; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double shot_muki = base_muki + (i - shot_num / 2) * angle_step;

            // 描画用の向き（初期値は発射方向と同じ）
            pEnemyShot->muki = shot_muki;

            // 実際の移動方向をパラメータに保存して直進させる
            pEnemyShot->param_d[0] = shot_muki;

            pEnemyShot->speed = 3.0;

            // 素材選定：矢印の先端として視認性が高く、回転錯覚が際立つ「菱形弾(4.5 x 2.5)」
            // 色は警告色である「赤(0)」を採用（配列インデックスは0〜7が安全なため）
            pEnemyShot->kind = img_enemyShotLaser[0];

            pEnemyShot->margin = 100;

            // 双方向リストへの追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動と錯覚処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 保存しておいた初期方向へ直進させる（ actual movement ）
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->param_d[0]);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->param_d[0]);

        // 【重要】描画用の向きだけを毎フレーム現在の自機方向に向ける（ illusion ）
        // これにより、直進弾であるにもかかわらずホーミングしているように見える
        pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Arrow_Zai()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 15フレームごとに発射
    if (count % 50 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHomingIllusion;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 弾セットの循環リスト用ダミーノード生成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルの弾セットリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}