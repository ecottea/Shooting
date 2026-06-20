#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 紅葉の舞（弾幕パターン）
static void ShotAutumnLeaves(sEnemyShotSet* pEnemyShotSet)
{
    // フェーズ1：突風の発生（100フレームの間、3フレームに1回葉っぱを生成）
    if (pEnemyShotSet->count < 100 && pEnemyShotSet->count % 1 == 0) {

        // 効果音は鳴りすぎを防ぐため適度に間引く
        if (pEnemyShotSet->count % 5 == 0) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 1回の生成で2枚の葉を舞い散らせる
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 発射位置を敵の中心から少し散らす
            pEnemyShot->x = pEnemyShotSet->x + GetRand(60) - 30;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;

            // 上方向を中心に、扇状に吹き飛ばす
            pEnemyShot->muki = -DX_PI / 2.0 + (GetRand(140) - 70) / 180.0 * DX_PI;

            // 初速（この微小な小数差が、後でひらひら舞い落ちる際の「揺らぎの個性」になる）
            pEnemyShot->speed = (150 + GetRand(300)) / 100.0 + 10.0; // 1.5 ～ 4.5

            // 色の決定（赤を多め、次いで黄、たまに緑を混ぜて紅葉を表現）
            // GetRand(9) は 0～9 の10種類
            int colorRand = GetRand(9);
            int color = 0;
            if (colorRand < 5)      color = 0; // 0:赤 (50%)
            else if (colorRand < 8) color = 1; // 1:黄 (30%)
            else                    color = 2; // 2:緑 (20%)

            // 鱗弾を葉っぱに見立てる
            pEnemyShot->kind = img_enemyShotScale[color];
            pEnemyShot->margin = 999.;

            // リストの末尾に追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 登録されている葉っぱ（弾）の物理挙動更新
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        if (pEnemyShot->count < 45) {
            // 空気抵抗フェーズ：風に巻き上げられた後、急激に減速する
            pEnemyShot->speed *= 0.94;
        }
        else {
            // 舞い落ちフェーズ：ひらひらと重力に従って落ちる
            // 終端速度までゆっくりと加速
            if (pEnemyShot->speed < 1.8) {
                pEnemyShot->speed += 0.03;
            }

            // 揺らぎの計算（sin波）
            // 各弾が持つ固有の speed の端数をオフセットに使い、揺れるタイミングをバラバラにする
            double phaseOffset = pEnemyShot->speed * 100.0;
            double swayAngle = DX_PI / 2.0 + sin(pEnemyShot->count * 0.07 + phaseOffset) * 0.6;
            pEnemyShot->muki = swayAngle;
        }

        // 座標更新
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Maple_Gemini()
{
    // 出現時の初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 秋風に乗って漂うように、ゆったりと8の字軌道を描いて移動
        enemy.x = 240.0 + 100.0 * sin(count * 0.015);
        enemy.y = 80.0 + 15.0 * sin(count * 0.03);
    }

    // 180フレーム（約3秒）ごとに紅葉の突風（ShotSet）を発生させる
    if (count % 180 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAutumnLeaves;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0; // 今回は使用しないため0

        // ダミーヘッドの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットリストの末尾に追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}