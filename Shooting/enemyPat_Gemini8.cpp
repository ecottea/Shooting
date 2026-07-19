// enemyPat_lightning.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================================
// 弾制御関数：稲妻弾（一瞬でジグザグに配置され、高速直進しつつ軌跡に火花を残す）
// ============================================================================
static void ShotLightning(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 最初のフレーム（count == 0）で、稲妻の形状に弾を一挙に生成・配置する
    if (pEnemyShotSet->count == 0) {
        // 雷鳴をイメージした重い発射音を再生
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double curX = pEnemyShotSet->x;
        double curY = pEnemyShotSet->y;
        double base_muki = pEnemyShotSet->muki; // 弾幕が全体として進むベースの向き

        // ジグザグに折れ曲がる最初の方向（1.0:右傾き / -1.0:左傾き）をランダム決定
        // GetRand(1) は 0 または 1 を返すため、50%の確率で分岐
        double sign = (GetRand(1) == 0) ? 1.0 : -1.0;

        // 稲妻の折れ曲がりセグメント数（6〜7箇所に屈折させる）
        int numSegments = 6 + GetRand(1);

        for (int seg = 0; seg < numSegments; seg++) {
            // ベース方向から「何度傾けてカクつかせるか」を決定（30度〜45度の範囲でランダム）
            double angleOffset = (30 + GetRand(15)) / 180.0 * DX_PI;
            double seg_muki = base_muki + (sign * angleOffset);

            // 1つの直線セグメントあたりに配置する弾の数（4〜5発）
            int bulletsInSeg = 4 + GetRand(1);

            for (int b = 0; b < bulletsInSeg; b++) {

                // ─── 1. 稲妻の本体を構成する高速弾 ───
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = curX;
                pEnemyShot->y = curY;
                // 稲妻のジグザグ形状を保ったまま直進させるため、移動向きは base_muki に固定
                pEnemyShot->muki = base_muki;
                // 雷らしく超高速（5.0 〜 6.0 の間でランダム）
                pEnemyShot->speed = 5.0 + (GetRand(100) / 100.0);

                // バチバチした光を表現するため、黄色(1)、白(6)、シアン(3)をランダムに混色
                int colorRand = GetRand(2); // 0, 1, 2
                int color = 1; // デフォルトは黄色
                if (colorRand == 1)      color = 6; // 白色
                else if (colorRand == 2) color = 3; // シアン

                // 稲妻のシャープなイメージに合う「菱形弾」を使用
                pEnemyShot->kind = img_enemyShotDiamond[color];

                // 双方向リストへ挿入
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;


                // ─── 2. 稲妻の軌跡に残る電撃の火花（残光演出） ───
                // 約3分の1の確率で、その場に飛び散る火花を生成
                if (GetRand(2) == 0) {
                    sEnemyShot* pSpark = new sEnemyShot;
                    pSpark->x = curX + (GetRand(6) - 3); // 配置をわずかに散らす
                    pSpark->y = curY + (GetRand(6) - 3);
                    pSpark->muki = (GetRand(360) / 180.0) * DX_PI; // 361方向（全方位）ランダム
                    pSpark->speed = 0.2 + (GetRand(60) / 100.0);   // 0.2 〜 0.8 の超低速

                    // 火花は黄色か白色の「小玉」
                    int sparkColor = (GetRand(1) == 0) ? 1 : 6;
                    pSpark->kind = img_enemyShotSmallBall[sparkColor];

                    // 双方向リストへ挿入
                    pSpark->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pSpark->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pSpark;
                    pEnemyShotSet->pEnemyShotHead->prev = pSpark;
                }

                // 次の弾の配置座標を、セグメントの傾き方向に進める
                // 弾同士の間隔を 6.5px と狭くすることで、一本の太い光線（レーザー）のように見せます
                double step = 6.5;
                curX += step * cos(seg_muki);
                curY += step * sin(seg_muki);
            }

            // 次のセグメントへ進む際、折れ曲がる向きを反転させる（ジグザグにする）
            sign *= -1.0;
        }
    }

    // 全ての所属弾（本体高速弾・残光火花弾）の毎フレーム移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================================
// 敵本体の行動パターン関数
// ============================================================================
void EnemyPat_Thunder_Gemini()
{
    static int muki;

    // カウント1（初期化フレーム）：ボスの初期位置とHPを設定
    if (count == 1) {
        // ゲーム画面仕様 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 雷ボス想定の耐久力
        muki = 1;
    }
    else {
        // 敵本体の移動：画面上部をゆったりと左右に往復移動
        enemy.x += 0.5 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // ────────────────────────────────────────
    // 【落雷パターン①】35フレーム周期：敵本体から自機を狙う高速稲妻
    // ────────────────────────────────────────
    if (count % 35 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLightning;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // 現在の自機座標に向けたベース角度を計算
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 弾セット内のダミーヘッド初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 管理用グローバルリストの末尾へ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ────────────────────────────────────────
    // 【落雷パターン②】90フレーム周期：画面上空のランダム位置から真下への落雷（2本同時）
    // ────────────────────────────────────────
    if (count % 60 == 0) {
        for (int i = 0; i < 2; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotLightning;

            // 画面の端すぎない範囲（40px 〜 440px）でランダムなX座標に落とす
            pEnemyShotSet->x = 40.0 + GetRand(400);
            pEnemyShotSet->y = 0.0;            // 画面の一番上から
            pEnemyShotSet->muki = DX_PI / 2.0; // 真下（90度）の方向へ直進させる

            // 弾セット内のダミーヘッド初期化
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            // 管理用グローバルリストの末尾へ追加
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}