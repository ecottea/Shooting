#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：直進（AとBの両方で使い回す単純な移動パターン）
static void ShotStraight(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_LunaticGun_Qwen()
{
    static int muki;
    static int shot_count;

    // 弾幕Bのパターン管理用
    static int b_state = 0;      // 0: 待機, 1: 1セット目発射中, 2: 2セット目発射中
    static int b_timer = 0;
    static int b_shot_count = 0; // 0〜4 (5回で1セット)

    // 弾幕Aのパターン管理用
    static double base_angle = 0.0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;

        // 静的変数の初期化
        b_state = 0;
        b_timer = 0;
        b_shot_count = 0;
        base_angle = 0.0;
    }
    else {
        // ボスの移動（サンプルと同じく左右移動）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // =========================================================
    // 弾幕A: 間隔が狭い箇所と広い箇所が交互に並んだ10way全方位弾
    // 時計回りに回転しながら常時撃ち続ける
    // =========================================================
    if (count % 10 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotStraight;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 狭い間隔(18度)と広い間隔(54度)を交互に配置 (18*5 + 54*5 = 360度)
        double d1 = 18.0 * DX_PI / 180.0;
        double d2 = 54.0 * DX_PI / 180.0;

        double current_angle = base_angle;
        for (int i = 0; i < 10; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = current_angle;
            pEnemyShot->speed = 3.0;
            // 銃符なので銃弾(5.0x2.0)を採用。色は青(4)
            pEnemyShot->kind = img_enemyShotBullet[4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 狭い・広いを交互に加算
            if (i % 2 == 0) {
                current_angle += d1;
            }
            else {
                current_angle += d2;
            }
        }

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 時計回りに回転（DxLibの座標系では角度増加が時計回り）
        base_angle += 2.0 * DX_PI / 180.0;
        if (base_angle > DX_PI * 2.0) {
            base_angle -= DX_PI * 2.0;
        }
    }

    // =========================================================
    // 弾幕B: 自機に向かって荒く、1辺の弾数が8の逆三角形を成した密な弾列
    // 5つ僅かに時間差を付けて撃つ。1セット撃ったら間髪空けずにもう1セット撃ち、少し間を空ける
    // =========================================================
    if (b_state == 0) {
        b_timer++;
        if (b_timer >= 60) { // 少し間を空ける
            b_state = 1;
            b_timer = 0;
            b_shot_count = 0;
        }
    }
    else if (b_state == 1 || b_state == 2) {
        b_timer++;
        if (b_timer >= 3) { // 僅かな時間差で発射
            b_timer = 0;

            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotStraight;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = 0.0;
            pEnemyShotSet->kind = 0;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            double aim_angle = atan2(player.y - enemy.y, player.x - enemy.x);

            // 1辺の弾数が8の逆三角形の格子点を生成
            // r=0(ボス側)で8発、r=7(自機側)で1発となるように配置
            for (int r = 0; r < 8; r++) {
                int num_shots = 8 - r;
                double y_offset = r * 16.0; // 進行方向の間隔（荒くするために広めに設定）

                for (int c = 0; c < num_shots; c++) {
                    // 垂直方向の間隔（中心揃え）
                    double x_offset = (c - (num_shots - 1) / 2.0) * 16.0;

                    // aim_angle 方向に座標を回転
                    double rx = x_offset * cos(aim_angle) - y_offset * sin(aim_angle);
                    double ry = x_offset * sin(aim_angle) + y_offset * cos(aim_angle);

                    sEnemyShot* pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = pEnemyShotSet->x + rx;
                    pEnemyShot->y = pEnemyShotSet->y + ry;
                    pEnemyShot->muki = aim_angle;
                    pEnemyShot->speed = 5.0; // 速く（荒く）
                    // 密な弾列なので視認性の高い小玉(2.5x2.5)を採用。色はシアン(3)
                    pEnemyShot->kind = img_enemyShotSmallBall[3];

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;

            b_shot_count++;
            if (b_shot_count >= 5) {
                b_shot_count = 0;
                if (b_state == 1) {
                    b_state = 2; // 間髪空けずにもう1セット
                }
                else {
                    b_state = 0; // 2セット終了したので少し間を空ける
                    b_timer = 0;
                }
            }
        }
    }
}