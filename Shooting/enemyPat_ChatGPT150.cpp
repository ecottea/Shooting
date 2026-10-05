#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// サボテン本体を構成する弾を追加する。
static void AddCactusBodyShot(sEnemyShotSet* pEnemyShotSet, double x, double y, bool joint)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = pEnemyShotSet->x + x;
    pEnemyShot->y = pEnemyShotSet->y + y;
    pEnemyShot->muki = DX_PI / 2.0;
    pEnemyShot->speed = 0.0;
    pEnemyShot->kind = joint ? img_enemyShotMediumBall[2] : img_enemyShotSmallBall[2];
    pEnemyShot->param_i[0] = 1; // サボテン本体
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// 針を追加する。銃弾を針に見立て、発射方向に合わせて向きを設定する。
static void AddCactusNeedle(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = pEnemyShotSet->x + x;
    pEnemyShot->y = pEnemyShotSet->y + y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = img_enemyShotBullet[8];
    pEnemyShot->param_i[0] = 2; // 針
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// 枝先からプレイヤーを狙う弾を追加する。
static void AddCactusAimedShot(sEnemyShotSet* pEnemyShotSet, double x, double y)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = pEnemyShotSet->x + x;
    pEnemyShot->y = pEnemyShotSet->y + y;
    pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
    pEnemyShot->speed = 2.6;
    pEnemyShot->kind = img_enemyShotMediumOval[5];
    pEnemyShot->param_i[0] = 3; // 枝先からの狙い弾
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// 弾幕：サボテン
static void ShotCactus(sEnemyShotSet* pEnemyShotSet)
{
    // サボテンの幹を下から上へ成長させる。
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        AddCactusBodyShot(pEnemyShotSet, 0.0, 16.0, true);
    }
    if (pEnemyShotSet->count == 6)  AddCactusBodyShot(pEnemyShotSet, 0.0, 28.0, false);
    if (pEnemyShotSet->count == 12) AddCactusBodyShot(pEnemyShotSet, 0.0, 40.0, false);
    if (pEnemyShotSet->count == 18) AddCactusBodyShot(pEnemyShotSet, 0.0, 52.0, false);
    if (pEnemyShotSet->count == 24) AddCactusBodyShot(pEnemyShotSet, 0.0, 64.0, true);
    if (pEnemyShotSet->count == 30) AddCactusBodyShot(pEnemyShotSet, 0.0, 76.0, false);
    if (pEnemyShotSet->count == 36) AddCactusBodyShot(pEnemyShotSet, 0.0, 88.0, true);

    // 左枝を横へ伸ばし、その先を上へ折り曲げる。
    if (pEnemyShotSet->count == 42) AddCactusBodyShot(pEnemyShotSet, -10.0, 52.0, false);
    if (pEnemyShotSet->count == 48) AddCactusBodyShot(pEnemyShotSet, -20.0, 52.0, true);
    if (pEnemyShotSet->count == 54) AddCactusBodyShot(pEnemyShotSet, -30.0, 52.0, false);
    if (pEnemyShotSet->count == 60) AddCactusBodyShot(pEnemyShotSet, -30.0, 40.0, true);
    if (pEnemyShotSet->count == 66) AddCactusBodyShot(pEnemyShotSet, -30.0, 28.0, true);
    if (pEnemyShotSet->count == 72) AddCactusBodyShot(pEnemyShotSet, -30.0, 16.0, true);

    // 右枝は少し遅れて伸ばし、左右非対称のサボテンにする。
    if (pEnemyShotSet->count == 48) AddCactusBodyShot(pEnemyShotSet, 10.0, 64.0, false);
    if (pEnemyShotSet->count == 54) AddCactusBodyShot(pEnemyShotSet, 20.0, 64.0, true);
    if (pEnemyShotSet->count == 60) AddCactusBodyShot(pEnemyShotSet, 30.0, 64.0, false);
    if (pEnemyShotSet->count == 66) AddCactusBodyShot(pEnemyShotSet, 30.0, 52.0, true);
    if (pEnemyShotSet->count == 72) AddCactusBodyShot(pEnemyShotSet, 30.0, 40.0, true);
    if (pEnemyShotSet->count == 78) AddCactusBodyShot(pEnemyShotSet, 30.0, 28.0, true);

    // 枝先・頂点から針を放射する。
    if (pEnemyShotSet->count >= 82 && (pEnemyShotSet->count - 82) % 40 == 0) {
        const double centerDirs[] = {
            DX_PI,
            0.0,
            -DX_PI / 2.0
        };
        const double baseX[] = { -30.0, 30.0, 0.0 };
        const double baseY[] = { 16.0, 28.0, 88.0 };

        for (int b = 0; b < 3; b++) {
            for (int i = 0; i < 5; i++) {
                double muki = centerDirs[b] + (i - 2) * 0.18;
                AddCactusNeedle(pEnemyShotSet, baseX[b], baseY[b] + 1.8 * max(pEnemyShotSet->count - 108, 0), muki, 2.5);
            }
        }

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 枝先からプレイヤー狙いの弾を飛ばし、中央へ少し圧力をかける。
    if (pEnemyShotSet->count == 96) {
        AddCactusAimedShot(pEnemyShotSet, -30.0, 16.0);
        AddCactusAimedShot(pEnemyShotSet, 30.0, 28.0);
        AddCactusAimedShot(pEnemyShotSet, 0.0, 88.0);

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // サボテン本体がまとまったまま落下し、針と重なった見た目になる。
    if (pEnemyShotSet->count == 108) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {
                pShot->muki = DX_PI / 2.0;
                pShot->speed = 1.8;
            }
            pShot = pShot->next;
        }
    }

    // すべての弾を移動させる。本体は速度0の間は形状を維持する。
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Cactus_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.85 * (double)muki;
        if (enemy.x < 75.0) muki = 1;
        if (enemy.x > 405.0) muki = -1;
    }

    // サボテンが完成する前後で間隔を取り、画面内で連続して現れるようにする。
    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCactus;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 5.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
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
