// EnemyPat_Fan.cpp
// 扇風機をモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --------------------------------------------------
// 扇風機の羽根パターン（回転する直線状の弾）
// --------------------------------------------------
static void FanPattern(sEnemyShotSet* pEnemyShotSet)
{
    const int   BLADE_COUNT = 8;       // 羽根の枚数
    const int   STEPS = 8;       // 1本の羽根あたりの弾数
    const double STEP_DIST = 18.0;    // 弾同士の間隔
    const double BULLET_SPEED = 2.5;     // 弾速
    const double ROTATE_SPEED = 0.07;    // 回転速度[rad/frame]
    const int   SPAWN_INTERVAL = 5;      // 何フレームごとに羽根を出すか

    // 初期化（ショットセット生成直後のみ）
    if (pEnemyShotSet->count == 0) {
        // 軽い効果音を再生（扇風機の回転開始イメージ）
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShotSet->kind = 0;   // 羽根の基準角度
    }

    // 一定間隔で羽根を構成する弾を発生
    if ((pEnemyShotSet->count % SPAWN_INTERVAL) == 0) {
        double baseAngle = pEnemyShotSet->kind / 100000.;

        // 敵の現在位置に追随させる（扇風機の中心）
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        for (int blade = 0; blade < BLADE_COUNT; ++blade) {
            double angle = baseAngle + blade * (2.0 * DX_PI / BLADE_COUNT);
            for (int step = 0; step < STEPS; ++step) {
                double dist = step * STEP_DIST;
                double spawnX = pEnemyShotSet->x + dist * cos(angle);
                double spawnY = pEnemyShotSet->y + dist * sin(angle);

                sEnemyShot* p = new sEnemyShot;
                p->x = spawnX;
                p->y = spawnY;
                p->muki = angle;
                p->speed = BULLET_SPEED;

                // 色は kind をもとに循環させる（小玉・9色）
                int colorIdx = (pEnemyShotSet->kind * 3 + blade * STEPS + step) % 9;
                p->kind = img_enemyShotSmallBall[colorIdx];

                // 双方向リストに追加
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
        // 羽根の角度を進めて回転させる
        pEnemyShotSet->kind = (int)((baseAngle + ROTATE_SPEED) * 100000);
    }

    // 全弾の移動（サンプルに倣いパターン関数内で行う）
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// --------------------------------------------------
// 敵本体パターン
// --------------------------------------------------
void EnemyPat_ElectricFan_DeepSeek()
{
    static int  moveDir;       // 1 or -1
    static int  shotCount;     // ショットセット生成用カウンタ
    static bool initialized = false;

    if (count == 1) {
        // ゲーム画面 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;
        shotCount = 0;
        initialized = false;
    }

    // 移動：ゆっくり左右に振れる
    enemy.x += 0.8 * moveDir;
    if (count % 100 == 0) {
        moveDir *= -1;
    }

    // 扇風機の弾幕は最初の一度だけショットセットを作り、あとはその中で継続的に回転させる
    if (!initialized) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = FanPattern;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;               // 特に使わない
        pSet->kind = shotCount++;       // 色変化の種

        // ダミーヘッドの作成
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルリストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        initialized = true;
    }
}