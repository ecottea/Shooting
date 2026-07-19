// enemyPat_Tmp.cpp
// 立体的に見える弾幕パターン「スパイラルトンネル・パースペクティブ」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//----------------------------------------------------------
// 弾幕パターン関数（トンネル）
//----------------------------------------------------------
static void SpiralTunnel(sEnemyShotSet* pEnemyShotSet)
{
    // 初回のみ効果音を鳴らす
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium))
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 0;   // 生成した弾の総数
    }

    // 一定フレームごとに弾を追加（総数が多くなりすぎないように制限）
    if (pEnemyShotSet->count % 3 == 0 && pEnemyShotSet->param_i[0] < 200) {
        const double R = 180.0;            // 螺旋の基本半径
        const double rotSpeed = 0.01;      // 回転速度 (rad/frame)

        // 1フレームあたり 8 個の弾を生み出す
        for (int i = 0; i < 8; ++i) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 弾固有のパラメータを保存
            pEnemyShot->param_d[0] = GetRand(360) / 180.0 * DX_PI;   // 角度オフセット
            pEnemyShot->param_d[1] = GetRand(360) / 180.0 * DX_PI;   // 深度振動の位相オフセット
            pEnemyShot->param_d[2] = 0.03 + (GetRand(10) - 5) * 0.001; // 深度振動の速さ

            // 生成時の位置を計算（pEnemyShotSet->count はこのフレームの値）
            double angle = pEnemyShot->param_d[0] + pEnemyShotSet->count * rotSpeed;
            double depth = 0.5 + 0.5 * sin(pEnemyShot->param_d[1] + pEnemyShotSet->count * pEnemyShot->param_d[2] * 0.25);
            double scale = depth * 2.0;   // 遠近感（depth 0→0, 1→2.0）

            pEnemyShot->x = pEnemyShotSet->x + R * cos(angle) * scale;
            pEnemyShot->y = pEnemyShotSet->y + R * sin(angle) * scale;

            // 深度に応じて弾のサイズと色を変える
            int colorIdx;
            if (depth > 0.8)      colorIdx = 0;   // 赤（近い）
            else if (depth > 0.6) colorIdx = 8;   // 橙
            else if (depth > 0.4) colorIdx = 1;   // 黄
            else if (depth > 0.2) colorIdx = 3;   // シアン
            else                  colorIdx = 4;   // 青（遠い）

            if (depth > 0.66)
                pEnemyShot->kind = img_enemyShotLargeBall[colorIdx];    // 大玉
            else if (depth > 0.33)
                pEnemyShot->kind = img_enemyShotMediumBall[colorIdx];   // 中玉
            else
                pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];    // 小玉

            pEnemyShot->margin = 9999;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            pEnemyShotSet->param_i[0]++;   // 生成カウント
        }
    }

    // 既存の弾すべてを更新（深度・位置・サイズ・色を毎フレーム再計算）
    const double R = 180.0;
    const double rotSpeed = 0.01;
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double angle = pEnemyShot->param_d[0] + pEnemyShotSet->count * rotSpeed;
        double depth = 0.5 + 0.5 * sin(pEnemyShot->param_d[1] + pEnemyShotSet->count * pEnemyShot->param_d[2] * 0.25);
        double scale = depth * 2.0;

        pEnemyShot->x = pEnemyShotSet->x + R * cos(angle) * scale;
        pEnemyShot->y = pEnemyShotSet->y + R * sin(angle) * scale;

        int colorIdx;
        if (depth > 0.8)      colorIdx = 0;
        else if (depth > 0.6) colorIdx = 8;
        else if (depth > 0.4) colorIdx = 1;
        else if (depth > 0.2) colorIdx = 3;
        else                  colorIdx = 4;

        if (depth > 0.66)
            pEnemyShot->kind = img_enemyShotLargeBall[colorIdx];
        else if (depth > 0.33)
            pEnemyShot->kind = img_enemyShotMediumBall[colorIdx];
        else
            pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];

        pEnemyShot = pEnemyShot->next;
    }
}

//----------------------------------------------------------
// 敵本体のパターン
//----------------------------------------------------------
void EnemyPat_3D_DeepSeek()
{
    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 0.0;
        enemy.maxHp = enemy.hp = 200;

        // トンネル用の ShotSet をひとつだけ生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = SpiralTunnel;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵をゆっくり左右に振る（トンネルの中心が動く）
        if (enemy.y == 200) enemy.x = 240.0 + 80.0 * sin((count - 300) * 0.01);
        enemy.y += 2.0 / 3.0;
        if (enemy.y > 200) enemy.y = 200;

        // 既存 ShotSet の中心も敵に追従させる
        // （先頭からたどるのは省略し、生成した ShotSet は１つだけと決め打ち）
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == SpiralTunnel) {
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                break;
            }
            pSet = pSet->next;
        }
    }
}