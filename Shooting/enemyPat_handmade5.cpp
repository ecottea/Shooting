// EnemyPat_Tmp.cpp
// EnemyDobur の弾幕を忠実に再現（改良版）
// ・弾の count インクリメント、画面外消去はメインルーチン側で行われる
// ・ShotSet の kind を speed * 100 として保存し、内部で利用

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  各弾幕パターン（敵本体 EnemyPat_Tmp から呼ばれる）
// ------------------------------------------------------------

// くろいまなざし（二重リング、プレイヤー位置に出現、加速度による独自の動き）
static void PatternMeanLook(sEnemyShotSet* pSet)
{
    // 最終地点へ向けて移動
    const double TARGET_X = 240.0;
    const double TARGET_Y = 320.0;
    const double MOVE_RATIO = 1.0 / 60.0;

    double dx = (TARGET_X - pSet->x) * MOVE_RATIO;
    double dy = (TARGET_Y - pSet->y) * MOVE_RATIO;
    pSet->x += dx;
    pSet->y += dy;

    const int NUM = 100;
    const double RADIUS = 250.0;
    const double BASE_ANGLE_SPEED = 2.0 * DX_PI / 360.0;  // 基本角速度

    // 最初のリング（時計回り）
    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        for (int i = 0; i < NUM; i++) {
            double theta = 2.0 * DX_PI * i / NUM;
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x + RADIUS * cos(theta);
            p->y = pSet->y + RADIUS * sin(theta);
            p->muki = theta + DX_PI / 2.0;    // 時計回り（接線方向）
            p->speed = RADIUS * BASE_ANGLE_SPEED;  // 初速
            p->kind = img_enemyShotScale[7];  // 黒い鱗弾
            p->margin = 9999;

            // リストに追加
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // 二重目のリング（反時計回り、60フレーム後）
    if (pSet->count == 60) {
        for (int i = 0; i < NUM; i++) {
            double theta = 2.0 * DX_PI * i / NUM;
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x + RADIUS * cos(theta);
            p->y = pSet->y + RADIUS * sin(theta);
            p->muki = theta - DX_PI / 2.0;    // 反時計回り
            p->speed = RADIUS * BASE_ANGLE_SPEED;
            p->kind = img_enemyShotScale[7];
            p->margin = 9999;
            
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // 全弾の更新（加速度を再現）
    sEnemyShot* p = pSet->pEnemyShotHead->next; int cnt = 0;
    while (p != pSet->pEnemyShotHead) {
        // セット中心の移動に追従
        p->x += dx;
        p->y += dy;

        // 弾の経過フレーム数（メイン側でインクリメント）
        int t = p->count;
        // 速度の減速（120フレームかけて初速→半分）
        double speed = RADIUS * BASE_ANGLE_SPEED;
        if (t < 120)
            speed = speed * (1.0 - t / 240.0);  // 120フレームで半減
        else
            speed = speed * 0.5;
        p->speed = speed;

        // 回転方向の判定（接線方向の符号）
        int sign = (cnt++ < NUM) ? 1 : -1;

        // 角速度：基本値→3倍まで加速（120フレーム）
        double angleSpd = BASE_ANGLE_SPEED * sign;
        if (t < 120)
            angleSpd = angleSpd * (1.0 + 2.0 * t / 120.0);  // 最終3倍
        else
            angleSpd = angleSpd * 3.0;

        // 回転と移動
        p->muki += angleSpd;
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);

        p = p->next;
    }
}

// n-way 胞子（小玉・緑・低速）
static void PatternSpore(sEnemyShotSet* pSet)
{
    double speed = (double)pSet->kind / 100.0;

    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        const int WAY = 10;
        const double OPEN = DX_PI * 2.0 / 3.0;
        double baseAngle = pSet->muki;   // 真下
        for (int i = 0; i < WAY; i++) {
            double angle = baseAngle + OPEN * ((double)i / (WAY - 1) - 0.5);
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = angle;
            p->speed = speed;
            p->kind = img_enemyShotSmallBall[2]; // 小玉・緑
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }

    if (pSet->pEnemyShotHead->next == pSet->pEnemyShotHead) {
        pSet->prev->next = pSet->next;
        pSet->next->prev = pSet->prev;
        delete pSet->pEnemyShotHead;
        delete pSet;
    }
}

// ロックオン（黄色弾、自機に近づいて停止）
static void PatternLockOn(sEnemyShotSet* pSet)
{
    double speed = (double)pSet->kind / 150.0;

    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        const int NUM = 45;
        const double CX = 240.0, CY = 320.0;
        const double RADIUS = 200.0;
        for (int i = 0; i < NUM; i++) {
            double theta = pSet->muki + 2.0 * DX_PI * i / NUM;
            sEnemyShot* p = new sEnemyShot;
            p->x = CX + RADIUS * cos(theta);
            p->y = CY + RADIUS * sin(theta);
            p->muki = atan2(player.y - p->y, player.x - p->x);
            p->speed = speed;
            p->kind = img_enemyShotMediumBall[1]; // 黄色の中玉
            p->margin = 9999;
            
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double dx = player.x - p->x;
        double dy = player.y - p->y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist < 55.0) {
            p->speed = 0.0;
        }
        else {
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }

    if (pSet->pEnemyShotHead->next == pSet->pEnemyShotHead) {
        pSet->prev->next = pSet->next;
        pSet->next->prev = pSet->prev;
        delete pSet->pEnemyShotHead;
        delete pSet;
    }
}

// つのドリル（V字型、進行方向後方に開く）
static void PatternHornDrill(sEnemyShotSet* pSet)
{
    double speed = (double)pSet->kind / 100.0;

    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        const int LINE = 18;
        double angle = pSet->muki;
        double perp1 = angle + DX_PI / 2.0;
        double perp2 = angle - DX_PI / 2.0;
        for (int i = 0; i < LINE; i++) {
            // V字の上側（進行方向後方へ伸びる）
            sEnemyShot* p1 = new sEnemyShot;
            p1->x = pSet->x - i * (12.0 * cos(angle) + 5.0 * cos(perp1));
            p1->y = pSet->y - i * (12.0 * sin(angle) + 5.0 * sin(perp1));
            p1->muki = angle;
            p1->speed = speed;
            p1->kind = img_enemyShotScale[0];    // 赤い鱗弾
            p1->margin = 9999;
            p1->prev = pSet->pEnemyShotHead->prev;
            p1->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p1;
            pSet->pEnemyShotHead->prev = p1;

            // V字の下側
            sEnemyShot* p2 = new sEnemyShot;
            p2->x = pSet->x - i * (12.0 * cos(angle) + 5.0 * cos(perp2));
            p2->y = pSet->y - i * (12.0 * sin(angle) + 5.0 * sin(perp2));
            p2->muki = angle;
            p2->speed = speed;
            p2->kind = img_enemyShotScale[0];
            p2->margin = 9999;
            p2->prev = pSet->pEnemyShotHead->prev;
            p2->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p2;
            pSet->pEnemyShotHead->prev = p2;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }

    if (pSet->pEnemyShotHead->next == pSet->pEnemyShotHead) {
        pSet->prev->next = pSet->next;
        pSet->next->prev = pSet->prev;
        delete pSet->pEnemyShotHead;
        delete pSet;
    }
}

// ------------------------------------------------------------
//  敵本体パターン（EnemyDobur 再現）
// ------------------------------------------------------------
void EnemyPat_Smeargle()
{
    static int phase = 0;
    static int phaseStartCount = 0;

    // つのドリル用の移動変数
    static double distX_seg = 0.0, distY_seg = 0.0;
    static double speedX = 0.0, speedY = 0.0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 300;
        phase = 0;
        phaseStartCount = 1;
    }

    int t = count - phaseStartCount;

    switch (phase) {

        // フェイズ0：くろいまなざし（二重）
    case 0:
        if (t == 1) {
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        else if (t == 70) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = PatternMeanLook;
            pSet->x = player.x;      // 自機位置から出現
            pSet->y = player.y;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        else if (t == 270) {
            phase = 1;
            phaseStartCount = count;
        }
        break;

        // フェイズ1：キノコのほうし（小玉・緑・低速）
    case 1:
        if (t == 1) {
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        if (t >= 70 && t < 70 + 30 * 15 && (t - 70) % 15 == 0) {
            int n = (t - 70) / 15;
            if (n < 30) {
                double x = GetRand(480);
                double y = GetRand(48);
                // 低速化（元の7倍→1.5倍に）
                double rawSpeed = (double)(n % 7) / 6.0 * 1.3 + 0.9;
                double speed = rawSpeed * 1.5;
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = PatternSpore;
                pSet->x = x;
                pSet->y = y;
                pSet->muki = DX_PI / 2.0;
                pSet->kind = (int)(speed * 100.0);
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
        if (t >= 600) {
            phase = 2;
            phaseStartCount = count;
        }
        break;

        // フェイズ2：ロックオン（黄色、停止距離拡大）
    case 2:
        if (t == 1) {
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        else if (t == 70) {
            for (int i = 0; i < 3; i++) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = PatternLockOn;
                pSet->x = 0.0;
                pSet->y = 0.0;
                pSet->muki = DX_PI * 2.0 / 45.0 * i / 3.0;
                double spd = 1.5 + i * 0.75;
                pSet->kind = (int)(spd * 100.0);
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
        else if (t == 350) {
            phase = 3;
            phaseStartCount = count;
        }
        break;

        // フェイズ3：連続つのドリル（V字、無限ループ）
    case 3:
    {
        const int INTERVAL = 180;
        const int DECEL_COUNT = 70;
        const double SUM = (DECEL_COUNT - 1) * DECEL_COUNT / 2.0;
        int phaseLocal = (count - phaseStartCount) % INTERVAL;

        if (phaseLocal == 1) {
            distX_seg = 240.0 - enemy.x;
            distY_seg = 70.0 - enemy.y;
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        else if (phaseLocal == DECEL_COUNT) {
            distX_seg = player.x - enemy.x;
            distY_seg = player.y - enemy.y;
            double angle = atan2(distY_seg, distX_seg);
            double distToPlayer = sqrt(distX_seg * distX_seg + distY_seg * distY_seg);
            double speed = distToPlayer / 30.0;
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = PatternHornDrill;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = angle;
            pSet->kind = (int)(speed * 100.0);
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        double factor = 0.0;
        if (phaseLocal < DECEL_COUNT)
            factor = (double)(DECEL_COUNT - phaseLocal) / SUM;
        else if (phaseLocal < 2 * DECEL_COUNT)
            factor = (double)(2 * DECEL_COUNT - phaseLocal) / SUM;

        speedX = distX_seg * factor;
        speedY = distY_seg * factor;
        enemy.x += speedX;
        enemy.y += speedY;
    }
    break;
    }
}