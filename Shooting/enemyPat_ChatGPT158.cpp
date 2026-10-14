#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 二重スリット「干渉する弾道」
// 画面上部からスリットへ粒子弾を送り、通過後は干渉縞を模した複数の軌道へ分岐させる。
// count / sEnemyShotSet::count / sEnemyShot::count の加算と画面外弾の削除はメインルーチン側で行う。

static const double DS_GATE_Y = 158.0;
static const double DS_LEFT_SLIT_X = 180.0;
static const double DS_RIGHT_SLIT_X = 300.0;
static const double DS_SINGLE_SLIT_X = 240.0;
static const double DS_TARGET_Y = 470.0;
static const int DS_CYCLE = 480;

static void AddEnemyShotSet(sEnemyShotSet* pEnemyShotSet)
{
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

static sEnemyShotSet* CreateEnemyShotSet(sEnemyShotSet::PatternFunc patternFunc)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    AddEnemyShotSet(pEnemyShotSet);
    return pEnemyShotSet;
}

static void AddShotToSet(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static void PlayPatternSound(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

// スリットを表す横一列の弾。二重スリット時は左右二か所、単スリット時は中央だけを開ける。
static void ShotDoubleSlitScreen(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const bool singleSlit = pEnemyShotSet->param_i[0] != 0;
        const double slitX1 = singleSlit ? DS_SINGLE_SLIT_X : DS_LEFT_SLIT_X;
        const double slitX2 = DS_RIGHT_SLIT_X;
        const double gapHalfWidth = 19.0;

        for (int i = 0; i <= 55; ++i) {
            const double x = 20.0 + i * 8.0;
            if (fabs(x - slitX1) < gapHalfWidth) continue;
            if (!singleSlit && fabs(x - slitX2) < gapHalfWidth) continue;

            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x;
            pEnemyShot->y = DS_GATE_Y;
            pEnemyShot->speed = 0.0;
            pEnemyShot->muki = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白い中玉で障壁を描く
            pEnemyShot->param_i[0] = 1; // このセットの寿命管理用
            AddShotToSet(pEnemyShotSet, pEnemyShot);
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        // スリットの表示時間が終わったら弾列を消す。画面外弾の削除とは別の、パターン固有の寿命処理。
        if (pShot->count >= 205) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        pShot = pNext;
    }
}

// 一粒の弾がスリットへ向かい、通過した瞬間に干渉縞の軌道へ進む。
static void ShotDoubleSlitParticle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const bool singleSlit = pEnemyShotSet->param_i[1] != 0;
        const int slitIndex = pEnemyShotSet->param_i[0];
        const double targetX = singleSlit ? DS_SINGLE_SLIT_X : (slitIndex == 0 ? DS_LEFT_SLIT_X : DS_RIGHT_SLIT_X);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = atan2(DS_GATE_Y - pEnemyShot->y, targetX - pEnemyShot->x);
        pEnemyShot->speed = 4.5;
        pEnemyShot->kind = img_enemyShotSmallBall[6]; // スリットへ向かう白い粒子
        pEnemyShot->param_i[0] = slitIndex;
        pEnemyShot->param_i[1] = singleSlit ? 1 : 0;
        pEnemyShot->param_i[2] = pEnemyShotSet->param_i[2]; // 干渉縞の番号
        pEnemyShot->param_i[3] = 0; // スリット通過前
        AddShotToSet(pEnemyShotSet, pEnemyShot);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[3] == 0 && pShot->y >= DS_GATE_Y) {
            const bool singleSlit = pShot->param_i[1] != 0;
            const int slitIndex = pShot->param_i[0];
            const int fringeIndex = pShot->param_i[2];
            double targetX;

            if (singleSlit) {
                // 一方のスリットだけを開けると、はっきりした干渉縞ではなく広い回折状の扇になる。
                targetX = 105.0 + (fringeIndex % 11) * 27.0;
                pShot->kind = img_enemyShotSmallBall[1]; // 黄色
            }
            else {
                // 二つのスリットから出た粒子は、同じ明線位置へ向かう。
                // 明るい筋の間には弾がほとんど通らず、暗い干渉縞として見える。
                targetX = 240.0 + (fringeIndex % 7 - 3) * 32.0;
                pShot->kind = (slitIndex == 0) ? img_enemyShotSmallBall[4] : img_enemyShotSmallBall[3]; // 青 / シアン
            }

            pShot->muki = atan2(DS_TARGET_Y - pShot->y, targetX - pShot->x);
            pShot->param_i[3] = 1; // 通過済み
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

static void SpawnDoubleSlitScreen(bool singleSlit)
{
    sEnemyShotSet* pEnemyShotSet = CreateEnemyShotSet(ShotDoubleSlitScreen);
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = DS_GATE_Y;
    pEnemyShotSet->param_i[0] = singleSlit ? 1 : 0;
}

static void SpawnDoubleSlitParticle(bool singleSlit, int slitIndex, int fringeIndex)
{
    sEnemyShotSet* pEnemyShotSet = CreateEnemyShotSet(ShotDoubleSlitParticle);
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 12.0;
    pEnemyShotSet->param_i[0] = slitIndex;
    pEnemyShotSet->param_i[1] = singleSlit ? 1 : 0;
    pEnemyShotSet->param_i[2] = fringeIndex;
}

// 敵本体の弾幕パターン
void EnemyPat_DoubleSlit_ChatGPT()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 42.0;
        enemy.maxHp = enemy.hp = 200; // 仕様上、変更しない
    }
    else {
        enemy.x = 240.0 + 70.0 * sin(count * 0.018);
        enemy.y = 42.0 + 4.0 * sin(count * 0.025);
    }

    const int phase = (count - 1) % DS_CYCLE;

    // 二重スリットを設置。粒子は左右の開口へ交互に向かう。
    if (phase == 0) {
        SpawnDoubleSlitScreen(false);
        PlayPatternSound(sound_enemyShot_medium);
    }
    if (phase >= 12 && phase <= 144 && (phase - 12) % 3 == 0) {
        const int particleIndex = (phase - 12) / 3;
        const int slitIndex = particleIndex % 2; // 左右交互
        const int fringeIndex = (particleIndex / 2) % 7;
        SpawnDoubleSlitParticle(false, slitIndex, fringeIndex);
        if ((phase - 12) % 36 == 0) PlayPatternSound(sound_enemyShot_light);
    }

    // 片方を閉じる前に予告音。その後は中央のスリットだけを開ける。
    if (phase == 228) PlayPatternSound(sound_enemyCharge);
    if (phase == 240) {
        SpawnDoubleSlitScreen(true);
        PlayPatternSound(sound_enemyShot_medium);
    }
    if (phase >= 252 && phase <= 371 && (phase - 252) % 4 == 0) {
        const int fringeIndex = (phase - 252) / 4;
        SpawnDoubleSlitParticle(true, 0, fringeIndex);
        if ((phase - 252) % 35 == 0) PlayPatternSound(sound_enemyShot_light);
    }
}
