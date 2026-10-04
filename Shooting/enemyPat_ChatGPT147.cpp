#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 三節棍「旋棍連鎖」
// 短レーザーを3本の節、中玉を2つの接続部、小玉を振り回しの軌跡として使用する。
static void SpawnSansetsukonShot(
    sEnemyShotSet* pEnemyShotSet,
    int kind,
    double x,
    double y,
    double muki,
    double speed,
    int role,
    int index = 0)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[0] = role;
    pEnemyShot->param_i[1] = index;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static void ShotSansetsukon(sEnemyShotSet* pEnemyShotSet)
{
    const double LINK_LEN = 58.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 3節を短レーザー、2つの節を中玉で構成する。
        SpawnSansetsukonShot(pEnemyShotSet, img_enemyShotLaser[5], 0.0, 0.0, 0.0, 0.0, 0, 0);
        SpawnSansetsukonShot(pEnemyShotSet, img_enemyShotLaser[5], 0.0, 0.0, 0.0, 0.0, 0, 1);
        SpawnSansetsukonShot(pEnemyShotSet, img_enemyShotLaser[5], 0.0, 0.0, 0.0, 0.0, 0, 2);
        SpawnSansetsukonShot(pEnemyShotSet, img_enemyShotMediumBall[5], 0.0, 0.0, 0.0, 0.0, 1, 0);
        SpawnSansetsukonShot(pEnemyShotSet, img_enemyShotMediumBall[5], 0.0, 0.0, 0.0, 0.0, 1, 1);

        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
        pEnemyShotSet->param_d[2] = 0.0;
        pEnemyShotSet->param_d[3] = 0.0;
    }

    const int t = pEnemyShotSet->count;
    double originX = pEnemyShotSet->x;
    double originY = pEnemyShotSet->y;
    double a0 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;

    if (t < 120) {
        // まず大きく振り回して、三節棍の形を見せる。
        const double theta = -0.9 + 0.028 * t;
        const double bend = 0.95 + 0.18 * sin(0.055 * t);
        a0 = theta + bend;
        a1 = theta - 0.85 * bend;
        a2 = theta + 0.55 * bend;
    }
    else if (t < 240) {
        // 回転方向を反転して、跳ね返るような往復運動にする。
        const int u = t - 120;
        const double theta = 2.45 - 0.036 * u;
        const double bend = 0.85 + 0.20 * sin(0.06 * u + 1.3);
        a0 = theta - 0.72 * bend;
        a1 = theta + 0.95 * bend;
        a2 = theta - 0.48 * bend;

        if (t == 120) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }
    else if (t < 300) {
        // プレイヤーを狙う角度を固定し、折れた形から一直線へ畳み込む。
        if (t == 240) {
            pEnemyShotSet->param_d[2] = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        }

        const double k = (double)(t - 240) / 60.0;
        const double bendScale = 1.0 - k;
        const double theta = pEnemyShotSet->param_d[2];
        const double wobble = 0.10 * sin(k * DX_PI);

        a0 = theta + 0.90 * bendScale + wobble;
        a1 = theta - 0.65 * bendScale + wobble;
        a2 = theta + 0.42 * bendScale + wobble;
    }
    else {
        // 三節を一直線に伸ばし、そのままプレイヤー方向へ振り抜く。
        const int u = t - 300;
        const double targetAngle = pEnemyShotSet->param_d[2];
        const double bendWobble = 0.18 * sin(0.055 * u);

        // 最初は加速し、後半はそのまま押し切る。
        double advance;
        if (u < 120) {
            advance = 0.020 * u * u;
        }
        else {
            advance = 288.0 + 3.6 * (u - 120);
        }

        originX = pEnemyShotSet->param_d[0] + cos(targetAngle) * advance;
        originY = pEnemyShotSet->param_d[1] + sin(targetAngle) * advance;

        a0 = targetAngle + bendWobble;
        a1 = targetAngle + bendWobble;
        a2 = targetAngle + bendWobble;

        if (t == 300) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
    }

    // 4点を求め、3本のレーザーを隣り合わせて三節棍にする。
    const double p0x = originX;
    const double p0y = originY;
    const double p1x = p0x + LINK_LEN * cos(a0);
    const double p1y = p0y + LINK_LEN * sin(a0);
    const double p2x = p1x + LINK_LEN * cos(a1);
    const double p2y = p1y + LINK_LEN * sin(a1);
    const double p3x = p2x + LINK_LEN * cos(a2);
    const double p3y = p2y + LINK_LEN * sin(a2);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            const int index = pShot->param_i[1];
            double ax = a0;
            double bx = p0x;
            double by = p0y;
            double ex = p1x;
            double ey = p1y;

            if (index == 1) {
                ax = a1;
                bx = p1x;
                by = p1y;
                ex = p2x;
                ey = p2y;
            }
            else if (index == 2) {
                ax = a2;
                bx = p2x;
                by = p2y;
                ex = p3x;
                ey = p3y;
            }

            pShot->x = (bx + ex) * 0.5;
            pShot->y = (by + ey) * 0.5;
            pShot->muki = ax;
            pShot->speed = 0.0;
        }
        else if (pShot->param_i[0] == 1) {
            const int index = pShot->param_i[1];
            if (index == 0) {
                pShot->x = p1x;
                pShot->y = p1y;
            }
            else {
                pShot->x = p2x;
                pShot->y = p2y;
            }
            pShot->muki = 0.0;
            pShot->speed = 0.0;
        }

        pShot = pShot->next;
    }

    // 回転中は先端を中心に小玉の軌跡を残す。
    // 軌跡弾は少しだけ外向きに流し、画面外へ出ればメインルーチン側で消去される。
    if (t < 240 && t % 2 == 0) {
        const double trailAngle = atan2(p3y - originY, p3x - originX);
        SpawnSansetsukonShot(
            pEnemyShotSet,
            img_enemyShotSmallBall[1],
            p3x,
            p3y,
            trailAngle,
            0.75,
            2,
            0);

        if (t % 4 == 0) {
            SpawnSansetsukonShot(
                pEnemyShotSet,
                img_enemyShotSmallBall[1],
                p1x,
                p1y,
                trailAngle + 0.7,
                0.60,
                2,
                1);
        }
    }

    // 軌跡用小玉だけ通常移動させる。画面外判定と消去はメインルーチンに任せる。
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 2) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_ThreeSectionStaff_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.75 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 三節棍の動作が終わって次の一振りへ移るタイミングで新しい弾幕セットを出す。
    if (count % 320 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSansetsukon;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
