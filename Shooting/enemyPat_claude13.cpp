// enemyPat_tmp.cpp
// ============================================================
// はっぱカッター弾幕
// ============================================================
// 【弾幕の特徴】
//   img_enemyShotScale（鱗弾）を葉っぱに見立て、扇状に放つ。
//   発射直後は直進するが、30フレーム後から各葉がヒラヒラ揺れ始め、
//   120フレームで最大振れ幅に達する。各葉の揺れは初期方向を位相シード
//   にすることで扇全体がバラバラのタイミングで揺れる。
//   また25%の確率で「急所」の葉が混じり、やや速く飛ぶ。
//
// 【攻撃パターン】
//   通常（count % 90 == 0）    : 葉7枚  / 緑  / sound_medium
//   強化（count % 270 == 135） : 葉13枚 / 黄  / sound_heavy
//   ※ 90k と 270m+135 は重ならない（証明：90k≡135(mod 270)
//     → 2k≡3(mod 6) → 左辺偶数・右辺奇数で不成立）
//
// 【使用素材】
//   弾: img_enemyShotScale[2]（緑）, img_enemyShotScale[1]（黄）
//   SE: sound_enemyShot_medium, sound_enemyShot_heavy
// ============================================================

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// ShotHappaKutter
// 扇状に葉を放ち、一定フレーム後からヒラヒラ揺れる
//
// pEnemyShotSet->kind : 葉の枚数
//    7  = 通常はっぱカッター
//   13  = 強化はっぱカッター
// ------------------------------------------------------------
static void ShotHappaKutter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ---- 初回フレーム：葉を生成 --------------------------------
    if (pEnemyShotSet->count == 0) {
        if (pEnemyShotSet->kind >= 13) {
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        else {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        const int    numLeaves = pEnemyShotSet->kind;
        const double spreadAngle = 110.0 / 180.0 * DX_PI; // 扇の開き角 110°
        const double baseAngle = pEnemyShotSet->muki;

        for (int i = 0; i < numLeaves; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 扇の中に等間隔配置
            const double ratio = (numLeaves > 1) ? (double)i / (numLeaves - 1) : 0.5;
            pEnemyShot->muki = baseAngle - spreadAngle * 0.5 + spreadAngle * ratio;

            // GetRand(3)==0 は 1/4（25%）の確率 → 急所の葉はやや速い
            if (GetRand(3) == 0) {
                pEnemyShot->speed = 3.5 + GetRand(5) * 0.1; // 3.5〜4.0
            }
            else {
                pEnemyShot->speed = 2.2 + GetRand(6) * 0.1; // 2.2〜2.8
            }

            // 色：通常=緑、強化=黄（黄緑に見立てる）
            pEnemyShot->kind = (numLeaves >= 13) ? img_enemyShotScale[1]
                : img_enemyShotScale[2];

            // 循環リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---- 毎フレーム：各葉を移動 --------------------------------
    // ct <  30 : 直進（揺れなし）
    // ct < 120 : 揺れが徐々に大きくなる（線形ランプ）
    // ct >= 120: 最大振れ幅で揺れ続ける
    const double ct = (double)pEnemyShotSet->count;
    const double maxAmp = 1.5;  // 最大横速度 [px/frame]
    const double freq = 0.12; // 角周波数 [rad/frame]（約52フレームで1周期）

    double amp; double mul;
    if (ct < 30.0) {
        amp = 0.0;
        mul = 1.5;
    }
    else if (ct < 120.0) {
        amp = maxAmp * (ct - 30.0) / 90.0;
        mul = 1.5 - (ct - 30.0) / 90.0 / 2.0;
    }
    else {
        amp = maxAmp;
        mul = 1.0;
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 葉ごとに異なる位相を与える（初期方向 muki をシードに使用）
        // 隣接する葉の位相差は約 2.2 rad（≒128°）になり、
        // 扇全体がバラバラのタイミングで揺れるように見える
        const double phase = pEnemyShot->muki * 7.0;
        const double wobble = amp * sin(ct * freq + phase);

        // wobble を進行方向と垂直な成分として加算
        const double perp_x = -sin(pEnemyShot->muki);
        const double perp_y = cos(pEnemyShot->muki);

        pEnemyShot->x += mul * pEnemyShot->speed * cos(pEnemyShot->muki) + wobble * perp_x;
        pEnemyShot->y += mul * pEnemyShot->speed * sin(pEnemyShot->muki) + wobble * perp_y;

        pEnemyShot = pEnemyShot->next;
    }
}

// ------------------------------------------------------------
// EnemyPat_RazorLeaf_Claude
// 画面上部をリサージュ的に左右移動し、はっぱカッターを放つ
// ------------------------------------------------------------
void EnemyPat_RazorLeaf_Claude()
{
    static double moveAngle;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        moveAngle = 0.0;
    }
    else {
        // 横：約240フレーム（4秒@60fps）で1往復、可動域 x=[60, 420]
        moveAngle += DX_PI / 120.0;
        enemy.x = 240.0 + 180.0 * sin(moveAngle);
        // 縦：横の2倍速で小さく揺れる → 8の字に近いリサージュ軌跡
        enemy.y = 60.0 + 18.0 * sin(moveAngle * 2.0);
    }

    // 通常：90フレームごとに葉7枚
    if (count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHappaKutter;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 7; // 葉7枚

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 強化：270フレームごとに（90フレームずれた位相で）葉13枚
    if (count % 90 == 75) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHappaKutter;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 13; // 葉13枚

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
