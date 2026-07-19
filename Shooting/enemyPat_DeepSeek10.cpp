// enemyPat_Tmp.cpp
// 竜モチーフ弾幕：炎のブレス（小玉の扇状放射）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//------------------------------------------------
// 弾幕パターン：竜の炎ブレス
//   pEnemyShotSet->count == 0 のときだけ弾を生成し、
//   その後は毎フレーム全弾の座標更新のみ行う。
//------------------------------------------------
static void ShotDragonBreath(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0)
    {
        // 効果音（中弾用。必要に応じて専用SEに差し替え）
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 12～18発の火炎弾を扇状にばらまく
        int numBullets = 42 + GetRand(6);               // [12,18]
        double baseAngle = pEnemyShotSet->muki;         // 自機方向

        for (int i = 0; i < numBullets; ++i)
        {
            pEnemyShot = new sEnemyShot;

            // ±30°の範囲でランダムに角度をずらす
            double spread = (GetRand(60) - 30) / 180.0 * DX_PI;  // -30°～30°をラジアンに
            pEnemyShot->muki = baseAngle + spread;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 速度は 2.0 ～ 3.5
            pEnemyShot->speed = (200 + GetRand(350)) / 100.0;

            // 色は赤（0）か黄（1）をランダムに選ぶ（炎のイメージ）
            int color = (GetRand(1) == 0) ? 0 : 1;
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム：生成済み弾の移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

//------------------------------------------------
// 敵本体パターン：竜
//   count（グローバルフレームカウンタ）を利用して
//   動きの制御と弾幕セットの定期生成を行う。
//------------------------------------------------
void EnemyPat_Dragon_DeepSeek()
{
    // 静的な内部状態（敵の移動方向や位相）
    static int moveDir = 1;    // 左右移動の向き（1:右, -1:左）
    static int movePhase = 0;    // サイン波の位相カウンタ

    // 初回フレームで初期化
    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;   // 竜らしくやや高めの耐久力
        moveDir = 1;
        movePhase = 0;
    }
    else
    {
        // 竜が左右に大きくうねりながら飛ぶ動き
        const double baseX = 240.0;
        const double amplitude = 120.0;                // 横振幅
        const double period = 150.0;                // 周期（フレーム）
        const double phaseRad = movePhase * 2.0 * DX_PI / period;

        enemy.x = baseX + amplitude * sin(phaseRad);
        // 上下にもわずかに揺らす
        enemy.y = 40.0 + 15.0 * sin(phaseRad * 2.0);

        ++movePhase;
    }

    // 30フレームごとに炎ブレス弾幕セットを作成
    if (count % 30 == 0)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDragonBreath;

        // 発射位置：竜の口元を想定（やや前方）
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // 自機の方向を向く
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);

        // 弾の双方向リストを初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // enemyShotSetHead のリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}