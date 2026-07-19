// enemyPat_thunder.cpp
// 雷をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 雷弾幕：稲妻状にジグザグしながら進む弾
static void ShotThunder(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 雷の起点は敵の位置
        double baseX = pEnemyShotSet->x;
        double baseY = pEnemyShotSet->y;

        // プレイヤー方向を基準に、左右に分岐する稲妻を生成
        double baseAngle = atan2(player.y - baseY, player.x - baseX);

        // 左右に分岐する2本の稲妻
        for (int branch = 0; branch < 2; ++branch) {
            // 1本あたり 3〜5 個の節（ジグザグの頂点）を持つ
            int segments = (3 + GetRand(2))*10; // 3〜5

            // 前の節の位置を覚えておく
            double prevX = baseX;
            double prevY = baseY;

            // 左右に振る角度（±20度程度）
            double branchAngle = (branch == 0 ? -1.0 : 1.0) * (20.0 / 180.0 * DX_PI);

            for (int i = 0; i < segments; ++i) {
                pEnemyShot = new sEnemyShot;

                // 節ごとに少しランダムにずらす
                double segAngle = baseAngle + branchAngle + (GetRand(30) - 15) * 2 / 180.0 * DX_PI;
                double segLen = (40.0 + GetRand(20)) * 0.3; // 40〜60 ピクセル

                pEnemyShot->x = prevX + segLen * cos(segAngle);
                pEnemyShot->y = prevY + segLen * sin(segAngle);

                // 進行方向は前の節から今の節へ
                pEnemyShot->muki = atan2(pEnemyShot->y - prevY, pEnemyShot->x - prevX);
                pEnemyShot->speed = (150 + GetRand(100)) / 100.0; // 1.5〜2.5 程度

                // 雷らしい見た目：鱗弾 or 菱形弾 + 青系 or シアン or 白
                int type = GetRand(1) + 4; // 4 or 5: 鱗弾 or 菱形弾
                int color = GetRand(2) + 3; // 3 or 4 or 5: シアン or 青 or マゼンタ
                if (GetRand(3) == 0) color = 6; // たまに白

                switch (type) {
                case 4:
                    pEnemyShot->kind = img_enemyShotScale[color];
                    break;
                case 5:
                    pEnemyShot->kind = img_enemyShotDiamond[color];
                    break;
                }

                // リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

                prevX = pEnemyShot->x;
                prevY = pEnemyShot->y;
            }
        }
    }

    // 生成済みの弾を移動
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（指定どおりの関数名）
void EnemyPat_Thunder_Sakana()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 雷弾幕を一定間隔で発射
    if (count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotThunder;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}