#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 北斗七星「七星点線」
// ============================================================

static void PlaySEOnce(int sound)
{
    if (CheckSoundMem(sound)) StopSoundMem(sound);
    PlaySoundMem(sound, DX_PLAYTYPE_BACK);
}

static void AddShot(
    sEnemyShotSet* set,
    double x, double y,
    double muki, double speed,
    int kind,
    int param0, int param1)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = param0;
    p->param_i[1] = param1;

    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;
}

// 北斗七星の基準座標（中心からの相対座標）
static const double HOKUTO_BASE_X[7] = {
    -163.75, -151.25, -61.25, -48.75, 36.25, 108.75, 163.75
};
static const double HOKUTO_BASE_Y[7] = {
    -57.5, 40.0, 57.5, -5.0, 7.5, -20.0, -57.5
};

static void ShotHokutoShichisei(sEnemyShotSet* pEnemyShotSet)
{
    const double CX = pEnemyShotSet->x;
    const double CY = pEnemyShotSet->y;

    // ゆっくり回転（約6度/秒）
    const double angle =
        pEnemyShotSet->count * (6.0 * DX_PI / 180.0 / 60.0);

    double sx[7], sy[7];
    for (int i = 0; i < 7; i++) {
        sx[i] = CX + HOKUTO_BASE_X[i] * cos(angle) - HOKUTO_BASE_Y[i] * sin(angle);
        sy[i] = CY + HOKUTO_BASE_X[i] * sin(angle) + HOKUTO_BASE_Y[i] * cos(angle);
    }

    // 0フレーム目：7つの星を大玉で配置
    if (pEnemyShotSet->count == 0) {
        PlaySEOnce(sound_enemyShot_heavy);
        for (int i = 0; i < 7; i++) {
            AddShot(
                pEnemyShotSet,
                sx[i], sy[i],
                0.0, 0.0,
                img_enemyShotLargeBall[6], // 白い大玉
                1, i                       // param0=1:星, param1=星番号
            );
        }
    }

    // 弾の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 星
            int idx = pShot->param_i[1];

            if (pEnemyShotSet->count >= 360) {
                // 終了後は外側へ飛ばして画面外消去に任せる
                if (pShot->speed == 0.0) {
                    double dx = pShot->x - CX;
                    double dy = pShot->y - CY;
                    pShot->muki = atan2(dy, dx);
                    pShot->speed = 3.0;
                }
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                // 星座として回転
                pShot->x = sx[idx];
                pShot->y = sy[idx];
            }
        }
        else {
            // 星座線の点線弾：一定時間後に外へ飛ばす
            if (pShot->param_i[0] == 2) {
                int life = pShot->param_i[1];
                if (pShot->count > life && pShot->speed == 0.0) {
                    double dx = pShot->x - CX;
                    double dy = pShot->y - CY;
                    pShot->muki = atan2(dy, dx);
                    pShot->speed = 4.0;
                }
            }

            if (pShot->speed != 0.0) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }

        pShot = pShot->next;
    }

    // 星座線を点線状に発生
    if (pEnemyShotSet->count > 0 &&
        pEnemyShotSet->count < 360 &&
        pEnemyShotSet->count % 72 == 10) {

        PlaySEOnce(sound_enemyShot_light);

        // P1-P2, P2-P3, P3-P4, P4-P1, P4-P5, P5-P6, P6-P7
        const int edge[7][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, {3, 4}, {4, 5}, {5, 6}
        };

        for (int e = 0; e < 7; e++) {
            int a = edge[e][0];
            int b = edge[e][1];

            double ax = sx[a], ay = sy[a];
            double bx = sx[b], by = sy[b];
            double dx = bx - ax;
            double dy = by - ay;
            double len = sqrt(dx * dx + dy * dy);

            int n = (int)(len / 16.0);
            if (n < 2) continue;

            for (int k = 1; k < n; k++) {
                double t = (double)k / (double)n;
                double x = ax + dx * t;
                double y = ay + dy * t;

                AddShot(
                    pEnemyShotSet,
                    x, y,
                    0.0, 0.0,
                    img_enemyShotSmallBall[3], // シアン小玉
                    2, 30                      // param0=2:線, param1=寿命
                );
            }
        }
    }

    // 星のまたたき
    if (pEnemyShotSet->count > 60 &&
        pEnemyShotSet->count < 360 &&
        pEnemyShotSet->count % 120 == 60) {

        PlaySEOnce(sound_enemyShot_medium);

        for (int i = 0; i < 7; i++) {
            for (int j = 0; j < 6*2; j++) {
                double muki = angle + j * (DX_PI * 2.0 / 6.0/2) + i * 0.3;
                AddShot(
                    pEnemyShotSet,
                    sx[i], sy[i],
                    muki, 2.4,
                    img_enemyShotMediumBall[1], // 黄色中玉
                    0, 0
                );
            }
        }
    }

    // 終了演出：七星放射
    if (pEnemyShotSet->count == 360) {
        PlaySEOnce(sound_enemyShot_extreme);

        for (int i = 0; i < 7; i++) {
            for (int j = 0; j < 16*2; j++) {
                double muki = j * (DX_PI * 2.0 / 16.0/2) + i * 0.2;
                AddShot(
                    pEnemyShotSet,
                    sx[i], sy[i],
                    muki, 3.0,
                    img_enemyShotSmallBall[8], // 橙色小玉
                    0, 0
                );
            }
        }
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_BigDipper_DeepSeek()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }
    
    if (count % 500 == 1) {
        // 北斗七星パターンを1セットだけ生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHokutoShichisei;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 220.0;
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