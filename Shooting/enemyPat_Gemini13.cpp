// enemyPat_razorLeaf.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：はっぱカッター
// 鋭い葉っぱ（鱗弾）が扇状に放たれ、ヒラヒラと軌道を揺らしながら加速して飛んでいくパターン
static void ShotRazorLeaf(sEnemyShotSet* pEnemyShotSet)
{
    // pEnemyShotSet->count が 0〜40 の間、10カウントごとに発射（計5波）
    if (pEnemyShotSet->count % 10 == 0 && pEnemyShotSet->count <= 40) {
        // 葉を投げるような少し重めの効果音
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 1波につき扇状に9枚の葉っぱを発射
        for (int i = -4*3; i <= 4*3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // プレイヤーを狙った基準角（muki）から、左右に少しずつズラして扇状に配置
            // 15度(15.0 / 180.0 * DX_PI)ずつ角度を開く
            pEnemyShot->muki = pEnemyShotSet->muki + (i * 15.0 / 3) / 180.0 * DX_PI;

            // 初期速度は少しランダム性を持たせる（1.0 〜 1.5）
            pEnemyShot->speed = 1.0 + GetRand(50) / 100.0;

            // 見た目の設定: 基本は緑(2)の鱗弾、たまに枯れ葉として黄色(1)を混ぜる
            int color = (GetRand(10) > 8) ? 1 : 2;
            pEnemyShot->kind = img_enemyShotScale[color];

            // リストの末尾（headの直前）に追加する処理
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既に発射された弾（葉っぱ）の軌道更新
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 【特徴1】カッターのように徐々に加速する（最大速度 7.0 まで）
        if (pEnemyShot->speed < 7.0) {
            pEnemyShot->speed += 0.08;
        }

        // 【特徴2】葉っぱが舞うような揺らぎ（ウォブル）を加える
        // pEnemyShot->count を使って sin波を作り、進行方向(muki)に加算することで軌道が波打ちます
        // countのインクリメントはメインルーチンで行われるため、ここでは読み取るだけ
        double wobble = sin(pEnemyShot->count * 0.15) * 0.025;
        pEnemyShot->muki += wobble;

        // X, Y 座標の更新
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 次の弾へ
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（指定関数名）
void EnemyPat_RazorLeaf_Gemini()
{
    // 初期化処理
    if (count == 1) {
        // 画面上部の中央付近からスタート
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 敵本体の移動：sin波を使って画面上部を滑らかに8の字（無限軌道）を描くように飛ぶ
        // DX_PI を使って周期を計算
        enemy.x = 240.0 + sin(count * 1.5 * DX_PI / 180.0) * 120.0;
        enemy.y = 80.0 + sin(count * 3.0 * DX_PI / 180.0) * 20.0;
    }

    // 120フレーム（約2秒）ごとに「はっぱカッター」をセット
    if (count % 120 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRazorLeaf; // 作成した弾幕関数をセット

        // 敵の現在位置から発射
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // セット全体の基準角を、現在のプレイヤーの位置へ向ける
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 弾セット用のダミーヘッドノードを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストの末尾に追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}