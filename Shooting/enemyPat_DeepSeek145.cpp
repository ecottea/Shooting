// EnemyPat_Escape_DeepSeek.cpp
// 弾幕：逃げ影弾幕「八方逃げ火」
// ボスは臆病なAI。被弾すると残像を残して逃げ、残像は時間差で自機狙い弾を撃つ。
// 逃走経路には遅延機雷が残る。撃たなすぎると「いじけ溜め」から大弾幕。
// 
// 使用素材：
//   効果音：sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyCharge
//   弾画像：img_enemyShotSmallBall[], img_enemyShotMediumBall[]
//   色インデックス：0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒, 8:橙
// 
// メインルーチンが count, pEnemyShotSet->count, pEnemyShot->count のインクリメントと
// 画面外弾の消去を行う。このファイルではインクリメントしない。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 残像・機雷の同時存在数管理（ファイルスコープ）
static int afterimageCount = 0;
static int mineCount = 0;

//============================================================
// 残像：1.2秒後に3way自機狙い弾を発射
//============================================================
static void ShotAfterimage(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 72) { // 1.2秒後
        if (afterimageCount > 0) afterimageCount--;

        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            double baseAngle = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->muki = baseAngle + i * 10.0 / 180.0 * DX_PI;
            pShot->speed = 3.0;
            pShot->kind = img_enemyShotSmallBall[5]; // マゼンタ小玉
            pShot->margin = 240;
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 弾移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//============================================================
// 機雷：0.8秒後に6方向へ小弾を発射
//============================================================
static void ShotMine(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 48) { // 0.8秒後
        if (mineCount > 0) mineCount--;

        for (int i = 0; i < 6*2; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = i * 60.0 / 180.0/2 * DX_PI;
            pShot->speed = 1.5;
            pShot->kind = img_enemyShotSmallBall[0]; // 赤小玉
            pShot->margin = 240;
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 弾移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//============================================================
// 通常攻撃：自機狙い1発
//============================================================
static void ShotSingleAim(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki;
        pShot->speed = 3.0;
        pShot->kind = img_enemyShotMediumBall[1]; // 黄中玉
        pShot->margin = 240;
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//============================================================
// いじけ溜め：全方位リング＋回転螺旋
//============================================================
static void ShotChargeAttack(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 全方位リング24発
        for (int i = 0; i < 24*2; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = i * 15.0/2 / 180.0 * DX_PI;
            pShot->speed = 2.0;
            pShot->kind = img_enemyShotMediumBall[5]; // マゼンタ中玉
            pShot->margin = 240;
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 回転螺旋：10フレームごとに3発、6回
    if (pEnemyShotSet->count % 10 == 0 && pEnemyShotSet->count < 60) {
        double baseAngle = pEnemyShotSet->count * 7.0 / 180.0 * DX_PI;
        for (int i = 0; i < 3*2; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = baseAngle + i/2 * 120.0 / 180.0 * DX_PI;
            pShot->speed = 2.5;
            pShot->kind = img_enemyShotSmallBall[4]; // 青小玉
            pShot->margin = 240;
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

//============================================================
// 敵本体パターン：逃げ影弾幕「八方逃げ火」
//============================================================
void EnemyPat_Escape_DeepSeek()
{
    static int state = 0;          // 0:通常 1:逃走ダッシュ 2:壁際 3:いじけ溜め
    static int muki = 1;
    static int shot_count = 0;
    static int prevHp = 200;
    static int damageAccum = 0;
    static int hitAccum = 0;
    static int escapeCooldown = 0;
    static int lastHitCount = 0;
    static int dashTimer = 0;
    static double dashVx = 0.0, dashVy = 0.0;
    static int wallSlideDir = 1;
    static int chargeTimer = 0;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 100; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
        prevHp = enemy.hp;
        damageAccum = 0;
        hitAccum = 0;
        escapeCooldown = 0;
        lastHitCount = 0;
        state = 0;
        dashTimer = 0;
        chargeTimer = 0;
        afterimageCount = 0;
        mineCount = 0;
    }

    // 被弾検知
    bool isHit = false;
    if (enemy.hp < prevHp) {
        isHit = true;
    }

    if (isHit && state != 1) { // 逃走中は無敵とみなし被弾処理しない
        int damage = prevHp - enemy.hp;
        damageAccum += damage;
        hitAccum++;
        lastHitCount = count;

        if (state == 0 && escapeCooldown <= 0) {
            if (damageAccum >= 100 || hitAccum >= 3) {
                // 逃走開始
                state = 1;
                dashTimer = 9; // 0.15秒
                double baseAngle = atan2(enemy.y - player.y, enemy.x - player.x);
                double randAngle = (GetRand(60) - 30) / 180.0 * DX_PI; // ±30°
                double angle = baseAngle + randAngle;
                double dist = 120.0;
                dashVx = dist / dashTimer * cos(angle);
                dashVy = dist / dashTimer * sin(angle);

                // 残像生成（上限4）
                if (afterimageCount < 4) {
                    afterimageCount++;
                    sEnemyShotSet* pSet = new sEnemyShotSet;
                    pSet->count = 0;
                    pSet->patternFunc = ShotAfterimage;
                    pSet->x = enemy.x;
                    pSet->y = enemy.y;
                    pSet->muki = 0;
                    pSet->kind = 0;
                    pSet->pEnemyShotHead = new sEnemyShot;
                    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                    pSet->prev = enemyShotSetHead.prev;
                    pSet->next = &enemyShotSetHead;
                    enemyShotSetHead.prev->next = pSet;
                    enemyShotSetHead.prev = pSet;
                }

                damageAccum = 0;
                hitAccum = 0;
                escapeCooldown = 24; // 0.4秒

                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
        }
        else if (state == 2 && escapeCooldown <= 0) {
            // 壁際からの逃走
            state = 1;
            dashTimer = 9;
            double baseAngle = atan2(enemy.y - player.y, enemy.x - player.x);
            double randAngle = (GetRand(60) - 30) / 180.0 * DX_PI;
            double angle = baseAngle + randAngle;
            double dist = 120.0;
            dashVx = dist / dashTimer * cos(angle);
            dashVy = dist / dashTimer * sin(angle);

            if (afterimageCount < 4) {
                afterimageCount++;
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotAfterimage;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->muki = 0;
                pSet->kind = 0;
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
            escapeCooldown = 24;
        }
    }
    prevHp = enemy.hp;

    if (escapeCooldown > 0) escapeCooldown--;

    // 状態別処理
    switch (state) {
    case 0: // 通常
        enemy.x += 0.98 * muki;
        if (count % 120 == 60) muki *= -1;
        if (enemy.x < 30) { enemy.x = 30; muki = 1; }
        if (enemy.x > 450) { enemy.x = 450; muki = -1; }

        // 通常攻撃（40フレームごとに自機狙い1発）
        if (count % 40 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSingleAim;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->kind = shot_count++;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // いじけ溜めチェック（2秒間被弾なし）
        if (count - lastHitCount > 120 && chargeTimer == 0) {
            state = 3;
            chargeTimer = 90; // 1.5秒
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        break;

    case 1: // 逃走ダッシュ
        if (dashTimer > 0) {
            // 機雷設置（ダッシュ開始時と中間、上限6）
            if ((dashTimer == 9 || dashTimer == 5) && mineCount < 6) {
                mineCount++;
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotMine;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->muki = 0;
                pSet->kind = 0;
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
            enemy.x += dashVx;
            enemy.y += dashVy;
            dashTimer--;
        }
        else {
            // ダッシュ終了
            state = 0;
            // 壁際判定
            if (enemy.x < 30 || enemy.x > 450 || enemy.y < 30 || enemy.y > 450) {
                state = 2;
                if (enemy.x < 30) wallSlideDir = 1;
                else if (enemy.x > 450) wallSlideDir = -1;
                else if (enemy.y < 30) wallSlideDir = 1;
                else if (enemy.y > 450) wallSlideDir = -1;
            }
        }
        break;

    case 2: // 壁際
        enemy.x += 0.5 * wallSlideDir;
        if (enemy.x > 40 && enemy.x < 440 && enemy.y > 40 && enemy.y < 440) {
            state = 0;
        }
        break;

    case 3: // いじけ溜め
        chargeTimer--;
        if (chargeTimer <= 0) {
            // 大弾幕発射
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotChargeAttack;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;

            state = 0;
            lastHitCount = count;
        }
        break;
    }
}