// enemyPat_match.cpp
// マッチをモチーフにした弾幕「擦炎燐舞」
// 敵本体の関数名: EnemyPat_Match_DeepSeek

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//------------------------------------------------
// 火花（擦過）パターン
//------------------------------------------------
static void SparkScatter(sEnemyShotSet* pSet)
{
    // 火花の波を一定間隔で6回放出
    const int waveInterval = 15;
    const int maxWaves = 6;
    if (pSet->count <= (maxWaves - 1) * waveInterval &&
        pSet->count % waveInterval == 0)
    {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        int num = 12;
        double baseAngle = 0.0;                     // 右方向
        double spread = 2.0 * DX_PI / 3.0;          // 120度
        for (int i = 0; i < num; ++i) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = baseAngle + (GetRand(1000) / 1000.0 - 0.5) * spread;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = (300 + GetRand(200)) / 100.0;  // 3.0 ～ 5.0
            pShot->kind = img_enemyShotSmallBall[8];      // 橙 小玉
            pShot->count = 0;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 移動と寿命による消去（30フレーム）
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* next = pShot->next;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        if (pShot->count > 30) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        pShot = next;
    }
}

//------------------------------------------------
// 炎の螺旋パターン（成長三段階）
//------------------------------------------------
static void FlameSpiral(sEnemyShotSet* pSet)
{
    int stage = pSet->param_i[0];
    int& spawnTimer = pSet->param_i[1];
    double& rotation = pSet->param_d[0];

    int interval;
    int numArms;
    double rotSpeed;      // 反時計回り
    double bulletSpeed;

    if (stage == 0) {          // 炎I
        interval = 4;
        numArms = 2;
        rotSpeed = -0.03;
        bulletSpeed = 1.5;
    }
    else if (stage == 1) {   // 炎II
        interval = 3;
        numArms = 4;
        rotSpeed = -0.045;
        bulletSpeed = 2.0;
    }
    else {                   // 炎III
        interval = 2;
        numArms = 6;
        rotSpeed = -0.06;
        bulletSpeed = 2.5;
    }

    // 螺旋弾の生成
    spawnTimer++;
    if (spawnTimer >= interval) {
        spawnTimer = 0;
        double baseAngle = rotation;
        for (int i = 0; i < numArms; ++i) {
            double angle = baseAngle + i * 2.0 * DX_PI / numArms;
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = bulletSpeed;
            pShot->kind = img_enemyShotScale[8];   // 橙 鱗弾（炎のしずく型に見立てる）
            pShot->count = 0;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        rotation += rotSpeed;
    }

    // 炎II：谷間に火の粉（ランダム方向の粒弾）
    if (stage == 1) {
        int& sparkTimer = pSet->param_i[2];
        sparkTimer++;
        if (sparkTimer >= 10) {
            sparkTimer = 0;
            int numSparks = 2 + GetRand(2);      // 2～4個
            for (int i = 0; i < numSparks; ++i) {
                double angle = GetRand(360) * DX_PI / 180.0;
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;
                pShot->muki = angle;
                pShot->speed = 1.0 + GetRand(100) / 100.0;  // 1.0 ～ 2.0
                pShot->kind = img_enemyShotSmallBall[8];
                pShot->count = 0;

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 炎III：炎槍（高速自機狙い）
    if (stage == 2) {
        int& lanceTimer = pSet->param_i[3];
        lanceTimer++;
        if (lanceTimer >= 40) {
            lanceTimer = 0;
            double angle = atan2(player.y - pSet->y, player.x - pSet->x);
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = 5.0;
            pShot->kind = img_enemyShotDiamond[8];   // 橙 菱形弾
            pShot->count = 0;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 全弾の移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//------------------------------------------------
// 燃え尽き爆散パターン
//------------------------------------------------
static void FinalExplosion(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 内層：密・低速
        int innerCount = 60;
        for (int i = 0; i < innerCount; ++i) {
            double angle = i * (2.0 * DX_PI) / innerCount;
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = 1.0 + GetRand(50) / 100.0;
            pShot->kind = img_enemyShotSmallBall[8];
            pShot->count = 0;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // 中層
        int middleCount = 40;
        for (int i = 0; i < middleCount; ++i) {
            double angle = i * (2.0 * DX_PI) / middleCount + GetRand(50) / 100.0;
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = 2.0 + GetRand(100) / 100.0;
            pShot->kind = img_enemyShotMediumBall[8];
            pShot->count = 0;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // 外層：疎・高速
        int outerCount = 20;
        for (int i = 0; i < outerCount; ++i) {
            double angle = i * (2.0 * DX_PI) / outerCount;
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = angle;
            pShot->speed = 3.5 + GetRand(100) / 100.0;
            pShot->kind = img_enemyShotLargeBall[8];
            pShot->count = 0;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 全弾の移動
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//------------------------------------------------
// 敵本体のメインパターン
//------------------------------------------------
void EnemyPat_Match_DeepSeek()
{
    static int phase = 0;
    static int phaseTimer = 0;
    static int startCount = 0;
    static sEnemyShotSet* pFlameSet = nullptr;

    if (count == 1) {
        // 初期化
        phase = 0;
        phaseTimer = 0;
        startCount = count;
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        pFlameSet = nullptr;
    }

    int localTime = count - startCount;

    //---------- フェーズ0：預兆（60フレーム待機） ----------
    if (phase == 0) {
        if (localTime >= 60) {
            phase = 1;
            phaseTimer = 0;

            // 擦過火花用の弾幕セットを生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = SparkScatter;
            pSet->x = enemy.x + 60.0;   // マッチ頭薬（右端）
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    //---------- フェーズ1：擦過火花（90フレーム） ----------
    else if (phase == 1) {
        if (++phaseTimer >= 90) {
            phase = 2;
            phaseTimer = 0;

            // 炎螺旋用の弾幕セットを生成（炎Iでスタート）
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = FlameSpiral;
            pSet->x = enemy.x + 60.0;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->param_i[0] = 0;   // 炎I
            pSet->param_i[1] = 0;
            pSet->param_i[2] = 0;
            pSet->param_i[3] = 0;
            pSet->param_d[0] = 0.0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;

            pFlameSet = pSet;   // ポインタを保持
        }
    }
    //---------- フェーズ2：炎I（200フレーム） ----------
    else if (phase == 2) {
        if (++phaseTimer >= 200) {
            phase = 3;
            phaseTimer = 0;
            if (pFlameSet) {
                pFlameSet->param_i[0] = 1;   // 炎IIへ
                pFlameSet->param_i[1] = 0;
                pFlameSet->param_i[2] = 0;
                pFlameSet->param_i[3] = 0;
                pFlameSet->param_d[0] = 0.0;
            }
        }
    }
    //---------- フェーズ3：炎II（200フレーム） ----------
    else if (phase == 3) {
        if (++phaseTimer >= 200) {
            phase = 4;
            phaseTimer = 0;
            if (pFlameSet) {
                pFlameSet->param_i[0] = 2;   // 炎IIIへ
                pFlameSet->param_i[1] = 0;
                pFlameSet->param_i[2] = 0;
                pFlameSet->param_i[3] = 0;
                pFlameSet->param_d[0] = 0.0;
            }
        }
    }
    //---------- フェーズ4：炎III（200フレーム） ----------
    else if (phase == 4) {
        if (++phaseTimer >= 200) {
            phase = 5;
            phaseTimer = 0;

            // 最終爆散用の弾幕セットを生成
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = FinalExplosion;
            pSet->x = enemy.x + 60.0;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    // フェーズ5以降は何もしない（弾幕終了）
}