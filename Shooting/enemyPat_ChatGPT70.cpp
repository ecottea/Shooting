// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// クラドニ図形弾
// ============================================================
static void ShotChladni(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;

    // --------------------------------------------------------
    // セット生成時に弾を大量に配置
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int mode = pEnemyShotSet->kind % 4;
        int num = 64 * 10;

        for (int i = 0; i < num; i++) {
            pShot = new sEnemyShot;

            double a = DX_PI * 2.0 * i / num;
            double r = 25.0 + (double)((i * 37) % 190);

            // 最初は中心付近の円形に配置
            pShot->x = 240.0 + cos(a) * 15.0;
            pShot->y = 240.0 + sin(a) * 15.0;

            pShot->muki = a;
            pShot->speed = 0.0;

            // クラドニ図形のモード
            pShot->param_i[0] = mode;

            // 各弾の位相
            pShot->param_d[0] = a;
            pShot->param_d[1] = r;

            // 収束開始までの時間差
            pShot->param_d[2] = (double)((i * 7) % 30);

            // 小玉を基本とし、一部を中玉にする
            if (i % 8 == 0)
                pShot->kind = img_enemyShotMediumBall[(mode + i) % 8];
            else
                pShot->kind = img_enemyShotSmallBall[(mode + i) % 8];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // --------------------------------------------------------
    // クラドニ図形を形成
    // --------------------------------------------------------
    pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        double t = (double)pShot->count - pShot->param_d[2];

        if (t < 0.0)
            t = 0.0;

        // ----------------------------------------------------
        // モードごとの振動数
        // ----------------------------------------------------
        int mode = pShot->param_i[0];

        double n, m;

        switch (mode) {
        case 0:
            n = 2.0;
            m = 3.0;
            break;

        case 1:
            n = 3.0;
            m = 5.0;
            break;

        case 2:
            n = 4.0;
            m = 7.0;
            break;

        default:
            n = 5.0;
            m = 8.0;
            break;
        }

        // ----------------------------------------------------
        // 正規化座標
        // -1 ～ +1 の範囲を振動板として扱う
        // ----------------------------------------------------
        double x = (pShot->x - 240.0) / 220.0;
        double y = (pShot->y - 240.0) / 220.0;

        // ----------------------------------------------------
        // クラドニ関数
        // F = 0 が節線
        // ----------------------------------------------------
        double px = DX_PI * x;
        double py = DX_PI * y;

        double cn = cos(n * px);
        double cm = cos(m * px);
        double dn = cos(n * py);
        double dm = cos(m * py);

        double F = cn * dm - cm * dn;

        // 勾配
        double Fx =
            -n * DX_PI * sin(n * px) * dm
            + m * DX_PI * sin(m * px) * dn;

        double Fy =
            -m * DX_PI * cn * sin(m * py)
            + n * DX_PI * cm * sin(n * py);

        double g = Fx * Fx + Fy * Fy + 0.0001;

        // ----------------------------------------------------
        // 節線へ吸着
        // ----------------------------------------------------
        double strength = 0.11;

        if (t > 15.0) {
            x -= F * Fx / g * strength;
            y -= F * Fy / g * strength;
        }

        // ----------------------------------------------------
        // 外側へ広がる力
        // ----------------------------------------------------
        double a = pShot->param_d[0];

        double targetR = pShot->param_d[1];

        double progress = t / 100.0;

        if (progress > 1.0)
            progress = 1.0;

        double desiredR = 15.0 + targetR * progress;

        double rr = sqrt(x * x + y * y);

        if (rr > 0.001) {
            double push = (desiredR / 220.0 - rr) * 0.045;

            x += x / rr * push;
            y += y / rr * push;
        }

        // ----------------------------------------------------
        // クラドニ図形が呼吸するような微振動
        // ----------------------------------------------------
        double wave =
            sin((t + pShot->param_d[0] * 30.0) * 0.035) * 0.025;

        x += cos(a) * wave;
        y += sin(a) * wave;

        // ----------------------------------------------------
        // 画面座標へ戻す
        // ----------------------------------------------------
        pShot->x = 240.0 + x * 220.0;
        pShot->y = 240.0 + y * 220.0;

        // 節線に沿った向き
        pShot->muki = atan2(Fx, -Fy);

        if (pShot->count >= 570) pShot->margin = -9999;

        pShot = pShot->next;
    }
}


// ============================================================
// 敵本体
// ============================================================
void EnemyPat_Chladni_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;

        enemy.maxHp = enemy.hp = 200;

        muki = 1;
        shot_count = 0;
    }
    else {
        // 上部をゆっくり左右移動
        enemy.x += 0.75 * (double)muki;

        if (enemy.x < 100.0)
            muki = 1;

        if (enemy.x > 380.0)
            muki = -1;
    }

    // --------------------------------------------------------
    // 90フレームごとに新しいクラドニ図形を発生
    // --------------------------------------------------------
    if (count % 600 == 1) {

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotChladni;

        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        pEnemyShotSet->muki =
            atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x
            );

        // 4種類のクラドニ図形を順番に使用
        pEnemyShotSet->kind = shot_count++ % 4;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;

        pEnemyShotSet->pEnemyShotHead->prev =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->pEnemyShotHead->next =
            pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;

        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}