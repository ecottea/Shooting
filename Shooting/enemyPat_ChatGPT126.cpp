#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// メントスコーラ弾幕
// 黒い楕円弾でボトル、白い小玉で泡、橙の銃弾でコーラの飛沫を表現する。
// ------------------------------------------------------------

static void AddShot(sEnemyShotSet* pEnemyShotSet,
    double x, double y, double muki, double speed, int kind,
    int type = 0, double p0 = 0.0, double p1 = 0.0, double p2 = 0.0, double p3 = 0.0)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[0] = type;
    pEnemyShot->param_d[0] = p0;
    pEnemyShot->param_d[1] = p1;
    pEnemyShot->param_d[2] = p2;
    pEnemyShot->param_d[3] = p3;
    pEnemyShot->margin = 240;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// 敵弾セット：ボトル本体＋メントス投入直後の泡＋大噴水
static void ShotMentosCola(sEnemyShotSet* pEnemyShotSet)
{
    // 0:ボトル、1:泡、2:噴水、3:飛沫、4:キャップ
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // ボトル本体。楕円弾は縦向きにして瓶の胴体を表現する。
        const double bodyX[] = { -30, -15,   0,  15,  30,
                                 -30, -15,   0,  15,  30,
                                 -24, -12,   0,  12,  24,
                                 -18,  -6,   6,  18 };
        const double bodyY[] = { 0,   0,   0,   0,   0,
                                   14,  14,  14,  14,  14,
                                   28,  28,  28,  28,  28,
                                   42,  42,  42,  42 };

        for (int i = 0; i < 19; i++) {
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x + bodyX[i],
                pEnemyShotSet->y + 40.0 + bodyY[i],
                -DX_PI * 0.5,
                0.0,
                img_enemyShotMediumOval[2],
                0,
                bodyX[i], bodyY[i]);
        }

        // 首と口。上へ噴き出す起点を細く見せる。
        for (int i = -1; i <= 1; i++) {
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x + i * 8.0,
                pEnemyShotSet->y + 18.0,
                -DX_PI * 0.5,
                0.0,
                img_enemyShotMediumOval[2],
                0,
                i * 8.0, -22.0);
        }

        // 王冠のようなキャップ。黄色の菱形弾で「メントス投入」の視認性を出す。
        for (int i = -2; i <= 2; i++) {
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x + i * 7.0,
                pEnemyShotSet->y - 1.0,
                -DX_PI * 0.5,
                0.0,
                img_enemyShotDiamond[1],
                4,
                i * 7.0, -42.0);
        }
    }

    // セット全体を少し下げて、ボトルが落ちてきてから噴水になる印象を作る。
    const double bottleFall = (pEnemyShotSet->count > 72)
        ? (pEnemyShotSet->count - 72) * 0.42
        : 0.0;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int type = pShot->param_i[0];

        if (type == 0) {
            // ボトル本体・首
            const double localX = pShot->param_d[0];
            const double localY = pShot->param_d[1];

            pShot->x = pEnemyShotSet->x + localX;
            pShot->y = pEnemyShotSet->y + 40.0 + localY + bottleFall;
            pShot->muki = -DX_PI * 0.5;
        }
        else if (type == 1) {
            // 泡：加速して上昇し、左右へゆるく蛇行する。
            const double life = (double)pShot->count;
            const double ang = pShot->param_d[0];
            const double phase = pShot->param_d[1];
            const double originX = pShot->param_d[2];
            const double originY = pShot->param_d[3];
            const double rise = 0.5 * 0.045 * life * life;

            pShot->x = pEnemyShotSet->x + originX
                + sin(life * 0.16 + phase) * (5.0 + life * 0.02);
            pShot->y = pEnemyShotSet->y + originY - rise;
            pShot->muki = ang;
        }
        else if (type == 2) {
            // 噴水：最初は低速、徐々に加速してからほぼ一定速になる。
            const double life = (double)pShot->count;
            const double ang = pShot->param_d[0];
            const double terminal = pShot->param_d[1];
            const double accelTime = 24.0;
            const double t = life < accelTime ? life : accelTime;
            const double speed = terminal * (t / accelTime);
            const double distance = terminal * (life < accelTime
                ? 0.5 * accelTime * (life / accelTime) * (life / accelTime)
                : (life - accelTime) + 0.5 * accelTime);

            pShot->x = pEnemyShotSet->x + cos(ang) * distance;
            pShot->y = pEnemyShotSet->y + sin(ang) * distance;
            pShot->muki = ang;
            pShot->speed = speed;
        }
        else if (type == 3) {
            // 飛沫：噴水の頂点付近から大きく横へ散る。
            const double life = (double)pShot->count;
            const double ang = pShot->param_d[0];
            const double v = pShot->param_d[1];
            const double gravity = 0.075/2;

            pShot->x = pEnemyShotSet->x + cos(ang) * v * life;
            pShot->y = pEnemyShotSet->y + sin(ang) * v * life + gravity * life * life;
            pShot->muki = atan2(sin(ang) * v + gravity * life, cos(ang) * v);
        }
        else if (type == 4) {
            // キャップ：投入されたメントスが跳ね上がってから落ちる。
            const double jump = pShot->count < 18
                ? -pShot->count * 1.5
                : -27.0;
            pShot->x = pEnemyShotSet->x + pShot->param_d[0];
            pShot->y = pEnemyShotSet->y + 40.0 + pShot->param_d[1]
                + jump + bottleFall;
            pShot->muki = -DX_PI * 0.5;
        }

        pShot = pShot->next;
    }

    // 泡の小玉。噴水の中心から絶え間なく湧き出させる。
    if (pEnemyShotSet->count >= 12 && pEnemyShotSet->count < 180
        && pEnemyShotSet->count % 4 == 0) {
        const int phase = pEnemyShotSet->count / 4;
        const double xOffset = (double)((phase % 7) - 3) * 7.0;
        const double side = (phase % 2 == 0) ? 1.0 : -1.0;

        AddShot(pEnemyShotSet,
            pEnemyShotSet->x + xOffset,
            pEnemyShotSet->y + 6.0,
            -DX_PI * 0.5,
            0.0,
            img_enemyShotSmallBall[6],
            1,
            -DX_PI * 0.5,
            phase * 0.7,
            xOffset,
            6.0);

        if (phase % 3 != 0) {
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x + xOffset * 0.45,
                pEnemyShotSet->y + 10.0,
                -DX_PI * 0.5,
                0.0,
                img_enemyShotSmallBall[6],
                1,
                -DX_PI * 0.5,
                phase * 0.9 + DX_PI,
                xOffset * 0.45,
                10.0);
        }
        (void)side;
    }

    // 本命の大噴水。上向きの中心流と左右へ広がる流れを同時に出す。
    if (pEnemyShotSet->count >= 18 && pEnemyShotSet->count < 210
        && pEnemyShotSet->count % 6 == 0) {
        const int wave = pEnemyShotSet->count / 6;
        const double center = -DX_PI * 0.5;

        for (int i = -3; i <= 3; i++) {
            double angle = center + i * 0.085;
            double terminal = 2.1 + fabs((double)i) * 0.16;

            AddShot(pEnemyShotSet,
                pEnemyShotSet->x,
                pEnemyShotSet->y + 4.0,
                angle,
                0.0,
                img_enemyShotBullet[8],
                2,
                angle,
                terminal,
                wave,
                0.0);
        }

        // 外周は少しだけ太い楕円弾にして、噴水の輪郭を見せる。
        for (int i = 0; i < 2; i++) {
            const double side = (i == 0) ? -1.0 : 1.0;
            const double angle = center + side * (0.34 + 0.03 * (wave % 4));

            AddShot(pEnemyShotSet,
                pEnemyShotSet->x,
                pEnemyShotSet->y + 4.0,
                angle,
                0.0,
                img_enemyShotMediumOval[0],
                2,
                angle,
                2.45 + 0.12 * (wave % 3),
                wave + i,
                0.0);
        }
    }

    // 噴水の頂点から飛び散る飛沫。毎回左右の形を入れ替えて、単調にならないようにする。
    if (pEnemyShotSet->count >= 66 && pEnemyShotSet->count < 210
        && pEnemyShotSet->count % 18 == 0) {
        const int burst = pEnemyShotSet->count / 18;
        const double dir = (burst % 2 == 0) ? 1.0 : -1.0;
        const double sourceY = pEnemyShotSet->y - 110.0 - (burst % 3) * 10.0;

        for (int i = 0; i < 9; i++) {
            const double spread = -0.85 + i * 0.21;
            const double angle = -DX_PI * 0.5 + dir * spread;
            const double v = 1.7 + (i % 3) * 0.25;

            AddShot(pEnemyShotSet,
                pEnemyShotSet->x + dir * 4.0,
                sourceY,
                angle,
                0.0,
                (i % 2 == 0) ? img_enemyShotDiamond[8] : img_enemyShotBullet[8],
                3,
                angle,
                v,
                burst,
                sourceY);
        }
    }

    // 終盤はボトルの口から横向きの強い噴出を追加して、画面上部全体へ広げる。
    if (pEnemyShotSet->count >= 132 && pEnemyShotSet->count < 228
        && pEnemyShotSet->count % 12 == 0) {
        const int sidePhase = pEnemyShotSet->count / 12;

        for (int i = 0; i < 5; i++) {
            const double side = (sidePhase + i) % 2 == 0 ? 1.0 : -1.0;
            const double angle = side * (0.18 + i * 0.10) - DX_PI * 0.5;

            AddShot(pEnemyShotSet,
                pEnemyShotSet->x,
                pEnemyShotSet->y + 2.0,
                angle,
                0.0,
                img_enemyShotMediumOval[8],
                2,
                angle,
                2.25 + i * 0.12,
                sidePhase + i,
                0.0);
        }
    }
}

// 敵本体のパターン
void EnemyPat_MentosCola_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 52.0+50;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.72 * (double)muki;
        if (count % 140 == 70) muki *= -1;
    }

    // 噴水が画面中央に見えるよう、ボスの左右移動に合わせて発射位置も動かす。
    if (count % 150 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMentosCola;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 92.0;
        pEnemyShotSet->muki = -DX_PI * 0.5;
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
