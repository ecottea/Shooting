#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 飛鉢「伝説の飛空円盤」
// 円形レール上を進む札弾を、3箇所から同時に展開する。
// 難易度はHard/Lunatic相当の弾数を採用。

static void FlyingSaucerShot(sEnemyShotSet* pEnemyShotSet)
{
    // 1回の発射で、3つの円盤状の弾幕を生成する。
    if (pEnemyShotSet->count == 0) {
        const int bulletNum = pEnemyShotSet->param_i[0];
        const int waveIndex = pEnemyShotSet->param_i[1];
        const int phase = pEnemyShotSet->param_i[2];
        const int color = pEnemyShotSet->param_i[3];
        const int aimMode = (waveIndex % 4 == 1 || waveIndex % 4 == 2);

        const double fixedAngle[3][3] = {
            { 0.92, 1.57, 2.22 },
            { 0.92, 1.57, 2.22 },
            { 0.92, 1.57, 2.22 }
        };

        for (int source = 0; source < 3; source++) {
            // ボスの左右と中央に発射位置を設ける。
            double startX = pEnemyShotSet->x + (source - 1) * 112.0*1.5;
            double startY = pEnemyShotSet->y + (source == 1 ? 38.0 : 66.0);

            double direction;
            if (aimMode) {
                // 自機狙いは発射時点の自機位置で固定する。
                direction = atan2(player.y - startY, player.x - startX);
            }
            else {
                // 固定波は自機位置に依存しない。
                direction = fixedAngle[waveIndex % 3][source];
                if (waveIndex % 4 == 3) {
                    if (source == 0) direction -= 0.18;
                    if (source == 2) direction += 0.18;
                }
            }

            const double centerSpeed = aimMode ? 0.78 + phase * 0.035 : 0.62 + phase * 0.025;
            const double centerVx = cos(direction) * centerSpeed;
            const double centerVy = sin(direction) * centerSpeed;
            const double ringFrequency = 0.026 + phase * 0.0012;
            const double ringSpin = ((source % 2) == 0 ? 1.0 : -1.0) * (0.0052 + phase * 0.00035);
            const double radiusMin = 28.0;
            const double radiusAmp = 70.0 + phase * 7.0;
            const double phaseOffset = (waveIndex % 5) * 0.31 + source * 0.47;

            for (int i = 0; i < bulletNum; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = startX;
                pEnemyShot->y = startY;
                pEnemyShot->muki = direction;
                pEnemyShot->speed = 0.0;
                // 素材は中楕円弾。札弾に近い輪郭で、円周上でも視認しやすい。
                pEnemyShot->kind = img_enemyShotMediumOval[color];
                pEnemyShot->margin = 240;

                pEnemyShot->param_i[0] = i;
                pEnemyShot->param_i[1] = bulletNum;
                pEnemyShot->param_i[2] = source;
                pEnemyShot->param_i[3] = phase;
                pEnemyShot->param_d[0] = startX;
                pEnemyShot->param_d[1] = startY;
                pEnemyShot->param_d[2] = direction;
                pEnemyShot->param_d[3] = phaseOffset;
                pEnemyShot->param_d[4] = ringSpin;
                pEnemyShot->param_d[5] = centerVx;
                pEnemyShot->param_d[6] = centerVy;
                pEnemyShot->param_d[7] = radiusMin*5;
                pEnemyShot->param_d[8] = radiusAmp;
                pEnemyShot->param_d[9] = ringFrequency;

                // sEnemyShot の双方向リスト末尾へ追加する。
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 円の中心を前進させながら、半径を周期的に拡大・縮小させる。
    // count はメインルーチン側で増加するため、この関数内では変更しない。
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double age = (double)pShot->count;
        const int index = pShot->param_i[0];
        const int bulletNum = pShot->param_i[1];
        const double startX = pShot->param_d[0];
        const double startY = pShot->param_d[1];
        const double phaseOffset = pShot->param_d[3];
        const double spin = pShot->param_d[4];
        const double centerVx = pShot->param_d[5];
        const double centerVy = pShot->param_d[6];
        const double radiusMin = pShot->param_d[7];
        const double radiusAmp = pShot->param_d[8];
        const double frequency = pShot->param_d[9];

        const double radiusPhase = age * frequency + phaseOffset;
        const double radius = radiusMin + radiusAmp * (0.5 - 0.5 * cos(radiusPhase));
        const double radiusVelocity = radiusAmp * 0.5 * frequency * sin(radiusPhase);
        const double angle = (2.0 * DX_PI * index / bulletNum) + phaseOffset + spin * age;

        const double centerX = startX + centerVx * age;
        const double centerY = startY + centerVy * age;
        const double cosAngle = cos(angle);
        const double sinAngle = sin(angle);

        pShot->x = centerX + radius * cosAngle;
        pShot->y = centerY + radius * sinAngle;

        // 楕円弾の向きを、円周上の移動方向（接線方向）に合わせる。
        const double velocityX = centerVx + radiusVelocity * cosAngle - radius * spin * sinAngle;
        const double velocityY = centerVy + radiusVelocity * sinAngle + radius * spin * cosAngle;
        pShot->muki = atan2(velocityY, velocityX);
        pShot = pShot->next;
    }
}

static void AddFlyingSaucerSet(int phase, int waveIndex)
{
    // 青→水色→黄→紫相当（マゼンタ）→赤。
    // 紫色の専用素材がないため、マゼンタを代用する。
    static const int bulletCount[6] = { 24, 24, 26, 28, 30, 32 };
    static const int bulletColor[6] = { 4, 3, 1, 5, 0, 0 };

    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = FlyingSaucerShot;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    pEnemyShotSet->kind = phase;
    pEnemyShotSet->param_i[0] = bulletCount[phase]*3/4;
    pEnemyShotSet->param_i[1] = waveIndex;
    pEnemyShotSet->param_i[2] = phase;
    pEnemyShotSet->param_i[3] = bulletColor[phase];

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 敵本体のパターン
void EnemyPat_HikouEnban_ChatGPT()
{
    static int nextShotFrame;
    static int waveIndex;
    static int currentPhase;
    static int frenzyStarted;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 46.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと

        nextShotFrame = 70;
        waveIndex = 0;
        currentPhase = -1;
        frenzyStarted = 0;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // ボスは画面上部を緩やかに往復する。
        enemy.x = 240.0 + 74.0 * sin(count * 0.006);
        enemy.y = 46.0 + 9.0 * sin(count * 0.011);
    }

    // HPによる攻撃段階。Hard/Lunatic相当の段階境界を使用する。
    int phase = 0;
    if (enemy.hp * 100 <= enemy.maxHp * 70) phase = 1;
    if (enemy.hp * 100 <= enemy.maxHp * 55) phase = 2;
    if (enemy.hp * 100 <= enemy.maxHp * 40) phase = 3;
    if (enemy.hp * 100 <= enemy.maxHp * 30) phase = 4;

    // 原作の制限時間120秒・残り33秒で始まる発狂を、60fps換算で再現。
    if (count >= 87 * 60) phase = 5;

    if (phase != currentPhase) {
        if (phase == 5 && frenzyStarted == 0) {
            frenzyStarted = 1;
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        currentPhase = phase;
    }

    // 発射間隔：青300、水250、黄230、紫200、赤150、発狂150フレーム。
    static const int fireInterval[6] = { 300, 250, 230, 200, 150, 150 };

    if (count >= nextShotFrame) {
        AddFlyingSaucerSet(phase, waveIndex);
        waveIndex++;

        // 1つの波につき効果音を1回だけ鳴らす。
        if (phase == 5) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        else {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        nextShotFrame = count + fireInterval[phase];
    }
}
