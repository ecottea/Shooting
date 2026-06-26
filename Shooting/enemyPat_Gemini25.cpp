// enemyPat_fan.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：扇風機（回転する羽 ＋ 首振りの風）
static void ShotFan(sEnemyShotSet* pEnemyShotSet)
{
    // 敵本体が移動した場合に備えて、発射点の座標を敵に追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // 扇風機の首振り角度（真下[DX_PI/2]を中心に、左右45度ずつスイング）
    double neck_angle = DX_PI / 2.0 + (DX_PI / 4.0) * sin(pEnemyShotSet->count * 0.03);

    // --- 弾の生成 ---

    // 【1】扇風機の羽（高速回転するシアン色の鱗弾）
    // 3フレームごとに生成して連続した羽の軌跡を作る
    if (pEnemyShotSet->count % 3 == 0) {
        // 回転音・風切音のイメージで軽い発射音を鳴らす（連続再生でうるさくならないよう6フレーム毎）
        if (pEnemyShotSet->count % 6 == 0) {
            if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        int num_blades = 4; // 4枚羽
        double blade_spin_angle = pEnemyShotSet->count * 0.15; // 高速で角度を回す

        for (int i = 0; i < num_blades; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = blade_spin_angle + (DX_PI * 2.0 / num_blades) * i;
            pEnemyShot->speed = 2.5;

            // img_enemyShotScaleの [3] はシアン（風や刃のイメージ）
            pEnemyShot->kind = img_enemyShotScale[3];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 【2】扇風機の風（首振り方向に吹き出す白色の小玉）
    // 毎フレーム生成し、ランダムなばらつきで「風」の広がりを表現
    for (int i = 0; i < 2; i++) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;

        // 首振り方向をベースに、-20度 ～ +20度 のランダムなブレを加える
        // GetRand(40)は0～40なので、-20で±20となる
        double spread = (GetRand(40) - 20) / 180.0 * DX_PI;
        pEnemyShot->muki = neck_angle + spread;

        // 風の速さにもランダム性を持たせる (2.0 ～ 3.5)
        pEnemyShot->speed = (200 + GetRand(150)) / 100.0;

        // img_enemyShotSmallBallの [6] は白（空気・風のイメージ）
        pEnemyShot->kind = img_enemyShotSmallBall[6];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // --- 弾の移動 ---
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン：扇風機
void EnemyPat_ElectricFan_Gemini()
{
    // 登場時に一度だけ初期化し、延々と持続する弾幕セット(扇風機本体)を1つだけ登録
    if (count == 1) {
        enemy.x = 240.0; // 画面中央
        enemy.y = 80.0;  // 画面上部
        enemy.maxHp = enemy.hp = 200;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFan;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        // 弾リストのダミーヘッド（番兵）の初期化
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 弾幕セットのリストへ追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 扇風機自体は固定位置で首を振る（ShotFan側で角度計算）ので、
    // ここでは enemy の座標は動かさない仕様にしています。
}