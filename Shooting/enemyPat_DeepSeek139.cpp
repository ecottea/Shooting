#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾追加ヘルパー
// ============================================================
static void AddEnemyShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;

    pEnemyShot->prev = pSet->pEnemyShotHead->prev;
    pEnemyShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pEnemyShot;
    pSet->pEnemyShotHead->prev = pEnemyShot;
}

// ============================================================
// 弾移動ヘルパー
// ============================================================
static void MoveEnemyShots(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕セット作成ヘルパー
// ============================================================
static sEnemyShotSet* CreateEnemyShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

// ============================================================
// 螺旋雨帯
// ============================================================
static void ShotSpiralRain(sEnemyShotSet* pSet)
{
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    // 再発達：HP50%以下で逆回転＋速度アップ
    if (pSet->param_i[0] == 0 && enemy.hp <= 100) {
        pSet->param_i[0] = 1;
        pSet->param_d[2] = -10.0 / 180.0 * DX_PI;
        pSet->param_d[1] = 3.4;
    }

    int interval = (pSet->param_i[0] ? 2 : 3);
    if (pSet->count % interval == 0) {
        double angle = pSet->param_d[0];

        for (int i = 0; i < 3; i++) {
            double a = angle + i * 2.0 * DX_PI / 3.0;
            double spd = pSet->param_d[1] + (GetRand(40) - 20) / 100.0;

            // 雨粒：小型弾・シアン
            AddEnemyShot(pSet, pSet->x, pSet->y, a, spd, img_enemyShotSmallBall[3]);
        }

        pSet->param_d[0] = angle + pSet->param_d[2];
        if (pSet->param_d[0] > 2.0 * DX_PI) pSet->param_d[0] -= 2.0 * DX_PI;
        if (pSet->param_d[0] < 0.0)       pSet->param_d[0] += 2.0 * DX_PI;
    }

    MoveEnemyShots(pSet);
}

// ============================================================
// アイウォール
// ============================================================
static void ShotEyeWall(sEnemyShotSet* pSet)
{
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    int interval = (enemy.hp <= 100 ? 150 : 180); // 2.5秒 or 3秒

    if (pSet->count % interval == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int n = 36*2;
        for (int i = 0; i < n; i++) {
            double a = i * 2.0 * DX_PI / n;
            // 壁雲：中玉・白
            AddEnemyShot(pSet, pSet->x, pSet->y, a, 1.5, img_enemyShotMediumBall[6]);
        }
        pSet->param_i[0] = 18; // 0.3秒後に二重
    }

    if (pSet->param_i[0] > 0) {
        pSet->param_i[0]--;
        if (pSet->param_i[0] == 0) {
            int n = 36*2;
            for (int i = 0; i < n; i++) {
                double a = (i + 0.5) * 2.0 * DX_PI / n;
                AddEnemyShot(pSet, pSet->x, pSet->y, a, 2.2, img_enemyShotMediumBall[6]);
            }
        }
    }

    MoveEnemyShots(pSet);
}

// ============================================================
// 突風
// ============================================================
static void ShotGust(sEnemyShotSet* pSet)
{
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    int interval = (enemy.hp <= 100 ? 240 : 300); // 4秒 or 5秒

    if (pSet->count % interval == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double base = atan2(player.y - pSet->y, player.x - pSet->x);
        for (int i = -2; i <= 2; i++) {
            double a = base + i * 8.0 / 180.0 * DX_PI;
            // 突風：銃弾・黄
            AddEnemyShot(pSet, pSet->x, pSet->y, a, 4.0, img_enemyShotBullet[1]);
        }
        pSet->param_i[0] = 12;
    }

    if (pSet->param_i[0] > 0) {
        pSet->param_i[0]--;
        if (pSet->param_i[0] == 0) {
            double base = atan2(player.y - pSet->y, player.x - pSet->x);
            for (int i = -3; i <= 3; i++) {
                double a = base + i * 5.0 / 180.0 * DX_PI;
                // 風の余韻：小型弾・白
                AddEnemyShot(pSet, pSet->x, pSet->y, a, 3.0, img_enemyShotSmallBall[6]);
            }
        }
    }

    MoveEnemyShots(pSet);
}

// ============================================================
// 台風の目
// ============================================================
static void ShotEye(sEnemyShotSet* pSet)
{
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        int n = 24*2;
        for (int i = 0; i < n; i++) {
            double a = i * 2.0 * DX_PI / n;
            // 目：大玉・青
            AddEnemyShot(pSet, pSet->x, pSet->y, a, 0.5, img_enemyShotLargeBall[4]);
        }
    }

    MoveEnemyShots(pSet);
}

// ============================================================
// 最後の雷
// ============================================================
static void ShotFinalLightning(sEnemyShotSet* pSet)
{
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double dirs[4] = { 0.0, DX_PI * 0.5, DX_PI, DX_PI * 1.5 };
        for (int i = 0; i < 4; i++) {
            // 雷：短レーザー・黄
            AddEnemyShot(pSet, pSet->x, pSet->y, dirs[i], 5.0, img_enemyShotLaser[1]);
        }

        for (int i = 0; i < 8; i++) {
            double a = i * 2.0 * DX_PI / 8.0;
            // 飛散する暴風：小型弾・橙
            AddEnemyShot(pSet, pSet->x, pSet->y, a, 6.0, img_enemyShotSmallBall[8]);
        }
    }

    MoveEnemyShots(pSet);
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_Typhoon_DeepSeek()
{
    static int eyeCreated;
    static int finalCreated;
    static sEnemyShotSet* spiralSet;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと

        eyeCreated = 0;
        finalCreated = 0;

        // 螺旋雨帯
        spiralSet = CreateEnemyShotSet(ShotSpiralRain, enemy.x, enemy.y + 10.0, 0.0, 0);
        spiralSet->param_d[0] = 0.0;
        spiralSet->param_d[1] = 2.8;
        spiralSet->param_d[2] = 10.0 / 180.0 * DX_PI;
        spiralSet->param_i[0] = 0;

        // アイウォール
        CreateEnemyShotSet(ShotEyeWall, enemy.x, enemy.y + 10.0, 0.0, 1);

        // 突風
        CreateEnemyShotSet(ShotGust, enemy.x, enemy.y + 10.0, 0.0, 2);
    }
    else {
        // 台風の中心が小さく円運動
        enemy.x = 240.0 + 60.0 * cos(count * 0.03);
        enemy.y = 80.0 + 20.0 * sin(count * 0.05);
    }

    // HP50%以下：台風の目
    if (!eyeCreated && enemy.hp <= 133) {
        eyeCreated = 1;
        CreateEnemyShotSet(ShotEye, enemy.x, enemy.y + 10.0, 0.0, 3);
    }

    // HP25%以下：最後の雷
    if (!finalCreated && enemy.hp <= 66) {
        finalCreated = 1;
        CreateEnemyShotSet(ShotFinalLightning, enemy.x, enemy.y + 10.0, 0.0, 4);
    }
}