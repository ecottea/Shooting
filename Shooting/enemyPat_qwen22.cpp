// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン1：16分音符・加速蛇行ノーツ
// 初速は遅いが加速し、かつ左右に蛇行する。
// ============================================================
static void ShotMelodyChase(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 効果音（刻み）
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;

        // プレイヤー狙いだが、少しランダム性を加える
        double target_muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
        pEnemyShot->muki = target_muki + (GetRand(30) - 15) * 0.01;

        pEnemyShot->speed = 1.8; // 初速は遅め
        pEnemyShot->kind = img_enemyShotBullet[pEnemyShotSet->kind % 8]; // ビートに応じて色を変える

        // 蛇行の位相をずらすためにcountを初期化（任意）
        pEnemyShot->param_i[0] = pEnemyShot->count;

        // リスト登録
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 1. 加速運動 (時間経過で速度UP)
        pEnemyShot->speed += 0.06;

        // 2. 蛇行運動 (サイン波で角度を揺らす)
        // count * 0.15 が周波数、0.04 が振幅
        pEnemyShot->muki += -cos((pEnemyShot->count - pEnemyShot->param_i[0]) * 0.15) * 0.04;

        // 3. 座標更新
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 弾幕パターン2：収縮する螺旋スライダー
// 回転しながら半径が縮み、プレイヤーを押しつぶそうとする。
// ============================================================
static void ShotShrinkingSpiral(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 効果音（重め）
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 螺旋の腕の数
        int arms = 3;
        // 1つの腕あたりの弾数
        int density = 15;

        for (int i = 0; i < arms; i++) {
            for (int j = 0; j < density; j++) {
                pEnemyShot = new sEnemyShot;

                // 極座標パラメータ設定
                // param_d[0]: 中心X, param_d[1]: 中心Y
                // param_d[2]: 初期角度, param_d[3]: 初期半径
                pEnemyShot->param_d[0] = pEnemyShotSet->x;
                pEnemyShot->param_d[1] = pEnemyShotSet->y;
                pEnemyShot->param_d[2] = (DX_PI * 2.0 / arms) * i + (j * 0.3); // 角度
                pEnemyShot->param_d[3] = 30.0 + j * 15.0;                      // 半径（外側へ伸びる）

                // 座標計算
                pEnemyShot->x = pEnemyShot->param_d[0] + pEnemyShot->param_d[3] * cos(pEnemyShot->param_d[2]);
                pEnemyShot->y = pEnemyShot->param_d[1] + pEnemyShot->param_d[3] * sin(pEnemyShot->param_d[2]);

                pEnemyShot->speed = 0.0; // 自前制御のため0
                pEnemyShot->kind = img_enemyShotScale[3]; // シアンの鱗弾

                // リスト登録
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double angle = pEnemyShot->param_d[2];
        double radius = pEnemyShot->param_d[3];

        // 回転
        angle += 0.05;
        // 縮小 (中心へ寄りながら縮む)
        radius -= 1.2;

        // 半径が0以下になったら消す（あるいは反転させる）
        if (radius < 0) {
            // 今回は簡単のため、縮小しきったら速度を持って直線運動させる
            pEnemyShot->speed = 3.0;
            pEnemyShot->muki = angle;
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        else {
            // 極座標更新
            double nx = pEnemyShot->param_d[0] + radius * cos(angle);
            double ny = pEnemyShot->param_d[1] + radius * sin(angle);
            pEnemyShot->muki = atan2(ny - pEnemyShot->y, nx - pEnemyShot->x);
            pEnemyShot->x = nx;
            pEnemyShot->y = ny;

            pEnemyShot->param_d[2] = angle;
            pEnemyShot->param_d[3] = radius;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_RhythmGame_Qwen()
{
    // BPM 150 相当 (60fps時 1beat = 24frames)
    const int BEAT_FRAMES = 24;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 硬め
    }
    else {
        // リズムに合わせて左右へワープ（ステップ）
        // 2beat (48frames) ごとに位置を入れ替える
        if (count % (BEAT_FRAMES * 2) == 0) {
            // 左(100) か 右(380) へテレポート
            enemy.x = ((count / (BEAT_FRAMES * 4)) % 2 == 0) ? 100.0 : 380.0;
            enemy.y = 60.0 + GetRand(40); // Yも少しずらす
        }
    }

    // --------------------------------------------------------
    // 16分音符刻み (6framesごと) にメロディ（加速ノーツ）発射
    // --------------------------------------------------------
    if (count % 6 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMelodyChase;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0;

        // ビート番号をkindに渡して色変化に利用
        pEnemyShotSet->kind = (count / BEAT_FRAMES);

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // --------------------------------------------------------
    // 1beatごと (24framesごと) に螺旋スライダー展開
    // --------------------------------------------------------
    if (count % BEAT_FRAMES == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShrinkingSpiral;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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
}