// enemyPat_Tmp.cpp
// はかいこうせん（破壊光線）をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// 破壊光線パターン本体（太い光線を模した高速弾の連なり）
// 特徴：中心に強力な主光線、周囲に若干の拡散・波打つ副光線
// =============================================
static void ShotDestructionRay(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射タイミング（光線チャージ＆発射）
    if (pEnemyShotSet->count == 0)
    {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);  // 重い射撃音

        const double baseAngle = pEnemyShotSet->muki;
        const int beamLength = 48;   // 光線の長さ（弾の数）

        for (int i = 0; i < beamLength; i++)
        {
            pEnemyShot = new sEnemyShot;
            double offset = (i - beamLength / 2) * 4.0; // 縦方向に間隔

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + offset * sin(baseAngle);

            // 主光線はまっすぐ下向き（プレイヤー方向）
            pEnemyShot->muki = baseAngle;
            pEnemyShot->speed = 6.0 + i * 0.1;  // 先端ほど少し速く（光線らしい伸び）

            // 色と種類（赤～白の強力な光線イメージ）
            int color = (i % 3 == 0) ? 0 : 6; // 主に赤と白
            if (i % 8 == 0) color = 7;         // 稀に黒でアクセント

            if (i % 6 == 0) {
                pEnemyShot->kind = img_enemyShotLargeBall[color];   // 太い部分
            }
            else if (i % 3 == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[color];
            }

            // リスト連結
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 副光線（左右に少し広がる波打つ光線）
        for (int side = -1; side <= 1; side += 2)
        {
            if (side == 0) continue;
            for (int i = 0; i < 6; i++)
            {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + side * 12.0;
                pEnemyShot->y = pEnemyShotSet->y;

                pEnemyShot->muki = baseAngle + side * 0.06 + sin(i * 0.8) * 0.08;
                pEnemyShot->speed = 5.5 + GetRand(80) / 100.0;

                int color = GetRand(1) ? 0 : 6; // 赤or白
                pEnemyShot->kind = img_enemyShotScale[color];  // 鱗弾で光線らしさ

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレーム更新（直進）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead)
    {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 光線らしい微かな揺らぎ（後半の弾に適用）
        if (pEnemyShot->count > 8)
        {
            pEnemyShot->muki += sin(pEnemyShot->count * 0.3) * 0.007;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// =============================================
// 敵本体パターン
// =============================================
void EnemyPat_Hakaikousen_Grok()
{
    static int phase = 0;
    static int muki = 1;

    if (count == 1)
    {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        muki = 1;
    }

    // 敵の動き：ゆったり左右移動＋時々中央に戻る
    if (count % 180 < 120)
    {
        enemy.x += 1.15 * (double)muki;
    }
    else
    {
        enemy.x += (240.0 - enemy.x) * 0.04;  // 中央に戻る
    }

    if (count % 150 == 75) muki *= -1;

    // 破壊光線発射（約1.2秒間隔）
    const int T = 24;
    if (count % (4 * T) == 0)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDestructionRay;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;

        // プレイヤーに向けた角度（少し予測）
        double tx = player.x + (player.x - enemy.x) * 0.12;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, tx - pEnemyShotSet->x);

        // 光線ヘッドのダミー
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // ShotSetをリンク
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 補助：細かい小弾を時々ばらまく（破壊光線の合間に）
    if (count % T == 0 && count % (4 * T) > T)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDestructionRay;  // 簡易版として再利用（count==0以外は無害）
        pEnemyShotSet->x = enemy.x + GetRand(60) - 30;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x) +
            (GetRand(80) - 40) / 180.0 * DX_PI * 0.6;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}