// enemyPat_Shuriken.cpp
// 手裏剣モチーフの弾幕「四方手裏剣の舞」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------------
// 手裏剣パターン本体（ショットセットのパターン関数として呼ばれる）
// ------------------------------------------------------------------
static void ShurikenPattern(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    sEnemyShot* pEnemyShotCenter;

    // ---- 初回呼び出し：手裏剣群を生成 ---------------------------------
    if (pEnemyShotSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 波のパラメータ（pEnemyShotSet の kind で切り替え）
        double wave_speed = 2.0;       // 外向きの速度（ドット/フレーム）
        double rot_speed = 15.0 * DX_PI / 180.0 / 60.0 * 4; // 回転速度（ラジアン/フレーム）
        double offset_angle = 0.0;       // 角度オフセット（ラジアン）
        bool   clockwise = true;      // 回転方向

        if (pEnemyShotSet->kind == 1) {
            wave_speed = 3.5;
            rot_speed = 20.0 * DX_PI / 180.0 / 60.0 * 4;
            offset_angle = 15.0 * DX_PI / 180.0;
            clockwise = false;
        }
        if (!clockwise) rot_speed = -rot_speed;

        const int shuriken_num = 18;        // 手裏剣の数
        const double spawn_radius = 40.0;   // ボスからの発生距離

        for (int i = 0; i < shuriken_num; ++i) {
            double angle = i * (2.0 * DX_PI / shuriken_num) + offset_angle;

            // ---------- 中心弾 ----------
            pEnemyShotCenter = new sEnemyShot;
            pEnemyShotCenter->x = pEnemyShotSet->x + cos(angle) * spawn_radius;
            pEnemyShotCenter->y = pEnemyShotSet->y + sin(angle) * spawn_radius;
            pEnemyShotCenter->muki = angle;
            pEnemyShotCenter->speed = wave_speed;
            pEnemyShotCenter->kind = img_enemyShotSmallBall[6]; // 白の小玉
            // 自由パラメータ
            pEnemyShotCenter->param_i[0] = (int)(intptr_t)pEnemyShotCenter; // 自分自身（未使用）
            pEnemyShotCenter->param_i[1] = 0;   // 種類：中心弾
            pEnemyShotCenter->param_d[0] = 0.0; // 現在の回転角
            pEnemyShotCenter->param_d[1] = rot_speed;
            pEnemyShotCenter->param_d[2] = wave_speed;
            pEnemyShotCenter->param_d[3] = angle;

            pEnemyShotCenter->margin = 200;

            // 連結リストに追加（末尾挿入 → 中心弾が先頭になる）
            pEnemyShotCenter->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShotCenter->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShotCenter;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotCenter;

            // ---------- 刃の弾（上下左右 × 2個） ----------
            const double blade_dirs[4] = { 0.0, DX_PI / 2.0, DX_PI, -DX_PI / 2.0 }; // 右、下、左、上
            const double blade_dists[5] = { 8.0, 16.0, 24.0, 32.0, 40.0 }; // 中心からの距離2段階

            for (int d = 0; d < 4; ++d) {
                for (int dist_i = 0; dist_i < 5; ++dist_i) {
                    pEnemyShot = new sEnemyShot;
                    // 初期位置（まだ回転していないので local 方向そのまま）
                    double lx = cos(blade_dirs[d]) * blade_dists[dist_i];
                    double ly = sin(blade_dirs[d]) * blade_dists[dist_i];
                    pEnemyShot->x = pEnemyShotCenter->x + lx;
                    pEnemyShot->y = pEnemyShotCenter->y + ly;
                    pEnemyShot->muki = 0.0;   // 個別の速度は使わない
                    pEnemyShot->speed = 0.0;
                    pEnemyShot->kind = img_enemyShotDiamond[6]; // 白の菱形弾

                    long long ptr_val = (long long)(intptr_t)pEnemyShotCenter;     // 親の中心弾
                    pEnemyShot->param_i[0] = (int)(ptr_val & 0xFFFFFFFF);          // 下位32bit
                    pEnemyShot->param_i[1] = (int)((ptr_val >> 32) & 0xFFFFFFFF);  // 上位32bit

                    pEnemyShot->param_i[2] = 1;   // 種類：刃弾
                    pEnemyShot->param_d[0] = blade_dirs[d]; // ローカル角度
                    pEnemyShot->param_d[1] = blade_dists[dist_i]; // 距離

                    // 連結リストに追加
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
        return;
    }

    // ---- 2フレーム以降：全弾の移動と回転 --------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[2] == 0) {
            // ★ 中心弾
            // 直線移動
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            // 回転角を進める
            pEnemyShot->param_d[0] += pEnemyShot->param_d[1];
        }
        else {
            // ★ 刃弾
            // 親の中心弾を取得
            long long ptr_val = ((long long)pEnemyShot->param_i[1] << 32) | (pEnemyShot->param_i[0] & 0xFFFFFFFFULL);
            sEnemyShot* center = (sEnemyShot*)(intptr_t)ptr_val;
            double local_angle = pEnemyShot->param_d[0];
            double dist = pEnemyShot->param_d[1];
            double rot = center->param_d[0];
            double world_angle = local_angle + rot;

            pEnemyShot->x = center->x + dist * cos(world_angle);
            pEnemyShot->y = center->y + dist * sin(world_angle);
            // 向き・速度は不要（位置を直接上書きしている）

            pEnemyShot->muki = local_angle;
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// ------------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------------
void EnemyPat_Shuriken_DeepSeek()
{
    static int muki;
    static int wave_timer;   // 次の波を発射するまでのフレーム

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        wave_timer = 0;       // 即座に1波目を撃つ
    }
    else {
        // 横移動（サンプルと同程度の動き）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 波の発射タイミング
    --wave_timer;
    if (wave_timer <= 0) {
        // 交互に2種類の波を撃つ
        static int wave_kind = 0;
        int kind = wave_kind % 2;   // 0→波1, 1→波2

        // ショットセット作成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShurikenPattern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;   // 今回は未使用
        pEnemyShotSet->kind = kind;  // 波の種類を伝える

        // 弾の連結リスト（ヘッダ）作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルなショットセットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 次波までの間隔を設定（約0.6秒：36フレーム）
        wave_timer = 36;
        ++wave_kind;
    }
}