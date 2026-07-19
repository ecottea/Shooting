// enemyPat_match.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕：マッチ棒（擦火→発火→燃焼）
// ------------------------------------------------------------
static void ShotMatch(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --- 初期生成：擦火（マッチ棒1本をプレイヤー方向に発射） ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 3.5;

        // マッチ棒：銃弾（細長）・赤色
        pEnemyShot->kind = img_enemyShotBullet[0];

        // param_i[0] : 状態識別（0=マッチ棒飛行中, 1=炎弾, 2=灰）
        // param_i[1] : マッチ棒の発火までのフレーム
        // param_i[2] : 発火済みフラグ
        pEnemyShot->param_i[0] = 0;
        pEnemyShot->param_i[1] = 20 + GetRand(10); // 20～30フレーム後に発火
        pEnemyShot->param_i[2] = 0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // --- 弾更新 ---
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pEnemyShot->next;

        if (pEnemyShot->param_i[0] == 0) {
            // ===== マッチ棒（飛行中）=====
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

            // 弱誘導：プレイヤー方向に向きを少しずつ修正
            double targetMuki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            double diff = targetMuki - pEnemyShot->muki;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            pEnemyShot->muki += diff * 0.03;

            // 発火判定
            if (pEnemyShot->count >= pEnemyShot->param_i[1] && pEnemyShot->param_i[2] == 0) {
                pEnemyShot->param_i[2] = 1; // 発火済み

                // 発火音
                if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                // 扇状に炎弾を5個生成（中心±40°）
                const int flameCount = 5;
                for (int i = 0; i < flameCount; i++) {
                    sEnemyShot* pFlame = new sEnemyShot;
                    pFlame->x = pEnemyShot->x;
                    pFlame->y = pEnemyShot->y;

                    double spread = 80.0 / 180.0 * DX_PI; // 全体80°
                    pFlame->muki = pEnemyShot->muki - spread / 2.0 + spread * i / (flameCount - 1);
                    pFlame->speed = 2.0 + GetRand(100) / 100.0; // 2.0～3.0

                    // 炎の色：赤(0)、黄(1)、橙(8) からランダム
                    int colorTbl[3] = { 0, 1, 8 };
                    pFlame->kind = img_enemyShotMediumBall[colorTbl[GetRand(2)]];

                    // 炎弾パラメータ
                    pFlame->param_i[0] = 1;   // 状態：炎弾
                    pFlame->param_d[0] = 0.96; // 減衰率

                    // リスト挿入（マッチ棒の直前に追加）
                    pFlame->prev = pEnemyShot->prev;
                    pFlame->next = pEnemyShot;
                    pEnemyShot->prev->next = pFlame;
                    pEnemyShot->prev = pFlame;
                }

                // マッチ棒本体は燃え尽きて灰に変化
                pEnemyShot->param_i[0] = 2; // 状態：灰
                pEnemyShot->speed = 0.8;
                pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒（灰）
            }
        }
        else if (pEnemyShot->param_i[0] == 1) {
            // ===== 炎弾（滞留・減速）=====
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            pEnemyShot->speed *= pEnemyShot->param_d[0]; // 徐々に減速して滞留
            if (pEnemyShot->speed < 1) pEnemyShot->speed = 1;

            // 微細な揺らぎ（火の粉のような動き）
            pEnemyShot->muki += (GetRand(10) - 5) / 180.0 * DX_PI * 0.05;
        }
        else if (pEnemyShot->param_i[0] == 2) {
            // ===== 灰（ゆっくり落下・消滅へ）=====
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            pEnemyShot->speed *= 0.95; // さらに減速
            pEnemyShot->y += 0.75;     // 重力でゆっくり落下
        }

        pEnemyShot = pNext;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Match_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.7 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // マッチ弾幕を連射（擦火のイメージ）
    if (count % 25 == 0) {
        for (int i = -1; i <= 1; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotMatch;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 15.0;

            // プレイヤー方向を基本に、少しランダムにばらつき
            double baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x) + i * DX_PI/2;
            pEnemyShotSet->muki = baseMuki + (GetRand(20) - 10) / 180.0 * DX_PI;

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