// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --- 弾幕パターン1：薄明（ぼんやりとした光） ---
// 自機に向けた狭い扇状の黄色い弾を高頻度で連射する
static void ShotDimLight(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 自機方向に絞った狭い扇状（7ウェイ）
        int num = 7;
        double spreadAngle = DX_PI / 9.0; // 20度の扇形
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki = pEnemyShotSet->muki - (spreadAngle / 2.0) + (spreadAngle * i / (double)(num - 1));
            pEnemyShot->speed = 3.0 + GetRand(15) / 10.0; // 3.0 〜 4.5
            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 中玉、黄(1)

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

// --- 弾幕パターン2：放電（フィラメントの火花） ---
// 自機付近を狙い撃つ超高速の橙弾を大量にばら撒く
static void ShotSparking(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 自機方向に強く偏らせたランダム弾（12発）
        int num = 12 / 4;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 自機方向から±30度のブレ
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(60) - 30) / 180.0 * DX_PI;
            pEnemyShot->speed = 6.0 + GetRand(40) / 10.0; // 6.0 〜 10.0 の超高速
            pEnemyShot->kind = img_enemyShotSmallBall[8]; // 小玉、橙(8) ※黄色と見間違いやすくプレッシャーを向上

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

// --- 弾幕パターン3：閃光の輪（最後の力を振り絞る点灯） ---
// 3重の速度帯を持つ高密度な白色全方位弾
static void ShotFlashRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 24方向 × 3重（速度変化）= 72発
        int num = 24;
        double speeds[3] = { 2.0, 3.5, 5.0 };
        for (int s = 0; s < 3; s++) {
            for (int i = 0; i < num; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                pEnemyShot->muki = (2.0 * DX_PI * i) / num + (s * DX_PI / num); // 位置をずらして隙間を減らす
                pEnemyShot->speed = speeds[s];
                pEnemyShot->kind = img_enemyShotMediumBall[6]; // 中玉、白(6)

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// --- 弾幕パターン4：消灯（完全なる闇） ---
// 視認困難な極低速の白色弾をランダムにばら撒き、後続の弾幕のための地雷原を作る
static void ShotBlackout(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 消灯時は音を鳴らさない（静寂による不気味さ）

        // 全方向ランダムな極低速弾（40発）
        int num = 40;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.3 + GetRand(15) / 10.0; // 0.3 〜 1.8 の極低速
            pEnemyShot->kind = img_enemyShotSmallBall[7]; // 小玉、白(6) ※暗闇で見えにくい

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


// --- ヘルパー関数：Setの生成処理の共通化 ---
static void CreateShotSet(void (*patternFunc)(sEnemyShotSet*))
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;
    pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
    pEnemyShotSet->kind = 0;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}


// ======================================================================
// 敵本体のパターン
// ======================================================================
void EnemyPat_FlickeringLight_Zai()
{
    static int muki;
    static int currentState; // 0:薄明, 1:放電, 2:閃光, 3:消灯
    static int stateTimer;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        currentState = 0;
        stateTimer = 30; // 最初の0.5秒は薄明
    }
    else {
        // 左右にゆらゆら移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;

        stateTimer--;

        // 状態遷移タイミング
        if (stateTimer <= 0) {
            int r = GetRand(4); // 0〜4
            if (r <= 1) {
                currentState = 0; // 薄明 (40%)
                stateTimer = 36;  // 0.6秒間持続
            }
            else if (r == 2) {
                currentState = 1; // 放電 (20%)
                stateTimer = 10;  // 0.16秒間持続
            }
            else if (r == 3) {
                currentState = 2; // 閃光 (20%)
                stateTimer = 10;  // 閃光後の硬直時間
            }
            else {
                currentState = 3; // 消灯 (20%)
                stateTimer = 60;  // 1.0秒間持続
            }
        }

        // 状態に応じた弾幕の発射
        switch (currentState) {
        case 0: // 薄明：高頻度で扇状弾を連射
            if (stateTimer % 6 == 0) {
                CreateShotSet(ShotDimLight);
            }
            break;

        case 1: // 放電：毎フレーム大量の高速弾を叩き込む
            if (stateTimer % 2 == 0) {
                CreateShotSet(ShotSparking);
            }
            break;

        case 2: // 閃光：1回だけ高密度な全方位輪を放出
            if (stateTimer == 10) { // 遷移直後の1フレーム目のみ
                CreateShotSet(ShotFlashRing);
            }
            break;

        case 3: // 消灯：地雷原の形成
            if (stateTimer == 60) { // 遷移直後の1フレーム目のみ
                CreateShotSet(ShotBlackout);
            }
            break;
        }
    }
}