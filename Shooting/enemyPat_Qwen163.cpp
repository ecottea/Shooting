#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：氷華の六芒旋舞（雪の結晶モチーフ）
// ============================================================
static void ShotSnowCrystal(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 60度間隔で6方向に主軸（骨組み）を発射
        for (int i = 0; i < 6; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = pEnemyShotSet->muki + (i * 60.0) / 180.0 * DX_PI;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = angle;
            pShot->speed = 1.5; // ゆっくり進ませて線の視認性を高める
            pShot->kind = img_enemyShotMediumBall[6]; // 白、中玉
            pShot->param_i[0] = 0; // 0: 主軸フラグ
            pShot->param_i[1] = 0; // 枝分かれ進行フラグ

            // 双方向リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 既存の弾の移動と枝分かれ処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next; // 弾追加時にnextが壊れないよう保存

        if (pShot->param_i[0] == 0) {
            // 主軸の弾の移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 第1枝分かれ（count == 25 で左右45度にトゲを生やす）
            if (pShot->param_i[1] == 0 && pEnemyShotSet->count == 25) {
                for (int j = -1; j <= 1; j += 2) {
                    sEnemyShot* pBranch = new sEnemyShot;
                    pBranch->x = pShot->x;
                    pBranch->y = pShot->y;
                    pBranch->muki = pShot->muki + (j * 45.0) / 180.0 * DX_PI;
                    pBranch->speed = 2.2;
                    pBranch->kind = img_enemyShotScale[3]; // シアン、鱗弾（氷のトゲ表現）
                    pBranch->param_i[0] = 1; // 1: 枝フラグ

                    pBranch->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pBranch->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pBranch;
                    pEnemyShotSet->pEnemyShotHead->prev = pBranch;
                }
                pShot->param_i[1] = 1;
            }
            // 第2枝分かれ（count == 50 で先端にさらに細かい枝を生やす）
            else if (pShot->param_i[1] == 1 && pEnemyShotSet->count == 50) {
                for (int j = -1; j <= 1; j += 2) {
                    sEnemyShot* pBranch = new sEnemyShot;
                    pBranch->x = pShot->x;
                    pBranch->y = pShot->y;
                    pBranch->muki = pShot->muki + (j * 30.0) / 180.0 * DX_PI;
                    pBranch->speed = 1.8;
                    pBranch->kind = img_enemyShotSmallBall[4]; // 青、小玉（細かい氷粒表現）
                    pBranch->param_i[0] = 1; // 1: 枝フラグ

                    pBranch->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pBranch->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pBranch;
                    pEnemyShotSet->pEnemyShotHead->prev = pBranch;
                }
                pShot->param_i[1] = 2;
            }
        }
        else {
            // 枝の弾は単純な直進移動のみ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = nextShot;
    }
}

// ============================================================
// ラススパイク：凍てつく旋舞（結晶の爆発拡散）
// ============================================================
static void ShotSnowBurst(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 30度間隔で12方向に大玉を発射
        for (int i = 0; i < 12; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = (i * 30.0) / 180.0 * DX_PI;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = angle;
            pShot->speed = 3.5;
            pShot->kind = img_enemyShotLargeBall[3]; // シアン、大玉
            pShot->param_i[0] = 0;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        // 隙間を埋めるように短レーザーを発射
        for (int i = 0; i < 12; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = (i * 30.0 + 15.0) / 180.0 * DX_PI;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = angle;
            pShot->speed = 4.0;
            pShot->kind = img_enemyShotLaser[6]; // 白、短レーザー
            pShot->param_i[0] = 0;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Snowflake_Qwen()
{
    static int muki;
    static double rotation = 0.0;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0; // やや上めに配置し、下に広がる結晶を見やすくする
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        rotation = 0.0;
    }
    else {
        // 敵の移動（左右往復）
        enemy.x += 1.2 * (double)muki;
        if (count % 180 == 90) muki *= -1;

        // 弾幕全体の回転角度を更新（ゆっくりと回転）
        rotation += 0.015;
        if (rotation > DX_PI * 2.0) rotation -= DX_PI * 2.0;
    }

    // 雪の結晶弾幕の発射（120フレームごとに1セット生成）
    if (count % 30 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSnowCrystal;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = rotation; // 現在の回転角度を基準方向として渡す
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ラススパイク的な大爆発（例: count == 600 のときに1回だけ発動）
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSnowBurst;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
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