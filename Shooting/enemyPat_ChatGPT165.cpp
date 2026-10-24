#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// サブタレイニアンサン
// A: 画面外接円上から中心へ吸い込まれる赤中玉
// B: 核から前後へ撃ち出され、回転する赤小玉
// C: HP低下に応じて層が増える静止した赤い核

static const double SUN_CENTER_X = 240.0;
static const double SUN_CENTER_Y = 240.0;
static const double SUN_SCREEN_RADIUS = 339.4112549695428;
static const int SUN_GRAVITY_RING_SHOTS = 160/5-2;
static double gSubterraneanPullSpeed = 0.45;

static void LinkShot(sEnemyShotSet* pSet, sEnemyShot* pShot,
    double x, double y, double angle, double speed, int kind, double margin = 20.0)
{
    pShot->x = x;
    pShot->y = y;
    pShot->muki = angle;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->margin = margin;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

static sEnemyShotSet* CreateSunShotSet(sEnemyShotSet::PatternFunc patternFunc,
    double x, double y, double angle)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = patternFunc;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = angle;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

static double GetSunHpRatio()
{
    if (enemy.maxHp <= 0) return 0.0;
    double hpRatio = (double)enemy.hp / (double)enemy.maxHp;
    if (hpRatio < 0.0) hpRatio = 0.0;
    if (hpRatio > 1.0) hpRatio = 1.0;
    return hpRatio;
}

// A: 発生時は速度0。次フレーム以降、現在の引力と同じ速さで中心へ進む。
static void ShotSunGravityRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < SUN_GRAVITY_RING_SHOTS; ++i) {
            const double angle = pEnemyShotSet->muki
                + (2.0 * DX_PI) * (double)i / (double)SUN_GRAVITY_RING_SHOTS;
            const double x = SUN_CENTER_X + SUN_SCREEN_RADIUS * cos(angle);
            const double y = SUN_CENTER_Y + SUN_SCREEN_RADIUS * sin(angle);

            // 生成時の速度は0。margin=240で、画面外からも消されずに中心へ向かう。
            LinkShot(pEnemyShotSet, new sEnemyShot,
                x, y, angle + DX_PI, 0.0, img_enemyShotMediumBall[0], 240.0);
        }
        return;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        const double dx = SUN_CENTER_X - pShot->x;
        const double dy = SUN_CENTER_Y - pShot->y;
        const double distance = sqrt(dx * dx + dy * dy);
        pShot->muki = atan2(dy, dx);
        pShot->speed = gSubterraneanPullSpeed;

        // 中心に到達した弾は、このパターン内で消滅させる。
        if (distance <= pShot->speed) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pNext;
    }
}

// B: 核から赤小玉を前後2方向へ撃ち、発射軸を時間経過で回転させる。
static void ShotSunRotatingPair(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const double angleJitter1 = (GetRand(20) - 10) * 0.001;
        const double angleJitter2 = (GetRand(20) - 10) * 0.001;

        LinkShot(pEnemyShotSet, new sEnemyShot,
            SUN_CENTER_X, SUN_CENTER_Y,
            pEnemyShotSet->muki + angleJitter1,
            3.0, img_enemyShotSmallBall[0]);

        LinkShot(pEnemyShotSet, new sEnemyShot,
            SUN_CENTER_X, SUN_CENTER_Y,
            pEnemyShotSet->muki + DX_PI + angleJitter2,
            3.0, img_enemyShotSmallBall[0]);
        return;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

static void AddSunCoreShot(sEnemyShotSet* pEnemyShotSet, double x, double y)
{
    LinkShot(pEnemyShotSet, new sEnemyShot,
        x, y, 0.0, 0.0, img_enemyShotLargeBall[0]);
}

static void AddSunCoreRing(sEnemyShotSet* pEnemyShotSet, double radius, int layer)
{
    int shotCount = (int)((2.0 * DX_PI) * radius / 13.0);
    if (shotCount < 8) shotCount = 8;

    const double angleOffset = (double)layer * 0.13;
    for (int i = 0; i < shotCount; ++i) {
        const double angle = angleOffset
            + (2.0 * DX_PI) * (double)i / (double)shotCount;

        AddSunCoreShot(pEnemyShotSet,
            SUN_CENTER_X + radius * cos(angle),
            SUN_CENTER_Y + radius * sin(angle));
    }
}

// C: 赤大玉を静止配置して核を作り、HPが減るたびに外側の層を追加する。
static void ShotSunCore(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->param_i[1] == 0) {
        pEnemyShotSet->param_i[1] = 1;

        AddSunCoreShot(pEnemyShotSet, SUN_CENTER_X, SUN_CENTER_Y);

        for (int i = 0; i < 6; ++i) {
            const double angle = (2.0 * DX_PI) * (double)i / 6.0;
            AddSunCoreShot(pEnemyShotSet,
                SUN_CENTER_X + 5.5 * cos(angle),
                SUN_CENTER_Y + 5.5 * sin(angle));
        }

        pEnemyShotSet->param_i[0] = 0; // 追加済みの外側リング数
    }

    const double damageRatio = 1.0 - GetSunHpRatio();
    int targetLayer = (int)floor(damageRatio * 5.0 + 1.0e-9);
    if (targetLayer < 0) targetLayer = 0;
    if (targetLayer > 5) targetLayer = 5;

    while (pEnemyShotSet->param_i[0] < targetLayer) {
        const int layer = pEnemyShotSet->param_i[0] + 1;
        const double radius = 12.0 + (double)(layer - 1) * 10.0;
        AddSunCoreRing(pEnemyShotSet, radius, layer);
        pEnemyShotSet->param_i[0] = layer;
    }
}

// 敵本体のパターン
void EnemyPat_SubterraneanSun_ChatGPT()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // サンプルの仕様に合わせて変更しない
        gSubterraneanPullSpeed = 0.45;
    }

    // HPが減るほど引力を連続的に強める。
    // 弾は加速せず、その時点の引力の強さを速度として使う。
    const double hpRatio = GetSunHpRatio();
    gSubterraneanPullSpeed = 0.45 + (1.80 - 0.45) * (1.0 - hpRatio);

    // 自機にも中心向きの引力を適用し、力のエフェクトを発生させる。
    double dx = SUN_CENTER_X - player.x;
    double dy = SUN_CENTER_Y - player.y;
    double distance = sqrt(dx * dx + dy * dy);
    double pullSpeed = gSubterraneanPullSpeed;
    if (distance < pullSpeed) pullSpeed = distance;

    double angle = atan2(dy, dx);
    double ratio = 3.0;

    spawnForceParticles(player.x, player.y,
        pullSpeed * cos(angle) * ratio,
        pullSpeed * sin(angle) * ratio);

    player.x += pullSpeed * cos(angle);
    player.y += pullSpeed * sin(angle);

    // C: 核は常駐する1セットで管理し、HP低下時に新しいリングだけを追加する。
    if (count == 1) {
        CreateSunShotSet(ShotSunCore, SUN_CENTER_X, SUN_CENTER_Y, 0.0);
    }

    // A: 外接円上から狭い間隔の中玉リングを供給。
    // 基準角は時間とともに回転する。
    if (count % 48 == 1) {
        CreateSunShotSet(ShotSunGravityRing,
            SUN_CENTER_X, SUN_CENTER_Y, (double)count * 0.008);
    }

    // B: 核から前後へ小玉を高頻度で撃つ。軸は連続回転する。
    if (count % 4 == 1) {
        CreateSunShotSet(ShotSunRotatingPair,
            SUN_CENTER_X, SUN_CENTER_Y, (double)count * 0.027);
    }
}