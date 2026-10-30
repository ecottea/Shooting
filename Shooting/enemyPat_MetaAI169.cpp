#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 爆発フェーズかどうかの共有フラグ
static bool g_isExplosion = false;

// ---------------------------------------------------------
// インク溜まりをランダムにばら撒く：塗り
// 中玉(7x7) + 小玉(2.5x2.5) を重ねてベチャッとした形を作る
// ---------------------------------------------------------
static void ShotInkPuddle(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 1セットで9クラスター = 1クラスター5発で45発
        for (int c = 0; c < 9-1; c++) {
            // GetRand(x)は0〜xのx+1種類なので端に寄り過ぎないように余白を取る
            double baseX = (double)(GetRand(420+60) + 30-30);   // 30〜450
            double baseY = (double)(GetRand(280+50) + 150);  // 150〜430 下2/3を塗る
            while (hypot(baseX - player.x, baseY - player.y) < 30) {
                baseX = (double)(GetRand(420 + 60) + 30 - 30);
                baseY = (double)(GetRand(280 + 50) + 150);
            }

            for (int k = 0; k < 5; k++) {
                pEnemyShot = new sEnemyShot;
                double ofsX = (double)(GetRand(16) - 8);
                double ofsY = (double)(GetRand(16) - 8);
                pEnemyShot->x = baseX + ofsX;
                pEnemyShot->y = baseY + ofsY;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0; // 置き弾として残る = 塗り跡
                pEnemyShot->margin = 120.0; // 画面外判定を緩くして残留させる
                pEnemyShot->count = 0;

                // 中心は中玉シアン(3)、周りは小玉シアン/青で濃淡を表現
                if (k == 0) {
                    pEnemyShot->kind = img_enemyShotMediumBall[3]; // 3:シアン
                }
                else if (k % 2 == 0) {
                    pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン小
                }
                else {
                    pEnemyShot->kind = img_enemyShotSmallBall[4]; // 4:青で影っぽく
                }

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 爆発フェーズになったら塗り跡が跳ねる
        if (g_isExplosion && pShot->count > 60 && pShot->speed == 0.0) {
            double toPlayer = atan2(player.y - pShot->y, player.x - pShot->x);
            // 少しバラけさせる : GetRand(60)-30 = -30〜30度
            double spread = (GetRand(60) - 30) / 180.0 * DX_PI * 0.3;
            pShot->muki = toPlayer + spread;
            pShot->speed = 0.4 + GetRand(80) / 100.0; // 0.4〜1.2
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// ローラー突進の塗り帯
// 中楕円弾(10.5x7.0)でローラーの線を表現
// ---------------------------------------------------------
static void ShotRollerTrail(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 帯を作る
        for (int dx = -180; dx <= 180; dx += 15) {
            for (int dy = 0; dy < 2; dy++) {
                pEnemyShot = new sEnemyShot;
                // ローラーのガタつき
                pEnemyShot->x = pEnemyShotSet->x + dx + (GetRand(6) - 3);
                pEnemyShot->y = pEnemyShotSet->y + dy * 12 + (GetRand(6) - 3);
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->margin = 150.0;
                pEnemyShot->count = 0;

                // 交互に楕円と中玉で塗りムラを出す
                if (dy == 0) {
                    pEnemyShot->kind = img_enemyShotMediumOval[3]; // シアン楕円
                }
                else {
                    pEnemyShot->kind = img_enemyShotMediumBall[3];
                }

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (g_isExplosion && pShot->count > 30 && pShot->speed == 0.0) {
            double toPlayer = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->muki = toPlayer + (GetRand(40) - 20) / 180.0 * DX_PI * 0.2 + (GetRand(2) - 1) * DX_PI / 3;
            pShot->speed = 0.6 + GetRand(100) / 100.0;
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 敵本体：スプラトゥーン塗り弾幕
// ---------------------------------------------------------
void EnemyPat_Splatoon_MetaAI()
{
    static int muki;
    static int rollerDir;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        rollerDir = 1;
        shot_count = 0;
        g_isExplosion = false;
    }

    // 780Fループ (13秒 @60fps) で Phase1->2->3
    int loop = (count - 1) % 780;

    // ---------------- Phase1: 下塗り 0-239F ----------------
    if (loop < 240) {
        if (loop == 0) g_isExplosion = false;
        enemy.x += 0.5 * (double)muki;
        enemy.y = 70.0 + sin(loop * 0.05) * 5.0;
        if (count % 120 == 0) muki *= -1;

        if (count % 18 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotInkPuddle;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
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
    }
    // ---------------- Phase2: ローラー 240-539F ----------------
    else if (loop < 540) {
        if (loop == 240) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            rollerDir = (GetRand(1) == 0) ? 1 : -1;
            enemy.x = (rollerDir == 1) ? 30.0 : 450.0;
        }
        enemy.x += 3.8 * (double)rollerDir;
        enemy.y += 0.35;

        if (enemy.x < 30.0 || enemy.x > 450.0) {
            rollerDir *= -1;
            enemy.y += 20.0;
            if (enemy.y > 200.0) enemy.y = 90.0;
        }

        if (count % 10 == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotRollerTrail;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 18.0;
            pSet->muki = 0;
            pSet->kind = shot_count++;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    // ---------------- Phase3: 塗り爆発 540-779F ----------------
    else {
        if (loop == 540) {
            g_isExplosion = true;
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            enemy.x = 240.0;
            enemy.y = 50.0;
        }
        // ボスは中央でプルプル震える
        enemy.x = 240.0 + sin(loop * 0.25) * 15.0;
    }
}