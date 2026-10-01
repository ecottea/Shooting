#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// Hilbert曲線 (空間充填曲線) のインデックス→座標変換
// n は 2^order、d は 0 ～ n*n-1
static void d2xy(int n, int d, int* x, int* y)
{
    int rx, ry, s, t = d;
    *x = *y = 0;
    for (s = 1; s < n; s *= 2) {
        rx = 1 & (t / 2);
        ry = 1 & (t ^ rx);
        if (ry == 0) {
            if (rx == 1) {
                *x = s - 1 - *x;
                *y = s - 1 - *y;
            }
            // x と y を入れ替え
            int tmp = *x;
            *x = *y;
            *y = tmp;
        }
        *x += s * rx;
        *y += s * ry;
        t /= 4;
    }
}

// 弾幕：ペアノ曲線モチーフの空間充填弾幕「ペアノ・ウェブ」
// （実装は計算が軽く連続性の高い Hilbert 曲線を使用。視覚的に平面を埋め尽くす軌跡になる）
static void ShotPeanoWeb(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初期化（count==0 のときのみ）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // param_i[0] : 現在のステップ (d)
        // param_i[1] : 次数 (order)  4→16x16=256点 / 5→32x32=1024点
        // param_i[2] : 最大ステップ数
        // param_i[3] : フェーズ (0:生成中  1:展開中)
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 5;          // order=5 で画面をかなり埋める
        int n = 1 << pEnemyShotSet->param_i[1];
        pEnemyShotSet->param_i[2] = n * n;
        pEnemyShotSet->param_i[3] = 0;

        // 画面内に収めるためのパラメータ
        pEnemyShotSet->param_d[0] = 40.0;       // マージン
        pEnemyShotSet->param_d[1] = 400.0;      // 有効サイズ (480-40*2)
    }

    // 生成フェーズ：曲線上に少しずつ弾を配置していく
    if (pEnemyShotSet->param_i[3] == 0) {
        // 1フレームに2発程度生成（密度と時間のバランス）
        for (int spawn = 0; spawn < 2; spawn++) {
            if (pEnemyShotSet->param_i[0] >= pEnemyShotSet->param_i[2]) break;

            int n = 1 << pEnemyShotSet->param_i[1];
            int hx, hy;
            d2xy(n, pEnemyShotSet->param_i[0], &hx, &hy);

            // グリッド座標を画面座標に変換
            double bx = pEnemyShotSet->param_d[0] + (hx / (double)(n - 1)) * pEnemyShotSet->param_d[1];
            double by = pEnemyShotSet->param_d[0] + (hy / (double)(n - 1)) * pEnemyShotSet->param_d[1];

            // わずかな揺らぎを加えて完全な格子感を消す
            bx += (GetRand(6) - 3) * 0.4;
            by += (GetRand(6) - 3) * 0.4;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = bx;
            pEnemyShot->y = by;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;            // 最初は静止して「ウェブ」を形成

            // 色は進行に合わせて少し変化（シアン寄り→青寄り）
            int col = 3 + (pEnemyShotSet->param_i[0] * 2 / pEnemyShotSet->param_i[2]); // 3 or 4
            if (col > 4) col = 4;
            pEnemyShot->kind = img_enemyShotSmallBall[col];

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            pEnemyShotSet->param_i[0]++;
        }

        // 全点配置完了 → 展開フェーズへ
        if (pEnemyShotSet->param_i[0] >= pEnemyShotSet->param_i[2]) {
            pEnemyShotSet->param_i[3] = 1;

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            // 既存の全弾を中心から外側へゆっくり押し出す
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                double dx = pShot->x - 240.0;
                double dy = pShot->y - 240.0;
                pShot->muki = atan2(dy, dx);
                // 速度に少しばらつきを持たせる
                pShot->speed = 0.55 + GetRand(40) / 100.0;
                pShot = pShot->next;
            }
        }
    }

    // 毎フレームの移動処理（速度0のときは止まっている）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Tmp()
{
    static int move_dir;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        move_dir = 1;
    }
    else {
        // ゆっくり左右に移動（弾幕の主役を邪魔しない程度）
        enemy.x += 0.6 * (double)move_dir;
        if (enemy.x < 120.0) move_dir = 1;
        if (enemy.x > 360.0) move_dir = -1;
    }

    // パターン開始（1回だけショットセットを生成）
    if (count % 600 == 20) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPeanoWeb;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 空の循環リストを用意
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // ショットセットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
