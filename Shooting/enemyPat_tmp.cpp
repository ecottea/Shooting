// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：竜のブレスと鱗（うねる扇状弾＋ばら撒き）
// ============================================================
static void ShotDragon(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 5フレームごとに扇状のブレス（銃弾）を発射
    if (pEnemyShotSet->count % 5 == 0 && pEnemyShotSet->count <= 570) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int num_bullets = 9;  // 9方向へ発射
        double spread = 0.8;  // 広がる角度（ラジアン）

        for (int i = 0; i < num_bullets; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double base_angle = DX_PI / 2.0; // 下方向
            // countに応じてブレス全体が左右にうねる
            double wave = sin(pEnemyShotSet->count * 0.06) * 0.7;

            pEnemyShot->muki = base_angle + wave - spread / 2.0 + (spread / (num_bullets - 1.0)) * i;

            pEnemyShot->speed = 3.5;
            pEnemyShot->kind = img_enemyShotBullet[3]; // シアンの銃弾（ブレスをイメージ）

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 15フレームごとに鱗弾をばら撒く
    if (pEnemyShotSet->count % 15 == 7 && pEnemyShotSet->count <= 570) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            // GetRand(x) は 0～x を返すため、-30～30 の範囲になるよう調整
            pEnemyShot->x = pEnemyShotSet->x + GetRand(60) - 30;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20);

            // 下方向に向けてランダムな角度で発射 (-45度～+45度)
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(90) - 45) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.0 + GetRand(150) / 100.0;
            pEnemyShot->kind = img_enemyShotScale[2]; // 緑の鱗弾

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 弾自体も少しうねる（竜が空中をくねるような軌道）
        // pEnemyShot->count はメインルーチンでインクリメントされる仕様
        pEnemyShot->muki += sin(pEnemyShot->count * 0.1) * 0.02;

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体のパターン（竜）
// ============================================================
void EnemyPat_Tmp()
{
    static double enemy_y_add;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = -40.0;
        enemy.maxHp = enemy.hp = 200;
        enemy_y_add = 0.7;
    }
    else {
        // 竜が空を飛ぶように、大きくサインカーブでうねりながら下降
        enemy.y += enemy_y_add;
        enemy.x = 240.0 + sin(count * 0.025) * 160.0;

        if (enemy.y >= 240) if (enemy_y_add >= -0.7) enemy_y_add -= 0.1;
        if (enemy.y <= 10) if (enemy_y_add <= 0.7) enemy_y_add += 0.1;
    }

    // 弾幕セットの生成（30フレームごと）
    if (count % 600 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDragon;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0; // 敵の少し下（口元）から発射
        pEnemyShotSet->muki = DX_PI / 2.0; // 下方向

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}