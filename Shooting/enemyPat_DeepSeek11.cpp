// 破壊光線モチーフ：伸びるレーザー
// レーザーが発射点から照準方向へ徐々に伸び、しばらく残留した後に消える

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//------------------------------------------------------------------------------
// レーザーを構成する弾を毎フレーム追加し、光線が伸びていくパターン
//------------------------------------------------------------------------------
static void GrowingLaser(sEnemyShotSet* pEnemyShotSet)
{
    // レーザー生成期間（0～24フレームの間だけ新たな弾を追加）
    const int GROW_FRAMES = 50;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    if (pEnemyShotSet->count < GROW_FRAMES) {
        double baseMuki = pEnemyShotSet->muki;
        double startDist = 16.0;                       // 最初の弾までの距離
        double distStep = 14.0;                       // 奥行き方向の間隔
        double thickness = 8.0;                        // ビームの太さ（左右の幅）
        int    cols = 3;                          // 太さ方向の列数（奇数推奨）
        int    layers = 2;                          // 同じ距離での密度（多重配置）

        // 現在のフレームでの先端距離を計算
        double currentMaxDist = startDist + pEnemyShotSet->count * distStep;

        // レーザーの全長にわたって弾を配置（すでに配置済みの距離にも再配置されるが、
        // メインルーチン側で既存の弾は残っているため、二重配置は避ける）
        // 今回は単純に先端部分だけ追加する方式にする。
        // 前回のフレームで追加した距離の次の位置から追加する。
        double prevMaxDist = startDist + (pEnemyShotSet->count - 1) * distStep;
        double startAddDist = prevMaxDist + 1.0; // 少し重ねて切れ目をなくす
        double endAddDist = currentMaxDist;

        // 追加すべき距離の範囲に弾を生成
        for (double d = startAddDist; d <= endAddDist + 0.001; d += distStep) {
            double baseX = pEnemyShotSet->x + d * cos(baseMuki);
            double baseY = pEnemyShotSet->y + d * sin(baseMuki);

            // 太さ方向に配置
            for (int col = 0; col < cols; ++col) {
                // 列を -1, 0, 1 のようにオフセット
                double offset = (col - (cols - 1) / 2.0) * thickness;
                double perpMuki = baseMuki + DX_PI / 2.0;
                double x = baseX + offset * cos(perpMuki);
                double y = baseY + offset * sin(perpMuki);

                // 同一位置に複数重ねて密度を上げる
                for (int l = 0; l < layers; ++l) {
                    // 微小なランダム性
                    x += (GetRand(3) - 1);
                    y += (GetRand(3) - 1);

                    sEnemyShot* pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = x;
                    pEnemyShot->y = y;
                    pEnemyShot->muki = baseMuki;    // 速度0なので角度は飾り
                    pEnemyShot->speed = 0.0;        // その場に留まる
                    // 白い小玉を使用（光線らしさ）
                    pEnemyShot->kind = img_enemyShotSmallBall[6]; // 6:白

                    // 双方向リストに追加
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
    }

    // 既存の弾はすべて速度0なので移動処理は不要だが、
    // メインルーチンが移動処理を呼ぶ前にここで何もしないと、
    // このパターン関数を抜けた後にメインルーチンが全弾の座標更新を行う可能性がある。
    // しかし、サンプルのShotScatterを見る限り、パターン関数内で座標更新を行っている。
    // つまり、メインルーチンはパターン関数を呼び、その後は個別に弾を動かさない設計かもしれない。
    // ここではサンプルに従い、全弾に対して移動計算を行う（速度0なので何も起きない）。
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

//------------------------------------------------------------------------------
// 敵本体パターン：破壊光線（伸びるレーザー）
//------------------------------------------------------------------------------
void EnemyPat_Hakaikousen_DeepSeek()
{
    static double moveDir = 1.0;
    static int    shotInterval = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1.0;
        shotInterval = 60;
        return;
    }

    // 敵の動き：ゆっくり左右移動 + 上下に微小な揺れ
    enemy.x += 1.0 * moveDir;
    enemy.y = 60.0 + 5.0 * sin(count * 0.05);
    if (enemy.x > 400.0 || enemy.x < 80.0) {
        moveDir *= -1.0;
    }

    // 180カウントごとに破壊光線を発射
    ++shotInterval;
    if (shotInterval >= 120) {
        shotInterval = 0;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = GrowingLaser;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);

        // 弾リストのダミーヘッド作成
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