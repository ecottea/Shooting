// enemyPat_invaderFormation.cpp
//
// 「インベーダー・フォーメーション」パターン (難易度調整版)
//
// 構成:
//   Phase1: 弾を7x5のグリッド状に配置し、横移動+端で反転して1段下がる
//           (インベーダーゲームの象徴的な行進)を再現。下降が進むほど
//           後述の斉射が激しくなる。
//   Phase2: 編隊の下降段数(dropCount)に応じて、同時に狙撃する列数・
//           弾速・間隔がスケールする斉射弾を発射(近づくほど激化)
//   Phase3: 画面上部を横切るUFO(別スプライト・別軌道)が、横断中に
//           自機狙いの追加弾(爆弾)を時々投下する
//   Phase4: 一定フレーム経過でグリッド全弾が自機方向へ列・行ごとに
//           時間差で加速降下する総攻撃に移行。降下中も一定間隔で
//           軌道を弱く再照準し、事前の位置取りだけでは避けきれない
//           ようにしている
//
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 調整用パラメータ
// ------------------------------------------------------------
static const int    INV_COLS = 9;      // グリッドの列数 (斉射の同時列数の上限も兼ねる)
static const int    INV_ROWS = 5;      // グリッドの段数
static const double INV_DX = 34.0;     // 列間隔
static const double INV_DY = 26.0;     // 段間隔
static const double DROP_STEP = 18.0;  // 端反転1回あたりの下降量

static const int    MARCH_PERIOD = 80;  // 横移動の反転周期(フレーム)
static const double MARCH_SPEED = 1.2;  // 横移動速度

static const int    DIVE_TRIGGER = 620;    // 編隊生成からこのフレーム数でダイブ(総攻撃)開始
static const double DIVE_ACCEL = 0.06;     // ダイブ中の加速度
static const double DIVE_MAX_SPEED = 4.2;  // ダイブ中の最大速度
static const int    RETARGET_INTERVAL = 45; // ダイブ中、この間隔ごとに軌道を再照準(弱ホーミング)

static const int    VOLLEY_BASE_INTERVAL = 55; // 斉射の基本間隔(フレーム)
static const int    VOLLEY_MIN_INTERVAL = 10;  // 斉射間隔の下限
static const int    VOLLEY_INTERVAL_PER_DROP = 7; // 下降段数1につき間隔をこれだけ短縮
static const double SNIPER_BASE_SPEED = 3.6;   // 斉射弾の基本速度
static const double SNIPER_MAX_SPEED = 5.5;    // 斉射弾の最高速度
static const double SNIPER_SPEED_PER_DROP = 0.15; // 下降段数1につき速度をこれだけ上昇

static const int    UFO_FIRST_WAIT = 150;  // 最初のUFO出現までの待機
static const int    UFO_INTERVAL = 240;    // UFOの出現間隔
static const double UFO_SPEED = 4.5;       // UFOの速度
static const double UFO_Y = 45.0;          // UFOの高さ
static const int    UFO_DROP_INTERVAL = 4; // UFOが横断中に爆弾を投下する間隔(フレーム)
static const double UFO_DROP_SPEED = 5.0;   // UFOの投下弾速度

// 段ごとの色(赤=狙撃弾、マゼンタ=UFOに温存するため編隊では使わない)
static const int GRID_COLOR_BY_ROW[5] = { 2, 3, 1, 6, 4 }; // 緑,シアン,黄,白,青

// ------------------------------------------------------------
// Phase1/4: 編隊(グリッド)本体の弾幕
//   1つの sEnemyShotSet が INV_ROWS x INV_COLS 個の弾を管理する。
//   非ダイブ中は行row・列colから直接座標を計算し、
//   ダイブ中は各弾自身の muki/speed で自機方向へ加速しながら飛ぶ
//   (一定間隔で軌道を再照準する弱ホーミング付き)。
// ------------------------------------------------------------
static void ShotFormation(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回: グリッド状に弾(インベーダー本体)を配置
    if (pEnemyShotSet->count == 0) {
        for (int row = 0; row < INV_ROWS; row++) {
            for (int col = 0; col < INV_COLS; col++) {
                pEnemyShot = new sEnemyShot;

                pEnemyShot->param_i[0] = row; // 行番号を記憶
                pEnemyShot->param_i[1] = col; // 列番号を記憶
                pEnemyShot->param_i[2] = 0;   // ダイブ開始までの遅延フレーム(ダイブ突入時に設定)
                pEnemyShot->x = pEnemyShotSet->x + col * INV_DX;
                pEnemyShot->y = pEnemyShotSet->y + row * INV_DY;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;

                // 弾の種類一覧: 小玉、中玉、大玉、銃弾、鱗弾、菱形弾、中楕円弾、レーザー
                // 弾の色一覧:   0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
                // インベーダー本体は針状の銃弾(img_enemyShotBullet)を使用し、段ごとに色を変えて視認性を確保
                pEnemyShot->kind = img_enemyShotBullet[GRID_COLOR_BY_ROW[row]];
                pEnemyShot->margin = 999;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        pEnemyShotSet->param_d[0] = 0.0;  // 横方向の移動オフセット
        pEnemyShotSet->param_d[1] = 1.0;  // 移動方向 (+1.0 or -1.0)
        pEnemyShotSet->param_i[0] = 0;    // 段下がり回数
        pEnemyShotSet->param_i[1] = 0;    // 0:編隊移動中 1:ダイブ(総攻撃)中
    }

    if (pEnemyShotSet->param_i[1] == 0) {
        // --- Phase1: 横移動 + 端で反転して1段下がる ---
        if (pEnemyShotSet->count % MARCH_PERIOD == MARCH_PERIOD / 2) {
            pEnemyShotSet->param_d[1] *= -1.0;
            pEnemyShotSet->param_i[0]++;
        }
        pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[1] * MARCH_SPEED;

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            pEnemyShot->x = pEnemyShotSet->x + pEnemyShot->param_i[1] * INV_DX + pEnemyShotSet->param_d[0];
            pEnemyShot->y = pEnemyShotSet->y + pEnemyShot->param_i[0] * INV_DY + pEnemyShotSet->param_i[0] * DROP_STEP;
            pEnemyShot = pEnemyShot->next;
        }

        // --- Phase4突入トリガー: 一定フレーム経過でダイブ(総攻撃)開始 ---
        if (pEnemyShotSet->count == DIVE_TRIGGER) {
            pEnemyShotSet->param_i[1] = 1;
            pEnemyShotSet->param_d[2] = (double)pEnemyShotSet->count; // ダイブ開始フレームを記録

            pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
                pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
                pEnemyShot->speed = 0.0;
                // 列・行に応じて発進タイミングをずらし、順番に降下させる
                pEnemyShot->param_i[2] = pEnemyShot->param_i[1] * 5 + pEnemyShot->param_i[0] * 2;
                pEnemyShot = pEnemyShot->next;
            }
        }
    }
    else {
        // --- Phase4: 総攻撃(ダイブ)。一定間隔で軌道を再照準し、単純な事前避けを許さない ---
        double elapsed = pEnemyShotSet->count - pEnemyShotSet->param_d[2];

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (elapsed >= pEnemyShot->param_i[2]) {
                int sinceStart = (int)elapsed - pEnemyShot->param_i[2];
                if (sinceStart % RETARGET_INTERVAL == 0) {
                    pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
                }
                if (pEnemyShot->speed < DIVE_MAX_SPEED) pEnemyShot->speed += DIVE_ACCEL;
                pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
                pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            }
            pEnemyShot = pEnemyShot->next;
        }
    }
}

// ------------------------------------------------------------
// Phase2: 狙撃弾(単発・自機方向への直進)
//   速度は呼び出し側が pEnemyShotSet->param_d[0] に設定した値を使う。
//   複数列同時斉射は、これを呼び出し側で複数回(列数分)生成することで実現する。
// ------------------------------------------------------------
static void ShotSniper(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = (pEnemyShotSet->param_d[0] > 0.0) ? pEnemyShotSet->param_d[0] : SNIPER_BASE_SPEED;
        // グリッド本体とは形・色を変えて視認性を確保: 菱形弾・赤
        pEnemyShot->kind = img_enemyShotDiamond[0];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ------------------------------------------------------------
// Phase3: UFO(画面を横切る専用弾)。横断中、時々自機狙いの爆弾を投下する。
// ------------------------------------------------------------
static void ShotUFO(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = UFO_SPEED;
        // グリッド・狙撃弾のどちらとも一線を画す専用スプライト: 中楕円弾・マゼンタ
        pEnemyShot->kind = img_enemyShotMediumOval[5];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // UFO本体(先頭の1発)が一定間隔で自機狙いの爆弾を投下する
        if (pEnemyShot == pEnemyShotSet->pEnemyShotHead->next &&
            pEnemyShot->count > 0 && pEnemyShot->count % UFO_DROP_INTERVAL == 0) {
            sEnemyShotSet* pDropSet = new sEnemyShotSet;
            pDropSet->count = 0;
            pDropSet->patternFunc = ShotSniper;
            pDropSet->x = pEnemyShot->x;
            pDropSet->y = pEnemyShot->y;
            pDropSet->muki = DX_PI / 2;
            pDropSet->param_d[0] = UFO_DROP_SPEED;
            pDropSet->kind = 0;

            pDropSet->pEnemyShotHead = new sEnemyShot;
            pDropSet->pEnemyShotHead->prev = pDropSet->pEnemyShotHead;
            pDropSet->pEnemyShotHead->next = pDropSet->pEnemyShotHead;

            pDropSet->prev = enemyShotSetHead.prev;
            pDropSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pDropSet;
            enemyShotSetHead.prev = pDropSet;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Invader_Claude()
{
    static sEnemyShotSet* pFormationSet;
    static int nextVolleyCount;
    static int nextUFOCount;
    static int ufoDir;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 30.0;
        enemy.maxHp = enemy.hp = 200;

        // 編隊(インベーダー本体)を1つだけ生成
        pFormationSet = new sEnemyShotSet;
        pFormationSet->count = 0;
        pFormationSet->patternFunc = ShotFormation;
        pFormationSet->x = enemy.x - (INV_COLS - 1) * INV_DX / 2.0; // グリッド中央がenemy.xに来るよう左端を計算
        pFormationSet->y = 70.0;
        pFormationSet->muki = 0.0;
        pFormationSet->kind = 0;

        pFormationSet->pEnemyShotHead = new sEnemyShot;
        pFormationSet->pEnemyShotHead->prev = pFormationSet->pEnemyShotHead;
        pFormationSet->pEnemyShotHead->next = pFormationSet->pEnemyShotHead;

        pFormationSet->prev = enemyShotSetHead.prev;
        pFormationSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pFormationSet;
        enemyShotSetHead.prev = pFormationSet;

        nextVolleyCount = VOLLEY_BASE_INTERVAL;
        nextUFOCount = UFO_FIRST_WAIT;
        ufoDir = 1;
    }

    // 敵本体(母艦)はグリッドの少し上をゆっくり左右に漂う
    enemy.x = 240.0 + 40.0 * sin(count / 90.0);

    // --- Phase2: 複数列同時斉射(ダイブ開始後は発射しない) ---
    // 編隊の下降段数(dropCount)が進むほど、同時発射列数・弾速が増し、間隔も短くなる。
    if (count == nextVolleyCount && pFormationSet->param_i[1] == 0) {
        int dropCount = pFormationSet->param_i[0];

        int volleyCols = 1 + dropCount / 2;
        if (volleyCols > INV_COLS) volleyCols = INV_COLS;

        double speed = SNIPER_BASE_SPEED + dropCount * SNIPER_SPEED_PER_DROP;
        if (speed > SNIPER_MAX_SPEED) speed = SNIPER_MAX_SPEED;

        for (int i = 0; i < volleyCols; i++) {
            int col = GetRand(INV_COLS - 1);
            double sx = pFormationSet->x + pFormationSet->param_d[0] + col * INV_DX;
            double sy = pFormationSet->y + (INV_ROWS - 1) * INV_DY + dropCount * DROP_STEP;

            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotSniper;
            pSet->x = sx;
            pSet->y = sy;
            pSet->muki = atan2(player.y - sy, player.x - sx);
            pSet->param_d[0] = speed;
            pSet->kind = 0;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        int interval = VOLLEY_BASE_INTERVAL - dropCount * VOLLEY_INTERVAL_PER_DROP;
        if (interval < VOLLEY_MIN_INTERVAL) interval = VOLLEY_MIN_INTERVAL;
        nextVolleyCount = count + interval;
    }

    // --- Phase3: UFO横断(横断中に自機狙いの爆弾を投下) ---
    if (count == nextUFOCount) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotUFO;
        pSet->y = UFO_Y;
        if (ufoDir > 0) {
            pSet->x = -20.0;
            pSet->muki = 0.0;
        }
        else {
            pSet->x = 500.0;
            pSet->muki = DX_PI;
        }
        ufoDir *= -1;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        nextUFOCount = count + UFO_INTERVAL;
    }

    // Phase4(総攻撃/ダイブ)への突入と挙動は ShotFormation 内部で処理する
}