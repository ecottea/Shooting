// enemyPat_triSignal.cpp
// 信号機モチーフ弾幕「トライ・シグナル」
// 赤(停止・滞留) → 黄(注意・蛇行誘導) → 青(進行・高速直進) → 全消灯

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------------------------
// 信号機ランプ表示（敵に追従する赤黄青の大玉＋点灯時の輝き）
// ---------------------------------------------------------------------------
static void ShotSignalLamp(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回：ランプ本体（大玉）＋ 輝き用（小玉）を生成
    if (pEnemyShotSet->count == 0) {
        // 赤ランプ（敵の上30px）
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y - 30.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->muki = 0.0;
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤大玉
        pEnemyShot->param_i[0] = 0; // 0=赤ランプ本体
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 黄ランプ（敵と同じ高さ）
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->speed = 0.0;
        pEnemyShot->muki = 0.0;
        pEnemyShot->kind = img_enemyShotLargeBall[1]; // 黄大玉
        pEnemyShot->param_i[0] = 1; // 1=黄ランプ本体
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 青ランプ（敵の下30px）
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y + 30.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->muki = 0.0;
        pEnemyShot->kind = img_enemyShotLargeBall[4]; // 青大玉
        pEnemyShot->param_i[0] = 2; // 2=青ランプ本体
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 輝き弾×3（白玉。点灯時だけ脈動、消灯時はランプ本体に埋もれる）
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y - 30.0 + i * 30.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->muki = 0.0;
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白玉
            pEnemyShot->param_i[0] = 10 + i; // 10=赤輝き, 11=黄輝き, 12=青輝き
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム：敵位置に追従＋点灯/消灯制御
    int phase = pEnemyShotSet->param_i[0]; // 0=赤,1=黄,2=青,3=消灯

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int type = pShot->param_i[0];

        if (type == 0) { // 赤ランプ本体
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y - 30.0;
        }
        else if (type == 1) { // 黄ランプ本体
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
        }
        else if (type == 2) { // 青ランプ本体
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y + 30.0;
        }
        else if (type >= 10 && type <= 12) { // 輝き弾
            int lampIdx = type - 10; // 0=赤,1=黄,2=青
            double baseX = pEnemyShotSet->x;
            double baseY = pEnemyShotSet->y - 30.0 + lampIdx * 30.0;

            if (phase == lampIdx) {
                // 点灯中：脈動させて輝かせる
                double pulse = sin(pEnemyShotSet->count * 0.25) * 3.0;
                pShot->x = baseX + pulse;
                pShot->y = baseY;
            }
            else {
                // 消灯中：ランプ本体と同じ座標に埋もれて見えなくする
                pShot->x = baseX;
                pShot->y = baseY;
            }
        }

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------------------
// 赤ランプ：全方位12way・大玉・極低速（滞留弾）
// ---------------------------------------------------------------------------
static void ShotRed(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12 * 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->muki = i * DX_PI / 6.0 / 2; // 30°間隔の全方位
            pEnemyShot->speed = 0.8 + GetRand(20) / 100.0; // 0.80〜1.00 の極低速
            pEnemyShot->x = pEnemyShotSet->x + 50 * cos(pEnemyShot->muki);
            pEnemyShot->y = pEnemyShotSet->y - 30 + 50 * sin(pEnemyShot->muki);
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤大玉

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

// ---------------------------------------------------------------------------
// 黄ランプ：自機狙い3way・中玉・蛇行・中速
// ---------------------------------------------------------------------------
static void ShotYellow(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseMuki = pEnemyShotSet->muki; // 自機狙い角度
        for (int i = 0; i < 3+2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            double spread = (i - 1-1) * 0.12; // -0.12, 0, +0.12 rad
            pEnemyShot->muki = baseMuki + spread;
            pEnemyShot->param_d[0] = baseMuki + spread; // 基準角度を保存（蛇行用）
            pEnemyShot->speed = 2.5; // 中程度の速さ
            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄中玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double base = pShot->param_d[0];
        // 進行方向に対して垂直方向にサイン波で蛇行
        double wave = sin(pShot->count * 0.15) * 1.2;
        pShot->x += pShot->speed * cos(base) + wave * cos(base + DX_PI / 2.0);
        pShot->y += pShot->speed * sin(base) + wave * sin(base + DX_PI / 2.0);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------------------
// 青ランプ：自機狙い5way・小玉・高速直線
// ---------------------------------------------------------------------------
static void ShotBlue(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double baseMuki = pEnemyShotSet->muki; // 自機狙い角度
        for (int i = 0; i < 5+2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 30;
            double spread = (i - 2-1) * 0.06; // -0.12〜+0.12 rad の扇状5way
            pEnemyShot->muki = baseMuki + spread;
            pEnemyShot->speed = 7.0; // 非常に高速
            pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青小玉

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

// ---------------------------------------------------------------------------
// 敵本体パターン
// ---------------------------------------------------------------------------
void EnemyPat_TrafficLight_Kimi()
{
    static int muki;
    static int shot_count;
    static int lampCreated = 0;
    static sEnemyShotSet* pLampSet = nullptr;

    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;        // ランプを上下に出す余裕を持たせて少し下げる
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
        lampCreated = 0;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // ---- 信号機ランプセット（1回だけ生成、後は追従） ----
    if (!lampCreated) {
        pLampSet = new sEnemyShotSet;
        pLampSet->count = 0;
        pLampSet->patternFunc = ShotSignalLamp;
        pLampSet->x = enemy.x;
        pLampSet->y = enemy.y;
        pLampSet->muki = 0.0;
        pLampSet->kind = 0;
        pLampSet->param_i[0] = 0; // 点灯色：0=赤,1=黄,2=青,3=消灯

        pLampSet->pEnemyShotHead = new sEnemyShot;
        pLampSet->pEnemyShotHead->prev = pLampSet->pEnemyShotHead;
        pLampSet->pEnemyShotHead->next = pLampSet->pEnemyShotHead;

        pLampSet->prev = enemyShotSetHead.prev;
        pLampSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pLampSet;
        enemyShotSetHead.prev = pLampSet;

        lampCreated = 1;
    }

    // ランプを敵に追従させる
    if (pLampSet) {
        pLampSet->x = enemy.x;
        pLampSet->y = enemy.y;
    }

    // ---- サイクル管理（420フレーム周期） ----
    // 1〜120:赤  121〜240:黄  241〜360:青  361〜420:全消灯
    int cycle = (count - 1) % 420 + 1;
    int phase = 0;
    if (cycle <= 120)       phase = 0; // 赤
    else if (cycle <= 240)  phase = 1; // 黄
    else if (cycle <= 360)  phase = 2; // 青
    else                    phase = 3; // 消灯

    // ランプの点灯色を更新
    if (pLampSet) {
        pLampSet->param_i[0] = phase;
    }

    // フェーズ切り替わり時に予告音
    if (cycle == 1 || cycle == 121 || cycle == 241 || cycle == 361) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ---- 攻撃発射 ----
    if (phase == 0 && cycle % 30 == 1) {      // 赤：60f間隔で2回
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotRed;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else if (phase == 1 && cycle % 20 == 1) { // 黄：40f間隔で3回
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotYellow;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    else if (phase == 2 && cycle % 10 == 1) { // 青：20f間隔で6回
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBlue;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}