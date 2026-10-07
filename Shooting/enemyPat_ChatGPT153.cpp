#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static double NormalizeAngle(double angle)
{
    while (angle > DX_PI) angle -= DX_PI * 2.0;
    while (angle < -DX_PI) angle += DX_PI * 2.0;
    return angle;
}

static double ApproachAngle(double current, double target, double maxStep)
{
    double diff = NormalizeAngle(target - current);
    if (diff > maxStep) diff = maxStep;
    if (diff < -maxStep) diff = -maxStep;
    return current + diff;
}

static void AddPhaseShot(
    sEnemyShotSet* pEnemyShotSet,
    double x,
    double y,
    double muki,
    double speed,
    int kind,
    int slot)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;

    // param_i[0]: 管理用スロット番号
    // param_i[1]: 状態（0:気体、1:液体、2:個体、3:崩壊）
    // param_d[0]: 気体状態での基準角度
    // param_d[1]: 個体状態での目標X
    // param_d[2]: 個体状態での目標Y
    pEnemyShot->param_i[0] = slot;
    pEnemyShot->param_i[1] = 0;
    pEnemyShot->param_d[0] = muki;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static void PhaseTransition(sEnemyShotSet* pEnemyShotSet)
{
    constexpr int CYCLE = 600;
    constexpr int GAS_END = 180;
    constexpr int LIQUID_END = 330;
    constexpr int SOLID_END = 480;

    const int t = pEnemyShotSet->count % CYCLE;

    // 初回生成：自由に飛び回る「気体」
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 0;

        for (int i = 0; i < 36*2; i++) {
            const double angle =
                i * (DX_PI * 2.0 / 36.0/2)
                + (GetRand(1000) / 1000.0 - 0.5) * 0.22;
            const double speed = 1.25 + GetRand(100) / 100.0;

            AddPhaseShot(
                pEnemyShotSet,
                pEnemyShotSet->x,
                pEnemyShotSet->y,
                angle,
                speed,
                img_enemyShotSmallBall[6],
                pEnemyShotSet->param_i[0]++
            );
        }
    }

    // =========================================================
    // 気体：分子が自由に飛び回る
    // =========================================================
    if (t < GAS_END) {
        if (t % 4 == 0 && t != 0) {
            for (int i = 0; i < 2; i++) {
                const double angle =
                    DX_PI * 0.5
                    + (GetRand(1000) / 1000.0 - 0.5) * DX_PI * 1.6;

                AddPhaseShot(
                    pEnemyShotSet,
                    pEnemyShotSet->x + (GetRand(40) - 20),
                    pEnemyShotSet->y + (GetRand(20) - 10),
                    angle,
                    1.15 + GetRand(115) / 100.0,
                    img_enemyShotSmallBall[6],
                    pEnemyShotSet->param_i[0]++
                );
            }
        }

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            const double thermal =
                sin(pShot->count * 0.045 + pShot->param_i[0] * 0.37) * 0.025;

            pShot->muki += thermal;
            pShot->speed += 0.002;
            if (pShot->speed > 2.9) pShot->speed = 2.9;

            pShot->kind = img_enemyShotSmallBall[6];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
        return;
    }

    // =========================================================
    // 気体→液体：ばらけた分子が流れへ吸い込まれていく
    // =========================================================
    if (t < LIQUID_END) {
        if (t == GAS_END) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        const double centerX = 240.0;
        const double centerY = 265.0;

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            const double dx = pShot->x - centerX;
            const double dy = pShot->y - centerY;
            const double radius = sqrt(dx * dx + dy * dy) + 0.001;

            // 中心の周囲を回る接線方向へ徐々に向きを変える。
            const double radialAngle = atan2(dy, dx);
            const double tangent = radialAngle + DX_PI * 0.5;

            pShot->muki = ApproachAngle(pShot->muki, tangent, 0.055);

            if (pShot->speed < 3.2) pShot->speed += 0.018;
            if (pShot->speed > 3.2) pShot->speed = 3.2;

            // 半径が小さくなりすぎた弾は、わずかに外向き成分を加えて
            // 中心へ潰れず、流れのある「液体の帯」を維持する。
            if (radius < 65.0) {
                const double outward = radialAngle;
                pShot->muki = ApproachAngle(pShot->muki, outward + DX_PI * 0.5, 0.04);
            }

            pShot->kind = img_enemyShotMediumOval[3];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot = pShot->next;
        }
        return;
    }

    // =========================================================
    // 液体→個体：流れが止まり、規則正しい結晶格子に集まる
    // =========================================================
    if (t < SOLID_END) {
        if (t == LIQUID_END) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            const int slot = pShot->param_i[0];

            // 10列×8行の格子。偶数行と奇数行を半マスずらして
            // 結晶らしい詰まった配置にする。
            const int col = slot % 10;
            const int row = (slot / 10) % 8;

            pShot->param_d[1] = 70.0 + col * 40.0 + ((row & 1) ? 20.0 : 0.0);
            pShot->param_d[2] = 105.0 + row * 40.0;

            const double dx = pShot->param_d[1] - pShot->x;
            const double dy = pShot->param_d[2] - pShot->y;
            const double dist = sqrt(dx * dx + dy * dy);

            if (dist > 2.0) {
                const double targetAngle = atan2(dy, dx);
                pShot->muki = ApproachAngle(pShot->muki, targetAngle, 0.085);

                const double targetSpeed = dist > 80.0 ? 3.0 : (dist > 30.0 ? 2.0 : 1.0);
                if (pShot->speed < targetSpeed) pShot->speed += 0.08;
                if (pShot->speed > targetSpeed) pShot->speed -= 0.10;
                if (pShot->speed < 0.35) pShot->speed = 0.35;

                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                // ほぼ静止して格子を形成。
                pShot->speed *= 0.82;
                if (pShot->speed < 0.05) pShot->speed = 0.0;
                pShot->muki = 0.0;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }

            pShot->kind = img_enemyShotDiamond[8];

            // 固体化の完了間際だけ、結晶が少し脈動する。
            if (t >= SOLID_END - 28) {
                pShot->x += sin(pShot->count * 0.12 + slot) * 0.08;
                pShot->y += cos(pShot->count * 0.11 + slot) * 0.08;
            }

            pShot = pShot->next;
        }
        return;
    }

    // =========================================================
    // 個体→気体：結晶が熱で崩れ、一斉に分解して飛び散る
    // =========================================================
    if (t == SOLID_END) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int slot = pShot->param_i[0];

        if (pShot->param_i[1] == 0) {
            pShot->param_i[1] = 3;
        }

        const double dx = pShot->x - 240.0;
        const double dy = pShot->y - 250.0;
        const double dist = sqrt(dx * dx + dy * dy) + 0.001;

        // 格子から離れる方向へ徐々に加速し、気体へ戻る。
        const double outwardAngle =
            atan2(dy, dx) + sin(slot * 1.7) * 0.18;

        pShot->muki = ApproachAngle(pShot->muki, outwardAngle, 0.06);

        if (pShot->speed < 3.8) pShot->speed += 0.045;

        // 中心付近の弾には少し強い押し出しを与える。
        if (dist < 75.0) {
            pShot->speed += 0.02;
        }

        pShot->kind = img_enemyShotSmallBall[6];
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }

    // 周期の終盤で、画面上に弾が少なくなっても気体状態へ復帰できるよう補充。
    if (t % 6 == 0 && t >= 540) {
        const double angle =
            DX_PI * 0.5
            + (GetRand(1000) / 1000.0 - 0.5) * DX_PI * 1.7;

        AddPhaseShot(
            pEnemyShotSet,
            pEnemyShotSet->x,
            pEnemyShotSet->y,
            angle,
            1.2 + GetRand(90) / 100.0,
            img_enemyShotSmallBall[6],
            pEnemyShotSet->param_i[0]++
        );
    }
}

void EnemyPat_PhaseTransition_ChatGPT()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 142.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        enemy.x += 0.82 * (double)muki;
        if (enemy.x <= 90.0 || enemy.x >= 390.0) {
            muki *= -1;
        }
    }

    // 弾幕本体は1つのEnemyShotSetで相転移を循環させる。
    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = PhaseTransition;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = DX_PI * 0.5;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;

        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
