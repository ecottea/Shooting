// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：遺伝子複製（ダブルヘリックス・リプリケーション）
static void ShotDNAHelix(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    double baseAngle = DX_PI / 2.0; // 下方向を基準角度とする
    double t = pEnemyShotSet->count * 0.1; // 螺旋の回転位相
    int interval = 2; // 骨格弾の発射間隔（3フレームに1回）

    // 予告音（セット生成直後の1回のみ）
    //if (pEnemyShotSet->count == 0) {
    //    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
    //    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    //}

    // =========================================
    // 骨格弾（螺旋を描く弾）の発射処理
    // =========================================
    if (pEnemyShotSet->count % interval == 0) {
        // フェーズ1：二重螺旋の形成 (0〜179フレーム)
        if (pEnemyShotSet->count < 180) {
            double a1 = baseAngle + sin(t) * 0.8;
            double a2 = baseAngle + sin(t + DX_PI) * 0.8;

            // シアンの骨格弾
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = a1;
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotMediumOval[3];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // マゼンタの骨格弾
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = a2;
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotMediumOval[5];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            if (!CheckSoundMem(sound_enemyShot_heavy)) {
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            }
        }
        // フェーズ2：螺旋の解裂 (180〜299フレーム)
        else if (pEnemyShotSet->count < 300) {
            double offset = 0.8 - 0.4;
            double a1 = baseAngle - offset + sin(t) * 0.4;
            double a2 = baseAngle + offset + sin(t) * 0.4;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = a1;
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotMediumOval[3];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = a2;
            pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotMediumOval[5];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        // フェーズ3：複製と増殖 (300〜479フレーム)
        else if (pEnemyShotSet->count < 480) {
            double offset1 = 0.8 - 0.4;
            double offset2 = 1.2 - 0.4;

            double a1 = baseAngle - offset2 + sin(t) * 0.4;
            double a2 = baseAngle - offset1 + sin(t + DX_PI) * 0.4;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x; pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = a1; pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotMediumOval[3];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x; pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = a2; pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotMediumOval[5];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            double a3 = baseAngle + offset1 + sin(t) * 0.4;
            double a4 = baseAngle + offset2 + sin(t + DX_PI) * 0.4;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x; pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = a3; pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotMediumOval[3];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x; pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = a4; pEnemyShot->speed = 2.5;
            pEnemyShot->kind = img_enemyShotMediumOval[5];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // =========================================
    // 塩基対弾（架橋する弾）の発射処理
    // =========================================
    if (pEnemyShotSet->count % 9 == 0) {
        if (pEnemyShotSet->count < 180) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle;
            pEnemyShot->speed = 5.0;
            pEnemyShot->kind = (pEnemyShotSet->count / 9 % 2 == 0) ? img_enemyShotSmallBall[1] : img_enemyShotSmallBall[2];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            if (!CheckSoundMem(sound_enemyShot_light)) {
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
        }
        else if (pEnemyShotSet->count < 300) {
            double randAngle = baseAngle + (GetRand(160) - 80) / 100.0;
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = randAngle;
            pEnemyShot->speed = 4.0;
            pEnemyShot->kind = img_enemyShotSmallBall[1];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        else if (pEnemyShotSet->count < 480) {
            double offsetL = -(0.8 + 1.2) / 2.0;
            double offsetR = (0.8 + 1.2) / 2.0;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x; pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + offsetL;
            pEnemyShot->speed = 5.0;
            pEnemyShot->kind = img_enemyShotSmallBall[1];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x; pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + offsetR;
            pEnemyShot->speed = 5.0;
            pEnemyShot->kind = img_enemyShotSmallBall[2];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            if (!CheckSoundMem(sound_enemyShot_light)) {
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}


// 敵本体のパターン
void EnemyPat_DNA_Zai()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    if (count % 480 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDNAHelix;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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