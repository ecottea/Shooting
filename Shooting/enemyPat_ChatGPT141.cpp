#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 罔両「禅寺に棲む妖蝶」風
// 青：反時計回りの卍型レーザー + 外へ飛んで戻る鱗弾
// 赤：時計回りの卍型レーザー + 自機狙いの鱗弾

static const double ZENJI_PI2 = DX_PI * 2.0;
static const int ZENJI_LASER = 0;
static const int ZENJI_BUTTERFLY = 1;
static const int ZENJI_BLUE = 0;
static const int ZENJI_RED = 1;

static void ZenjiSetColorByPhase(sEnemyShot* pEnemyShot, int phase)
{
    if (pEnemyShot->param_i[0] == ZENJI_LASER) {
        pEnemyShot->kind = (phase == ZENJI_BLUE) ? img_enemyShotLaser[4] : img_enemyShotLaser[0];
    }
    else {
        pEnemyShot->kind = (phase == ZENJI_BLUE) ? img_enemyShotScale[4] : img_enemyShotScale[0];
    }
}

static void ZenjiPrepareBlueShot(sEnemyShot* pEnemyShot, sEnemyShotSet* pEnemyShotSet)
{
    const double dx = pEnemyShot->x - pEnemyShotSet->x;
    const double dy = pEnemyShot->y - pEnemyShotSet->y;
    const double r = sqrt(dx * dx + dy * dy);

    pEnemyShot->kind = img_enemyShotScale[4];
    pEnemyShot->param_d[0] = r;
    pEnemyShot->param_d[1] = atan2(dy, dx);
    pEnemyShot->param_i[1] = (r >= 150.0) ? -1 : 1;
    pEnemyShot->speed = 0.0;
    pEnemyShot->muki = pEnemyShot->param_d[1];
}

static void ZenjiPrepareRedShot(sEnemyShot* pEnemyShot)
{
    pEnemyShot->kind = img_enemyShotScale[0];
    pEnemyShot->speed = 2.8;
    pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
    pEnemyShot->param_d[0] = 0.0;
    pEnemyShot->param_d[1] = 0.0;
}

// 弾幕本体
static void ShotZenji(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = ZENJI_BLUE;
        pEnemyShotSet->param_d[0] = 0.0; // レーザー回転角

        // 卍型を4本のL字アームで構成。各アーム2本の短レーザー。
        for (int arm = 0; arm < 4; arm++) {
            const double base = arm * DX_PI / 2.0;
            for (int segment = 0; segment < 2; segment++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->param_i[0] = ZENJI_LASER;
                pEnemyShot->param_i[1] = arm;
                pEnemyShot->param_i[2] = segment;
                pEnemyShot->kind = img_enemyShotLaser[4];
                pEnemyShot->speed = 0.0;
                pEnemyShot->muki = base + ((segment == 0) ? 0.0 : DX_PI / 2.0);
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 初期波：青い蝶弾の代用として鱗弾を放射状に展開。
        for (int i = 0; i < 12; i++) {
            const double angle = ZENJI_PI2 * i / 12.0;
            const double x = pEnemyShotSet->x + cos(angle) * 20.0;
            const double y = pEnemyShotSet->y + sin(angle) * 20.0;
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x;
            pEnemyShot->y = y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotScale[4];
            pEnemyShot->param_i[0] = ZENJI_BUTTERFLY;
            pEnemyShot->param_i[1] = 1;
            pEnemyShot->param_d[0] = 20.0; // 半径
            pEnemyShot->param_d[1] = angle; // 放射角
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;

    int& phase = pEnemyShotSet->param_i[0];
    double& rotate = pEnemyShotSet->param_d[0];
    const int cycle = pEnemyShotSet->count % 360;

    // 音を合図に青→赤、赤→青を切り替える。周期化しているので同じ演出を繰り返す。
    if (cycle == 150 || cycle == 330) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    const int nextPhase = (cycle >= 180) ? ZENJI_RED : ZENJI_BLUE;
    if (nextPhase != phase) {
        phase = nextPhase;
        rotate = 0.0;
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* pChangeShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pChangeShot != pEnemyShotSet->pEnemyShotHead) {
            ZenjiSetColorByPhase(pChangeShot, phase);
            if (pChangeShot->param_i[0] == ZENJI_BUTTERFLY) {
                if (phase == ZENJI_RED) {
                    ZenjiPrepareRedShot(pChangeShot);
                }
                else {
                    ZenjiPrepareBlueShot(pChangeShot, pEnemyShotSet);
                }
            }
            pChangeShot = pChangeShot->next;
        }
    }

    // レーザーは青で反時計回り、赤で時計回り。
    rotate += (phase == ZENJI_BLUE) ? -0.020 : 0.024;
    if (rotate > ZENJI_PI2) rotate -= ZENJI_PI2;
    if (rotate < -ZENJI_PI2) rotate += ZENJI_PI2;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == ZENJI_LASER) {
            const int arm = pShot->param_i[1];
            const int segment = pShot->param_i[2];
            const double base = arm * DX_PI / 2.0 + rotate;

            double localAngle;
            double radius;
            if (segment == 0) {
                radius = 28.0;
                localAngle = base;
            }
            else {
                radius = 56.0;
                localAngle = base + DX_PI / 2.0;
            }

            // 画面座標では角度増加が時計回りになるため、mukiも同じ向きで設定。
            pShot->x = pEnemyShotSet->x + cos(base) * radius;
            pShot->y = pEnemyShotSet->y + sin(base) * radius;
            pShot->muki = localAngle;
        }
        else {
            if (phase == ZENJI_BLUE) {
                double radius = pShot->param_d[0];
                const double angle = pShot->param_d[1];

                radius += 2.5 * pShot->param_i[1];
                if (radius >= 210.0*1.5) {
                    radius = 210.0*1.5;
                    pShot->param_i[1] = -1;
                }
                if (radius <= 18.0) {
                    radius = 18.0;
                    pShot->param_i[1] = 1;
                    pShot->param_d[1] += DX_PI / 36.0;
                }

                pShot->param_d[0] = radius;
                pShot->muki = angle + (pShot->param_i[1] > 0 ? 0.0 : DX_PI);
                pShot->x = pEnemyShotSet->x + cos(angle) * radius;
                pShot->y = pEnemyShotSet->y + sin(angle) * radius;
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }

        pShot = pShot->next;
    }

    // 青フェーズ：外へ逃げた鱗弾が戻ってくる波。
    if (phase == ZENJI_BLUE && pEnemyShotSet->count % 32 == 0) {
        const double wave = (double)((pEnemyShotSet->count / 32) % 4);
        for (int i = 0; i < 6; i++) {
            const double angle = ZENJI_PI2 * i / 6.0 + wave * DX_PI / 12.0;
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * 34.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * 34.0;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotScale[4];
            pEnemyShot->param_i[0] = ZENJI_BUTTERFLY;
            pEnemyShot->param_i[1] = 1;
            pEnemyShot->param_d[0] = 34.0;
            pEnemyShot->param_d[1] = angle;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 赤フェーズ：複数方向からの自機狙い鱗弾。
    if (phase == ZENJI_RED && pEnemyShotSet->count % 24 == 0) {
        for (int i = 0; i < 8; i++) {
            const double angle = ZENJI_PI2 * i / 8.0 + rotate * 0.5;
            const double x = pEnemyShotSet->x + cos(angle) * 32.0;
            const double y = pEnemyShotSet->y + sin(angle) * 32.0;
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x;
            pEnemyShot->y = y;
            pEnemyShot->muki = atan2(player.y - y, player.x - x);
            pEnemyShot->speed = 2.8;
            pEnemyShot->kind = img_enemyShotScale[0];
            pEnemyShot->param_i[0] = ZENJI_BUTTERFLY;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Zendera_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 148.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.75 * (double)muki;
        if (count % 140 == 70) muki *= -1;
        if (enemy.x < 130.0) {
            enemy.x = 130.0;
            muki = 1;
        }
        if (enemy.x > 350.0) {
            enemy.x = 350.0;
            muki = -1;
        }
    }

    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotZenji;
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

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
}
