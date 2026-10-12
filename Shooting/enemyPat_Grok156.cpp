#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ルーローの三角形ケージ弾幕
// 3本の円弧を小玉で近似し、全体を回転させながら縮小→拡散を繰り返す
static void ShotReuleauxCage(sEnemyShotSet* pEnemyShotSet)
{
    // param_d[0] : 現在のスケール
    // param_d[1] : 現在の回転角度 (rad)
    // param_d[2] : 中心X
    // param_d[3] : 中心Y
    // param_d[4] : 基準サイズ (辺の長さ)
    // param_i[0] : フェーズ (0:縮小中, 1:拡散中)
    // param_i[1] : 色インデックス

    const int NUM_PER_ARC = 18/2;          // 1弧あたりの弾数
    const double ROT_SPEED = 0.018;      // 回転速度
    const int SHRINK_FRAMES = 160;       // 縮小にかかるフレーム
    const int EXPAND_FRAMES = 40;        // 拡散にかかるフレーム
    const double MIN_SCALE = 0.35/10;       // 最小スケール
    const double MAX_SCALE = 1.8;        // 拡散時の最大スケール

    if (pEnemyShotSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 初期パラメータ設定
        pEnemyShotSet->param_d[0] = 1.0;                    // scale
        pEnemyShotSet->param_d[1] = (GetRand(360) / 180.0) * DX_PI; // 初期回転角をランダムに
        pEnemyShotSet->param_d[2] = pEnemyShotSet->x;       // centerX
        pEnemyShotSet->param_d[3] = pEnemyShotSet->y;       // centerY
        pEnemyShotSet->param_d[4] = 130.0*2;                  // base size (辺長)
        pEnemyShotSet->param_i[0] = 0;                      // phase: shrink
        pEnemyShotSet->param_i[1] = 4;                      // 色: シアン (見やすい)

        double cx = pEnemyShotSet->param_d[2];
        double cy = pEnemyShotSet->param_d[3];
        double s = pEnemyShotSet->param_d[4];              // 辺の長さ = 弧の半径
        double R = s / sqrt(3.0);                          // 外接円半径 (中心→頂点)

        // 正三角形の頂点を計算 (上向き)
        double verts[3][2];
        for (int i = 0; i < 3; i++) {
            double ang = -DX_PI / 2.0 + i * (2.0 * DX_PI / 3.0);
            verts[i][0] = cx + R * cos(ang);
            verts[i][1] = cy + R * sin(ang);
        }

        // 3つの弧を生成
        for (int i = 0; i < 3; i++) {
            int j1 = (i + 1) % 3;
            int j2 = (i + 2) % 3;
            double vx = verts[i][0];
            double vy = verts[i][1];

            // 他の2頂点への角度
            double ang1 = atan2(verts[j1][1] - vy, verts[j1][0] - vx);
            double ang2 = atan2(verts[j2][1] - vy, verts[j2][0] - vx);

            // 短い方の弧 (60度) になるように差分を正規化
            double delta = ang2 - ang1;
            while (delta > DX_PI) delta -= 2.0 * DX_PI;
            while (delta < -DX_PI) delta += 2.0 * DX_PI;
            // 必ず ±60度付近になるはずなので、符号を保ったまま使う
            // (外向きの弧になる)

            for (int k = 0; k <= NUM_PER_ARC; k++) {
                double t = (double)k / (double)NUM_PER_ARC;
                double ang = ang1 + delta * t;

                double bx = vx + s * cos(ang);
                double by = vy + s * sin(ang);

                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = bx;
                pEnemyShot->y = by;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;                    // 位置は毎フレーム再計算するため移動しない
                pEnemyShot->kind = img_enemyShotSmallBall[pEnemyShotSet->param_i[1]];
                // 相対座標を保存 (形成時のスケール1.0基準)
                pEnemyShot->param_d[0] = bx - cx;
                pEnemyShot->param_d[1] = by - cy;
                pEnemyShot->margin = 240;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        return;
    }

    // ----- 毎フレーム更新 -----
    double& scale = pEnemyShotSet->param_d[0];
    double& rot = pEnemyShotSet->param_d[1];
    double  cx = pEnemyShotSet->param_d[2];
    double  cy = pEnemyShotSet->param_d[3];
    int& phase = pEnemyShotSet->param_i[0];

    // 回転は常に行う
    rot += ROT_SPEED;

    // フェーズ処理
    if (phase == 0) {
        // 縮小フェーズ
        double t = (double)pEnemyShotSet->count / (double)SHRINK_FRAMES;
        if (t > 1.0) t = 1.0;
        scale = 1.0 - t * (1.0 - MIN_SCALE);

        if (pEnemyShotSet->count >= SHRINK_FRAMES) {
            phase = 1;  // 拡散へ
        }
    }
    else {
        // 拡散フェーズ
        int expandCount = pEnemyShotSet->count - SHRINK_FRAMES;
        double t = (double)expandCount / (double)EXPAND_FRAMES;
        //if (t > 1.0) t = 1.0;
        scale = MIN_SCALE + t * (MAX_SCALE - MIN_SCALE);
        // 拡散後は弾が画面外に出てメインルーチンで消える想定
        // ショットセット自体は残るが、弾がなくなれば自然消滅に近い
    }

    // 全弾の位置を回転＋スケールで再配置
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double rx = pShot->param_d[0];
        double ry = pShot->param_d[1];

        // 回転
        double cosR = cos(rot);
        double sinR = sin(rot);
        double rx2 = rx * cosR - ry * sinR;
        double ry2 = rx * sinR + ry * cosR;

        // スケール
        pShot->x = cx + rx2 * scale;
        pShot->y = cy + ry2 * scale;

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_ReuleauxTriangle_Grok()
{
    static int cage_timer;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;   // 200から変更しないこと
        cage_timer = 0;
    }
    else {
        // 軽く左右に揺れる
        enemy.x = 240.0 + 40.0 * sin(count * 0.02);
    }

    // 一定間隔で新しいケージを生成
    // 縮小160 + 拡散40 + 余裕 = 約220フレームごと
    if (count == 1 || (count > 30 && (count - 30) % 220 == 0)) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotReuleauxCage;
        pEnemyShotSet->x = player.x;
        pEnemyShotSet->y = player.y;  // 敵の少し下を中心に
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
}