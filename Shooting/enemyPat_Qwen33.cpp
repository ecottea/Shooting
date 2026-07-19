// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：絶対に回避できない弾幕
static void ShotUnavoidable(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 効果音は最も重厚な sound_enemyShot_extreme を使用
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 画面全体を隙間なく埋め尽くす弾幕 (480x480を16ピクセル間隔で埋める)
        // 30x30 = 900発。全色がランダムに使用され、視覚的に圧倒する
        for (int i = 0; i < 480; i += 16) {
            for (int j = 0; j < 480; j += 16) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = i;
                pEnemyShot->y = j;

                // プレイヤーに向かって収束させる
                pEnemyShot->muki = atan2(player.y - j, player.x - i);
                pEnemyShot->speed = 4.0 + GetRand(20) / 10.0;

                // 鱗弾（Scale）の全色（0～8）をランダムに使用
                // GetRand(8) は 0～8 の 9種類を返すため、色指定と完全に一致する
                pEnemyShot->kind = img_enemyShotScale[GetRand(8)];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // トドメ：プレイヤーの座標に直接弾を生成（物理的に絶対に回避できない）
        // 大玉（LargeBall）の全色を使用し、視認性を下げる
        for (int i = 0; i < 10; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = player.x;
            pEnemyShot->y = player.y;
            pEnemyShot->muki = GetRand(360) * DX_PI / 180.0;
            pEnemyShot->speed = 5.0 + GetRand(20) / 10.0;
            pEnemyShot->kind = img_enemyShotLargeBall[GetRand(8)];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    // countのインクリメントや画面外削除はメインルーチンで行われるため、ここでは座標更新のみ行う
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Unavoidable_Qwen()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 100; // 200で固定
    }
    else {
        // 敵を適度に移動させる
        enemy.x += 1.5 * cos(count * 0.05);
        enemy.y += 1.0 * sin(count * 0.03);

        // 画面外に出ないように補正
        if (enemy.x < 20.0) enemy.x = 20.0;
        if (enemy.x > 460.0) enemy.x = 460.0;
        if (enemy.y < 20.0) enemy.y = 20.0;
        if (enemy.y > 200.0) enemy.y = 200.0;
    }

    // 240フレームごとに絶対に回避できない弾幕を放つ
    // 弾のプールサイズ(4096)を考慮し、弾が画面外へ出る時間を考慮して間隔を調整している
    if (count % 240 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotUnavoidable;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        pEnemyShotSet->kind = count / 240;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}