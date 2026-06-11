#include "DxLib.h"
#include "gv.h"
#include <math.h>

// 波と粒の境界 用 弾更新関数
static void ShotWaveParticle(sEnemyShotSet* pSet)
{
    pSet->count++;

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead)
    {
        // 直進（粒）
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体パターン：波と粒の境界
void Pattern_WaveParticleBoundary_grok()
{
    static int phase = 0;
    static double rotAngle = 0.0;
    static double rotSpeed = 0.0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;   // 耐久長め
        phase = 0;
        rotAngle = 0.0;
        rotSpeed = 0.0;
    }

    // 敵のゆらゆら移動
    enemy.x = 240.0 + 120.0 * sin(count * 0.015);
    enemy.y = 60.0 + 30.0 * cos(count * 0.008);

    // 定期的に弾セット生成
    if (count % 4 == 0)   // 発射間隔（調整可）
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotWaveParticle;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 15.0;

        // 現在の回転角度を弾源の基準方向に
        pSet->muki = rotAngle;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 8方向（Lunatic寄り） or 5方向（Normal寄り）に調整
        const int ways = 8;
        for (int i = 0; i < ways; i++)
        {
            sEnemyShot* pShot = new sEnemyShot;

            double baseAngle = pSet->muki + i * (DX_PI * 2 / ways);

            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = baseAngle;
            pShot->speed = 2.8 + GetRand(40) / 100.0;   // やや速め

            // 米弾（小さい弾）
            pShot->kind = img_enemyShotScale[4];   // 青っぽい米弾がおすすめ

            // 双方向リストに追加
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // 弾セットをリンク
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 回転速度の不規則変化（これが「波と粒の境界」の肝）
    if (count % 45 == 0) {
        phase = GetRand(3);
    }

    switch (phase) {
    case 0:  // 加速回転
        rotSpeed += 0.0012;
        break;
    case 1:  // 減速
        rotSpeed *= 0.96;
        break;
    case 2:  // 逆回転加速
        rotSpeed -= 0.0018;
        break;
    }

    // 速度に上限・下限
    if (rotSpeed > 0.085) rotSpeed = 0.085;
    if (rotSpeed < -0.085) rotSpeed = -0.085;

    rotAngle += rotSpeed;

    // たまに大きく乱す
    if (GetRand(180) == 0) {
        rotSpeed += (GetRand(100) - 50) / 80.0;
    }
}