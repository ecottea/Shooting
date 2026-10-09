#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ============================================================
// 相撲弾幕「大一番・電磁大銀杏」
// 画面いっぱいの土俵、立ち合いの突進、近距離での斥力、
// 土俵沿いの連鎖弾、外周から内側へ押し寄せる決まり手を組み合わせる。
// ============================================================

namespace {
    const double SCREEN_CENTER_X = 240.0;
    const double SCREEN_CENTER_Y = 240.0;

    enum ShotSetKind {
        SHOTSET_FAN_DUST = 1,
        SHOTSET_FAN_RUSH = 2
    };

    // --------------------------------------------------------
    // 効果音
    // --------------------------------------------------------
    void PlayPatternSound(int soundHandle)
    {
        if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
        PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    // 敵弾セットと弾の登録
    // count類のインクリメントや画面外弾の削除はメインルーチンに任せる。
    // --------------------------------------------------------
    sEnemyShotSet* CreateShotSet(
        sEnemyShotSet::PatternFunc patternFunc,
        int kind,
        double x,
        double y,
        double muki)
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = patternFunc;
        pSet->x = x;
        pSet->y = y;
        pSet->muki = muki;
        pSet->kind = kind;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
        return pSet;
    }

    sEnemyShot* AddShot(
        sEnemyShotSet* pSet,
        double x,
        double y,
        double muki,
        double speed,
        int kind)
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
        return pShot;
    }

    // --------------------------------------------------------
    // 土俵：画面全体を囲む二重の円を敵弾で表現する。
    // 弾は円周上をゆっくり回り続け、通常攻撃とは別に常時残る。
    // --------------------------------------------------------
    void ShotDohyo(sEnemyShotSet* pSet)
    {
        const int BULLET_NUM = 96;

        if (pSet->count == 0) {
            for (int layer = 0; layer < 2; ++layer) {
                for (int i = 0; i < BULLET_NUM; ++i) {
                    double theta = 2.0 * DX_PI * i / BULLET_NUM;
                    int image = (layer == 0)
                        ? img_enemyShotMediumBall[8]  // 橙：外側の土俵
                        : img_enemyShotMediumBall[1]; // 黄：内側の俵
                    sEnemyShot* pShot = AddShot(pSet,
                        SCREEN_CENTER_X, SCREEN_CENTER_Y,
                        theta, 0.0, image);
                    pShot->param_i[0] = layer;
                    pShot->param_i[1] = i;
                    pShot->param_d[0] = theta;
                }
            }
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            int layer = pShot->param_i[0];
            double radius = (layer == 0) ? 225.0 : 204.0;
            double rotation = pSet->count * 0.0018 * ((layer == 0) ? 1.0 : -1.0);
            double theta = pShot->param_d[0] + rotation;
            pShot->x = SCREEN_CENTER_X + radius * cos(theta);
            pShot->y = SCREEN_CENTER_Y + radius * sin(theta);
            pShot->muki = theta + DX_PI / 2.0;
            pShot = pShot->next;
        }
    }

    // --------------------------------------------------------
    // 扇状弾：四股の砂煙、または突進に合わせた銃弾の連射。
    // mukiは発射方向と回転弾の向きの両方に使う。
    // --------------------------------------------------------
    void ShotFan(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            bool isDust = (pSet->kind == SHOTSET_FAN_DUST);
            int num = isDust ? 17 : 15;
            double spread = isDust ? 0.062 : 0.052;

            for (int i = 0; i < num; ++i) {
                double angle = pSet->muki + (i - (num - 1) / 2.0) * spread;
                double speed;
                int image;
                if (isDust) {
                    speed = 2.1 + GetRand(10) / 10.0;
                    image = img_enemyShotSmallBall[8];
                }
                else {
                    speed = 3.8 + GetRand(8) / 10.0;
                    image = img_enemyShotBullet[0];
                }
                AddShot(pSet, pSet->x, pSet->y, angle, speed, image);
            }
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }

    // --------------------------------------------------------
    // 土俵沿いの連鎖弾：まず外周を滑るように回り、少し遅れて自機へ向かう。
    // --------------------------------------------------------
    void ShotBoundaryChain(sEnemyShotSet* pSet)
    {
        const int BULLET_NUM = 20;
        const double RADIUS = 212.0;
        const int TRACE_FRAMES = 34;

        if (pSet->count == 0) {
            for (int i = 0; i < BULLET_NUM; ++i) {
                double theta = pSet->muki + 2.0 * DX_PI * i / BULLET_NUM;
                sEnemyShot* pShot = AddShot(pSet,
                    pSet->x + RADIUS * cos(theta),
                    pSet->y + RADIUS * sin(theta),
                    theta + DX_PI / 2.0, 0.0,
                    img_enemyShotDiamond[8]);
                pShot->param_i[0] = 0; // 0: 外周を移動中、1: 内側へ発射済み
                pShot->param_i[1] = (i % 2 == 0) ? 1 : -1;
                pShot->param_d[0] = theta;
            }
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                if (pShot->count < TRACE_FRAMES) {
                    double theta = pShot->param_d[0]
                        + pShot->param_i[1] * pShot->count * 0.023;
                    pShot->x = pSet->x + RADIUS * cos(theta);
                    pShot->y = pSet->y + RADIUS * sin(theta);
                    pShot->muki = theta
                        + pShot->param_i[1] * DX_PI / 2.0;
                }
                else {
                    pShot->param_i[0] = 1;
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->speed = 2.8;
                    pShot->kind = img_enemyShotScale[8];
                    pShot->x += pShot->speed * cos(pShot->muki);
                    pShot->y += pShot->speed * sin(pShot->muki);
                }
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }

    // --------------------------------------------------------
    // 決まり手「押し出し」：外周の弾が中心近くまで押し寄せて反転し、
    // その後、中心からも放射状の弾が広がる。
    // --------------------------------------------------------
    void ShotPushOut(sEnemyShotSet* pSet)
    {
        const int OUTER_NUM = 36/2;
        const int CENTER_NUM = 24/2;
        const double RADIUS = 210.0;

        if (pSet->count == 0) {
            for (int i = 0; i < OUTER_NUM; ++i) {
                double theta = pSet->muki + 2.0 * DX_PI * i / OUTER_NUM;
                sEnemyShot* pShot = AddShot(pSet,
                    pSet->x + RADIUS * cos(theta),
                    pSet->y + RADIUS * sin(theta),
                    theta + DX_PI, 3.8,
                    img_enemyShotMediumBall[8]);
                pShot->param_i[0] = 0; // 外周から内側へ進む弾
                pShot->param_d[0] = theta;
            }
            for (int i = 0; i < CENTER_NUM; ++i) {
                double theta = pSet->muki + 2.0 * DX_PI * i / CENTER_NUM;
                sEnemyShot* pShot = AddShot(pSet,
                    pSet->x, pSet->y, theta, 0.0,
                    img_enemyShotSmallBall[1]);
                pShot->param_i[0] = 1; // 時間差で中心から発射する弾
                pShot->param_d[0] = theta;
            }
        }

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                if (pShot->count < 52) {
                    pShot->x += pShot->speed * cos(pShot->muki);
                    pShot->y += pShot->speed * sin(pShot->muki);
                }
                else {
                    if (pShot->param_i[2] == 0) {
                        pShot->param_i[2] = 1;
                        pShot->muki = pShot->param_d[0];
                        pShot->speed = 4.3;
                        pShot->kind = img_enemyShotMediumBall[1];
                    }
                    pShot->x += pShot->speed * cos(pShot->muki);
                    pShot->y += pShot->speed * sin(pShot->muki);
                }
            }
            else {
                if (pShot->count >= 24 && pShot->param_i[2] == 0) {
                    pShot->param_i[2] = 1;
                    pShot->speed = 3.3;
                    pShot->kind = img_enemyShotMediumBall[1];
                }
                if (pShot->param_i[2] != 0) {
                    pShot->x += pShot->speed * cos(pShot->muki);
                    pShot->y += pShot->speed * sin(pShot->muki);
                }
            }
            pShot = pShot->next;
        }
    }
}

// ============================================================
// 敵本体の弾幕
// ============================================================
void EnemyPat_Sumou_ChatGPT()
{
    // 0:立ち合い前の構え、1:突進、2:間合いを取り直す
    static int state = 0;
    static int stateTimer = 0;
    static double bossVx = 0.0;
    static double bossVy = 0.0;
    static bool dohyoCreated = false;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 58.0;
        enemy.maxHp = enemy.hp = 200;

        state = 0;
        stateTimer = 0;
        bossVx = 0.0;
        bossVy = 0.0;
        dohyoCreated = false;
    }

    // 土俵は戦闘中ずっと残す。
    if (!dohyoCreated) {
        CreateShotSet(ShotDohyo, 0, SCREEN_CENTER_X, SCREEN_CENTER_Y, 0.0);
        dohyoCreated = true;
    }

    ++stateTimer;

    if (state == 0) {
        // 構え：自機の横位置をゆっくり追い、突進の予備動作を見せる。
        double aimX = player.x + (player.x - enemy.x) * 0.10;
        if (aimX < 55.0) aimX = 55.0;
        if (aimX > 425.0) aimX = 425.0;
        enemy.x += (aimX - enemy.x) * 0.045;
        enemy.y += (58.0 - enemy.y) * 0.05;

        if (stateTimer == 1) {
            PlayPatternSound(sound_enemyCharge);
        }
        if (stateTimer == 22) {
            // 四股の砂煙をイメージした扇状の小弾。
            double angleToPlayer = atan2(player.y - enemy.y, player.x - enemy.x);
            CreateShotSet(ShotFan, SHOTSET_FAN_DUST,
                enemy.x, enemy.y + 12.0, angleToPlayer);
            PlayPatternSound(sound_enemyShot_light);
        }
        if (stateTimer >= 50) {
            // 立ち合い：この瞬間の自機位置へ向けて進行方向を固定する。
            double angleToPlayer = atan2(player.y - enemy.y, player.x - enemy.x);
            bossVx = 8.5 * cos(angleToPlayer);
            bossVy = 8.5 * sin(angleToPlayer);
            state = 1;
            stateTimer = 0;

            PlayPatternSound(sound_enemyShot_extreme);
            CreateShotSet(ShotFan, SHOTSET_FAN_RUSH,
                enemy.x, enemy.y + 8.0, angleToPlayer);
        }
    }
    else if (state == 1) {
        // 立ち合いの突進。突進中も小刻みに銃弾を放ち、軌道の先を狭める。
        enemy.x += bossVx;
        enemy.y += bossVy;

        if (stateTimer % 9 == 0) {
            double angleToPlayer = atan2(player.y - enemy.y, player.x - enemy.x);
            CreateShotSet(ShotFan, SHOTSET_FAN_RUSH,
                enemy.x, enemy.y + 6.0, angleToPlayer);
            PlayPatternSound(sound_enemyShot_light);
        }

        if (stateTimer >= 36 || enemy.x < 14.0 || enemy.x > 466.0
            || enemy.y < 14.0 || enemy.y > 466.0) {
            state = 2;
            stateTimer = 0;
            PlayPatternSound(sound_enemyShot_medium);
        }
    }
    else {
        // 突進後は画面上部へ戻りながら、土俵に沿う弾と押し出しを放つ。
        enemy.x += (240.0 - enemy.x) * 0.035;
        enemy.y += (62.0 - enemy.y) * 0.035;

        if (stateTimer == 18) {
            double phase = GetRand(359) / 180.0 * DX_PI*0;
            CreateShotSet(ShotBoundaryChain, 0,
                SCREEN_CENTER_X, SCREEN_CENTER_Y, phase);
        }
        if (stateTimer == 48) {
            double phase = GetRand(359) / 180.0 * DX_PI*0;
            CreateShotSet(ShotPushOut, 0,
                SCREEN_CENTER_X, SCREEN_CENTER_Y, phase);
            PlayPatternSound(sound_enemyShot_heavy);
        }
        if (stateTimer >= 88) {
            state = 0;
            stateTimer = 0;
        }
    }

    // --------------------------------------------------------
    // 近距離の斥力：ボスから自機へ向かう方向を力の向きにする。
    // 自機とボスを反対方向へ押し返し、指定の力場エフェクトも発生させる。
    // --------------------------------------------------------
    {
        double dx = player.x - enemy.x;
        double dy = player.y - enemy.y;
        double distance = sqrt(dx * dx + dy * dy);
        const double FORCE_DISTANCE = 82.0+100;

        if (distance < FORCE_DISTANCE) {
            // 接近するほど斥力を強める。ゼロ距離付近でも方向が不定にならないようにする。
            double pullSpeed = 4.0 * (FORCE_DISTANCE - distance) / FORCE_DISTANCE;
            double angle = (distance > 0.001) ? atan2(dy, dx) : -DX_PI / 2.0;
            double ratio = 3.0;

            player.x += pullSpeed * cos(angle);
            player.y += pullSpeed * sin(angle);
            enemy.x -= pullSpeed * 0.70 * cos(angle);
            enemy.y -= pullSpeed * 0.70 * sin(angle);

            // 突進中は進行方向にも反発を反映し、互いに押し合う動きを作る。
            if (state == 1) {
                bossVx -= pullSpeed * 0.30 * cos(angle);
                bossVy -= pullSpeed * 0.30 * sin(angle);
            }

            // パーティクルの過剰発生を抑えつつ、斥力が働いていることを可視化する。
            if (count % 1 == 0) {
                spawnForceParticles(player.x, player.y, pullSpeed * cos(angle) * ratio, pullSpeed * sin(angle) * ratio);
            }

            // 自機が画面外へ押し出されないようにする。
            if (player.x < 8.0) player.x = 8.0;
            if (player.x > 472.0) player.x = 472.0;
            if (player.y < 8.0) player.y = 8.0;
            if (player.y > 472.0) player.y = 472.0;
        }
    }
}
