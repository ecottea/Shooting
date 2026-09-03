// enemyPat_tomatoFestival.cpp
// トマト投げ祭り弾幕「大収穫祭」
// 使用素材：赤い丸弾(トマト本体)、緑の小玉(ヘタ)、白い小玉(種)、黄の小玉(提灯演出)

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 果汁飛散（トマト破裂時の演出）
// ============================================================
static void ShotTomatoJuice(sEnemyShotSet* pSet)
{
    sEnemyShot* p;

    if (pSet->count == 0) {
        // 軽い破裂音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int n = pSet->param_i[0];          // 果汁の数
        double baseAngle = pSet->param_d[0]; // 基準角度

        for (int i = 0; i < n; i++) {
            // 果汁（赤い小玉）
            p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            double angle = baseAngle + DX_PI * 2.0 * i / n + (GetRand(20) - 10) / 180.0 * DX_PI;
            p->muki = angle;
            p->speed = (80 + GetRand(120)) / 100.0;
            p->kind = img_enemyShotSmallBall[0]; // 0:赤
            p->param_d[0] = 0.03/2; // 重力
            p->param_i[0] = 0;    // 種類: 果汁

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }

        // ヘタ（緑の小玉）- 上方向に弾け飛ぶ
        p = new sEnemyShot;
        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = -DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
        p->speed = (100 + GetRand(100)) / 100.0;
        p->kind = img_enemyShotSmallBall[2]; // 2:緑
        p->param_d[0] = 0.04/2; // 重力（少し強め）
        p->param_i[0] = 1;    // 種類: ヘタ

        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;

        // 種（白い小玉）- 高速で飛び散る
        for (int i = 0; i < 3; i++) {
            p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            p->muki = baseAngle + DX_PI * 2.0 / 3.0 * i + (GetRand(30) - 15) / 180.0 * DX_PI;
            p->speed = (200 + GetRand(150)) / 100.0;
            p->kind = img_enemyShotSmallBall[6]; // 6:白
            p->param_d[0] = 0.02/2; // 重力（弱め）
            p->param_i[0] = 2;    // 種類: 種

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // 移動処理（重力あり）
    sEnemyShot* cur = pSet->pEnemyShotHead->next;
    while (cur != pSet->pEnemyShotHead) {
        double vx = cur->speed * cos(cur->muki);
        double vy = cur->speed * sin(cur->muki);
        vy += cur->param_d[0];
        cur->speed = sqrt(vx * vx + vy * vy);
        cur->muki = atan2(vy, vx);
        cur->x += vx;
        cur->y += vy;

        cur = cur->next;
    }
}

// ============================================================
// トマト投げ（放物線運動）
// ============================================================
static void ShotTomatoThrow(sEnemyShotSet* pSet)
{
    sEnemyShot* p;

    if (pSet->count == 0) {
        // 投げる音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double speed = pSet->param_d[0];
        double angle = pSet->muki;

        // トマト本体（赤い中玉）
        p = new sEnemyShot;
        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = angle;
        p->speed = speed;
        p->kind = img_enemyShotMediumBall[0]; // 0:赤
        p->param_d[0] = 0.04;  // 重力
        p->param_i[0] = pSet->param_i[0]; // 破裂までの時間
        p->param_i[1] = 0;     // 種類: トマト本体

        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;

        // ヘタ（緑の小玉）- トマトの上部に付着
        p = new sEnemyShot;
        p->x = pSet->x + cos(angle + DX_PI / 2.0) * 4.0;
        p->y = pSet->y + sin(angle + DX_PI / 2.0) * 4.0;
        p->muki = angle;
        p->speed = speed;
        p->kind = img_enemyShotSmallBall[2]; // 2:緑
        p->param_d[0] = 0.04;  // 重力
        p->param_i[0] = pSet->param_i[0]; // 同じ時間で消滅
        p->param_i[1] = 1;     // 種類: ヘタ

        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }

    // 移動処理と破裂判定
    sEnemyShot* cur = pSet->pEnemyShotHead->next;
    while (cur != pSet->pEnemyShotHead) {
        double vx = cur->speed * cos(cur->muki);
        double vy = cur->speed * sin(cur->muki);
        vy += cur->param_d[0];
        cur->speed = sqrt(vx * vx + vy * vy);
        cur->muki = atan2(vy, vx);
        cur->x += vx;
        cur->y += vy;

        // トマト本体が破裂時間に達したら果汁飛散
        if (cur->param_i[1] == 0 && cur->count >= cur->param_i[0]) {
            sEnemyShotSet* pJuice = new sEnemyShotSet;
            pJuice->count = 0;
            pJuice->patternFunc = ShotTomatoJuice;
            pJuice->x = cur->x;
            pJuice->y = cur->y;
            pJuice->param_i[0] = 8;          // 果汁の数
            pJuice->param_d[0] = cur->muki;  // 基準角度

            pJuice->pEnemyShotHead = new sEnemyShot;
            pJuice->pEnemyShotHead->prev = pJuice->pEnemyShotHead;
            pJuice->pEnemyShotHead->next = pJuice->pEnemyShotHead;

            pJuice->prev = enemyShotSetHead.prev;
            pJuice->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pJuice;
            enemyShotSetHead.prev = pJuice;

            // この弾（トマト本体）を削除
            sEnemyShot* del = cur;
            cur = cur->next;
            del->prev->next = del->next;
            del->next->prev = del->prev;
            delete del;
            continue;
        }

        // ヘタも同じタイミングで消滅
        if (cur->param_i[1] == 1 && cur->count >= cur->param_i[0]) {
            sEnemyShot* del = cur;
            cur = cur->next;
            del->prev->next = del->next;
            del->next->prev = del->prev;
            delete del;
            continue;
        }

        cur = cur->next;
    }
}

// ============================================================
// 祭りの提灯（黄色い小玉が降ってくる背景演出）
// ============================================================
static void ShotFestivalLight(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = 0; i < 5; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = 20 + GetRand(440);
            p->y = -10 - GetRand(50);
            p->muki = DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
            p->speed = (80 + GetRand(60)) / 100.0;
            p->kind = img_enemyShotSmallBall[1]; // 1:黄
            p->param_d[0] = 0.01; // わずかな重力

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* cur = pSet->pEnemyShotHead->next;
    while (cur != pSet->pEnemyShotHead) {
        double vx = cur->speed * cos(cur->muki);
        double vy = cur->speed * sin(cur->muki);
        vy += cur->param_d[0];
        cur->speed = sqrt(vx * vx + vy * vy);
        cur->muki = atan2(vy, vx);
        cur->x += vx;
        cur->y += vy;

        cur = cur->next;
    }
}

// ============================================================
// 乱舞するトマト（壁でバウンド、ランダムに破裂）
// ============================================================
static void ShotTomatoDance(sEnemyShotSet* pSet)
{
    sEnemyShot* p;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int n = 12 - 4;
        for (int i = 0; i < n; i++) {
            p = new sEnemyShot;
            p->x = pSet->x + GetRand(60) - 30;
            p->y = pSet->y + GetRand(20) - 10;
            p->muki = DX_PI * 2.0 * i / n + (GetRand(30) - 15) / 180.0 * DX_PI;
            p->speed = (150 + GetRand(200)) / 100.0;

            // 大・中・小のランダムサイズ（全て赤系）
            int size = GetRand(2);
            if (size == 0)      p->kind = img_enemyShotSmallBall[0];   // 赤小玉
            else if (size == 1) p->kind = img_enemyShotMediumBall[0];  // 赤中玉
            else                p->kind = img_enemyShotLargeBall[0];   // 赤大玉

            p->param_d[0] = 0.03;              // 重力
            p->param_i[0] = 40 + GetRand(40);  // 破裂時間（ランダム）
            p->param_i[1] = 20;                // 乱舞トマトフラグ

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* cur = pSet->pEnemyShotHead->next;
    while (cur != pSet->pEnemyShotHead) {
        double vx = cur->speed * cos(cur->muki);
        double vy = cur->speed * sin(cur->muki);
        vy += cur->param_d[0];
        cur->speed = sqrt(vx * vx + vy * vy);
        cur->muki = atan2(vy, vx);
        cur->x += vx;
        cur->y += vy;

        // 左右の壁でバウンド
        if (cur->x < 10 || cur->x > 470) {
            cur->muki = DX_PI - cur->muki;
            cur->x = (cur->x < 10) ? 10 : 470;
        }
        // 天井でもバウンド
        if (cur->y < 10) {
            cur->muki = -cur->muki;
            cur->y = 10;
        }

        // 破裂判定
        if (cur->count >= cur->param_i[0]) {
            sEnemyShotSet* pJuice = new sEnemyShotSet;
            pJuice->count = 0;
            pJuice->patternFunc = ShotTomatoJuice;
            pJuice->x = cur->x;
            pJuice->y = cur->y;
            pJuice->param_i[0] = 6;
            pJuice->param_d[0] = cur->muki;

            pJuice->pEnemyShotHead = new sEnemyShot;
            pJuice->pEnemyShotHead->prev = pJuice->pEnemyShotHead;
            pJuice->pEnemyShotHead->next = pJuice->pEnemyShotHead;

            pJuice->prev = enemyShotSetHead.prev;
            pJuice->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pJuice;
            enemyShotSetHead.prev = pJuice;

            sEnemyShot* del = cur;
            cur = cur->next;
            del->prev->next = del->next;
            del->next->prev = del->prev;
            delete del;
            continue;
        }

        cur = cur->next;
    }
}

// ============================================================
// 巨大トマトの渦巻き（フィナーレ用）
// ============================================================
static void ShotGiantTomato(sEnemyShotSet* pSet)
{
    sEnemyShot* p;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int n = pSet->param_i[0];      // 発射数
        double baseSpeed = pSet->param_d[0];

        for (int i = 0; i < n; i++) {
            p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y;
            double angle = DX_PI * 2.0 * i / n;
            p->muki = angle;
            p->speed = baseSpeed;
            p->kind = img_enemyShotLargeBall[0]; // 0:赤 大玉
            p->param_d[0] = 0.02;  // 渦巻き用の下向き加速度
            p->param_i[0] = 90;    // 破裂までの時間
            p->param_i[1] = 10;    // 巨大トマトフラグ

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* cur = pSet->pEnemyShotHead->next;
    while (cur != pSet->pEnemyShotHead) {
        double vx = cur->speed * cos(cur->muki);
        double vy = cur->speed * sin(cur->muki);

        // 渦巻き運動：進行方向に直角に少しずつ曲がる
        double turn = 0.015;
        double nx = vx * cos(turn) - vy * sin(turn);
        double ny = vx * sin(turn) + vy * cos(turn);

        // 重力も加える
        ny += cur->param_d[0];

        cur->speed = sqrt(nx * nx + ny * ny);
        cur->muki = atan2(ny, nx);
        cur->x += nx;
        cur->y += ny;

        // 破裂して中玉に分裂
        if (cur->count >= cur->param_i[0]) {
            for (int i = 0; i < 4/2; i++) {
                sEnemyShotSet* pSplit = new sEnemyShotSet;
                pSplit->count = 0;
                pSplit->patternFunc = ShotTomatoJuice;
                pSplit->x = cur->x;
                pSplit->y = cur->y;
                pSplit->param_i[0] = 6;
                pSplit->param_d[0] = DX_PI * 2.0 * i / 4.0*2;

                pSplit->pEnemyShotHead = new sEnemyShot;
                pSplit->pEnemyShotHead->prev = pSplit->pEnemyShotHead;
                pSplit->pEnemyShotHead->next = pSplit->pEnemyShotHead;

                pSplit->prev = enemyShotSetHead.prev;
                pSplit->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSplit;
                enemyShotSetHead.prev = pSplit;
            }

            sEnemyShot* del = cur;
            cur = cur->next;
            del->prev->next = del->next;
            del->next->prev = del->prev;
            delete del;
            continue;
        }

        cur = cur->next;
    }
}

// ============================================================
// 敵本体のパターン：トマト投げ祭り「大収穫祭」
// ============================================================
void EnemyPat_Tomatina_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆるやかに動く
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // --------------------------------------------------------
    // フェーズ1: 収穫の始まり (count 1〜300)
    // 自機狙いのトマト投げ（放物線）
    // --------------------------------------------------------
    if (count <= 300 && count % 25 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTomatoThrow;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->param_d[0] = (250 + GetRand(100)) / 100.0; // 初速 2.5〜3.5
        pSet->param_i[0] = 45 + GetRand(20);             // 破裂まで 45〜65フレーム

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --------------------------------------------------------
    // フェーズ2: 祭りの高まり (count 300〜600)
    // 全方位から乱舞するトマト（壁でバウンド、ランダム破裂）
    // --------------------------------------------------------
    if (count > 300 && count <= 600 && count % 80 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTomatoDance;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --------------------------------------------------------
    // 祭りの演出：提灯風の黄色い小玉（常時降下）
    // --------------------------------------------------------
    if (count % 60 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFestivalLight;
        pSet->x = 240.0;
        pSet->y = 0.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --------------------------------------------------------
    // フェーズ3: 大収穫フィナーレ (count 600〜900)
    // 巨大トマトが渦巻きながら破裂
    // --------------------------------------------------------
    if (count > 600 && count <= 900 && count % 100 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotGiantTomato;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->param_i[0] = 8 - 2;                         // 発射数
        pSet->param_d[0] = (180 + GetRand(80)) / 100.0; // 初速 1.8〜2.6

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // --------------------------------------------------------
    // フィナーレ演出 (count 900〜)
    // 乱舞と巨大トマトの同時発射でクライマックス
    // --------------------------------------------------------
    if (count > 900 && count % 60 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotTomatoDance;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}