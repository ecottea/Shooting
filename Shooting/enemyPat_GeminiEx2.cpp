// enemyPat_lumia_demarcation.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 右旋回する全方位交差弾の挙動
static void ShotCircleSpinRight(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 10～50フレームの間だけ旋回させ、その後は直線軌道にする（綺麗な交差を作るため）
        if (pEnemyShot->count > 10 && pEnemyShot->count < 50) {
            pEnemyShot->muki += 0.012;
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 左旋回する全方位交差弾の挙動
static void ShotCircleSpinLeft(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 10～50フレームの間だけ旋回させ、その後は直線軌道にする
        if (pEnemyShot->count > 10 && pEnemyShot->count < 50) {
            pEnemyShot->muki -= 0.012;
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 全方位旋回弾（33WAY×2重）を生成する関数
static void CreateCircleSpin(double x, double y, int color, bool isRight)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = isRight ? ShotCircleSpinRight : ShotCircleSpinLeft;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    int ways = 33;
    // Lunatic仕様の2重弾（速度の異なる2つの波）
    double speeds[2] = { 1.6, 2.2 };

    // 発射ごとにランダムな初期角度オフセット（GetRand(360) で 0～360 の整数）
    double baseAngle = (GetRand(360) / 180.0) * DX_PI;

    for (int s = 0; s < 2; s++) {
        for (int i = 0; i < ways; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x;
            pEnemyShot->y = y;
            // 全方位に均等分配
            pEnemyShot->muki = baseAngle + (double)i * (2.0 * DX_PI / ways);
            pEnemyShot->speed = speeds[s];
            pEnemyShot->kind = img_enemyShotBullet[color]; // 銃弾（米粒弾の代用）
            pEnemyShot->count = 0;

            // 弾リストへの挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // セットリストへの挿入
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 一旦停止した後に自機を狙う弾塊の挙動
static void ShotClusterAim(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->count < 40) {
            // 最初は慣性で進みながら急減速し、一旦停止させる
            pEnemyShot->speed *= 0.90;
        }
        else if (pEnemyShot->count == 40) {
            // 40フレーム目で自機を狙い直す
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            // 本来の飛んでいく速度を設定（ランダムなバラつきを付与して塊を崩す）
            pEnemyShot->speed = (200 + GetRand(150)) / 100.0; // 2.0 ～ 3.5
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }

    // 自機を狙って動き出す瞬間に効果音を1回鳴らす
    if (pEnemyShotSet->count == 40) {
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
}

// 弾塊（自機狙い）を生成する関数
static void CreateClusterAim(double x, double y)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotClusterAim;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    // 発射時点での自機への基本角度
    double baseAngle = atan2(player.y - y, player.x - x);

    // 1つの弾塊を構成する弾数（Lunatic用に多めの16発）
    int bulletCount = 16;
    for (int i = 0; i < bulletCount; i++) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = x;
        pEnemyShot->y = y;

        // 塊として一体感を持たせつつ、初期のばら撒きを作るために角度と速度を少しランダム化
        pEnemyShot->muki = baseAngle + (GetRand(30) - 15) / 180.0 * DX_PI; // -15度 ～ +15度
        pEnemyShot->speed = (150 + GetRand(200)) / 100.0;                  // 1.5 ～ 3.5
        pEnemyShot->kind = img_enemyShotMediumBall[4];                     // 青の中玉（青光輪弾の代用）
        pEnemyShot->count = 0;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 敵本体のパターン関数
void EnemyPat_Demarcation_Gemini()
{
    static int timer = 0;
    static int moveMuki = 1;

    if (count == 1) {
        // 初期位置とHPの設定
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // スペルカード用に多めの耐久
        timer = 0;
        moveMuki = 1;
    }

    // 敵固有の行動管理用タイマー（countの代わりにループ制御に使用）
    timer++;
    int t = timer % 380; // 1セット380フレームのループ

    // 1. 全方位旋回弾の射出（青[4] -> 緑[2] -> 赤[0] の順に時間差で展開）
    if (t == 10) {
        // 青色：右旋回と左旋回を同時に出して交差させる
        CreateCircleSpin(enemy.x, enemy.y, 4, true);
        CreateCircleSpin(enemy.x, enemy.y, 4, false);
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
    else if (t == 50) {
        // 緑色
        CreateCircleSpin(enemy.x, enemy.y, 2, true);
        CreateCircleSpin(enemy.x, enemy.y, 2, false);
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
    else if (t == 90) {
        // 赤色
        CreateCircleSpin(enemy.x, enemy.y, 0, true);
        CreateCircleSpin(enemy.x, enemy.y, 0, false);
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 2. 移動しながら自機狙い弾塊を4条射出
    if (t == 140) {
        // 移動開始時に、画面端に追い詰められないよう自機の位置を見て移動方向を決定
        if (enemy.x > 320)      moveMuki = -1;
        else if (enemy.x < 160) moveMuki = 1;
        else                    moveMuki = (player.x > enemy.x) ? 1 : -1;
    }

    // 移動期間 (140～240フレーム)
    if (t >= 140 && t <= 240) {
        enemy.x += 1.5 * moveMuki;
        // 画面外へのハミ出し防止 (ゲーム画面480x480を考慮)
        if (enemy.x < 60) { enemy.x = 60;  moveMuki = 1; }
        if (enemy.x > 420) { enemy.x = 420; moveMuki = -1; }
    }

    // 移動中に20フレーム間隔で弾塊を計4回射出
    if (t == 150 || t == 170 || t == 190 || t == 210) {
        CreateClusterAim(enemy.x, enemy.y);
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
}