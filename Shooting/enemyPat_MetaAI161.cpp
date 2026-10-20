#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//============================================================
// A: 狭広交互 10way 全方位 回転弾
//============================================================
static void ShotA_LunaticGun(sEnemyShotSet* pSet)
{
    // ボスに追従
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    if (pSet->count == 0) {
        pSet->param_d[0] = 0.0; // base回転角
    }

    const int INTERVAL = 4; // 何Fごとに10wayを撃つか
    const double NARROW = 12.0 * DX_PI / 180.0;
    const double WIDE = 60.0 * DX_PI / 180.0; // NARROW+WIDE = 72° = 360°/5
    const double ROT = 4.0 * DX_PI / 180.0;  // 時計回り = +方向

    if (pSet->count % INTERVAL == 0) {
        if (pSet->count % (INTERVAL*2) == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        double base = pSet->param_d[0];
        double cur = base;
        for (int i = 0; i < 10; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = cur;
            // 少し速度にばらつき
            p->speed = 3.0 + GetRand(30) / 100.0;
            // 銃符なので銃弾、黄色
            p->kind = img_enemyShotBullet[1];

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;

            cur += (i % 2 == 0) ? NARROW : WIDE;
        }
        pSet->param_d[0] = base + ROT;
        if (pSet->param_d[0] > DX_TWO_PI) pSet->param_d[0] -= DX_TWO_PI;
    }

    // 弾移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//============================================================
// B: 逆三角形の密な弾列 x5 を2セット
//============================================================
static void ShotB_LunaticGun(sEnemyShotSet* pSet)
{
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;

    const int WAVE_INTERVAL = 8;  // 三角形同士の僅かな時間差
    const int WAVES_PER_SET = 5;
    const int SETS = 2;
    const int TOTAL_WAVES = WAVES_PER_SET * SETS; // 10
    const int PAUSE = 90; // 2セット撃った後の休み
    const int PERIOD = TOTAL_WAVES * WAVE_INTERVAL + PAUSE; // 10*8+90=170

    const double FORWARD_SPACING = 12.0;
    const double LATERAL_SPACING = 12.0;
    const double BASE_FORWARD = 10.0;

    int phase = pSet->count % PERIOD;
    if (phase % WAVE_INTERVAL == 0 && phase < TOTAL_WAVES * WAVE_INTERVAL) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 自機方向へ荒く
        double aim = atan2(player.y - pSet->y, player.x - pSet->x);
        // GetRand(x)は0..x inclusiveなので -10..10 を作る
        double jitter = (double)(GetRand(20) - 10) * DX_PI / 180.0*5;
        aim += jitter;

        double cosA = cos(aim);
        double sinA = sin(aim);
        // perp = (-sin, cos)
        double speed = 2.8 + GetRand(30) / 100.0;

        // 1辺8の逆三角形(塗りつぶし) = 36発
        // i=0: tip 1発、i=7: base 8発
        for (int i = 0; i < 8; i++) {
            int n = i + 1;
            double forward = BASE_FORWARD + (7 - i) * FORWARD_SPACING;
            for (int j = 0; j < n; j++) {
                double lateral = (j - (n - 1) * 0.5) * LATERAL_SPACING;

                sEnemyShot* p = new sEnemyShot;
                // 回転変換: forward*dir + lateral*perp
                p->x = pSet->x + forward * cosA + lateral * (-sinA);
                p->y = pSet->y + forward * sinA + lateral * cosA;
                p->muki = aim;
                p->speed = speed;
                // 密なので小玉、青
                p->kind = img_enemyShotSmallBall[4];

                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // 弾移動 (全弾同方向なので面を保ったまま進む)
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//============================================================
// 敵本体
//============================================================
void EnemyPat_LunaticGun_MetaAI()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // --- ShotSet A 生成 ---
        sEnemyShotSet* pSetA = new sEnemyShotSet;
        pSetA->count = 0;
        pSetA->patternFunc = ShotA_LunaticGun;
        pSetA->x = enemy.x;
        pSetA->y = enemy.y;
        pSetA->muki = 0.0;
        pSetA->kind = 0;
        for (int i = 0; i < 16; i++) { pSetA->param_i[i] = 0; pSetA->param_d[i] = 0.0; }
        pSetA->pEnemyShotHead = new sEnemyShot;
        pSetA->pEnemyShotHead->prev = pSetA->pEnemyShotHead;
        pSetA->pEnemyShotHead->next = pSetA->pEnemyShotHead;
        pSetA->prev = enemyShotSetHead.prev;
        pSetA->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetA;
        enemyShotSetHead.prev = pSetA;

        // --- ShotSet B 生成 ---
        sEnemyShotSet* pSetB = new sEnemyShotSet;
        pSetB->count = 0;
        pSetB->patternFunc = ShotB_LunaticGun;
        pSetB->x = enemy.x;
        pSetB->y = enemy.y;
        pSetB->muki = 0.0;
        pSetB->kind = 0;
        for (int i = 0; i < 16; i++) { pSetB->param_i[i] = 0; pSetB->param_d[i] = 0.0; }
        pSetB->pEnemyShotHead = new sEnemyShot;
        pSetB->pEnemyShotHead->prev = pSetB->pEnemyShotHead;
        pSetB->pEnemyShotHead->next = pSetB->pEnemyShotHead;
        pSetB->prev = enemyShotSetHead.prev;
        pSetB->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetB;
        enemyShotSetHead.prev = pSetB;
    }
    else {
        // ゆっくり左右に移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 100.0 || enemy.x > 380.0) muki *= -1;
    }
}