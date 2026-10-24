#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ============================================================
// A: 画面外接円上から発生し、中央へ引き寄せられる赤中玉のパターン
// ============================================================
static void ShotSunAttractBullets(sEnemyShotSet* pEnemyShotSet)
{
    // ボスの残り体力に応じた引力の強さを計算 (HPが減るほど引力が強くなる)
    double hpRate = (double)enemy.hp / enemy.maxHp;
    if (hpRate < 0.0) hpRate = 0.0;
    double pullSpeed = 0.6 + (1.0 - hpRate) * 1.8; // 0.6 〜 2.4

    // 中頻度で画面の外接円上に赤中玉を発生させる
    if (pEnemyShotSet->count % 25 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 発生の基準となる角度は時間経過で変化させる
        double baseAngle = pEnemyShotSet->count * 0.05;
        int spawnCount = 12; // 円周上に12個配置
        for (int i = 0; i < spawnCount; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double angle = baseAngle + (2.0 * DX_PI / spawnCount) * i;
            double radius = 320.0; // 画面全体の外接円相当の半径 (480x480の外接円は約339)

            pEnemyShot->x = 240.0 + radius * cos(angle);
            pEnemyShot->y = 240.0 + radius * sin(angle);
            pEnemyShot->speed = 0.0; // 速度0で発生
            pEnemyShot->muki = 0.0;
            pEnemyShot->margin = 240; // 指定されたマージン
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 0:赤の中玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動と中央到達時の消滅処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double dx = 240.0 - pShot->x;
        double dy = 240.0 - pShot->y;
        double dist = hypot(dx, dy);

        // 画面中央 (240.0, 240.0) に達すると消滅する
        if (dist < 8.0) {
            sEnemyShot* nextShot = pShot->next;
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
            pShot = nextShot;
            continue;
        }

        // 加速はさせず、常に引力の大きさと同じ速度で画面中央に向かって移動する
        double angleToCenter = atan2(dy, dx);
        pShot->x += pullSpeed * cos(angleToCenter);
        pShot->y += pullSpeed * sin(angleToCenter);

        pShot = pShot->next;
    }
}

// ============================================================
// B: 画面中央から高頻度で前後発射される赤小玉のパターン
// ============================================================
static void ShotSunEmitBullets(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % 6 == 0) {
        if (pEnemyShotSet->count % 12 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 撃つ方向は時間経過で回転させる
        double rotAngle = pEnemyShotSet->count * 0.04;

        // 前後2方向に撃つ
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            // 角度に微小な乱数を加える (GetRand(20) は 0〜20の21種類)
            double randVal = (double)(GetRand(20) - 10) / 100.0;
            double muki = rotAngle + i * DX_PI + randVal;

            pEnemyShot->x = 240.0;
            pEnemyShot->y = 240.0;
            pEnemyShot->speed = 3.2; // 一定の速度
            pEnemyShot->muki = muki;
            pEnemyShot->margin = 240;
            pEnemyShot->kind = img_enemyShotSmallBall[0]; // 0:赤的小玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 引力の影響を受けない直進弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// C: 画面中央に密集して核を表現する赤大玉のパターン（静止）
// ============================================================
static void ShotSunCore(sEnemyShotSet* pEnemyShotSet)
{
    // ボスの残り体力に応じて核の大きさを増加させる
    double hpRate = (double)enemy.hp / enemy.maxHp;
    if (hpRate < 0.0) hpRate = 0.0;

    // 残り体力が減るにつれて赤大玉の数が増え、核が大きくなる (4個〜16個)
    int coreCount = 4 + (int)((1.0 - hpRate) * 12.0);

    // 毎フレーム既存の核弾を再構成して最新の大きさに更新する
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;
        delete pShot;
        pShot = nextShot;
    }
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    // 核の大きさに応じた配置（画面中央に密集、ゆっくり回転）
    double coreRadius = 8.0 + (1.0 - hpRate) * 22.0;
    for (int i = 0; i < coreCount; i++) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        double angle = (2.0 * DX_PI / coreCount) * i + pEnemyShotSet->count * 0.015;
        double dist = (i == 0) ? 0.0 : (coreRadius * (0.4 + 0.6 * (i % 3)));

        pEnemyShot->x = 240.0 + dist * cos(angle);
        pEnemyShot->y = 240.0 + dist * sin(angle);
        pEnemyShot->speed = 0.0; // 静止
        pEnemyShot->muki = 0.0;
        pEnemyShot->margin = 240;
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 0:赤の大玉

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }
}

// ============================================================
// 敵本体のパターン：サブタレイニアンサン
// ============================================================
void EnemyPat_SubterraneanSun_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;

        // A: 赤中玉の吸い寄せパターン登録
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSunAttractBullets;
            pSet->x = 240.0;
            pSet->y = 240.0;
            pSet->muki = 0.0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // B: 赤小玉の前後発射パターン登録
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSunEmitBullets;
            pSet->x = 240.0;
            pSet->y = 240.0;
            pSet->muki = 0.0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // C: 赤大玉による核の表現パターン登録
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSunCore;
            pSet->x = 240.0;
            pSet->y = 240.0;
            pSet->muki = 0.0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    else {
        // ボス自身の移動（画面上部を左右に往復）
        enemy.x += 0.8 * (double)muki;
        if (enemy.x < 120.0 || enemy.x > 360.0) {
            muki *= -1;
        }
    }

    // ボスの残り体力に応じて引力の強さを計算
    double hpRate = (double)enemy.hp / enemy.maxHp;
    if (hpRate < 0.0) hpRate = 0.0;
    double pullSpeed = 0.6 + (1.0 - hpRate) * 1.8; // 残りHPが減るほど自機への引力も強まる

    // 自機から画面中央 (240.0, 240.0) に向かう引力のベクトル計算
    double angle = atan2(240.0 - player.y, 240.0 - player.x);
    double ratio = 3.0; // 比例定数

    // 引力の発生を表現するパーティクル関数を呼び出し、自機座標を変動させる
    spawnForceParticles(player.x, player.y, pullSpeed * cos(angle) * ratio, pullSpeed * sin(angle) * ratio);

    player.x += pullSpeed * cos(angle);
    player.y += pullSpeed * sin(angle);
}