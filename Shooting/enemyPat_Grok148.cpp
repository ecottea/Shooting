#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  四天王奥義「三歩必殺」 (Shitennou Ougi "Sanpo Hissatsu")
//  星熊勇儀の最終スペルを再現した弾幕パターン
//
//  概要:
//    勇儀が足を3回踏み鳴らすごとに弾幕を段階的に展開する。
//    1歩目: 内側に高密度の静止弾
//    2歩目: 中間リングにやや疎な静止弾
//    3歩目: 外側を高密度で埋め尽くす（ここが「必殺」）
//    その後、全弾が外側へ向かって一斉に飛び出す。
//
//  使用素材:
//    - 弾種: 菱形弾(結晶弾風), 鱗弾, 小玉, 中玉, 大玉
//    - 色  : 青/シアン/白 (奇数波), 赤/マゼンタ/橙 (偶数波)
//    - 音  : sound_enemyCharge (予告), sound_enemyShot_medium/heavy
// ============================================================

// 1波分の弾幕を管理するパターン関数
static void ShotSanpoHissatsu(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double cx = pEnemyShotSet->x;
    const double cy = pEnemyShotSet->y;
    const int wave = pEnemyShotSet->kind;          // 0始まりの波番号
    const bool isOdd = (wave % 2 == 0);            // 最初の波を奇数扱い

    // -------------------------------------------------------
    //  1歩目: 内側高密度静止弾 (count == 0)
    // -------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 内側リング (半径 35〜55)
        for (int layer = 0; layer < 3+1; layer++) {
            double radius = 35.0 + layer * 10.0;
            int num = 18*2 + layer * 6*2;               // 18, 24, 30
            for (int i = 0; i < num; i++) {
                double ang = (DX_PI * 2.0 * i) / num + (layer * 0.15);
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx + radius * cos(ang);
                pEnemyShot->y = cy + radius * sin(ang);
                pEnemyShot->muki = ang;             // 外側へ飛ぶ向きを予め設定
                pEnemyShot->speed = 0.0;            // 最初は静止
                pEnemyShot->param_i[0] = 1;         // フェーズフラグ (後で使用)

                // 色と弾種
                if (isOdd) {
                    // 青系
                    int col = (layer == 0) ? 4 : 3; // 青 / シアン
                    pEnemyShot->kind = img_enemyShotDiamond[col];
                }
                else {
                    // 赤系
                    int col = (layer == 0) ? 0 : 5; // 赤 / マゼンタ
                    pEnemyShot->kind = img_enemyShotDiamond[col];
                }

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // -------------------------------------------------------
    //  2歩目: 中間リング (count == 45)
    // -------------------------------------------------------
    else if (pEnemyShotSet->count == 45) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 中間リング (半径 90〜120)
        for (int layer = 0; layer < 2+1; layer++) {
            double radius = 90.0 + layer * 20.0;
            int num = 16*2 + layer * 8*2;               // 16, 24
            for (int i = 0; i < num; i++) {
                double ang = (DX_PI * 2.0 * i) / num + 0.08 * layer;
                // 少しバラけさせる
                ang += (GetRand(20) - 10) * 0.01;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx + radius * cos(ang);
                pEnemyShot->y = cy + radius * sin(ang);
                pEnemyShot->muki = ang;
                pEnemyShot->speed = 0.0;
                pEnemyShot->param_i[0] = 2;

                if (isOdd) {
                    pEnemyShot->kind = img_enemyShotScale[3];   // シアン鱗弾
                }
                else {
                    pEnemyShot->kind = img_enemyShotScale[5];   // マゼンタ鱗弾
                }

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // -------------------------------------------------------
    //  3歩目: 外側高密度埋め尽くし (count == 90)  ← ここが必殺
    // -------------------------------------------------------
    else if (pEnemyShotSet->count == 90) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 外側を広範囲に埋める (半径 140〜210)
        for (int layer = 0; layer < 4+1; layer++) {
            double radius = 140.0 + layer * 22.0;
            int num = 22*2 + layer * 6*2;               // 22〜40
            for (int i = 0; i < num; i++) {
                double ang = (DX_PI * 2.0 * i) / num + layer * 0.07;
                // ランダム性を加えて「埋め尽くし」感を出す
                double r = radius + (GetRand(16) - 8);
                ang += (GetRand(30) - 15) * 0.008;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx + r * cos(ang);
                pEnemyShot->y = cy + r * sin(ang);
                pEnemyShot->muki = ang;
                pEnemyShot->speed = 0.0;
                pEnemyShot->param_i[0] = 3;

                if (isOdd) {
                    // 奇数波: 白小玉 + 少し中玉
                    if (layer >= 2 && (i % 5 == 0)) {
                        pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白中玉
                    }
                    else {
                        pEnemyShot->kind = img_enemyShotSmallBall[6];   // 白小玉
                    }
                }
                else {
                    // 偶数波: 赤系大玉多め（画面が真っ白になるイメージ）
                    if (layer >= 1 && (i % 4 == 0)) {
                        pEnemyShot->kind = img_enemyShotLargeBall[0];   // 赤大玉
                    }
                    else if (i % 3 == 0) {
                        pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙中玉
                    }
                    else {
                        pEnemyShot->kind = img_enemyShotSmallBall[0];   // 赤小玉
                    }
                }

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // -------------------------------------------------------
    //  弾の移動処理
    //  count 100 以降で全弾に速度を与えて外側へ飛ばす
    // -------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 3歩目展開後、少し間を置いて一斉に動き出す
        if (pEnemyShotSet->count >= 110 && pShot->speed == 0.0) {
            // 外側ほど少し速く
            double baseSpeed = 1.4;
            if (pShot->param_i[0] == 1) baseSpeed = 1.6;
            else if (pShot->param_i[0] == 2) baseSpeed = 1.5;
            else baseSpeed = 1.3 + GetRand(40) / 100.0;

            pShot->speed = baseSpeed;
        }

        // 速度が付いている弾だけ動かす
        if (pShot->speed > 0.0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体パターン
//  関数名は指定通り void EnemyPat_SanpoHissatsu_Grok()
// ============================================================
void EnemyPat_SanpoHissatsu_Grok()
{
    static int waveCount = 0;       // 波の通し番号
    static int nextWaveFrame = 0;   // 次の波を出すフレーム

    if (count == 1) {
        // ゲーム画面は 480x480。中央やや上に配置
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;   // 200から変更しないこと
        waveCount = 0;
        nextWaveFrame = 30;             // 最初の波までの待機
    }

    // 勇儀はほぼ動かない（僅かに揺れる程度）
    // 足踏み感を出すためにごく小さく上下
    enemy.y = 240.0 + 1.5 * sin(count * 0.08);

    // 一定間隔で波を発生させる
    if (count >= nextWaveFrame) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSanpoHissatsu;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = waveCount;    // 波番号を渡す（奇偶で色を変える）
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        waveCount++;
        // 次の波までの間隔（弾が飛び終わってから少し空ける）
        // 3歩 + 移動時間を考慮して約 220 フレーム
        nextWaveFrame = count + 220;
    }
}