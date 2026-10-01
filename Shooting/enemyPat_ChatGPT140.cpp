#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// シュレッダー弾幕
//
// ・レーザー弾で「回転する刃」を表現
// ・刃が縮みながら回転した後、鱗弾＋小玉へ切り替えて「裁断片」を表現
// ・裁断片は放射＋緩やかな旋回で、帯状に飛び散る
// ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメントはメインルーチン側で行う想定
// ・画面外の弾の消去もメインルーチン側で行う想定

static void AddShredderShot(
    sEnemyShotSet* pEnemyShotSet,
    double x, double y,
    double muki, double speed,
    int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[0] = 1;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static void ShotShredder(sEnemyShotSet* pEnemyShotSet)
{
    // ------------------------------------------------------------
    // 初期化：レーザー弾を円形に並べて回転刃を作る
    // ------------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        const int bladeCount = 14;
        const double radius = 88.0;
        const double centerX = pEnemyShotSet->x;
        const double centerY = pEnemyShotSet->y + 36.0;

        for (int i = 0; i < bladeCount; i++) {
            const double a = 2.0 * DX_PI * i / bladeCount;
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = centerX + cos(a) * radius;
            pEnemyShot->y = centerY + sin(a) * radius;
            pEnemyShot->muki = a;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[5];

            pEnemyShot->param_i[0] = 0; // 0:刃 1:裁断片
            pEnemyShot->param_i[1] = i;
            pEnemyShot->param_d[0] = a;
            pEnemyShot->param_d[1] = radius;
            pEnemyShot->param_d[2] = 0.030 + 0.003 * (i % 3);
            pEnemyShot->param_d[3] = centerX;
            pEnemyShot->param_d[4] = centerY;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        // --------------------------------------------------------
        // 回転刃フェーズ
        // --------------------------------------------------------
        if (pShot->param_i[0] == 0) {
            if (pEnemyShotSet->count < 38) {
                // 円盤全体をゆっくり前進させながら回転
                pShot->param_d[4] += 1.55;
                pShot->param_d[1] = 88.0 - pEnemyShotSet->count * 0.20;
            }
            else if (pEnemyShotSet->count < 72) {
                // 裁断直前に半径を縮め、回転を加速させる
                pShot->param_d[4] += 2.45;
                pShot->param_d[1] -= 0.88;
            }
            else {
                // 仕上げの高速回転
                pShot->param_d[4] += 0.55;
                pShot->param_d[1] -= 0.24;
            }

            pShot->param_d[0] += pShot->param_d[2];

            pShot->x = pShot->param_d[3] + cos(pShot->param_d[0]) * pShot->param_d[1];
            pShot->y = pShot->param_d[4] + sin(pShot->param_d[0]) * pShot->param_d[1];
            pShot->muki = pShot->param_d[0];

            // 裁断する瞬間に刃そのものを細長い裁断片へ変化させる
            if (pEnemyShotSet->count == 78) {
                const double radial = pShot->param_d[0];
                pShot->param_i[0] = 1;
                pShot->kind = img_enemyShotScale[(pShot->param_i[1] % 3) + 3];
                pShot->speed = 3.3 + 0.15 * (pShot->param_i[1] % 4);
                pShot->muki = radial + ((pShot->param_i[1] % 3) - 1) * 0.10;
            }
        }
        // --------------------------------------------------------
        // 裁断片フェーズ
        // --------------------------------------------------------
        else {
            // 少しずつ向きを変えて、直線ではなく帯状に広がる
            pShot->muki += (pShot->param_i[1] % 2 == 0) ? 0.004 : -0.004;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }

    // ------------------------------------------------------------
    // 裁断直後：中央から細かい紙片が一気に噴き出す
    // ------------------------------------------------------------
    if (pEnemyShotSet->count == 78) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const double cx = pEnemyShotSet->x;
        const double cy = pEnemyShotSet->y + 92.0;
        const int burstCount = 56;

        for (int i = 0; i < burstCount; i++) {
            const double base = 2.0 * DX_PI * i / burstCount;
            const double jitter = (GetRand(40) - 20) / 1000.0 * DX_PI;
            const double a = base + jitter;
            const double spd = 2.25 + GetRand(90) / 100.0;
            const int col = (i % 4 == 0) ? 8 : ((i % 3 == 0) ? 6 : 3);

            AddShredderShot(
                pEnemyShotSet,
                cx + cos(a) * 8.0,
                cy + sin(a) * 8.0,
                a,
                spd,
                img_enemyShotSmallBall[col]
            );
        }

        // 裁断片の「帯」を複数本追加
        const int bandCount = 8;
        for (int b = 0; b < bandCount; b++) {
            const double a = DX_PI * 0.22 + DX_PI * 0.56 * b / (bandCount - 1);
            const int pieces = 7*2;

            for (int j = 0; j < pieces; j++) {
                const double side = (j - (pieces - 1) * 0.5) * 10.0;
                const double x = cx + cos(a + DX_PI * 0.5) * side;
                const double y = cy + sin(a + DX_PI * 0.5) * side;
                const double pieceAngle = a + ((j % 2 == 0) ? -0.035 : 0.035);

                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = x;
                pEnemyShot->y = y;
                pEnemyShot->muki = pieceAngle;
                pEnemyShot->speed = 3.0 + j * 0.08;
                pEnemyShot->kind = img_enemyShotScale[(b + 4) % 8];
                pEnemyShot->param_i[0] = 2; // 2:帯状の裁断片
                pEnemyShot->param_i[1] = b;
                pEnemyShot->param_i[2] = j;

                pEnemyShot->param_d[0] = pieceAngle;
                pEnemyShot->param_d[1] = 0.0035 * ((b % 2 == 0) ? 1.0 : -1.0);

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ------------------------------------------------------------
    // 帯状の裁断片：少しずつ曲がりながら落ちていく
    // ------------------------------------------------------------
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 2) {
            pShot->param_d[0] += pShot->param_d[1];
            pShot->muki = pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Shredder_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.72 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // シュレッダーの刃を一定間隔で生成
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShredder;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
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
