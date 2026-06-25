// enemyPat_Sakana3.cpp
// パターン18：花火状＋渦巻き弾幕（見栄え重視）
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

// 花火状放射＋外側渦巻き弾幕
static void ShotFireworkSpiral(sEnemyShotSet* p)
{
    sEnemyShot* shot;
    p->count++;

    // 初回のみ花火状放射弾を生成
    if (p->count == 1) {
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 放射状弾（内側・高速）
        const int N_RADIAL = 24;
        for (int i = 0; i < N_RADIAL; i++) {
            double angle = 2.0 * PI * i / N_RADIAL;
            shot = new sEnemyShot;
            shot->x = p->x;
            shot->y = p->y;
            shot->muki = angle;
            shot->speed = 4.5 + 0.5 * sin(angle * 2.0); // 少しばらつき
            shot->kind = img_enemyShotSmallBall[i % 4]; // 小玉で軽快に
            shot->prev = p->pEnemyShotHead->prev;
            shot->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = shot;
            p->pEnemyShotHead->prev = shot;
        }

        // 渦巻き弾（外側・ゆっくり）
        const int N_SPIRAL = 80;
        const double SPIRAL_RADIUS = 60.0;
        for (int i = 0; i < N_SPIRAL; i++) {
            double angle = 2.0 * PI * i / N_SPIRAL;
            double offset = angle * 1.5; // 角度に応じて半径をずらす
            shot = new sEnemyShot;
            shot->x = p->x + SPIRAL_RADIUS * cos(angle);
            shot->y = p->y + SPIRAL_RADIUS * sin(angle);
            shot->muki = angle + offset * 0.1; // 外側に少しずつ向きを変える
            shot->speed = 1.2 + 0.3 * sin(angle * 3.0); // ゆっくり＋揺らぎ
            shot->kind = img_enemyShotMediumBall[(i / 10) % 4]; // 中玉で存在感
            shot->prev = p->pEnemyShotHead->prev;
            shot->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = shot;
            p->pEnemyShotHead->prev = shot;
        }
    }

    // 更新処理
    shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        // 放射状弾は直進
        shot->x += shot->speed * cos(shot->muki);
        shot->y += shot->speed * sin(shot->muki);

        // 渦巻き弾は少しずつ回転しながら外側へ
        if (p->count > 20) {
            shot->muki += 0.02; // ゆっくり回転
            shot->speed *= 1.005; // 少しずつ加速
        }

        shot = shot->next;
    }

    //// 一定時間で弾幕セットを削除
    //if (p->count > 300) {
    //    sEnemyShot* next;
    //    shot = p->pEnemyShotHead->next;
    //    while (shot != p->pEnemyShotHead) {
    //        next = shot->next;
    //        delete shot;
    //        shot = next;
    //    }
    //    delete p->pEnemyShotHead;
    //    p->prev->next = p->next;
    //    p->next->prev = p->prev;
    //    delete p;
    //}
}

// 敵本体のパターン
void EnemyPat_Beautiful_Sakana()
{
    sEnemyShotSet* p;
    static double baseX = 240.0, baseY = 120.0;

    if (count == 1) {
        enemy.maxHp = 250;
        enemy.hp = enemy.maxHp;
        enemy.x = baseX;
        enemy.y = baseY;
    }

    // 敵のゆっくりした左右移動
    enemy.x = baseX + 50.0 * sin(count * 0.008);
    enemy.y = baseY + 20.0 * cos(count * 0.005);

    // 一定間隔で花火状弾幕を発射
    if (count % 90 == 0) {
        p = new sEnemyShotSet;
        p->count = 0;
        p->patternFunc = ShotFireworkSpiral;
        p->x = enemy.x;
        p->y = enemy.y;
        p->muki = count * 0.03; // 時間経過で発射角度を少しずつ回転
        p->kind = 0;

        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;

        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }
}