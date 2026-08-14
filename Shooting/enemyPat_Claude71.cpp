// enemyPat_HakunetsutouDanmatsuma.cpp
//
// 白熱灯断末魔 - 「切れかけの電球」モチーフの4フェーズパターン
//
// フェーズ1(1〜150F)  : フィラメント点弧    … 240発の密な螺旋でコイルが一気に形成される
// フェーズ2(151〜480F): 不規則明滅         … 点灯(3重リングバースト、1回あたり最大150発)と
//                                            消灯(安全地帯)がランダムな間隔で交互に発生
// フェーズ3(481〜620F): 断末魔の閃光       … 明滅間隔が加速度的に短くなり、最後に5重リング+
//                                            自機狙い15wayの大閃光(1回で約300発)
// フェーズ4(620F〜)   : 断線・飛散         … コイルの240発すべてがガラス片となって不揃いな
//                                            方向・速度で飛散
// 900Fで1サイクルとしてループする(飛散後はしばらく完全な暗闇が続く)

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const int FILAMENT_POINTS = 240;
static const int FILAMENT_SPAWN_END = 150;
static const int PHASE2_START = 151;
static const int OVERHEAT_START = 481;
static const int FLASH_TELEGRAPH = 590;
static const int FLASH_FIRE = 598;
static const int BREAK_FRAME = 620;
static const int CYCLE_LEN = 780;

static const double ANGULAR_VEL = 0.01; // フィラメントコイルの自転速度

// ---------------------------------------------------------
// 弾幕: フィラメントコイル
// フェーズ1で密な螺旋状(約10.7回転/240発)に生成され、緩やかに
// 自転しながら電球のガラスの中で光り続ける。BREAK_FRAME に達すると
// 各弾が自律的にガラス片として四散する。
// ---------------------------------------------------------
static void ShotFilamentCoil(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;
    if (pEnemyShotSet->count == 0) {
        pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->kind = img_enemyShotSmallBall[1]; // 黄色(白熱の色)
        pShot->param_d[0] = pEnemyShotSet->param_d[0]; // baseRadius
        pShot->param_d[1] = pEnemyShotSet->param_d[1]; // baseAngle
        pShot->param_d[6] = pEnemyShotSet->param_d[2]; // このサイクルの開始カウント
        pShot->param_i[0] = 0; // 0:コイル状態 1:飛散済み

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int localT = count - (int)pShot->param_d[6] + 1;
        double baseRadius = pShot->param_d[0];
        double baseAngle = pShot->param_d[1];

        // 断線の瞬間を一度だけ確定する
        if (pShot->param_i[0] == 0 && localT >= BREAK_FRAME) {
            pShot->param_i[0] = 1;

            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            double angleAtBreak = baseAngle + ANGULAR_VEL * pShot->count;
            double bx = enemy.x + baseRadius * cos(angleAtBreak);
            double by = enemy.y + baseRadius * sin(angleAtBreak) * 0.6;

            // ガラス片らしい不揃いな方向・速度(±30度、速度もばらつかせる)
            double shardAngle = angleAtBreak + (GetRand(60) - 30) / 180.0 * DX_PI;
            double shardSpeed = (150 + GetRand(250)) / 100.0; // 1.5〜4.0

            pShot->param_d[2] = bx;
            pShot->param_d[3] = by;
            pShot->param_d[4] = shardAngle;
            pShot->param_d[5] = shardSpeed;
            pShot->param_i[1] = count; // 割れたグローバルフレーム
            pShot->muki = shardAngle;
            pShot->kind = img_enemyShotDiamond[3]; // ガラス片(シアン)
        }

        if (pShot->param_i[0] == 0) {
            // コイル状態: 中心の周りをゆっくり自転しながら光る
            double angle = baseAngle + ANGULAR_VEL * pShot->count;
            double jitter = 0.0;
            if (localT >= OVERHEAT_START) {
                double p = (double)(localT - OVERHEAT_START) / (double)(FLASH_TELEGRAPH - OVERHEAT_START);
                if (p > 1.0) p = 1.0;
                // 過熱が進むほど小刻みに振動する
                jitter = p * 3.0 * sin(pShot->count * 1.3 + baseAngle * 5.0);
            }
            double radius = baseRadius + jitter;
            pShot->x = enemy.x + radius * cos(angle);
            pShot->y = enemy.y + radius * sin(angle) * 0.6; // 電球のガラス形状に合わせて縦に潰す
            pShot->muki = angle + DX_PI / 2.0; // 接線方向(自転する見た目のため)
        }
        else {
            // 飛散状態: 割れた瞬間から直線的に加速なく飛び続ける(formula-driven)
            int tSinceBreak = count - pShot->param_i[1];
            pShot->x = pShot->param_d[2] + pShot->param_d[5] * cos(pShot->param_d[4]) * tSinceBreak;
            pShot->y = pShot->param_d[3] + pShot->param_d[5] * sin(pShot->param_d[4]) * tSinceBreak;
        }

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 弾幕: 明滅バースト
// 「点灯」の瞬間に3重リング(速度・角度違い)で一気に全方位へ
// 弾を放つ。1回のバーストで最大150発。過熱が進むほど発射数・
// 速度が増える。
// ---------------------------------------------------------
static void ShotFlickerBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int n = pEnemyShotSet->param_i[0];        // 1リングあたりの発射数
        double speed = pEnemyShotSet->param_d[0]; // 基準速度
        double startAngle = GetRand(359) / 180.0 * DX_PI; // 毎回開始角をずらして単調さを避ける

        const int RING_COUNT = 3;
        for (int ring = 0; ring < RING_COUNT; ring++) {
            double ringSpeed = speed * (1.0 + ring * 0.35);
            double ringOffset = ring * (2.0 * DX_PI / n) / RING_COUNT;
            for (int i = 0; i < n; i++) {
                pShot = new sEnemyShot;
                double angle = startAngle + ringOffset + (2.0 * DX_PI) * i / n;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = angle;
                pShot->speed = ringSpeed;
                pShot->kind = img_enemyShotMediumBall[pEnemyShotSet->kind]; // 1:黄 or 8:橙

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count;
        pShot->x = pEnemyShotSet->x + pShot->speed * cos(pShot->muki) * t;
        pShot->y = pEnemyShotSet->y + pShot->speed * sin(pShot->muki) * t;
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 弾幕: 断末魔の閃光
// 速度違いの5重リング(275発)+自機狙い15wayを同時発射する
// 最大の一撃(1回で約290発)。
// ---------------------------------------------------------
static void ShotFlash(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int RING_N = 55;
        const int RING_COUNT = 5;
        const double speeds[RING_COUNT] = { 2.0, 2.8, 3.6, 4.4, 5.2 };
        for (int r = 0; r < RING_COUNT; r++) {
            double ringOffset = r * (2.0 * DX_PI / RING_N) / RING_COUNT;
            for (int i = 0; i < RING_N; i++) {
                pShot = new sEnemyShot;
                double angle = ringOffset + (2.0 * DX_PI) * i / RING_N;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = angle;
                pShot->speed = speeds[r];
                pShot->kind = img_enemyShotSmallBall[6]; // 白色の閃光

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }

        double aimAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double spread = 4.0 / 180.0 * DX_PI;
        for (int i = -7; i <= 7; i++) { // 自機狙い15way
            pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = aimAngle + spread * i;
            pShot->speed = 5.0;
            pShot->kind = img_enemyShotMediumOval[8]; // 橙色の自機狙い弾

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = pShot->count;
        pShot->x = pEnemyShotSet->x + pShot->speed * cos(pShot->muki) * t;
        pShot->y = pEnemyShotSet->y + pShot->speed * sin(pShot->muki) * t;
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 敵本体のパターン
// ---------------------------------------------------------
void EnemyPat_FlickeringLight_Claude()
{
    static bool isLit;
    static int  nextToggleCount;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 110.0;
        enemy.maxHp = enemy.hp = 200;
    }

    int localCount = (count - 1) % CYCLE_LEN + 1;
    int cycleStartCount = count - localCount + 1;

    // --- フェーズ1: フィラメント点弧(240発の密な螺旋を一気に形成) ---
    if (localCount <= FILAMENT_SPAWN_END) {
        for (int k = 0; k < 2; k++) { // 1フレームに2発ずつ生成
            int i = (localCount - 1) * 2 + k;
            if (i >= FILAMENT_POINTS) continue;

            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotFilamentCoil;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->param_d[0] = 6.0 + i * 0.35 * 1.5;          // baseRadius
            pSet->param_d[1] = i * 0.28 / 2;                // baseAngle(約10.7回転分の密な巻き)
            pSet->param_d[2] = (double)cycleStartCount; // このサイクルの開始カウント

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }

    // --- フェーズ2・3: 不規則な明滅(過熱が進むほど間隔が短く・激しくなる) ---
    if (localCount == PHASE2_START) {
        isLit = false;
        nextToggleCount = localCount + 25 + GetRand(30); // 最初の暗闇の長さ
    }
    if (localCount >= PHASE2_START && localCount < FLASH_TELEGRAPH && localCount == nextToggleCount) {
        isLit = !isLit;

        double progress = 0.0;
        if (localCount >= OVERHEAT_START) {
            progress = (double)(localCount - OVERHEAT_START) / (double)(FLASH_TELEGRAPH - OVERHEAT_START);
            if (progress > 1.0) progress = 1.0;
        }

        if (isLit) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotFlickerBurst;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->kind = (progress < 0.5) ? 1 : 8;          // 序盤は黄、過熱すると橙
            pSet->param_i[0] = 30 + (int)(20 * progress);    // 1リングあたりの発射数(3リング合計90〜150発)
            pSet->param_d[0] = 1.6 + 1.4 * progress;         // 基準速度 1.6〜3.0

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;

            int litMin = 8 - (int)(4 * progress);
            int litRange = 8 - (int)(4 * progress);
            nextToggleCount = localCount + litMin + GetRand(litRange); // 点灯時間(短い)
        }
        else {
            int darkMin = 18 - (int)(10 * progress);
            int darkRange = 40 - (int)(24 * progress);
            nextToggleCount = localCount + darkMin + GetRand(darkRange); // 消灯時間(安全地帯)
        }
    }

    // --- フェーズ3クライマックス: 断末魔の閃光 ---
    if (localCount == FLASH_TELEGRAPH) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 予告音
    }
    if (localCount == FLASH_FIRE) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFlash;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // フェーズ4(断線・飛散)は ShotFilamentCoil が
    // localT >= BREAK_FRAME を検知して各コイル弾が自律的に飛散へ切り替わる。
    // その後 CYCLE_LEN に達するまで暗闇が続き、次のサイクルの点弧へ戻る。
}