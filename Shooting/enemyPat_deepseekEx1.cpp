// EnemyPat_Hakaikousen_DeepSeek.cpp
// はかいこうせん（破壊光線）をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//------------------------------------------------------------------------------
// 破壊光線の弾幕パターン（直線状に並んだ弾が一斉に飛ぶビーム）
//------------------------------------------------------------------------------
static void BeamPattern(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 最初のフレームだけ弾を生成する（count == 0 のとき）
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 照準方向を基準にビームを構成
        double baseMuki = pEnemyShotSet->muki;

        const double startDist = 20.0;   // ビームの開始距離（自機とのめり込み防止）
        const double distStep = 18.0;   // ビームの長さ方向の間隔
        const int    segments = 16;     // ビームを構成するセグメント数（奥行き）
        const double thickness = 9.0;    // ビームの太さ方向の最大オフセット
        const int    layers = 2;      // 同一セグメント内の密度

        for (int i = 0; i < segments; ++i) {
            double dist = startDist + i * distStep;
            double baseX = pEnemyShotSet->x + dist * cos(baseMuki);
            double baseY = pEnemyShotSet->y + dist * sin(baseMuki);

            // 太さ方向に 3 列（中心、左、右）
            for (int col = -1; col <= 1; ++col) {
                double offset = col * thickness;
                double perpMuki = baseMuki + DX_PI / 2.0;
                double x = baseX + offset * cos(perpMuki);
                double y = baseY + offset * sin(perpMuki);

                // 各列に 2 発ずつ配置して密度を上げる
                for (int k = 0; k < layers; ++k) {
                    pEnemyShot = new sEnemyShot;

                    // 座標に微小なランダム性を与える
                    pEnemyShot->x = x + GetRand(5) - 2;
                    pEnemyShot->y = y + GetRand(5) - 2;

                    // 角度はベース方向 ±10° 程度のランダムばらつき
                    double randAngle = (GetRand(20) - 10) / 180.0 * DX_PI;
                    pEnemyShot->muki = baseMuki + randAngle;

                    // ビームは高速で直進
                    pEnemyShot->speed = 6.0;

                    // 黄色の小玉をビームの光弾として使用
                    pEnemyShot->kind = img_enemyShotSmallBall[1];

                    // 双方向リストに追加
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
    }

    // 全弾の移動処理（毎フレーム実行）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

//------------------------------------------------------------------------------
// 敵本体パターン：はかいこうせん
//------------------------------------------------------------------------------
void EnemyPat_Hakaikousen_DeepSeek2()
{
    static double moveDir = 1.0;

    // 初期化（count == 1 のときのみ）
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1.0;
        return;
    }

    // 左右移動
    enemy.x += 0.6 * moveDir;
    if (enemy.x > 400.0 || enemy.x < 80.0) {
        moveDir *= -1.0;
    }

    // 一定間隔でビーム（破壊光線）を発射
    if (count % 10 == 0 && count % 200 >= 50) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = BeamPattern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 6.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);

        // 弾リストのヘッダーノードを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルな敵弾セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}