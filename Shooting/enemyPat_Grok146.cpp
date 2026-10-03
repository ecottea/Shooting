#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 虹色スペクトル（赤→橙→黄→緑→シアン→青→マゼンタ）
static const int spectrum[7] = { 0, 8, 1, 2, 3, 4, 5 };

// プリズム分光弾幕
// 白い中玉を密集させて太い光線を作り、一定距離で虹色に分光して扇状に広げる
static void ShotPrismDispersion(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // チャージ音
    //if (pEnemyShotSet->count == 0) {
    //    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
    //    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    //}

    // チャージ後に白色光線を一斉生成
    if (pEnemyShotSet->count == 28) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double base_muki = pEnemyShotSet->muki;
        double perp = base_muki + DX_PI / 2.0;

        // 太い白色光線（中玉を密集配置）
        for (int i = 0; i < 21; i++) {
            pEnemyShot = new sEnemyShot;

            // 進行方向に対して垂直にオフセットして厚みを出す
            double offset = (i - 10) * 1.15;
            pEnemyShot->x = pEnemyShotSet->x + offset * cos(perp);
            pEnemyShot->y = pEnemyShotSet->y + offset * sin(perp);
            pEnemyShot->muki = base_muki;
            pEnemyShot->speed = 5.2;                       // 高速な白い光線
            pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白
            pEnemyShot->param_i[0] = i % 7;                // 分光後の色インデックス
            pEnemyShot->param_i[1] = 0;                    // 未分光フラグ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム移動と分光判定
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 発射点からの距離で分光（プリズム面に到達した瞬間）
        if (pShot->param_i[1] == 0) {
            double dx = pShot->x - pEnemyShotSet->x;
            double dy = pShot->y - pEnemyShotSet->y;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist > 145.0) {
                pShot->param_i[1] = 1; // 分光済み

                int col_idx = pShot->param_i[0];
                // 中央を0として左右に約±30度の扇形に分光
                double angle_offset = (col_idx - 3) * (DX_PI / 18.0);
                pShot->muki += angle_offset;

                // 虹色の中玉に変化
                pShot->kind = img_enemyShotMediumBall[spectrum[col_idx]];

                // 色ごとにわずかに速度差を付けて自然な広がりを出す
                pShot->speed = 2.7 + col_idx * 0.12;
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_Prism_Grok()
{
    static int muki_dir;
    static int shot_timer;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki_dir = 1;
        shot_timer = 0;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.75 * (double)muki_dir;
        if (enemy.x < 70.0) {
            enemy.x = 70.0;
            muki_dir = 1;
        }
        else if (enemy.x > 410.0) {
            enemy.x = 410.0;
            muki_dir = -1;
        }
    }

    // 一定間隔でプリズム光線を発射（自機狙い）
    if (count >= 30 && (count - 30) % 40 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPrismDispersion;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_timer++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}