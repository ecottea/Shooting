#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//=============================================================
// サボテン棘乱舞 (Cactus Spine Barrage)
//-------------------------------------------------------------
// 使用素材:
//   ・幹・腕 : 中玉 緑 (img_enemyShotMediumBall[2])  speed=0 で固定
//   ・棘     : 銃弾 黄 (img_enemyShotBullet[1])     高速放射
// 効果音:
//   ・形成時 : sound_enemyShot_medium
//   ・棘発射 : sound_enemyShot_light
//=============================================================

// 弾幕パターン本体
static void ShotCactus(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int t = pEnemyShotSet->count;

    //---------------------------------------------------------
    // 1. 幹の形成 (t = 1〜36, 4フレームごとに1個)
    //---------------------------------------------------------
    if (t >= 1 && t <= 36 && (t - 1) % 4 == 0) {
        if (t == 1) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        int seg = (t - 1) / 4;          // 0〜8
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y + 20.0 + seg * 18.0;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;        // 固定
        pEnemyShot->kind = img_enemyShotMediumBall[2]; // 緑・中玉
        pEnemyShot->param_i[0] = 0;     // 0 = 幹
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    //---------------------------------------------------------
    // 2. 腕の形成 (t = 48〜 左右に分岐)
    //---------------------------------------------------------
    if (t == 48 || t == 52 || t == 56) {
        // 左腕
        pEnemyShot = new sEnemyShot;
        int armSeg = (t - 48) / 4;      // 0,1,2
        pEnemyShot->x = pEnemyShotSet->x - 18.0 - armSeg * 16.0;
        pEnemyShot->y = pEnemyShotSet->y + 55.0 + armSeg * 6.0;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotMediumBall[2];
        pEnemyShot->param_i[0] = 1;     // 1 = 腕
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 右腕
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x + 18.0 + armSeg * 16.0;
        pEnemyShot->y = pEnemyShotSet->y + 55.0 + armSeg * 6.0;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotMediumBall[2];
        pEnemyShot->param_i[0] = 1;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    //---------------------------------------------------------
    // 3. 棘の噴出 (t >= 80, 28フレームごとに放射)
    //---------------------------------------------------------
    if (t >= 80 && (t - 80) % 28 == 0 && t < 220) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 幹・腕の弾を走査し、その位置から棘を発射
        sEnemyShot* pBody = pEnemyShotSet->pEnemyShotHead->next;
        while (pBody != pEnemyShotSet->pEnemyShotHead) {
            if (pBody->param_i[0] == 0 || pBody->param_i[0] == 1) {
                // 中心からの放射方向を基準に少しばらつかせる
                double baseMuki = atan2(pBody->y - pEnemyShotSet->y,
                    pBody->x - pEnemyShotSet->x);
                // 各ボディ弾から 3 本の棘
                for (int i = 0; i < 3; i++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = pBody->x;
                    pEnemyShot->y = pBody->y;
                    // GetRand(40) → 0〜40 → -20〜+20 度相当
                    double offset = (GetRand(40) - 20) / 180.0 * DX_PI;
                    pEnemyShot->muki = baseMuki + offset;
                    pEnemyShot->speed = 3.2 + GetRand(80) / 100.0; // 3.2〜4.0
                    pEnemyShot->kind = img_enemyShotBullet[1];     // 黄・銃弾
                    pEnemyShot->param_i[0] = 2;                    // 2 = 棘
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
            pBody = pBody->next;
        }
    }

    //---------------------------------------------------------
    // 4. 終了処理 (t == 220) : 幹・腕を外側へ拡散させてクリア
    //---------------------------------------------------------
    if (t == 220) {
        sEnemyShot* pBody = pEnemyShotSet->pEnemyShotHead->next;
        while (pBody != pEnemyShotSet->pEnemyShotHead) {
            if (pBody->param_i[0] == 0 || pBody->param_i[0] == 1) {
                double baseMuki = atan2(pBody->y - pEnemyShotSet->y,
                    pBody->x - pEnemyShotSet->x);
                pBody->muki = baseMuki;
                pBody->speed = 1.5 + GetRand(50) / 100.0;
                pBody->param_i[0] = 3; // 拡散中
            }
            pBody = pBody->next;
        }
    }

    //---------------------------------------------------------
    // 弾の移動 (全弾共通)
    //---------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//=============================================================
// 敵本体パターン
//=============================================================
void EnemyPat_Cactus_Grok()
{
    static int patternSpawned = 0;

    if (count == 1) {
        // 画面中央やや上に固定配置
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;   // 変更禁止
        patternSpawned = 0;
    }

    // パターンを1回だけ生成（敵が生きている間継続）
    if (patternSpawned == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCactus;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        patternSpawned = 1;
    }

    // パターン終了後に再生成したい場合はここを有効化
     if (count > 1 && count % 280 == 0) {
         patternSpawned = 0;
     }
}