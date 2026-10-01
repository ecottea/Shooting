#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ペアノ曲線「自己相似の庭」
static void ShotPeano(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 発動予告音
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // ペアノ曲線(1次)の9頂点の相対座標 (グリッドサイズ 160.0)
        // 順序: (-1,-1)→(-1,0)→(-1,1)→(0,1)→(0,0)→(0,-1)→(1,-1)→(1,0)→(1,1)
        int path[9][2] = {
            {-1, -1}, {-1,  0}, {-1,  1},
            { 0,  1}, { 0,  0}, { 0, -1},
            { 1, -1}, { 1,  0}, { 1,  1}
        };

        for (int i = 0; i < 9; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            // 始点はすべてペアノ曲線の開始点 (-1, -1) に対応する位置
            pShot->x = 240.0 + path[0][0] * 160.0;
            pShot->y = 240.0 + path[0][1] * 160.0;
            pShot->muki = 0;
            pShot->speed = 0;
            pShot->kind = img_enemyShotLargeBall[0]; // 0:赤 大玉

            pShot->param_i[0] = i;       // 弾のID (0~8)
            pShot->param_i[1] = 0;       // 現在の目標インデックス (0~8), 99は直進モード
            pShot->param_i[2] = i * 15;  // 移動開始までの待機フレーム (時間差で出発)

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    int path[9][2] = {
        {-1, -1}, {-1,  0}, {-1,  1},
        { 0,  1}, { 0,  0}, { 0, -1},
        { 1, -1}, { 1,  0}, { 1,  1}
    };
    double gridSize = 160.0;
    double baseX = 240.0; // 画面中央
    double baseY = 240.0;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int targetIdx = pShot->param_i[1];
        int waitTimer = pShot->param_i[2];

        if (targetIdx == 99) {
            // 直進モード（曲線完了後の誘導弾や分裂弾）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            if (waitTimer > 0) {
                pShot->param_i[2]--;
            }
            else {
                double targetX = baseX + path[targetIdx][0] * gridSize;
                double targetY = baseY + path[targetIdx][1] * gridSize;

                double dx = targetX - pShot->x;
                double dy = targetY - pShot->y;
                double dist = sqrt(dx * dx + dy * dy);

                if (dist < 6.0) {
                    // 頂点に到達
                    pShot->x = targetX;
                    pShot->y = targetY;

                    int nextIdx = targetIdx + 1;
                    if (nextIdx < 9) {
                        pShot->param_i[1] = nextIdx;
                        pShot->param_i[2] = 10; // 次の移動までの溜め

                        // 第2段階：自己相似的な分裂（偶数番目の頂点で発動）
                        if (targetIdx % 2 == 0) {
                            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                            for (int j = 0; j < 4; j++) {
                                sEnemyShot* pSub = new sEnemyShot;
                                pSub->x = pShot->x;
                                pSub->y = pShot->y;
                                // 頂点の進行方向に沿った分裂＋わずかなばらつき
                                pSub->muki = (targetIdx * 40.0 + j * 90.0 + GetRand(20) - 10) * DX_PI / 180.0;
                                pSub->speed = 2.5;
                                pSub->kind = img_enemyShotMediumBall[1]; // 1:黄 中玉
                                pSub->param_i[1] = 99; // 直進モード

                                pSub->prev = pEnemyShotSet->pEnemyShotHead->prev;
                                pSub->next = pEnemyShotSet->pEnemyShotHead;
                                pEnemyShotSet->pEnemyShotHead->prev->next = pSub;
                                pEnemyShotSet->pEnemyShotHead->prev = pSub;
                            }
                        }
                    }
                    else {
                        // 第3段階：曲線完了後、プレイヤー方向へ誘導
                        pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                        pShot->speed = 3.5;
                        pShot->kind = img_enemyShotSmallBall[2]; // 2:緑 小玉
                        pShot->param_i[1] = 99;
                    }
                }
                else {
                    // 移動中
                    pShot->muki = atan2(dy, dx);
                    pShot->speed = 6.0;
                    pShot->x += pShot->speed * cos(pShot->muki);
                    pShot->y += pShot->speed * sin(pShot->muki);
                }
            }
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_PeanoCurve_Qwen()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = -40.0; // 画面上から登場
        enemy.maxHp = enemy.hp = 200;
    }

    // 登場アニメーション
    if (count < 60) {
        enemy.y += 4.0;
    }
    else if (count == 60) {
        enemy.y = 40.0; // 上部で待機
    }

    // ペアノ曲線弾幕の発動
    if (count % 450 == 120) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPeano;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 敵の左右の揺れ動き（曲線発動中も動きを与える）
    if (count > 60) {
        enemy.x = 240.0 + sin((count - 60) * 0.05) * 120.0;
    }
}