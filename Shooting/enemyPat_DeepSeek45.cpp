// enemyPat_DNA_Helix.cpp
// 双螺旋断章 - ヘリックス・フラグメント（修正版）
// DNAをモチーフにした弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const double HELIX_RADIUS = 120.0;
static const double HELIX_START_Y_OFF = 10.0;
static const double HELIX_ROT_SPEED = (5.0 * DX_PI / 180.0); // 15度/フレーム
static const double HELIX_DOWN_SPEED = 2.0;     // 背骨の発生点が下がる速度

static const int    BACKBONE_INTERVAL = 5;       // 背骨弾の発生間隔
static const int    BASEPAIR_INTERVAL = 3;       // 塩基対の発生間隔
static const int    FORMATION_DURATION = 240;     // 形成フェーズの長さ(4秒)
static const int    UNWIND_DELAY = 30;      // 停止時間(0.5秒)

static const int    SCATTER_WAVES = 2;
static const int    SCATTER_DIRS = 16;
static const double SCATTER_SPEED = 2.0;

// ------------------------------------------------------------
// 弾パターン更新関数
// ------------------------------------------------------------

// 背骨（青・赤菱形弾）の更新：Y固定で横回転のみ
static void HelixBackboneUpdate(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int state = pSet->param_i[0];
        if (state == 0) {
            // 螺旋形成・維持：yは生成時位置のまま、xだけ回転
            double t = pShot->count;
            double cx = pShot->param_d[2];   // 中心X（生成時の敵座標）
            double a0 = pShot->param_d[0];   // 初期角度
            double sign = pShot->param_d[1];   // 回転方向

            pShot->x = cx + HELIX_RADIUS * cos(a0 + sign * t * HELIX_ROT_SPEED);
            // y は動かさない（螺旋の柱としてその場に留まる）
        }
        else if (state == 1) {
            // 停止（ほどけ前の停止期間）
        }
        else if (state == 2) {
            // 外向きに爆発
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 塩基対（黄・緑 小玉）の更新：水平移動のみ
static void BasePairUpdate(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        // Y は生成時のまま固定
        pShot = pShot->next;
    }
}

// 散弾（全方位低速菱形弾）の更新
static void ScatterUpdate(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_DNA_DeepSeek()
{
    enum Phase { PHASE_FORMATION, PHASE_UNWIND, PHASE_SCATTER };
    static Phase phase = PHASE_FORMATION;
    static int   phaseTimer = 0;
    static int   enemyMuki = 1;

    // 生成したセットのポインタ（静的保持）
    static sEnemyShotSet* pBackboneSet = nullptr;
    static sEnemyShotSet* pBasePairSet = nullptr;
    static bool scatterCreated = false;   // 散弾セット二重生成防止用

    static double helixY = 0.0;
    static int    backboneSpawnTimer = 0;
    static int    basepairSpawnTimer = 0;

    // 初回フレーム
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        enemyMuki = 1;
        phase = PHASE_FORMATION;
        phaseTimer = 0;
        scatterCreated = false;
        backboneSpawnTimer = 0;
        basepairSpawnTimer = 0;
        helixY = enemy.y + HELIX_START_Y_OFF;

        // 背骨弾セットの作成（ダミーヘッドの座標を画面中央に）
        pBackboneSet = new sEnemyShotSet;
        pBackboneSet->count = 0;
        pBackboneSet->patternFunc = HelixBackboneUpdate;
        pBackboneSet->x = enemy.x;
        pBackboneSet->y = enemy.y;
        pBackboneSet->muki = 0.0;
        pBackboneSet->kind = 0;
        pBackboneSet->param_i[0] = 0;
        pBackboneSet->pEnemyShotHead = new sEnemyShot;
        pBackboneSet->pEnemyShotHead->x = 240.0;   // 安全座標
        pBackboneSet->pEnemyShotHead->y = 240.0;
        pBackboneSet->pEnemyShotHead->prev = pBackboneSet->pEnemyShotHead;
        pBackboneSet->pEnemyShotHead->next = pBackboneSet->pEnemyShotHead;
        pBackboneSet->prev = enemyShotSetHead.prev;
        pBackboneSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBackboneSet;
        enemyShotSetHead.prev = pBackboneSet;

        // 塩基対セット
        pBasePairSet = new sEnemyShotSet;
        pBasePairSet->count = 0;
        pBasePairSet->patternFunc = BasePairUpdate;
        pBasePairSet->x = enemy.x;
        pBasePairSet->y = enemy.y;
        pBasePairSet->muki = 0.0;
        pBasePairSet->kind = 0;
        pBasePairSet->pEnemyShotHead = new sEnemyShot;
        pBasePairSet->pEnemyShotHead->x = 240.0;
        pBasePairSet->pEnemyShotHead->y = 240.0;
        pBasePairSet->pEnemyShotHead->prev = pBasePairSet->pEnemyShotHead;
        pBasePairSet->pEnemyShotHead->next = pBasePairSet->pEnemyShotHead;
        pBasePairSet->prev = enemyShotSetHead.prev;
        pBasePairSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBasePairSet;
        enemyShotSetHead.prev = pBasePairSet;
    }
    else {
        // 敵の左右移動
        enemy.x += 0.98 * (double)enemyMuki;
        if (count % 120 == 60) enemyMuki *= -1;
    }

    // フェーズ別処理
    switch (phase) {
    case PHASE_FORMATION:
    {
        // 背骨弾の追加
        if (backboneSpawnTimer <= 0) {
            backboneSpawnTimer = BACKBONE_INTERVAL;

            // 左の鎖（青、反時計回り）
            sEnemyShot* pBlue = new sEnemyShot;
            pBlue->x = enemy.x - HELIX_RADIUS;
            pBlue->y = helixY;          // 生成時Yを記憶
            pBlue->muki = 0.0;
            pBlue->speed = 0.0;
            pBlue->kind = img_enemyShotDiamond[4];      // 青
            pBlue->param_d[0] = DX_PI;                   // 角度オフセット (左端)
            pBlue->param_d[1] = +1.0;                    // 反時計回り
            pBlue->param_d[2] = enemy.x;                 // 中心X（生成時の敵X）
            pBlue->param_d[3] = helixY;                  // 生成時のY（固定用）
            pBlue->prev = pBackboneSet->pEnemyShotHead->prev;
            pBlue->next = pBackboneSet->pEnemyShotHead;
            pBackboneSet->pEnemyShotHead->prev->next = pBlue;
            pBackboneSet->pEnemyShotHead->prev = pBlue;

            // 右の鎖（赤、時計回り）
            sEnemyShot* pRed = new sEnemyShot;
            pRed->x = enemy.x + HELIX_RADIUS;
            pRed->y = helixY;
            pRed->muki = 0.0;
            pRed->speed = 0.0;
            pRed->kind = img_enemyShotDiamond[0];       // 赤
            pRed->param_d[0] = 0.0;
            pRed->param_d[1] = -1.0;
            pRed->param_d[2] = enemy.x;
            pRed->param_d[3] = helixY;
            pRed->prev = pBackboneSet->pEnemyShotHead->prev;
            pRed->next = pBackboneSet->pEnemyShotHead;
            pBackboneSet->pEnemyShotHead->prev->next = pRed;
            pBackboneSet->pEnemyShotHead->prev = pRed;

            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        else {
            backboneSpawnTimer--;
        }

        // 塩基対の追加
        if (basepairSpawnTimer <= 0) {
            basepairSpawnTimer = BASEPAIR_INTERVAL;

            // 左から黄（右向き）
            sEnemyShot* pYellow = new sEnemyShot;
            pYellow->x = enemy.x - HELIX_RADIUS;
            pYellow->y = helixY;
            pYellow->muki = 0.0;                        // 右へ
            pYellow->speed = 3.0;
            pYellow->kind = img_enemyShotSmallBall[1];  // 黄
            pYellow->margin = 480;
            pYellow->prev = pBasePairSet->pEnemyShotHead->prev;
            pYellow->next = pBasePairSet->pEnemyShotHead;
            pBasePairSet->pEnemyShotHead->prev->next = pYellow;
            pBasePairSet->pEnemyShotHead->prev = pYellow;

            // 右から緑（左向き）
            sEnemyShot* pGreen = new sEnemyShot;
            pGreen->x = enemy.x + HELIX_RADIUS;
            pGreen->y = helixY;
            pGreen->muki = DX_PI;                       // 左へ
            pGreen->speed = 3.0;
            pGreen->kind = img_enemyShotSmallBall[2];   // 緑
            pGreen->margin = 480;
            pGreen->prev = pBasePairSet->pEnemyShotHead->prev;
            pGreen->next = pBasePairSet->pEnemyShotHead;
            pBasePairSet->pEnemyShotHead->prev->next = pGreen;
            pBasePairSet->pEnemyShotHead->prev = pGreen;
        }
        else {
            basepairSpawnTimer--;
        }

        // 発生点を下に進行
        helixY += HELIX_DOWN_SPEED;

        phaseTimer++;
        if (phaseTimer >= FORMATION_DURATION) {
            phaseTimer = 0;
            phase = PHASE_UNWIND;
            pBackboneSet->param_i[0] = 1;   // 背骨停止

            // 塩基対を反転（結合を解くように）
            sEnemyShot* pShot = pBasePairSet->pEnemyShotHead->next;
            while (pShot != pBasePairSet->pEnemyShotHead) {
                pShot->muki = fmod(pShot->muki + DX_PI, DX_PI * 2.0);
                pShot = pShot->next;
            }

            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        break;
    }

    case PHASE_UNWIND:
    {
        phaseTimer++;
        if (phaseTimer == UNWIND_DELAY) {
            // 背骨を爆発状態に
            pBackboneSet->param_i[0] = 2;
            sEnemyShot* pShot = pBackboneSet->pEnemyShotHead->next;
            while (pShot != pBackboneSet->pEnemyShotHead) {
                double cx = pShot->param_d[2];
                pShot->muki = (pShot->x >= cx) ? 0.0 : DX_PI;
                pShot->speed = 5.0;
                pShot = pShot->next;
            }

            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        // 散弾セット生成（1回のみ）
        if (phaseTimer >= 120 && !scatterCreated) {
            scatterCreated = true;

            sEnemyShotSet* pScatterSet = new sEnemyShotSet;
            pScatterSet->count = 0;
            pScatterSet->patternFunc = ScatterUpdate;
            pScatterSet->x = enemy.x;
            pScatterSet->y = enemy.y + 30.0;
            pScatterSet->muki = 0.0;
            pScatterSet->kind = 0;
            pScatterSet->pEnemyShotHead = new sEnemyShot;
            pScatterSet->pEnemyShotHead->x = 240.0;
            pScatterSet->pEnemyShotHead->y = 240.0;
            pScatterSet->pEnemyShotHead->prev = pScatterSet->pEnemyShotHead;
            pScatterSet->pEnemyShotHead->next = pScatterSet->pEnemyShotHead;

            for (int wave = 0; wave < SCATTER_WAVES; wave++) {
                for (int i = 0; i < SCATTER_DIRS; i++) {
                    sEnemyShot* p = new sEnemyShot;
                    p->x = pScatterSet->x;
                    p->y = pScatterSet->y;
                    p->muki = (DX_PI * 2.0 / SCATTER_DIRS) * i;
                    p->speed = SCATTER_SPEED + wave * 0.5;
                    p->kind = img_enemyShotDiamond[5];
                    p->prev = pScatterSet->pEnemyShotHead->prev;
                    p->next = pScatterSet->pEnemyShotHead;
                    pScatterSet->pEnemyShotHead->prev->next = p;
                    pScatterSet->pEnemyShotHead->prev = p;
                }
            }

            pScatterSet->prev = enemyShotSetHead.prev;
            pScatterSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pScatterSet;
            enemyShotSetHead.prev = pScatterSet;

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 十分時間が経ったら次のサイクルへ
        if (phaseTimer >= 180) {   // 3秒ほどで再形成
            phaseTimer = 0;
            phase = PHASE_FORMATION;
            scatterCreated = false;
            backboneSpawnTimer = 0;
            basepairSpawnTimer = 0;
            helixY = enemy.y + HELIX_START_Y_OFF;

            // 新しい背骨・塩基対セットを作成（古いものはメインルーチンが不要になれば削除する想定）
            pBackboneSet = new sEnemyShotSet;
            pBackboneSet->count = 0;
            pBackboneSet->patternFunc = HelixBackboneUpdate;
            pBackboneSet->x = enemy.x;
            pBackboneSet->y = enemy.y;
            pBackboneSet->muki = 0.0;
            pBackboneSet->kind = 0;
            pBackboneSet->param_i[0] = 0;
            pBackboneSet->pEnemyShotHead = new sEnemyShot;
            pBackboneSet->pEnemyShotHead->x = 240.0;
            pBackboneSet->pEnemyShotHead->y = 240.0;
            pBackboneSet->pEnemyShotHead->prev = pBackboneSet->pEnemyShotHead;
            pBackboneSet->pEnemyShotHead->next = pBackboneSet->pEnemyShotHead;
            pBackboneSet->prev = enemyShotSetHead.prev;
            pBackboneSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pBackboneSet;
            enemyShotSetHead.prev = pBackboneSet;

            pBasePairSet = new sEnemyShotSet;
            pBasePairSet->count = 0;
            pBasePairSet->patternFunc = BasePairUpdate;
            pBasePairSet->x = enemy.x;
            pBasePairSet->y = enemy.y;
            pBasePairSet->muki = 0.0;
            pBasePairSet->kind = 0;
            pBasePairSet->pEnemyShotHead = new sEnemyShot;
            pBasePairSet->pEnemyShotHead->x = 240.0;
            pBasePairSet->pEnemyShotHead->y = 240.0;
            pBasePairSet->pEnemyShotHead->prev = pBasePairSet->pEnemyShotHead;
            pBasePairSet->pEnemyShotHead->next = pBasePairSet->pEnemyShotHead;
            pBasePairSet->prev = enemyShotSetHead.prev;
            pBasePairSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pBasePairSet;
            enemyShotSetHead.prev = pBasePairSet;
        }
        break;
    }

    case PHASE_SCATTER:
        // 散弾フェーズは特に何もせず、時間経過で自動的に再形成へ移行する
        // （実際には PHASE_UNWIND の後半で再形成に移行するため、ここには到達しない想定）
        break;
    }
}