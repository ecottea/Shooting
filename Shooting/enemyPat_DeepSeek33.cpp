// enemyPat_unavoidable.cpp
// 絶対に回避できない弾幕（追尾弾の雨）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕パターン：避けられない追尾弾
// ・毎フレーム、新たな追尾弾を1つ生成する
// ・すべての弾は毎フレーム、プレイヤーの方向へ角度を補正し直進する
// ------------------------------------------------------------
static void UnavoidableHoming(sEnemyShotSet* pEnemyShotSet)
{
    // 初回のみ効果音（ヘビー級）
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 毎フレーム、新しい追尾弾を1つ追加
    {
        sEnemyShot* pNew = new sEnemyShot;

        // 弾の発生位置：セットの座標にわずかな乱数を加える
        pNew->x = pEnemyShotSet->x + GetRand(20) - 10;
        pNew->y = pEnemyShotSet->y + GetRand(20) - 10;

        // 最初の向きはプレイヤー方向
        pNew->muki = atan2(player.y - pNew->y, player.x - pNew->x);
        pNew->speed = 3.5;                     // やや速めで回避困難に
        pNew->kind = img_enemyShotMediumBall[0]; // 赤色中玉（画像配列の添字0）

        // 循環双方向リストへ挿入（末尾に追加）
        pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pNew->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
        pEnemyShotSet->pEnemyShotHead->prev = pNew;
    }

    // 既存の弾すべてを更新：プレイヤー方向へ角度補正 ＋ 移動
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        // 現在のプレイヤー位置へ向きを再設定（完全追尾）
        p->muki = atan2(player.y - p->y, player.x - p->x);

        // 移動
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);

        p = p->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン（絶対回避不能）
// ・画面上部をゆっくり左右に移動
// ・一度だけ追尾弾幕セットを生成し、以降そのセットが永続的に弾を撒き続ける
// ------------------------------------------------------------
void EnemyPat_Unavoidable_DeepSeek()
{
    static int muki; // 移動方向（1 or -1）

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 100;
        muki = 1;

        // 追尾弾幕セットを作成し、グローバルリストに接続
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;                     // メインルーチンが自動インクリメント
        pSet->patternFunc = UnavoidableHoming;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        // 弾リストのヘッダ（ダミーノード）を作成
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルの enemyShotSetHead に挿入
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        // 敵の水平移動（サンプルと同じ動き）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) {
            muki *= -1;
        }
    }
}