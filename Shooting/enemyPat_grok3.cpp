// enemyPat_beautiful.cpp
// パターン：螺旋華蝶（美しい多重螺旋弾幕）
#include "DxLib.h"
#include "gv.h"
#include <math.h>

constexpr double PI = 3.14159265358979323846;

// ====================== 補助関数 ======================
static int GetColorCycle(int phase)
{
    return (phase / 6) % 6;  // 8色を滑らかに循環
}

// ====================== 弾幕本体 ======================
static void ShotSpiralFlower(sEnemyShotSet* p)
{
    sEnemyShot* s;
    const int layer = 3;           // 3層（大・中・小）
    const double baseAngle = p->muki;

    if (p->count == 0)
    {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int l = 0; l < layer; l++)          // 層ごとに
        {
            for (int i = 0; i < 24; i++)         // 1周24発
            {
                s = new sEnemyShot;

                double offset = (l - 1) * 0.12;  // 層による角度オフセット
                s->x = p->x;
                s->y = p->y;
                s->muki = baseAngle + i * (PI * 2 / 24) + offset;

                // 速度：外側ほど速く
                s->speed = 1.8 + l * 0.7 + (GetRand(40) - 20) / 100.0;

                // 種類と色
                int color = GetColorCycle(p->count + i * 3 + l * 8);

                if (l == 0)      s->kind = img_enemyShotLargeBall[color];
                else if (l == 1) s->kind = img_enemyShotMediumBall[color];
                else             s->kind = img_enemyShotSmallBall[color];

                // 双方向リストに追加
                s->prev = p->pEnemyShotHead->prev;
                s->next = p->pEnemyShotHead;
                p->pEnemyShotHead->prev->next = s;
                p->pEnemyShotHead->prev = s;
            }
        }
    }

    // ---------- 弾の更新 ----------
    s = p->pEnemyShotHead->next;
    while (s != p->pEnemyShotHead)
    {
        // 螺旋運動（角度を時間とともに変化）
        s->muki += 0.018;                    // 回転成分
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);

        // 徐々に加速（より派手に）
        if (p->count > 80) s->speed += 0.008;

        s = s->next;
    }

    p->count++;
}

// ====================== 敵本体パターン ======================
void EnemyPat_Beautiful_Grok()
{
    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = 100;
        enemy.hp = enemy.maxHp;
    }

    // 敵のゆらゆら移動
    enemy.y = 60.0 + sin(count * 0.03) * 25.0;
    enemy.x = 240.0 + sin(count * 0.015) * 90.0;

    // 発射タイミング
    if (count % 30 == 0)      // かなり高密度
    {
        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->patternFunc = ShotSpiralFlower;
        set->x = enemy.x;
        set->y = enemy.y + 20.0;

        // 徐々に回転方向・速度を変える
        double rotateSpeed = (count % 180 < 90) ? 0.0 : 0.035;
        set->muki = count * 0.027 + rotateSpeed * count;

        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        // リスト連結
        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }
}