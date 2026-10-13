#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 超新星爆発「スーパーノヴァ・リバース」
// カウンタの加算、画面外の弾の削除、空になった弾セットの削除はメインルーチン側で行う。

// 弾を弾セットの末尾へ追加する。
static void SupernovaAddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 弾セットを作成して、グローバルの弾セットリストへ接続する。
static sEnemyShotSet* SupernovaCreateSet(double x, double y,
    sEnemyShotSet::PatternFunc patternFunc)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = 0.0;
    pSet->patternFunc = patternFunc;
    pSet->kind = 0;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

static void SupernovaRestartSound(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

// チャージ：周囲に現れた小さな光が、中心へ吸い込まれる。
static void SupernovaCharge(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int shotNum = 40;
        for (int i = 0; i < shotNum; ++i) {
            double angle = 2.0 * DX_PI * i / shotNum;
            double radius = 78.0 + 7.0 * sin(angle * 3.0);
            double x = pSet->x + radius * cos(angle);
            double y = pSet->y + radius * sin(angle);
            double speed = 1.15 + (i % 4) * 0.13;
            int color = (i % 3 == 0) ? 6 : ((i % 3 == 1) ? 3 : 4);
            SupernovaAddShot(pSet, x, y, angle + DX_PI, speed,
                img_enemyShotSmallBall[color]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 爆発の衝撃波：白い大玉と橙色の楕円弾が、二重の輪になって広がる。
static void SupernovaShockwave(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int shotNum = 40;
        for (int i = 0; i < shotNum; ++i) {
            double angle = 2.0 * DX_PI * i / shotNum;
            SupernovaAddShot(pSet, pSet->x, pSet->y, angle, 4.35,
                img_enemyShotLargeBall[6]);
            SupernovaAddShot(pSet, pSet->x, pSet->y,
                angle + DX_PI / shotNum, 3.45,
                img_enemyShotMediumOval[8]);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 星骸：外へ飛び散った弾が減速し、再加速しながら緩やかに曲がる。
// 一部はプレイヤーへ向きを変え、残りは左右へ弧を描いて余波を表現する。
static void SupernovaDebris(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        const int shotNum = 56;
        for (int i = 0; i < shotNum; ++i) {
            double angle = 2.0 * DX_PI * i / shotNum
                + (GetRand(100) - 50) * 0.001;
            double radius = GetRand(12) * 0.2;
            double x = pSet->x + radius * cos(angle);
            double y = pSet->y + radius * sin(angle);
            double speed = 1.8 + GetRand(210) / 100.0;
            int kind;
            switch (i % 4) {
            case 0:
                kind = img_enemyShotMediumBall[8];  // 橙色の星骸
                break;
            case 1:
                kind = img_enemyShotDiamond[6];     // 白い破片
                break;
            case 2:
                kind = img_enemyShotScale[3];       // シアンの破片
                break;
            default:
                kind = img_enemyShotSmallBall[4];   // 青い光の粒
                break;
            }

            SupernovaAddShot(pSet, x, y, angle, speed, kind);
            sEnemyShot* pShot = pSet->pEnemyShotHead->prev;
            pShot->param_d[0] = speed;          // 基準速度
            pShot->param_i[0] = i % 3;          // 0:追尾、1:右カーブ、2:左カーブ
            pShot->param_d[1] = (i % 2 == 0) ? 1.0 : -1.0;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double age = (double)pShot->count;
        double baseSpeed = pShot->param_d[0];

        // 飛散直後は進み、途中で減速し、その後ふたたび加速する。
        if (age < 34.0) {
            pShot->speed = baseSpeed;
        }
        else if (age < 58.0) {
            pShot->speed = baseSpeed * (1.0 - 0.78 * (age - 34.0) / 24.0);
        }
        else if (age < 88.0) {
            pShot->speed = baseSpeed * (0.22 + 0.78 * (age - 58.0) / 30.0);
        }
        else {
            pShot->speed = baseSpeed * 1.05;
        }

        // 進行方向を瞬間移動させず、少しずつ曲げる。
        if (age >= 52.0 && age < 130.0) {
            if (pShot->param_i[0] == 0) {
                double target = atan2(player.y - pShot->y, player.x - pShot->x);
                double difference = atan2(sin(target - pShot->muki),
                    cos(target - pShot->muki));
                const double maxTurn = 0.023;
                if (difference > maxTurn) difference = maxTurn;
                if (difference < -maxTurn) difference = -maxTurn;
                pShot->muki += difference;
            }
            else {
                pShot->muki += (pShot->param_i[0] == 1 ? 1.0 : -1.0)
                    * 0.016;
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体の弾幕パターン
void EnemyPat_Supernova_ChatGPT()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 95.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 爆発の中心がゆっくり移動し、弾幕の広がり方に変化をつける。
    double t = (double)(count - 1);
    enemy.x = 240.0 + 65.0 * sin(t * 0.007);
    enemy.y = 95.0 + 14.0 * sin(t * 0.013);

    // 360フレーム周期。前半は収縮する光、中央で爆発、その後に星骸が飛散する。
    int phase = (count - 1) % 360;

    if (phase == 0) {
        SupernovaRestartSound(sound_enemyCharge);
    }

    // 収縮する光の輪を重ね、爆発前のエネルギー集中を表現する。
    if (phase == 1 || phase == 17 || phase == 33
        || phase == 49 || phase == 65 || phase == 81) {
        SupernovaCreateSet(enemy.x, enemy.y, SupernovaCharge);
    }

    // 超新星爆発の瞬間。
    if (phase == 85) {
        SupernovaRestartSound(sound_enemyShot_extreme);
        SupernovaCreateSet(enemy.x, enemy.y, SupernovaShockwave);
    }

    // 爆発に遅れて複数回、色や大きさの異なる星骸が飛び散る。
    if (phase == 89 || phase == 98 || phase == 107 || phase == 116) {
        SupernovaCreateSet(enemy.x, enemy.y, SupernovaDebris);
    }
}
