// enemyPat_Demarcation_Lunatic.cpp
// 闇符「ディマーケイション」(Lunatic) 完全実装

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// ルーミアのスペルパターン (Lunatic)
// ------------------------------------------------------------
static void DemarcationPattern(sEnemyShotSet* pSet)
{
    // 敵本体の現在座標に追従
    pSet->x = enemy.x;
    pSet->y = enemy.y + 20.0;

    const int t = pSet->count;   // 現在フレーム (0～)

    // ---- 定数 ----
    const int WAY = 33;
    const int LAYERS = 2;
    const double LAYER_SPEED[2] = { 1.6, 2.2 };
    const double ROTATE_AMOUNT = 0.012;
    const int ROTATE_START = 11;
    const int ROTATE_END = 50;

    const int CLUSTER_BULLETS = 16;
    const int CLUSTER_STOP_TIME = 40;

    // 効果音
    auto playWaveSE = []() {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    };
    auto playClusterSE = []() {
        if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    };

    // ---- 全方位旋回交差弾 生成 (青, 緑, 赤) ----
    auto spawnRotatingWave = [&](int colorIndex, int spawnTime) {
        if (t != spawnTime) return;
        playWaveSE();

        double offset = GetRand(628) / 100.0;   // 0.0 ～ 2π
        for (int i = 0; i < WAY; ++i) {
            double angle = offset + i * (2.0 * DX_PI) / WAY;
            for (int layer = 0; layer < LAYERS; ++layer) {
                double speed = LAYER_SPEED[layer];
                // 右旋回 (+), 左旋回 (-)
                for (int dirSign : {1, -1}) {
                    sEnemyShot* shot = new sEnemyShot;
                    shot->x = pSet->x;
                    shot->y = pSet->y;
                    shot->muki = angle;
                    shot->speed = dirSign * speed;   // 符号で回転方向を識別
                    shot->kind = img_enemyShotSmallBall[colorIndex];
                    shot->count = 0;

                    shot->prev = pSet->pEnemyShotHead->prev;
                    shot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = shot;
                    pSet->pEnemyShotHead->prev = shot;
                }
            }
        }
    };

    spawnRotatingWave(4, 10);  // 青 (color index 4)
    spawnRotatingWave(2, 50);  // 緑 (color index 2)
    spawnRotatingWave(0, 90);  // 赤 (color index 0)

    // ---- 自機狙い弾塊 (4回) 生成 ----
    if (t == 150 || t == 170 || t == 190 || t == 210) {
        double baseAngle = atan2(player.y - pSet->y, player.x - pSet->x);
        for (int i = 0; i < CLUSTER_BULLETS; ++i) {
            double angleOffset = (GetRand(300) - 150) * DX_PI / 1800.0; // ±15度
            double initSpeed = (GetRand(200) + 200) / 100.0;            // 2.0～4.0

            sEnemyShot* shot = new sEnemyShot;
            shot->x = pSet->x;
            shot->y = pSet->y;
            shot->muki = baseAngle + angleOffset;
            shot->speed = initSpeed;
            shot->kind = img_enemyShotMediumBall[4];   // 青中玉
            shot->count = 0;

            shot->prev = pSet->pEnemyShotHead->prev;
            shot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = shot;
            pSet->pEnemyShotHead->prev = shot;
        }
    }

    // ---- 全弾の移動と特殊挙動 ----
    bool clusterReAimed = false;   // 再ロックオンSE用

    sEnemyShot* shot = pSet->pEnemyShotHead->next;
    while (shot != pSet->pEnemyShotHead) {
        int kind = shot->kind;

        // ◆ 全方位交差弾 (小玉) の旋回処理
        if (kind == img_enemyShotSmallBall[0] || kind == img_enemyShotSmallBall[2] || kind == img_enemyShotSmallBall[4]) {
            double spd = fabs(shot->speed);
            int age = shot->count;   // メインループが事前に ++ している

            // 旋回期間 (11～50フレーム)
            if (age >= ROTATE_START && age <= ROTATE_END) {
                if (shot->speed > 0) shot->muki += ROTATE_AMOUNT;
                else                 shot->muki -= ROTATE_AMOUNT;
            }

            shot->x += spd * cos(shot->muki);
            shot->y += spd * sin(shot->muki);
        }
        // ◆ 自機狙い弾塊 (中玉) の一時停止 → 再発進
        else if (kind == img_enemyShotMediumBall[4]) {
            int age = shot->count;
            if (age < CLUSTER_STOP_TIME) {
                shot->speed *= 0.9;   // 減速
            }
            else if (age == CLUSTER_STOP_TIME) {
                // 現在の自機方向に再ロックオン
                double aim = atan2(player.y - shot->y, player.x - shot->x);
                shot->muki = aim;
                shot->speed = (GetRand(150) + 200) / 100.0; // 2.0～3.5
                clusterReAimed = true;
            }
            // それ以降は直進
            shot->x += shot->speed * cos(shot->muki);
            shot->y += shot->speed * sin(shot->muki);
        }
        // 他の種類（万が一）も移動
        else {
            shot->x += shot->speed * cos(shot->muki);
            shot->y += shot->speed * sin(shot->muki);
        }

        shot = shot->next;
    }

    // 再ロックオン時に警告音 (1フレーム1回)
    if (clusterReAimed) {
        playClusterSE();
    }

    // ---- 1セット (380フレーム) 完了 → ループ ----
    if (t >= 380) {
        pSet->count = -1;   // 次フレームでメインループが 0 に
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Tmp()
{
    static sEnemyShotSet* pMainSet = nullptr;

    if (count == 1) {
        // ---- 初期化 ----
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 600;

        // 弾幕セット生成
        pMainSet = new sEnemyShotSet;
        pMainSet->count = 0;
        pMainSet->patternFunc = DemarcationPattern;
        pMainSet->x = enemy.x;
        pMainSet->y = enemy.y + 20.0;
        pMainSet->muki = 0.0;
        pMainSet->kind = 0;

        pMainSet->pEnemyShotHead = new sEnemyShot;
        pMainSet->pEnemyShotHead->prev = pMainSet->pEnemyShotHead;
        pMainSet->pEnemyShotHead->next = pMainSet->pEnemyShotHead;

        // グローバルリストに接続
        pMainSet->prev = enemyShotSetHead.prev;
        pMainSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMainSet;
        enemyShotSetHead.prev = pMainSet;
    }
    else {
        // ---- 敵本体の移動 (140～240フレームのみ) ----
        int t = pMainSet->count;
        if (t >= 140 && t <= 240) {
            double moveSpeed = 0.7;
            // 自機の方向へ水平移動
            if (player.x < enemy.x)
                enemy.x -= moveSpeed;
            else
                enemy.x += moveSpeed;

            // 画面端に寄りすぎない
            if (enemy.x < 60.0)  enemy.x = 60.0;
            if (enemy.x > 420.0) enemy.x = 420.0;
        }
    }
}