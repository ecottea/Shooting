// enemyPat_mirageSandstorm.cpp
// 大砂嵐モチーフ弾幕「蜃気楼砂嵐（ミラージュ・サンドストーム）」
//
// 【使用素材の選定意図】
// 弾の種類：
//   ・小玉(img_enemyShotSmallBall)   : 砂粒・熱波の粒子表現に最適（2.5x2.5）
//   ・中玉(img_enemyShotMediumBall)   : 風穴の渦巻き表現（7.0x7.0）
//   ・大玉(img_enemyShotLargeBall)   : 風圧弾の重厚感（20.0x20.0）
//   ・銃弾(img_enemyShotBullet)      : 蜃気楼の高速弾・方向識別用（5.0x2.0）
//   ・鱗弾(img_enemyShotScale)       : 大砂嵐の砂塵っぽい質感（4.0x3.0）
//   ・中楕円弾(img_enemyShotMediumOval): 風の流れを表現（10.5x7.0）
// 弾の色：
//   ・黄(1) / 橙(8) : 砂・熱・砂嵐のイメージ
//   ・シアン(3) / 青(4) : 渦・風の冷たさ
//   ・白(6)         : 風圧・蜃気楼の閃光
//   ・黒(7)         : 視界不良・砂嵐の暗がり
//   ・マゼンタ(5)   : 幻影の不気味さ
// 効果音：
//   ・sound_enemyShot_light   : 熱波の砂粒（軽い連射）
//   ・sound_enemyShot_medium  : 渦巻き生成音
//   ・sound_enemyShot_heavy   : 風圧弾の重低音
//   ・sound_enemyShot_extreme : 蜃気楼の不気味な発射音
//   ・sound_enemyCharge      : 大砂嵐の予告・安全地帯出現を音で示唆

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  フェーズ1「熱波」
//  画面全体に砂粒のような小弾が降り注ぐ。
//  軌道をsin波でわずかに揺らし、熱ゆらぎの錯覚を演出。
// ============================================================
static void ShotHeatWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 画面上方から砂粒を12発生成
        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = (double)GetRand(480);
            pEnemyShot->y = -5.0;
            // 下方向を中心に±30度のばらつき
            pEnemyShot->muki = DX_PI / 2.0 + (double)(GetRand(60) - 30) / 180.0 * DX_PI;
            pEnemyShot->speed = (80.0 + (double)GetRand(120)) / 100.0;

            // 黄(1) or 橙(8)
            int color = (GetRand(1) == 0) ? 1 : 8;
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            // param_d[0] : ゆらぎ用の位相
            pEnemyShot->param_d[0] = (double)GetRand(360) / 180.0 * DX_PI;
            pEnemyShot->margin = 5.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        // 熱ゆらぎ：軌道をわずかに横揺れさせる
        pEnemyShot->x += sin(pEnemyShot->count * 0.05 + pEnemyShot->param_d[0]) * 0.4;

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  フェーズ2「風穴」渦巻き
//  画面中央から渦巻き状に中玉を放射。回転速度が加速する。
// ============================================================
static void ShotVortex(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int num = 16;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = (2.0 * DX_PI / (double)num) * (double)i;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 1.0;

            // シアン(3) or 青(4)
            pEnemyShot->kind = img_enemyShotMediumBall[3 + (i % 2)];

            // param_d[0] : 現在角度, param_d[1] : 角速度
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = 0.02 + (double)(i % 3) * 0.005;
            pEnemyShot->margin = 7.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 渦を加速しながら広がる
        pEnemyShot->param_d[0] += pEnemyShot->param_d[1];
        pEnemyShot->speed += 0.015;
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->param_d[0]);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->param_d[0]);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  フェーズ2「風圧弾」
//  画面四隅から中央へ向かう大玉・中楕円弾。
//  param_i[0] = 1 で「風圧弾」マーカー（メインルーチン側で
//  当たり判定時のプレイヤースライド処理等に利用可能）。
// ============================================================
static void ShotWindPressure(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            // 上下左右から画面外へ配置
            switch (i) {
            case 0: pEnemyShot->x = -20.0;                    pEnemyShot->y = (double)GetRand(480); break;
            case 1: pEnemyShot->x = (double)GetRand(480);     pEnemyShot->y = -20.0;                  break;
            case 2: pEnemyShot->x = 500.0;                    pEnemyShot->y = (double)GetRand(480); break;
            case 3: pEnemyShot->x = (double)GetRand(480);     pEnemyShot->y = 500.0;                  break;
            }
            pEnemyShot->muki = atan2(240.0 - pEnemyShot->y, 240.0 - pEnemyShot->x);
            pEnemyShot->speed = 2.5;

            // 白(6)の大玉 or 中楕円弾
            pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotLargeBall[6] : img_enemyShotMediumOval[6];

            // 風圧弾マーカー
            pEnemyShot->param_i[0] = 1;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  フェーズ3「蜃気楼」
//  3方向から弾を発射。2方向は幻影（真っ直ぐ）、1方向は本体（風で軌道が曲がる）。
//  軌道の微差を観察することで本体を識別する意図。
// ============================================================
static void ShotMirage(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const int dir = 3;   // 3方向
        const int way = 5;   // 各方向5way
        for (int i = 0; i < dir; i++) {
            for (int j = 0; j < way; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                double baseAngle = (2.0 * DX_PI / (double)dir) * (double)i + (double)(GetRand(20) - 10) / 180.0 * DX_PI;
                pEnemyShot->muki = baseAngle;
                pEnemyShot->speed = 1.5 + (double)j * 0.3;

                // マゼンタ(5) or 橙(8)
                pEnemyShot->kind = img_enemyShotBullet[5 + (i % 2)];

                // i==0 の方向のみが「本体」（風で軌道が曲がる）
                pEnemyShot->param_i[0] = (i == 0) ? 1 : 0;
                pEnemyShot->param_d[0] = baseAngle; // 曲がり用角度保持

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == 1) {
            // 本体：風で軌道が右に流され、ゆるやかに曲がる
            pEnemyShot->param_d[0] += 0.003;
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->param_d[0]);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->param_d[0]);
            pEnemyShot->x += 0.25; // 風による横スライド
        }
        else {
            // 幻影：真っ直ぐ進む
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  フェーズ4「大砂嵐」
//  ランダムな鱗弾（砂塵）が全画面を覆う。
//  中央(240,240)から放射状に白い小弾を配置し、中央に極小の安全地帯を形成。
//  発射時に sound_enemyCharge を鳴らし、音で安全地帯の存在を示唆する。
// ============================================================
static void ShotSandstorm(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 砂塵：黒(7) or 橙(8) の鱗弾を20発
        for (int i = 0; i < 15; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = (double)GetRand(480);
            pEnemyShot->y = -10.0;
            pEnemyShot->muki = DX_PI / 2.0 + (double)(GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = (150.0 + (double)GetRand(150)) / 100.0;

            int color = (GetRand(1) == 0) ? 7 : 8;
            pEnemyShot->kind = img_enemyShotScale[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 安全地帯の境界：中央から半径60pxの円周上に白(6)の小弾を24発
        // 外側へ向かうことで、中央が一時的な安全地帯となる
        const int safeNum = 24;
        for (int i = 0; i < safeNum; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = (2.0 * DX_PI / (double)safeNum) * (double)i;
            pEnemyShot->x = 240.0 + cos(angle) * 60.0;
            pEnemyShot->y = 240.0 + sin(angle) * 60.0;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.8;

            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体のパターン
//  約20秒（1200フレーム）で1サイクル。4フェーズを繰り返す。
// ============================================================
void EnemyPat_SandStorm_Kimi()
{
    static int muki;
    static int shot_count;
    static int phase;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
        phase = 1;
    }
    else {
        // ゆっくりと左右に揺れ、縦にも微動
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 90) muki *= -1;
        enemy.y = 40.0 + sin((double)count * 0.02) * 10.0;
    }

    // フェーズ遷移（1200フレームサイクル）
    int cycle = (count > 0) ? (count - 1) % 1200 : 0;
    if (cycle < 300)       phase = 1; // 0 〜299  : 熱波
    else if (cycle < 600)  phase = 2; // 300〜599 : 風穴
    else if (cycle < 900)  phase = 3; // 600〜899 : 蜃気楼
    else                   phase = 4; // 900〜1199: 大砂嵐

    // ---------- フェーズ1「熱波」 ----------
    if (phase == 1) {
        if (count % 10 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotHeatWave;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;
            pEnemyShotSet->muki = 0;
            pEnemyShotSet->kind = shot_count++;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    // ---------- フェーズ2「風穴」 ----------
    else if (phase == 2) {
        if (count % 60 == 0) {
            // 渦巻き（画面中央）
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotVortex;
            pEnemyShotSet->x = 240.0;
            pEnemyShotSet->y = 240.0;
            pEnemyShotSet->muki = 0;
            pEnemyShotSet->kind = shot_count++;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        if (count % 90 == 30) {
            // 風圧弾（四方向から中央へ）
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotWindPressure;
            pEnemyShotSet->x = 240.0;
            pEnemyShotSet->y = 240.0;
            pEnemyShotSet->muki = 0;
            pEnemyShotSet->kind = shot_count++;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    // ---------- フェーズ3「蜃気楼」 ----------
    else if (phase == 3) {
        if (count % 80 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotMirage;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;
            pEnemyShotSet->muki = atan2(player.y - (enemy.y + 10.0), player.x - enemy.x);
            pEnemyShotSet->kind = shot_count++;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    // ---------- フェーズ4「大砂嵐」 ----------
    else if (phase == 4) {
        if (count % 15 == 0) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSandstorm;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = 0;
            pEnemyShotSet->kind = shot_count++;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}