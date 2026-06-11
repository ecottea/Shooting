#include "DxLib.h"
#include "gv.h"
#include <math.h>


// =============================================
//  滝モチーフ弾幕：Waterfall Cascade
// =============================================
static void ShotWaterfall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回：弾生成
    if (pEnemyShotSet->count == 0)
    {
        // 音（重めが滝っぽい）
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int streamCount = 12;        // 滝の筋の数
        const double width = 280.0;        // 滝の横幅

        for (int i = 0; i < streamCount; i++)
        {
            pEnemyShot = new sEnemyShot;

            // 滝の横位置（中央を中心に均等に配置）
            double offsetX = (i - (streamCount - 1) / 2.0) * (width / (streamCount - 1));
            pEnemyShot->x = pEnemyShotSet->x + offsetX + GetRand(24) - 12; // 少し揺らぎ
            pEnemyShot->y = pEnemyShotSet->y;

            // 基本は真下だが、少し角度をつけて自然な流れに
            double angle = (offsetX * 0.008) + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->muki = DX_PI / 2.0 + angle;   // 下方向

            // 速度：上部は速く、下に行くほど少し遅くなるイメージ（視覚的に滝っぽく）
            pEnemyShot->speed = 4.2 + GetRand(80) / 100.0;

            // 弾の種類（小玉＋鱗弾を多用すると滝らしくなる）
            int type = GetRand(100);
            int color = (GetRand(100) < 70) ? 4 : 6; // 青～白中心（水色）

            if (type < 65)
                pEnemyShot->kind = img_enemyShotSmallBall[color];
            else if (type < 90)
                pEnemyShot->kind = img_enemyShotScale[color];
            else
                pEnemyShot->kind = img_enemyShotMediumBall[color];

            // リンク
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        // 基本移動
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 滝らしい揺らぎ（サイン波）
        int life = pEnemyShotSet->count;
        pEnemyShot->x += sin(life * 0.25 + pEnemyShot->y * 0.03) * 0.8;

        // 下に行くほど速度を少し落とす（滝の末端が広がる感じ）
        if (pEnemyShot->y > 280.0)
            pEnemyShot->speed *= 0.985;

        pEnemyShot = pEnemyShot->next;
    }
}

// 滝弾幕を使う敵パターン例
void Pattern_WaterfallBoss()
{
    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 150;
    }

    // ゆったり左右移動
    static double movePhase = 0.0;
    movePhase += 0.018;
    enemy.x = 240.0 + sin(movePhase) * 110.0;

    // 定期的に滝を落とす
    if (count % 20 == 0)   // かなり高密度
    {
        sEnemyShotSet* p = new sEnemyShotSet;
        p->count = 0;
        p->patternFunc = ShotWaterfall;
        p->x = enemy.x;
        p->y = enemy.y + 25.0;
        p->muki = 0.0;

        p->pEnemyShotHead = new sEnemyShot;
        p->pEnemyShotHead->prev = p->pEnemyShotHead;
        p->pEnemyShotHead->next = p->pEnemyShotHead;

        // 双方向リストに追加
        p->prev = enemyShotSetHead.prev;
        p->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = p;
        enemyShotSetHead.prev = p;
    }
}