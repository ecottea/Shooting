// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// -----------------------------------------------------------------
// 弾幕1：横風（サイン波でウネウネ進む細かい砂粒）
// -----------------------------------------------------------------
static void ShotSandWind(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 画面左端から40個の砂粒を生成
        for (int i = 0; i < 15; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = -10.0; // 画面左端
            // GetRand(480) は 0〜480 の 481通りの値を返す
            pEnemyShot->y = (double)GetRand(480);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0; // speedは使わずparam_dで独自の物理運動を行う

            // param_d[0]: 基本X速度, [1]: 波の振幅, [2]: 角速度, [3]: 初期位相
            pEnemyShot->param_d[0] = 2.5 + (double)GetRand(15) / 10.0; // 2.5 〜 4.0
            pEnemyShot->param_d[1] = 20.0 + (double)GetRand(20);       // 20.0 〜 40.0
            pEnemyShot->param_d[2] = 0.03 + (double)GetRand(5) / 100.0;// 0.03 〜 0.08
            pEnemyShot->param_d[3] = (double)GetRand(628) / 100.0;     // 0.00 〜 6.28

            pEnemyShot->param_d[1] /= 4;

            // 細かい砂のイメージで橙色の小玉を使用
            pEnemyShot->kind = img_enemyShotSmallBall[8];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // X方向は常に一定速度で進む
        pEnemyShot->x += pEnemyShot->param_d[0];

        // Y方向はサイン波の微分（Yの変位）を足してウネウネした軌道にする
        double vy = pEnemyShot->param_d[1] * pEnemyShot->param_d[2]
            * cos(pEnemyShot->param_d[2] * pEnemyShot->count + pEnemyShot->param_d[3]);
        pEnemyShot->y += vy;

        pEnemyShot = pEnemyShot->next;
    }
}

// -----------------------------------------------------------------
// 弾幕2：塵旋風（画面を彷徨う渦巻き弾幕＋プレイヤーに石をばら撒く）
// -----------------------------------------------------------------
static void ShotDustDevil(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 渦巻きの外周を構成する弾を24個生成
        int shotNum = 24;
        for (int i = 0; i < shotNum; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = DX_PI * 2.0 / shotNum * i;
            pEnemyShot->x = pEnemyShotSet->x + 30.0 * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + 30.0 * sin(angle);
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;

            // param_i[0] = 1 は「渦巻き構成弾」であるというフラグ
            pEnemyShot->param_i[0] = 1;
            // param_d[0]: 現在の角度, [1]: 半径
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = 30.0;

            // 渦を表現するため黄色の鱗弾を使用
            pEnemyShot->kind = img_enemyShotScale[1];
            pEnemyShot->margin = 100;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ShotSetのパラメータ設定（渦の中心の動き）
        pEnemyShotSet->param_d[0] = 1.5;   // 中心の基本移動速度
        pEnemyShotSet->param_d[1] = 60.0;   // 中心のY方向サイン波の振幅
        pEnemyShotSet->param_d[2] = 0.015;  // 中心のY方向サイン波の角速度
        pEnemyShotSet->param_d[3] = (double)GetRand(628) / 100.0; // 中心の位相
        pEnemyShotSet->param_d[4] = 0.08;   // 渦自体の回転速度
    }

    // 旋風の中心を移動させる（mukiの方向へ進みつつ、Y方向にウネウネ動く）
    pEnemyShotSet->x += pEnemyShotSet->param_d[0] * cos(pEnemyShotSet->muki);
    double center_vy = pEnemyShotSet->param_d[1] * pEnemyShotSet->param_d[2]
        * cos(pEnemyShotSet->param_d[2] * pEnemyShotSet->count + pEnemyShotSet->param_d[3]);
    pEnemyShotSet->y += center_vy;

    // 全弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == 1) {
            // 渦巻き構成弾：中心の周りを回り続ける
            pEnemyShot->param_d[0] += pEnemyShotSet->param_d[4]; // 角度更新
            pEnemyShot->x = pEnemyShotSet->x + pEnemyShot->param_d[1] * cos(pEnemyShot->param_d[0]);
            pEnemyShot->y = pEnemyShotSet->y + pEnemyShot->param_d[1] * sin(pEnemyShot->param_d[0]);
        }
        else {
            // 追加発射された通常弾：直進する
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        pEnemyShot = pEnemyShot->next;
    }

    // 30フレームごとに、旋風からプレイヤーに向かって「石（中玉）」を追加発射
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 30 == 0) {
        int addNum = 5;
        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        for (int i = 0; i < addNum; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // プレイヤー狙い ±20度のバラつき
            // GetRand(40) は 0〜40 なので、-20して ±20 の範囲にする
            pEnemyShot->muki = baseAngle + ((double)GetRand(40) - 20.0) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.5 + (double)GetRand(10) / 10.0; // 2.5 〜 3.5

            pEnemyShot->param_i[0] = 0; // 渦構成弾ではない(通常弾)フラグ
            // 飛ばされる石のイメージで橙色の中玉を使用
            pEnemyShot->kind = img_enemyShotMediumBall[8];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
}

// -----------------------------------------------------------------
// 弾幕3：フィナーレ（全方位に広がる砂の突風レーザー）
// -----------------------------------------------------------------
static void ShotSandBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 30度刻みで12本の極太レーザーを放射
        int shotNum = 12;
        for (int i = 0; i < shotNum; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI * 2.0 / shotNum * i;
            pEnemyShot->speed = 4.0;

            // 突風のイメージで橙色のレーザーを使用
            pEnemyShot->kind = img_enemyShotLaser[8];

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


// -----------------------------------------------------------------
// 敵本体のパターン
// -----------------------------------------------------------------
void EnemyPat_SandStorm_Zai()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // --- タイムライン制御 ---

    // フェーズ1：横風を吹かせる (count 60 〜 540 まで、40フレームごとに生成)
    if (count % 660 >= 60 && count % 660 <= 540 && count % 660 % 40 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSandWind;
        pEnemyShotSet->x = -10.0;
        pEnemyShotSet->y = 240.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // フェーズ2：左から塵旋風を発生 (count 180)
    if (count % 660 == 180) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDustDevil;
        pEnemyShotSet->x = -20.0;
        pEnemyShotSet->y = 150.0;
        pEnemyShotSet->muki = 0.0; // 右へ移動
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // フェーズ3：右からもう一つの塵旋風を発生 (count 300)
    if (count % 660 == 300) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDustDevil;
        pEnemyShotSet->x = 500.0;
        pEnemyShotSet->y = 350.0;
        pEnemyShotSet->muki = DX_PI; // 左へ移動
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // フィナーレ：風が止まり、全方向に砂の突風レーザーを放つ (count 600)
    if (count % 660 == 600) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSandBurst;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
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