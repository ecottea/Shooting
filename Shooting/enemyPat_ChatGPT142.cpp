#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

struct sPeanoPoint {
    int x;
    int y;
};

static sPeanoPoint g_peanoPath[81];
static bool g_peanoPathReady = false;

// 3x3 の蛇行を 3x3 ブロックへ再帰的に配置した、2次のペアノ曲線風パス。
// 各ブロックの出入口が隣接するように反転を切り替える。
static void BuildPeanoPath()
{
    if (g_peanoPathReady)
        return;

    static const int localX[9] = { 0, 1, 2, 2, 1, 0, 0, 1, 2 };
    static const int localY[9] = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };
    static const int blockX[9] = { 0, 1, 2, 2, 1, 0, 0, 1, 2 };
    static const int blockY[9] = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };
    static const bool flipX[9] = { false, false, false, true,  true,  true,  false, false, false };
    static const bool flipY[9] = { false, true,  false, false, true,  false, false, true,  false };

    int index = 0;
    for (int b = 0; b < 9; b++) {
        for (int i = 0; i < 9; i++) {
            int lx = localX[i];
            int ly = localY[i];
            if (flipX[b]) lx = 2 - lx;
            if (flipY[b]) ly = 2 - ly;
            g_peanoPath[index].x = blockX[b] * 3 + lx;
            g_peanoPath[index].y = blockY[b] * 3 + ly;
            index++;
        }
    }
    g_peanoPathReady = true;
}

static void GetPeanoPos(int index, double centerX, double centerY, double scale, double rotate, double* x, double* y)
{
    const double lx = (double)g_peanoPath[index].x - 4.0;
    const double ly = (double)g_peanoPath[index].y - 4.0;
    const double c = cos(rotate);
    const double s = sin(rotate);
    *x = centerX + (lx * c - ly * s) * scale;
    *y = centerY + (lx * s + ly * c) * scale;
}

static void SpawnPeanoBullet(sEnemyShotSet* pEnemyShotSet, int pathIndex, double scale, int kind, double speed)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    double x, y;
    double nx, ny;
    const double rotate = pEnemyShotSet->param_d[0];

    GetPeanoPos(pathIndex, pEnemyShotSet->x, pEnemyShotSet->y, scale, rotate, &x, &y);
    if (pathIndex < 80)
        GetPeanoPos(pathIndex + 1, pEnemyShotSet->x, pEnemyShotSet->y, scale, rotate, &nx, &ny);
    else
        GetPeanoPos(pathIndex - 1, pEnemyShotSet->x, pEnemyShotSet->y, scale, rotate, &nx, &ny);

    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = atan2(ny - y, nx - x);
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[0] = pathIndex;
    pEnemyShot->param_i[1] = 0; // 0: 曲線移動, 1: 崩壊
    pEnemyShot->param_i[2] = 0; // 内側曲線なら 1
    pEnemyShot->param_d[0] = 0.0; // セグメント内の進行度
    pEnemyShot->param_d[1] = scale;
    pEnemyShot->param_d[2] = rotate;
    pEnemyShot->param_d[3] = speed;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static void SpawnPeanoRadialShot(sEnemyShotSet* pEnemyShotSet, int pathIndex, double scale, int kind, double speed, double spread)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    double x, y;
    const double rotate = pEnemyShotSet->param_d[0];
    GetPeanoPos(pathIndex, pEnemyShotSet->x, pEnemyShotSet->y, scale, rotate, &x, &y);

    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = atan2(y - pEnemyShotSet->y, x - pEnemyShotSet->x);
    pEnemyShot->muki += spread;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[1] = 1;
    pEnemyShot->param_d[0] = 0.0;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static void UpdatePeanoBullet(sEnemyShot* pEnemyShot, sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShot->param_i[1] == 1) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        return;
    }

    int index = pEnemyShot->param_i[0];
    if (index >= 80) {
        pEnemyShot->speed = 0.0;
        return;
    }

    double x1, y1, x2, y2;
    GetPeanoPos(index, pEnemyShotSet->x, pEnemyShotSet->y, pEnemyShot->param_d[1], pEnemyShot->param_d[2], &x1, &y1);
    GetPeanoPos(index + 1, pEnemyShotSet->x, pEnemyShotSet->y, pEnemyShot->param_d[1], pEnemyShot->param_d[2], &x2, &y2);

    const double dx = x2 - x1;
    const double dy = y2 - y1;
    const double length = sqrt(dx * dx + dy * dy);
    if (length <= 0.0001) {
        pEnemyShot->param_i[0]++;
        pEnemyShot->param_d[0] = 0.0;
        return;
    }

    double move = pEnemyShot->speed;
    while (move > 0.0 && pEnemyShot->param_i[0] < 80) {
        const double remain = length * (1.0 - pEnemyShot->param_d[0]);
        if (move < remain) {
            pEnemyShot->param_d[0] += move / length;
            move = 0.0;
        }
        else {
            move -= remain;
            pEnemyShot->param_i[0]++;
            pEnemyShot->param_d[0] = 0.0;
            if (pEnemyShot->param_i[0] >= 80)
                break;
        }

        if (pEnemyShot->param_i[0] < 80) {
            GetPeanoPos(pEnemyShot->param_i[0], pEnemyShotSet->x, pEnemyShotSet->y, pEnemyShot->param_d[1], pEnemyShot->param_d[2], &x1, &y1);
            GetPeanoPos(pEnemyShot->param_i[0] + 1, pEnemyShotSet->x, pEnemyShotSet->y, pEnemyShot->param_d[1], pEnemyShot->param_d[2], &x2, &y2);
            const double angle = atan2(y2 - y1, x2 - x1);
            pEnemyShot->muki = angle;
        }
    }

    if (pEnemyShot->param_i[0] < 80) {
        GetPeanoPos(pEnemyShot->param_i[0], pEnemyShotSet->x, pEnemyShotSet->y, pEnemyShot->param_d[1], pEnemyShot->param_d[2], &x1, &y1);
        GetPeanoPos(pEnemyShot->param_i[0] + 1, pEnemyShotSet->x, pEnemyShotSet->y, pEnemyShot->param_d[1], pEnemyShot->param_d[2], &x2, &y2);
        pEnemyShot->x = x1 + (x2 - x1) * pEnemyShot->param_d[0];
        pEnemyShot->y = y1 + (y2 - y1) * pEnemyShot->param_d[0];
        pEnemyShot->muki = atan2(y2 - y1, x2 - x1);
    }
    else {
        GetPeanoPos(80, pEnemyShotSet->x, pEnemyShotSet->y, pEnemyShot->param_d[1], pEnemyShot->param_d[2], &pEnemyShot->x, &pEnemyShot->y);
    }
}

// ペアノ曲線弾幕
static void ShotPeano(sEnemyShotSet* pEnemyShotSet)
{
    BuildPeanoPath();

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 外側のペアノ曲線。楕円弾は接線方向へ向きを合わせる。
        for (int i = 0; i < 81; i++) {
            SpawnPeanoBullet(pEnemyShotSet, i, 39.0, img_enemyShotMediumOval[5], 2.2);
        }
    }

    // 曲線の内側に、さらに細かいペアノ曲線を追加する。
    if (pEnemyShotSet->count == 75) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 81; i += 2) {
            SpawnPeanoBullet(pEnemyShotSet, i, 20.0, img_enemyShotSmallBall[3], 2.4);
            pEnemyShotSet->pEnemyShotHead->prev->param_i[2] = 1;
        }
    }

    // 完成した曲線の一部から別色の弾を外へ押し出す。
    if (pEnemyShotSet->count == 135) {
        for (int i = 5; i < 81; i += 10) {
            SpawnPeanoRadialShot(pEnemyShotSet, i, 39.0, img_enemyShotDiamond[8], 2.8/2, 0.0);
        }
    }

    // 最終段階。曲線を崩して大量の弾を放射する。
    if (pEnemyShotSet->count == 220) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[1] == 0) {
                const double dx = pShot->x - pEnemyShotSet->x;
                const double dy = pShot->y - pEnemyShotSet->y;
                pShot->muki = atan2(dy, dx);
                pShot->speed = pShot->param_i[2] ? 3.9/2 : 4.6/2;
                pShot->kind = pShot->param_i[2] ? img_enemyShotSmallBall[3] : img_enemyShotDiamond[8];
                pShot->param_i[1] = 1;
            }
            pShot = pShot->next;
        }

        // 崩壊の瞬間だけ、現在のプレイヤー位置へ向かう短い追撃列を追加する。
        for (int i = 0; i < 12; i++) {
            SpawnPeanoRadialShot(pEnemyShotSet, i * 7, 39.0, img_enemyShotLargeBall[8], 4.2, (i - 5.5) * 0.035);
            pEnemyShotSet->pEnemyShotHead->prev->muki = atan2(player.y - pEnemyShotSet->pEnemyShotHead->prev->y, player.x - pEnemyShotSet->pEnemyShotHead->prev->x) + (i - 5.5) * 0.045;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[1] == 0)
            UpdatePeanoBullet(pShot, pEnemyShotSet);
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_PeanoCurve_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
        BuildPeanoPath();
    }
    else {
        enemy.x += 0.82 * (double)muki;
        if (count % 150 == 75)
            muki *= -1;
    }

    // ペアノ曲線を間隔を空けて重ね、画面全体を段階的に埋める。
    if (count % 240 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPeano;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 255.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->param_d[0] = (pEnemyShotSet->kind % 4) * (DX_PI / 12.0);
        pEnemyShotSet->param_d[1] = 0.0;
        pEnemyShotSet->param_d[2] = 0.0;
        pEnemyShotSet->param_d[3] = 0.0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
