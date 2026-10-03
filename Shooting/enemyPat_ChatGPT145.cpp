#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：逃げるボス
// 敵本体が被弾したことを enemy.hp の減少から検知し、
// 被弾地点から弾を撒きながらプレイヤーから離れる。
static void ShotEscape(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // ボスが逃げた直後の位置から、円形に弾を放つ。
        for (int i = 0; i < 16*2; i++) {
            pEnemyShot = new sEnemyShot;
            const double muki = 2.0 * DX_PI * i / 16.0/2;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = muki;
            pEnemyShot->speed = 2.2 + (i % 2) * 0.45;
            pEnemyShot->kind = img_enemyShotMediumBall[(i % 2 == 0) ? 3 : 5];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 円形弾の隙間を縫うように、少数の楕円弾を追加する。
        // 楕円弾の muki は進行方向に合わせて設定する。
        for (int i = 0; i < 8*2; i++) {
            pEnemyShot = new sEnemyShot;
            const double muki = 2.0 * DX_PI * (i + 0.5) / 8.0/2;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = muki;
            pEnemyShot->speed = 1.45;
            pEnemyShot->kind = img_enemyShotMediumOval[8];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

static double ClampBossPos(double value)
{
    if (value < 40.0) return 40.0;
    if (value > 440.0) return 440.0;
    return value;
}

// 敵本体のパターン
void EnemyPat_Escape_ChatGPT()
{
    static int previousHp;
    static int noHitCount;
    static int escapeChain;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 70; // 200から変更しないこと

        previousHp = enemy.hp;
        noHitCount = 0;
        escapeChain = 0;
    }
    else {
        const bool damaged = enemy.hp < previousHp;

        if (damaged) {
            const double oldX = enemy.x;
            const double oldY = enemy.y;

            // 連続して撃たれるほど逃走距離を伸ばす。
            escapeChain++;
            if (escapeChain > 4) escapeChain = 4;
            noHitCount = 0;

            // プレイヤーから離れる基本方向。
            double dx = oldX - player.x;
            double dy = oldY - player.y;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist < 0.001) {
                dx = 0.0;
                dy = -1.0;
                dist = 1.0;
            }

            const double baseAngle = atan2(dy, dx);
            const double escapeDistance = 70.0 + 25.0 * escapeChain;

            // 画面端が近い場合でも、プレイヤーからより離れられる方向を選ぶ。
            const double candidateOffset[] = {
                0.0,
                0.55, -0.55,
                1.10, -1.10,
                1.65, -1.65,
                DX_PI, 2.20, -2.20
            };

            double bestX = oldX;
            double bestY = oldY;
            double bestAngle = baseAngle;
            double bestDistance = -1.0;

            for (int i = 0; i < 9; i++) {
                const double angle = baseAngle + candidateOffset[i];
                const double targetX = ClampBossPos(oldX + cos(angle) * escapeDistance);
                const double targetY = ClampBossPos(oldY + sin(angle) * escapeDistance);

                const double pdx = targetX - player.x;
                const double pdy = targetY - player.y;
                const double playerDistance = pdx * pdx + pdy * pdy;

                if (playerDistance > bestDistance) {
                    bestDistance = playerDistance;
                    bestX = targetX;
                    bestY = targetY;
                    bestAngle = angle;
                }
            }

            // 弾を撒いた直後に、その場から大きく離れる。
            enemy.x = bestX;
            enemy.y = bestY;

            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotEscape;
            pEnemyShotSet->x = oldX;
            pEnemyShotSet->y = oldY;
            pEnemyShotSet->muki = bestAngle;
            pEnemyShotSet->kind = escapeChain;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
        else {
            noHitCount++;

            // 攻撃が止まると、少しずつ中央上部へ戻っていく。
            enemy.x += (240.0 - enemy.x) * 0.018;
            enemy.y += (80.0 - enemy.y) * 0.018;

            if (noHitCount > 45) {
                escapeChain = 0;
            }
        }

        previousHp = enemy.hp;
    }
}
