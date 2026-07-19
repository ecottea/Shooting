// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：はっぱカッター
// プレイヤーに向けて緑色の鱗弾を扇状にばら撒き、うねりながら飛んでいく
static void ShotLeafCutter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numShots = 15;
        double baseAngle = pEnemyShotSet->muki; // プレイヤーへの基本角度
        double spreadAngle = DX_PI / 6.0 / 3;       // 扇状に広がる角度 (30度分)

        for (int i = 0; i < numShots; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 弾ごとに角度をずらして扇状に配置
            double angleOffset = spreadAngle * (i - (numShots) / 2.0);
            pEnemyShot->muki = baseAngle + angleOffset;
            pEnemyShot->speed = 3.5 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotScale[2]; // 緑色の鱗弾

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動とうねりの処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // メインルーチンでインクリメント済みの count を参照してサインカーブでうねらせる
        double wave = cos(pEnemyShot->count * 0.15) * 0.06;
        pEnemyShot->muki += wave;

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // ※ pEnemyShot->count++ はメインルーチンで行われるため、ここでは行わない

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_RazorLeaf_Qwen()
{
    static int muki;
    if (count == 1) {
        // 初期位置とHP設定
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 画面上部を左右に移動
        enemy.x += 1.5 * (double)muki;
        if (enemy.x < 40.0 || enemy.x > 440.0) muki *= -1;
    }

    // 30フレームごとに弾幕を生成
    if (count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLeafCutter;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        // プレイヤーの方向を計算
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 弾管理用リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵の弾幕セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}