#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：牛乳を注ぐ（傾けるミルクポット）
// 白中玉の連続落下ストリーム + 飛び散る白小玉 + 最後の扇状跳ね
static void ShotMilkPour(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 注ぎ始めの効果音
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 注ぎ口の位置を敵の動きに追従させる（傾きの揺らぎ）
    pEnemyShotSet->x = enemy.x + 12.0 * sin(pEnemyShotSet->count * 0.07);
    pEnemyShotSet->y = enemy.y + 18.0;

    // ===== メインのミルクの流れ（白中玉の連続落下）=====
    // 約80フレーム間、3フレームごとに1発
    if (pEnemyShotSet->count < 80 && pEnemyShotSet->count % 3 == 0) {
        pEnemyShot = new sEnemyShot;
        // 注ぎ口から少しランダムにずらす
        pEnemyShot->x = pEnemyShotSet->x + (GetRand(12) - 6);
        pEnemyShot->y = pEnemyShotSet->y + (GetRand(6) - 2);

        // ほぼ真下（π/2）に、わずかな左右ブレ
        double baseMuki = DX_PI * 0.5;
        pEnemyShot->muki = baseMuki + (GetRand(30+10) - 15-5) / 180.0 * DX_PI;
        pEnemyShot->speed = 1.1 + GetRand(40) / 100.0; // 初速は控えめ

        // 重力用パラメータ
        pEnemyShot->param_d[0] = pEnemyShot->speed * cos(pEnemyShot->muki); // vx
        pEnemyShot->param_d[1] = pEnemyShot->speed * sin(pEnemyShot->muki); // vy
        pEnemyShot->param_i[0] = 1; // 重力適用フラグ

        // 白い中玉
        pEnemyShot->kind = img_enemyShotMediumBall[6];

        // リストに追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // ===== 飛び散るしずく（白小玉）=====
    // 流れの途中で時々分岐
    if (pEnemyShotSet->count >= 10 && pEnemyShotSet->count < 75 && pEnemyShotSet->count % 7 == 0) {
        for (int i = 0; i < 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(20) - 10);
            pEnemyShot->y = pEnemyShotSet->y + 10.0 + GetRand(15);

            // 横方向に飛ばす
            double scatterMuki = DX_PI * 0.5 + (GetRand(100) - 50) / 180.0 * DX_PI;
            pEnemyShot->muki = scatterMuki;
            pEnemyShot->speed = 1.8 + GetRand(80) / 100.0;

            pEnemyShot->param_d[0] = pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->param_d[1] = pEnemyShot->speed * sin(pEnemyShot->muki);
            pEnemyShot->param_i[0] = 1; // 重力あり

            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白い小玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ===== 注ぎ終わりの最後の一滴（扇状跳ね）=====
    if (pEnemyShotSet->count == 85) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 9+6; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 8.0;

            // 下方向に扇状
            double fanMuki = DX_PI * 0.5 + (i - 4-3) * 0.18;
            pEnemyShot->muki = fanMuki;
            pEnemyShot->speed = 2.4 + GetRand(30) / 100.0;

            // 扇弾は重力を弱めに、または一定速度でも良いがここでは軽く重力
            pEnemyShot->param_d[0] = pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->param_d[1] = pEnemyShot->speed * sin(pEnemyShot->muki);
            pEnemyShot->param_i[0] = 1;

            // 白小玉（または中楕円で少し存在感を）
            pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotSmallBall[6] : img_enemyShotMediumOval[6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ===== 全弾の移動更新 =====
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 重力適用（ミルクの落下加速）
            pShot->param_d[1] += 0.055;
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
        }
        else {
            // 通常移動（今回はほぼ使わないが保険）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン（牛乳を注ぐ女）
void EnemyPat_Milkmaid_Grok()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }
    else {
        // ゆっくり左右に揺れる（ポットを持って体を傾ける感じ）
        enemy.x = 240.0 + 55.0 * sin(count * 0.018);
        // yはほぼ固定、わずかに上下
        enemy.y = 70.0 + 4.0 * sin(count * 0.03);
    }

    // 一定周期で注ぎ動作を開始
    // 200フレーム周期、開始タイミングをずらして余韻を残す
    if (count % 200 == 40) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMilkPour;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;
        pEnemyShotSet->muki = DX_PI * 0.5; // 下向き基準
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
