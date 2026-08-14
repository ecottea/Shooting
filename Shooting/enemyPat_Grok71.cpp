// enemyPat_Tmp.cpp
// 切れかけの電球をモチーフにした弾幕パターン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 使える素材の整理（サンプルより）
// 効果音: sound_enemyShot_light / medium / heavy / extreme / sound_enemyCharge
// 弾種: SmallBall, MediumBall, LargeBall, Bullet, Scale, Diamond, MediumOval, Laser
// 色: 0赤 1黄 2緑 3シアン 4青 5マゼンタ 6白 7黒 8橙
// 本パターンで主に使用: 黄(1)・橙(8)・白(6) の SmallBall / MediumBall / Diamond
// ------------------------------------------------------------

// 放射状の光線弾（点灯フェーズ）
static void ShotLightRay(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 12〜18本の放射弾。角度と速度にわずかな揺らぎを付けて「安定しない光」を表現
        int num = 12 + GetRand(6);
        double baseAngle = pEnemyShotSet->muki; // プレイヤー方向を基準に少しずらしてもよい
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            double angleOffset = (i * 360.0 / num + (GetRand(20) - 10)) * DX_PI / 180.0;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + angleOffset;
            // 速度を少しばらつかせてチラつき感を出す
            pEnemyShot->speed = (160 + GetRand(120)) / 100.0;

            // 黄・橙・白をランダムに混ぜる
            int col = 1; // 黄
            int r = GetRand(9);
            if (r < 4) col = 1;      // 黄
            else if (r < 7) col = 8; // 橙
            else col = 6;            // 白

            // 小玉と中玉を混ぜて光の筋の太さを変化
            if (GetRand(2) == 0)
                pEnemyShot->kind = img_enemyShotSmallBall[col];
            else
                pEnemyShot->kind = img_enemyShotMediumBall[col];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動（等速直線運動）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 消灯時の火花弾（ランダム短距離）
static void ShotSparks(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 8〜15個の小さな火花
        int num = 8 + GetRand(7);
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(40) - 20);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(40) - 20);
            // 完全ランダム方向
            pEnemyShot->muki = GetRand(360) * DX_PI / 180.0;
            // 低速で短く飛ぶ
            pEnemyShot->speed = (80 + GetRand(100)) / 100.0;

            int col = (GetRand(1) == 0) ? 1 : 8; // 黄 or 橙
            pEnemyShot->kind = img_enemyShotSmallBall[col];

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

// 不安定な大点滅（密集放射 + 拡大リング）
static void ShotIntenseFlicker(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 密集放射（20〜28本）
        int num = 20 + GetRand(8) + 50;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = (i * 360.0 / num + (GetRand(12) - 6)) * DX_PI / 180.0;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = (200 + GetRand(150)) / 100.0;

            int col = 1;
            int r = GetRand(9);
            if (r < 3) col = 1;
            else if (r < 6) col = 8;
            else col = 6;

            if (GetRand(3) == 0)
                pEnemyShot->kind = img_enemyShotMediumBall[col];
            else
                pEnemyShot->kind = img_enemyShotSmallBall[col];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 同時に拡大リング（1周分）
        int ringNum = 16 + GetRand(8) + 50;
        for (int i = 0; i < ringNum; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = i * 360.0 / ringNum * DX_PI / 180.0;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = (140 + GetRand(60)) / 100.0; // やや遅めの波

            int col = (GetRand(1) == 0) ? 1 : 8;
            pEnemyShot->kind = img_enemyShotDiamond[col]; // 菱形でフィラメント感を出す

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

// 最終破裂（破片＋火花の混合）
static void ShotBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 大きめの破片
        int num = 10 + GetRand(6) + 100;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(30) - 15);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(30) - 15);
            pEnemyShot->muki = GetRand(360) * DX_PI / 180.0;
            pEnemyShot->speed = (180 + GetRand(200)) / 100.0;

            int col = (GetRand(1) == 0) ? 1 : 8;
            // 中玉と中楕円を混ぜる
            if (GetRand(1) == 0)
                pEnemyShot->kind = img_enemyShotLargeBall[col];
            else
                pEnemyShot->kind = img_enemyShotMediumOval[col];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 細かい火花を追加
        int sparkNum = 12 + GetRand(8) + 100;
        for (int i = 0; i < sparkNum; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(50) - 25);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(50) - 25);
            pEnemyShot->muki = GetRand(360) * DX_PI / 180.0;
            pEnemyShot->speed = (100 + GetRand(150)) / 100.0;

            int col = (GetRand(2) == 0) ? 1 : ((GetRand(1) == 0) ? 8 : 6);
            pEnemyShot->kind = img_enemyShotSmallBall[col];

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

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_FlickeringLight_Grok()
{
    // 静的変数で状態を保持
    static int phase;          // 0:通常点滅, 1:消灯火花, 2:大点滅, 3:破裂前チャージ, 4:破裂
    static int phaseTimer;     // 現在フェーズの残り時間
    static int nextAction;     // 次に弾を出すまでのカウント
    static int swayDir;        // 左右の揺れ方向
    static int cycleCount;     // サイクル回数（破裂のタイミング制御用）

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        phaseTimer = 90 + GetRand(40);
        nextAction = 20 + GetRand(20);
        swayDir = 1;
        cycleCount = 0;
    }
    else {
        // 電球らしいゆっくりした左右の揺れ
        enemy.x += 0.45 * (double)swayDir;
        if (enemy.x > 280.0) swayDir = -1;
        if (enemy.x < 200.0) swayDir = 1;

        // わずかな上下の浮遊感
        enemy.y = 70.0 + 6.0 * sin(count * 0.03);
    }

    // フェーズタイマー減少
    if (phaseTimer > 0) phaseTimer--;

    // 次アクションまでのカウント
    if (nextAction > 0) nextAction--;

    // フェーズ遷移と弾生成
    if (nextAction <= 0) {
        // phase 3 はチャージのみ（弾セットを作らない）
        if (phase == 3) {
            phase = 4;
            nextAction = 1;
        }
        else {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 8.0;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShotSet->kind = 0;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;

            switch (phase) {
            case 0: // 点灯光線
                pEnemyShotSet->patternFunc = ShotLightRay;
                // 点灯は短めの間隔で数回繰り返す
                nextAction = 18 + GetRand(12);      // 18〜30
                phaseTimer = 55 + GetRand(25);      // このフェーズ自体の寿命
                // 高確率で消灯へ（切れかけ感を出す）
                if (GetRand(2) == 0) {              // 約50%
                    phase = 1;
                    nextAction = 6 + GetRand(8);    // すぐ火花へ
                }
                break;

            case 1: // 消灯火花
                pEnemyShotSet->patternFunc = ShotSparks;
                nextAction = 12 + GetRand(10);      // 12〜22
                phaseTimer = 35 + GetRand(15);
                // 火花の後は点灯に戻るか、大点滅へ（大点滅へ行きやすく）
                if (GetRand(2) <= 1) {              // 約67% で大点滅へ
                    phase = 2;
                    nextAction = 8 + GetRand(6);
                }
                else {
                    phase = 0;
                }
                break;

            case 2: // 不安定大点滅
                pEnemyShotSet->patternFunc = ShotIntenseFlicker;
                nextAction = 28 + GetRand(12);      // 28〜40
                phaseTimer = 45 + GetRand(20);
                cycleCount++;
                // 2〜3回で破裂準備へ（以前より早く）
                if (cycleCount >= 2 + GetRand(1)) {
                    phase = 3;
                    nextAction = 25 + GetRand(15);  // チャージ時間
                    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
                }
                else {
                    // 大点滅の合間は火花を挟むことが多い
                    phase = (GetRand(3) == 0) ? 0 : 1;
                    nextAction = 10 + GetRand(8);
                }
                break;

            case 4: // 破裂
                pEnemyShotSet->patternFunc = ShotBurst;
                // 破裂後は少し間を置いて通常点滅に戻る
                phase = 0;
                cycleCount = 0;
                nextAction = 40 + GetRand(20);
                phaseTimer = 50 + GetRand(20);
                break;
            }
        }
    }

    // フェーズタイマーが尽きた場合の強制遷移
    // 点灯⇔消灯の往復を強制しつつ、大点滅へも押し出す
    if (phaseTimer <= 0 && phase < 3) {
        if (phase == 0) {
            phase = 1;                              // 点灯が続いたら必ず消灯へ
        }
        else if (phase == 1) {
            // 消灯が続いたら高確率で大点滅、低確率で点灯に戻る
            phase = (GetRand(3) == 0) ? 0 : 2;
        }
        else if (phase == 2) {
            // 大点滅タイマー切れは点灯か火花へ
            phase = (GetRand(2) == 0) ? 0 : 1;
        }
        phaseTimer = 30 + GetRand(25);              // 次の強制遷移までの時間を短めに
        nextAction = 4 + GetRand(8);                // すぐ次の弾を出せるように
    }
}