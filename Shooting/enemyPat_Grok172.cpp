#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ヘリンボーン・ウェーブ
// 1つのショットセット内で、交互に傾きを変えたV字/Λ字の弾列を
// 時間差で発射し、ヘリンボーン模様を形成する
static void ShotHerringbone(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射間隔（フレーム）
    const int interval = 32;
    // 最初の発射までのオフセット
    const int firstDelay = 20;

    // このフレームで新しい波を撃つか判定
    if (pEnemyShotSet->count >= firstDelay &&
        (pEnemyShotSet->count - firstDelay) % interval == 0) {

        // 中弾の発射音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 波番号（V/Λの切り替えに使用）
        int wave = (pEnemyShotSet->count - firstDelay) / interval;

        // パラメータ
        int isInverted = wave % 2;          // 0:V , 1:Λ
        int bulletsPerArm = 6;
        double baseSpeed = 2.15;
        double openAngle = 0.52 + (wave % 3) * 0.04;

        // 中心方向（真下）
        double centerMuki = DX_PI / 2.0;
        double leftMuki, rightMuki;
        if (isInverted == 0) {
            leftMuki = centerMuki - openAngle;
            rightMuki = centerMuki + openAngle;
        }
        else {
            leftMuki = centerMuki + openAngle;
            rightMuki = centerMuki - openAngle;
        }

        double spacing = 14.0;

        // 左右の腕を生成
        for (int arm = 0; arm < 2; arm++) {
            double muki = (arm == 0) ? leftMuki : rightMuki;
            int colorBase = (arm == 0) ? 4 : 3;  // 4:青 , 3:シアン

            for (int i = 0; i < bulletsPerArm; i++) {
                pEnemyShot = new sEnemyShot;

                double offset = (i - (bulletsPerArm - 1) / 2.0) * spacing;
                pEnemyShot->x = pEnemyShotSet->x + offset * cos(muki - DX_PI / 2.0);
                pEnemyShot->y = pEnemyShotSet->y + offset * sin(muki - DX_PI / 2.0);

                pEnemyShot->muki = muki;
                pEnemyShot->speed = baseSpeed + (i % 3) * 0.08;
                pEnemyShot->kind = img_enemyShotMediumBall[colorBase];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    pEnemyShotSet->y += 1.0;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
// count==1 のタイミングでショットセットを1個だけ作成する
void EnemyPat_Herringbone_Grok()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        // 左右にゆっくり往復移動
        enemy.x += 0.85 * (double)muki;
        if (enemy.x < 80.0) {
            enemy.x = 80.0;
            muki = 1;
        }
        if (enemy.x > 400.0) {
            enemy.x = 400.0;
            muki = -1;
        }
        if (count % 180 == 90) muki *= -1;
    }

    if (count % 120 == 1) {
        // ショットセットを1個だけ生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHerringbone;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
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