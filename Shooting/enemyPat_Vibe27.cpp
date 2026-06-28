// enemyPat_shuriken.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：旋風手裏剣
static void ShotShuriken(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音を再生
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 12個の手裏剣を円形に配置
        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = pEnemyShotSet->muki + 2.0 * DX_PI * i / 12.0; // 30度間隔
            pEnemyShot->x = pEnemyShotSet->x + 100.0 * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + 100.0 * sin(angle);
            pEnemyShot->muki = angle; // 手裏剣の向き（時計回りに回転させるための初期角度）
            pEnemyShot->speed = 2.0;

            // 移動方向を初期化（プレイヤー方向に向かって直進するための角度）
            pEnemyShot->param_d[0] = angle;

            // 手裏剣の見た目：菱形弾を使用
            pEnemyShot->kind = img_enemyShotDiamond[i % 6];
            // 色を白に設定
            pEnemyShot->param_i[0] = 6; // 白

            pEnemyShot->margin = 480;

            // リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 手裏剣の動きを更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 時計回りに回転
        pEnemyShot->muki += 0.05;

        // プレイヤーの方向に向かって徐々に移動方向を調整
        double targetAngle = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
        pEnemyShot->param_d[0] += 0.01 * (targetAngle - pEnemyShot->param_d[0]);

        // 移動方向に従って直進
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->param_d[0]);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->param_d[0]);

        // 画面外に出た場合、反対側に再出現
        if (pEnemyShot->x < -10 || pEnemyShot->x > 480 + 10 ||
            pEnemyShot->y < -10 || pEnemyShot->y > 480 + 10) {
            if (pEnemyShot->x < -10) pEnemyShot->x = 480 + 10;
            else if (pEnemyShot->x > 480 + 10) pEnemyShot->x = -10;
            if (pEnemyShot->y < -10) pEnemyShot->y = 480 + 10;
            else if (pEnemyShot->y > 480 + 10) pEnemyShot->y = -10;

            // 方向を反転
            //pEnemyShot->param_d[0] += DX_PI;
        }

        pEnemyShot = pEnemyShot->next;
    }
}


// 敵本体のパターン
void EnemyPat_Shuriken_Vibe()
{
    static int shot_count;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
    }

    // 定期的に旋風手裏剣を発射
    if (count % 100 == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShuriken;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = GetRand(359) / 180.0 * DX_PI;
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