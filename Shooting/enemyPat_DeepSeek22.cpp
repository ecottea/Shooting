// enemyPat_Tmp.cpp
// 音ゲーをモチーフにした弾幕パターン
//   - 敵が左右に揺れながら、画面下方向へ落ちる「ノーツ」弾をビートに合わせて発射する
//   - 8フレームごとにランダムなレーンの単ノート
//   - 32フレームごとに和音（3way）を発射

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 5つのレーン座標（画面幅 480 に収まるよう配置）
static const double laneX[5] = { 80.0, 160.0, 240.0, 320.0, 400.0 };
// レーンに対応する色（弾の色番号 0～7）
static const int    laneColor[5] = { 0, 1, 2, 4, 6 }; // 赤,黄,緑,青,白

// ノーツ落下パターン
static void ShotNoteFall(sEnemyShotSet* pEnemyShotSet)
{
    // 初回のみ効果音再生
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    int cnt = pEnemyShotSet->count;

    // ---- 8フレーム（ビート）ごとに単ノーツを1つ発射 ----
    if (cnt > 0 && cnt % 4 == 0) {
        int idx = GetRand(4);               // 0～4 のレーンインデックス
        double sx = laneX[idx];
        double sy = pEnemyShotSet->y;

        sEnemyShot* p = new sEnemyShot;
        p->x = sx;
        p->y = sy;
        p->muki = DX_PI / 2.0;             // 真下
        p->speed = 2.5 + (idx * 0.3);       // レーンごとに微妙な速度差
        p->kind = img_enemyShotBullet[laneColor[idx]];

        // 双方向循環リストに挿入
        p->prev = pEnemyShotSet->pEnemyShotHead->prev;
        p->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = p;
        pEnemyShotSet->pEnemyShotHead->prev = p;
    }

    // ---- 32フレーム（強拍）ごとに和音（3-way）を発射 ----
    if (cnt > 0 && cnt % 8 == 0) {
        int idx = GetRand(4);
        double baseX = laneX[idx];
        double sy = pEnemyShotSet->y;

        for (int i = -2; i <= 2; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = baseX + i * 18.0;   // 左右に少しずらす
            p->y = sy;
            p->muki = DX_PI / 2.0;
            p->speed = 2.5;
            p->kind = img_enemyShotMediumBall[laneColor[idx]];

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // ---- 既存の弾を移動 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_RhythmGame_DeepSeek()
{
    static double moveDir;      // 左右移動の向き (1:右, -1:左)

    if (count == 1) {
        // 初期配置
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1.0;

        // ノーツ落下用の ShotSet をひとつだけ生成（永続）
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotNoteFall;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;          // 弾の発生Y座標
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x); // ダミー

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 全体の ShotSet リストに登録
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else {
        // 左右に緩やかに移動、さらに上下に微小振動をつけてリズム感を演出
        enemy.x += moveDir * 0.9;
        if (enemy.x <= 70.0)  moveDir = 1.0;
        if (enemy.x >= 410.0) moveDir = -1.0;

        enemy.y = 60.0 + 8.0 * sin(count * 0.04);   // 上下の揺れ
    }
}