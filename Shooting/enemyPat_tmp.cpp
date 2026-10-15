#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾を生成してリストに追加するヘルパー
// perpOff : 斬撃方向に対して垂直方向へずらす量(刃の縁を作る用)
// param_i[0] = 0 : 刀身(構え〜一閃までセット側で制御する弾)
// param_i[0] = 1 : 自由弾(生成後は等速直線運動)
// ------------------------------------------------------------
static void AddShot(sEnemyShotSet * pSet, double x, double y, double muki,
    double speed, int kind, double perpOff)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_d[0] = x;          // 基準位置X(微振動の基準)
    p->param_d[1] = y;          // 基準位置Y
    p->param_d[2] = perpOff;    // 垂直方向オフセット
    p->param_i[0] = (speed > 0.0) ? 1 : 0;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// ------------------------------------------------------------
// 弾幕：居合・月影一閃
// 構え(刀の形をした弾が静止) → 一閃(超高速度) → 残影 → 月の欠片(拡散)
// ------------------------------------------------------------
static void ShotIai(sEnemyShotSet* pEnemyShotSet)
{
    const int FIRE = 90;    // 一閃のフレーム
    const int RING = 130;   // 月の欠片のフレーム
    const double BLADE_START = 26.0;   // 柄のオフセット
    const double BLADE_SPACING = 12.0; // 刀身の弾の間隔
    const int    BLADE_NUM = 22;       // 刀身の弾数
    const double BLADE_LEN = BLADE_START + (BLADE_NUM - 1) * BLADE_SPACING;

    sEnemyShot* pShot;
    int c = pEnemyShotSet->count;

    double muki = pEnemyShotSet->muki;
    double mx = cos(muki), my = sin(muki);   // 斬撃方向
    double px = -sin(muki), py = cos(muki);  // 垂直方向

    // ---- 構え：刀の形を作る ----
    if (c == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_d[0] = mx;
        pEnemyShotSet->param_d[1] = my;

        // 刀身：小玉(白)を高密度で直線配置
        for (int i = 0; i < BLADE_NUM; i++) {
            double d = BLADE_START + i * BLADE_SPACING;
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x + mx * d, pEnemyShotSet->y + my * d,
                muki, 0.0, img_enemyShotSmallBall[6], 0.0);
        }
        // 刃の縁：中玉(白)を両側に1列ずつ
        for (int i = 0; i < BLADE_NUM; i++) {
            double d = BLADE_START + i * BLADE_SPACING;
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x + mx * d, pEnemyShotSet->y + my * d,
                muki, 0.0, img_enemyShotMediumBall[6], 7.0);
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x + mx * d, pEnemyShotSet->y + my * d,
                muki, 0.0, img_enemyShotMediumBall[6], -7.0);
        }
    }

    // ---- 予告線：刀身の先から白い短レーザーを高速で走らせ、斬撃軌道を予告 ----
    if (c == 50) {
        double d = BLADE_LEN + 20.0;
        AddShot(pEnemyShotSet,
            pEnemyShotSet->x + mx * d, pEnemyShotSet->y + my * d,
            muki, 36.0, img_enemyShotLaser[6], 0.0);
    }

    // ---- 構え中：刀全体を微振動させる(溜めている演出) ----
    if (c >= 1 && c < FIRE) {
        double shake = (c % 6 < 3) ? 0.8 : -0.8;
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                pShot->x = pShot->param_d[0] + px * shake * pShot->param_d[2];
                pShot->y = pShot->param_d[1] + py * shake * pShot->param_d[2];
            }
            pShot = pShot->next;
        }
    }

    // ---- 一閃：刀の形の弾を全て超高速度で発射 ----
    if (c == FIRE) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                pShot->speed = 16.0;
                pShot->param_i[0] = 1; // 以降は自由弾として等速直線運動
            }
            pShot = pShot->next;
        }
    }

    // ---- 残影：斬撃軌道から扇状に時間差で「二の太刀・三の太刀」 ----
    if (c == FIRE + 8 || c == FIRE + 16 || c == FIRE + 24) {
        if (c == FIRE + 8) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        int idx = (c - FIRE) / 8 - 1;               // 0,1,2
        double ang = muki + (idx - 1) * 0.22;       // 中心±0.22radの扇
        double ax = cos(ang), ay = sin(ang);
        for (int j = 0; j < 16; j++) {
            double d = BLADE_START + j * 14.0;
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x + ax * d, pEnemyShotSet->y + ay * d,
                ang, 5.0, img_enemyShotSmallBall[5], 0.0); // マゼンタの小玉
        }
    }

    // ---- 月の欠片：斬撃の終点付近から放射状に拡散 ----
    if (c == RING) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 終点(画面内にクランプ)
        double tipX = pEnemyShotSet->x + mx * (BLADE_LEN + 20.0);
        double tipY = pEnemyShotSet->y + my * (BLADE_LEN + 20.0);
        if (tipX < 30.0) tipX = 30.0;
        if (tipX > 450.0) tipX = 450.0;
        if (tipY < 30.0) tipY = 30.0;
        if (tipY > 450.0) tipY = 450.0;

        const int N = 18;
        for (int i = 0; i < N; i++) {
            // GetRand(x) は 0〜x の x+1 種類の整数を返す
            double a = i * DX_PI * 2.0 / N + GetRand(10) / 100.0;
            double sp = 2.4 + GetRand(120) / 100.0;
            AddShot(pEnemyShotSet, tipX, tipY, a, sp,
                img_enemyShotSmallBall[1], 0.0); // 黄色の小玉
        }
    }

    // ---- 自由弾の移動(刀身は速度0 or 発射済み) ----
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// サイクル：移動(0〜70) → 構え開始(80) → 斬撃〜拡散はセット側で完結
// ------------------------------------------------------------
void EnemyPat_Tmp()
{
    static int cycle;
    static double targetX, targetY;
    const int CYCLE = 280;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        cycle = 0;
        targetX = enemy.x;
        targetY = enemy.y;
    }

    cycle++;

    // 次の斬撃位置を決定(画面上部〜やや下のランダム位置)
    if (cycle == 1) {
        targetX = 50.0 + GetRand(380);
        targetY = 45.0 + GetRand(90);
    }

    // サイクル前半で斬撃位置へ移動
    if (cycle <= 70) {
        enemy.x += (targetX - enemy.x) * 0.08;
        enemy.y += (targetY - enemy.y) * 0.08;
    }

    // 構え開始：その場で自機方向へ狙いを定めて弾幕セットを生成
    if (cycle == 80) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotIai;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    if (cycle >= CYCLE) cycle = 0;
}