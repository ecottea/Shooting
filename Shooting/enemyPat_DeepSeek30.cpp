// ShotgunPattern.cpp
// 散華掃射（スキャッターグレイズ）の実装
// 敵本体パターン EnemyPat_Shotgun_DeepSeek() を含む単一ファイル

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ショットガン散弾のパターン関数（ShotSetごとに呼ばれる）
static void ShotgunPattern(sEnemyShotSet* pSet)
{
    sEnemyShot* p;
    if (pSet->count == 0) {
        // ----- 発射時：親弾（スラッグ弾）を1発生成 -----
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        p = new sEnemyShot;
        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = pSet->muki;                       // 自機狙い（呼び出し元で設定）
        p->speed = 2.5;                              // かなり遅め
        p->kind = img_enemyShotMediumBall[0];        // 赤い中玉（スラッグ弾を表現）
        p->margin = 20.0;

        // 炸裂判定用：発射位置を記録し、フラグを0に
        p->param_d[0] = p->x;
        p->param_d[1] = p->y;
        p->param_i[0] = 0;                           // 0:親弾, -1:針弾

        // 双方向リンクリストの末尾に追加
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
        return;
    }

    // ----- 毎フレームの更新 -----
    p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* next = p->next;   // 後で消すかもしれないので次を退避

        if (p->param_i[0] == 0) {
            // ---- 親弾：一定距離移動で炸裂 ----
            double dx = p->x - p->param_d[0];
            double dy = p->y - p->param_d[1];
            if (dx * dx + dy * dy >= 160.0 * 160.0) {   // 画面の1/3程度
                // 炸裂音
                if (CheckSoundMem(sound_enemyShot_heavy) == 1)
                    StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                // 8方向弾の角度（ラジアン）: 中心は -5°,5° など、左右へ広がる
                const double dirAngles[8] = {
                    -30.0 * DX_PI / 180.0,
                    -20.0 * DX_PI / 180.0,
                    -10.0 * DX_PI / 180.0,
                    -5.0 * DX_PI / 180.0,
                     5.0 * DX_PI / 180.0,
                    10.0 * DX_PI / 180.0,
                    20.0 * DX_PI / 180.0,
                    30.0 * DX_PI / 180.0
                };

                // 8方向弾を生成（すべて等速、やや速め）
                for (int i = 0; i < 8; i++) {
                    sEnemyShot* pellet = new sEnemyShot;
                    pellet->x = p->x;
                    pellet->y = p->y;
                    pellet->muki = p->muki + dirAngles[i];
                    pellet->speed = 6.0;
                    pellet->kind = img_enemyShotBullet[6];   // 白い銃弾（金属ペレット）
                    pellet->margin = 20.0;
                    pellet->param_i[0] = -1;                // 針弾フラグ

                    pellet->prev = pSet->pEnemyShotHead->prev;
                    pellet->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pellet;
                    pSet->pEnemyShotHead->prev = pellet;
                }

                // ランダム拡散16発：±15°の範囲、速度にばらつき
                for (int i = 0; i < 16; i++) {
                    sEnemyShot* pellet = new sEnemyShot;
                    pellet->x = p->x;
                    pellet->y = p->y;
                    double angleOffset = (GetRand(300) - 150) / 10.0 * DX_PI / 180.0; // -15°～+15°
                    pellet->muki = p->muki + angleOffset;
                    pellet->speed = (200 + GetRand(200)) / 100.0 + 2;   // 2.0 ～ 4.0
                    pellet->kind = img_enemyShotBullet[6];
                    pellet->margin = 20.0;
                    pellet->param_i[0] = -1;

                    pellet->prev = pSet->pEnemyShotHead->prev;
                    pellet->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pellet;
                    pSet->pEnemyShotHead->prev = pellet;
                }

                // 親弾を画面外へ飛ばして消去（メインルーチンが自動除去）
                p->x = -1000.0;
                p->param_i[0] = 1;   // 処理済みフラグ
            }
        }
        else {
            // ---- 針弾：減速と寿命管理 ----
            // 減速
            p->speed *= 0.98;
            if (p->speed < 0.1) p->speed = 0.0;

            // 一定時間（180フレーム）で消える（画面外へ）
            if (p->count > 180) {
                p->x = -1000.0;
            }
        }

        // 位置更新（親弾も針弾も共通）
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);

        p = next;
    }
}

// ==================================================
// 敵本体パターン（散華掃射）
// ==================================================
void EnemyPat_Shotgun_DeepSeek()
{
    static int muki;       // 左右移動の向き
    static int shot_kind;  // 発射パターンのバリエーション用（今回は特に使わない）

    if (count == 1) {
        // 初期配置
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_kind = 0;
    }
    else {
        // 左右移動（120フレームごとに反転）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 60フレームに1回、3方向へショットガン発射
    if (count % 30 == 0) {
        // 中央：自機狙い
        double baseAngle = atan2(player.y - enemy.y, player.x - enemy.x) + (GetRand(60) - 30) * 0.005;
        double angles[3] = {
            baseAngle - 15.0 * DX_PI / 180.0,   // 左
            baseAngle,                           // 中央
            baseAngle + 15.0 * DX_PI / 180.0    // 右
        };

        for (int i = 0; i < 3; i++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotgunPattern;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = angles[i];
            pSet->kind = shot_kind++;   // 異なるkindを与える（見た目用に将来拡張可）

            // 双方向リンクリストの初期化
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            // 全体リストに追加
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}