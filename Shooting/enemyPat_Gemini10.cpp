// enemyPat_dragon.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：双竜の舞と炎のブレス
static void ShotDragonDance(sEnemyShotSet* pEnemyShotSet)
{
    // 120フレーム（約2秒）の間、連続して弾を生成し続ける
    if (pEnemyShotSet->count < 120) {

        // 1. 【鱗弾】うねる2本の竜の胴体（ワインダー弾幕）
        // 毎フレームだと多すぎるため、2フレームに1回発射
        if (pEnemyShotSet->count % 1 == 0) {
            // 効果音が重なりすぎないよう、適度な間隔で鳴らす
            if (pEnemyShotSet->count % 8 == 0) {
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            for (int i = 0; i < 2; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                // サイン波を利用して発射角度を揺らす。i=0 と i=1 で逆位相にして交差させる
                double wave = sin(pEnemyShotSet->count * 0.15) * (DX_PI / 2.5);
                if (i == 1) wave = -wave;

                pEnemyShot->muki = pEnemyShotSet->muki + wave;
                pEnemyShot->speed = 1.0; // 初速は遅めに設定（後で加速）

                // 竜の鱗をイメージ（0:赤炎の竜、4:青炎の竜）
                int color = (i == 0) ? 0 : 4;
                pEnemyShot->kind = img_enemyShotScale[color];

                // リストへ追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 2. 【大玉】竜のブレス（火球）
        // 30フレームごとに自機へ向かって火球を吐き出す
        if (pEnemyShotSet->count % 30 == 0) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 基準の角度にわずかなブレ（-5度〜+5度）を加える
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(10) - 5) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.5;
            pEnemyShot->kind = img_enemyShotLargeBall[1]; // 黄色の大玉

            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 弾の移動と状態更新 ---
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 竜の「生き物感」を演出するため、鱗弾は徐々に加速させる
        if (pEnemyShot->kind == img_enemyShotScale[0] || pEnemyShot->kind == img_enemyShotScale[4]) {
            if (pEnemyShot->speed < 4.5) {
                pEnemyShot->speed += 0.03;
            }
        }

        // 移動処理
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン：指定の関数名
void EnemyPat_Dragon_Gemini()
{
    // 登場時の初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // ボスや中ボスを想定して体力を多めに
    }
    else {
        // 空を舞う竜のように、8の字を描きながらゆっくり浮遊する
        enemy.x = 240.0 + sin(count * 0.015) * 120.0;
        enemy.y = 80.0 + sin(count * 0.030) * 30.0;
    }

    // 200フレーム周期で「双竜の舞」弾幕を発動
    if (count % 130 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDragonDance;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // 発動した瞬間の自機方向を「基準の角度」として記録する
        pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

        // ダミーヘッドの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットをリストに繋ぐ
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}