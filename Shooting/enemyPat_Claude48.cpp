// enemyPat_Shiokifuki.cpp
//
// 「怒濤潮吹き」
// 岩場の潮吹き穴のように、フィールド下部に複数の噴出口(sEnemyShotSet)を配置し、
// それぞれが独立したタイマーで「待機→噴出(水柱が連射で立ち上る)→クールダウン」を
// 繰り返す弾幕パターン。水柱は頂点(=鉛直速度が0以上になった瞬間)で消滅し、
// その場から扇状の水しぶきに転化して降り注ぐ。
//
// 【素材選定メモ】
//   ・水柱(上昇中の弾)   : img_enemyShotBullet[3](シアン) … 針状の見た目を
//                            「水の柱」の表現に転用。img_enemyShotLaser は
//                            既存方針どおり不採用。
//   ・水しぶき(飛散後の弾) : img_enemyShotMediumOval[3]/[6](シアン/白交互) …
//                            楕円=雫のシルエットが水滴の表現に合うため採用。
//   ・効果音             : sound_enemyCharge(噴出開始の合図)、
//                            sound_enemyShot_light(飛沫が弾ける音)

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 頂点に達した水柱を、その場で扇状の水しぶき弾に変換する
// ============================================================
static void SpawnSplash(sEnemyShotSet* pEnemyShotSet, double x, double y)
{
    const int    splashNum = 7;              // 飛沫の本数(奇数=中央が真上)
    const double spreadHalf = DX_PI / 3.0;    // 扇の半角(±60度)
    const double baseAngle = -DX_PI / 2.0;   // 真上方向

    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

    for (int i = 0; i < splashNum; i++) {
        sEnemyShot* pShot = new sEnemyShot;

        double rate = (double)i / (double)(splashNum - 1); // 0.0 ～ 1.0
        double angle = baseAngle - spreadHalf + spreadHalf * 2.0 * rate;

        pShot->x = x;
        pShot->y = y;
        pShot->muki = angle;
        pShot->speed = 1.4 + GetRand(15) / 10.0; // 1.4 ～ 2.9

        pShot->param_i[0] = 1;    // 1: 水しぶき(飛散)弾
        pShot->param_d[1] = x;    // x0(発生時の位置)
        pShot->param_d[2] = y;    // y0
        pShot->param_d[3] = 0.12; // 落下重力(緩やかに落ちる)

        pShot->kind = (i % 2 == 0) ? img_enemyShotMediumOval[3] : img_enemyShotMediumOval[6];

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }
}

// ============================================================
// 噴出口1つ分のパターン関数
//   ・pEnemyShotSet の param_i[0]/param_d[] … 噴出口自体の状態(待機/噴出/クールダウン)
//   ・各 sEnemyShot の param_i[0]           … 0:上昇中の水柱 / 1:水しぶき
// ============================================================
static void ShotShiokifuki(sEnemyShotSet* pEnemyShotSet)
{
    // ---- 初期化(噴出口が生成された最初のフレーム) ----
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = 0;               // state: 待機
        pEnemyShotSet->param_d[1] = 0.0;             // 状態開始の基準フレーム
        pEnemyShotSet->param_d[0] = 20 + GetRand(60); // 待機フレーム数(20～80)

        {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = -19;
            pShot->y = -19;
            pShot->kind = img_enemyShotSmallBall[0];
            pShot->param_i[0] = -1;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    int elapsed = pEnemyShotSet->count - (int)pEnemyShotSet->param_d[1];

    // ---- 噴出口のステートマシン ----
    switch (pEnemyShotSet->param_i[0]) {

    case 0: // 待機中
        if (elapsed >= (int)pEnemyShotSet->param_d[0]) {
            pEnemyShotSet->param_i[0] = 1;                 // state: 噴出中
            pEnemyShotSet->param_d[1] = pEnemyShotSet->count;
            pEnemyShotSet->param_i[1] = 0;                  // 発射済み本数
            pEnemyShotSet->param_i[2] = 3;      // 今回の本数(4～7)
            pEnemyShotSet->param_d[2] = 3.0;                 // 発射間隔(フレーム)

            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        break;

    case 1: // 噴出中(水柱を連射)
    {
        int interval = (int)pEnemyShotSet->param_d[2];

        if (pEnemyShotSet->param_i[1] < pEnemyShotSet->param_i[2] &&
            elapsed >= pEnemyShotSet->param_i[1] * interval) {

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = -DX_PI / 2;
            pShot->kind = img_enemyShotBullet[3]; // シアンの細長い弾で水柱を表現

            pShot->param_i[0] = 0;                 // 0: 上昇中の水柱
            pShot->param_d[0] = 6.0 + GetRand(4) + 2.5;   // 初速v0(6～10)
            pShot->param_d[1] = 0.18;               // 重力g
            pShot->param_d[2] = pEnemyShotSet->y;   // y0
            pShot->param_d[3] = pEnemyShotSet->x;   // x0

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;

            pEnemyShotSet->param_i[1]++;
        }

        // 発射し終え、かつ最後の水柱が頂点に達するのに十分な時間が経過したらクールダウンへ
        const int maxRiseFrame = 60; // v0最大10,g=0.18時の上昇フレーム数(約55.6)に安全マージン
        int       lastLaunchFrame = (pEnemyShotSet->param_i[2] - 1) * interval;

        if (pEnemyShotSet->param_i[1] >= pEnemyShotSet->param_i[2] &&
            elapsed >= lastLaunchFrame + maxRiseFrame) {
            pEnemyShotSet->param_i[0] = 2;                  // state: クールダウン
            pEnemyShotSet->param_d[1] = pEnemyShotSet->count;
            pEnemyShotSet->param_d[0] = 30 + GetRand(30);    // クールダウン(30～60)
        }
        break;
    }

    case 2: // クールダウン中
        if (elapsed >= (int)pEnemyShotSet->param_d[0]) {
            pEnemyShotSet->param_i[0] = 0;                  // state: 待機に戻る
            pEnemyShotSet->param_d[1] = pEnemyShotSet->count;
            pEnemyShotSet->param_d[0] = 20 + GetRand(60);
        }
        break;
    }

    // ---- 弾の移動(水柱の上昇/頂点での飛沫化、水しぶきの落下) ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 途中で削除する可能性があるため先に保存

        if (pShot->param_i[0] == 0) {
            // 水柱:等加速度運動を経過フレーム数から純粋計算(状態を保持しない)
            double t = (double)pShot->count;
            double v0 = pShot->param_d[0];
            double g = pShot->param_d[1];
            double y0 = pShot->param_d[2];
            double x0 = pShot->param_d[3];
            double vy = -v0 + g * t;

            pShot->x = x0;
            pShot->y = y0 - v0 * t + 0.5 * g * t * t;

            if (vy >= 0.0) {
                // 頂点到達 → その場で水しぶきに変換し、自身は消滅
                SpawnSplash(pEnemyShotSet, pShot->x, pShot->y);

                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        else {
            // 水しぶき:緩やかな重力落下のみの放物運動
            double t = (double)pShot->count;
            double x0 = pShot->param_d[1];
            double y0 = pShot->param_d[2];
            double g2 = pShot->param_d[3];

            pShot->x = x0 + pShot->speed * cos(pShot->muki) * t;
            pShot->y = y0 + pShot->speed * sin(pShot->muki) * t + 0.5 * g2 * t * t;
        }

        pShot = pNext;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Spout_Claude()
{
    const int spoutNum = 6; // 噴出口(岩の隙間)の数

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        // 画面下部(岩場)にほぼ等間隔で噴出口を配置。x座標に少しランダム性を持たせる
        for (int i = 0; i < spoutNum; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotShiokifuki;
            pEnemyShotSet->x = 40.0 + i * (400.0 / (spoutNum - 1)) + (GetRand(20) - 10);
            pEnemyShotSet->y = 470.0; // 画面下端付近(岩場)から噴き上がる
            pEnemyShotSet->kind = i;

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
    else {
        // 波にたゆたうような穏やかな上下運動
        enemy.y = 70.0 + 10.0 * sin(count * 0.02);
    }
}