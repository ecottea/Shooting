// enemyPat_lifeGame.cpp
//
// 「生々流転」 -- ライフゲーム(コンウェイのセル・オートマトン)モチーフの弾幕
//
// コンセプト:
//   画面上に仮想グリッド(10x8マス)を配置し、コンウェイのライフゲームのルールで
//   世代交代させる。生存セルがそのまま「壁」の待機弾となり、
//   誕生時には自機狙い弾(産声)、死亡時には飛散弾(爆散)を発生させる。
//
//   誕生: 死んでいるセルの周囲8近傍に生存セルがちょうど3つあれば次世代で誕生
//   生存: 生きているセルの周囲に生存セルが2つか3つあれば生存継続
//   死亡: それ以外(過疎・過密)は死亡
//
// 実装方針:
//   ・パターン開始時(count==0)に全世代分をあらかじめシミュレートしてテーブル化
//     -> 実行時は乱数を一切使わず、pShot->count とテーブル参照だけで
//        全弾の位置を毎フレーム式から再計算する(状態蓄積なし・リプレイ安全)。
//   ・生存セルは「誕生からの経過フレームがholdFrames未満なら静止、
//     超えたらグリッド中心から外向きへ直線移動」という式のみで表現するため、
//     deleteは一切呼ばず、画面外に出た時点でメインルーチンが自動的に破棄する。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 定数
// ============================================================
static const int    GRID_COLS = 10;   // グリッド横マス数
static const int    GRID_ROWS = 8;    // グリッド縦マス数
static const int    GEN_COUNT = 60;   // 事前計算する世代数
static const int    FRAMES_PER_GEN = 50;   // 1世代あたりのフレーム数
static const double CELL_SIZE = 40.0; // 1マスのサイズ(px)
static const double GRID_ORIGIN_X = 40.0; // グリッド左上のx座標
static const double GRID_ORIGIN_Y = 70.0; // グリッド左上のy座標(ゲーム画面は480x480)

// 弾の役割(param_i[0]に格納する)
enum ShotRole {
    ROLE_CELL = 0, // 生存セル本体(留まった後、寿命が尽きると飛散して消える)
    ROLE_BIRTH = 1, // 誕生時の自機狙い弾(産声)
    ROLE_BURST = 2, // 死亡時の飛散弾(爆散の破片)
};

// セルの中心座標を求める
static double CellCenterX(int col) { return GRID_ORIGIN_X + CELL_SIZE * col + CELL_SIZE * 0.5; }
static double CellCenterY(int row) { return GRID_ORIGIN_Y + CELL_SIZE * row + CELL_SIZE * 0.5; }

// ------------------------------------------------------------
// 世代テーブル(事前計算・リプレイ安全のため乱数は使わない)
// ------------------------------------------------------------
static bool s_life[GEN_COUNT][GRID_ROWS][GRID_COLS];        // 各世代の生死
static bool s_isBirth[GEN_COUNT][GRID_ROWS][GRID_COLS];      // その世代で誕生したか
static bool s_isDeath[GEN_COUNT + 1][GRID_ROWS][GRID_COLS];  // その世代で死亡したか(GEN_COUNTは終端強制飛散用)
static int  s_holdFrames[GEN_COUNT][GRID_ROWS][GRID_COLS];   // 誕生時から飛散開始までのフレーム数
static bool s_tableReady = false;

// 初期パターン(グライダー・Rペントミノ・ブロック・ブリンカーを配置)
static void SetupInitialPattern(bool grid[GRID_ROWS][GRID_COLS])
{
    for (int r = 0; r < GRID_ROWS; r++)
        for (int c = 0; c < GRID_COLS; c++)
            grid[r][c] = false;

    // グライダー(斜めに移動しながら盤面を横断する)
    //grid[0][2] = true;
    //grid[1][3] = true;
    //grid[2][1] = true;
    //grid[2][2] = true;
    //grid[2][3] = true;

    // Rペントミノ(中央付近に配置し、カオスな変化の起点にする)
    grid[3][6] = true;
    grid[3][7] = true;
    grid[4][5] = true;
    grid[4][6] = true;
    grid[5][6] = true;

    // ブロック(安定した静物。右下に固定の壁として残り続ける)
    //grid[6][8] = true;
    //grid[6][9] = true;
    //grid[7][8] = true;
    //grid[7][9] = true;

    // ブリンカー(周期2の振動子。左下でリズムを刻む)
    //grid[6][0] = true;
    //grid[6][1] = true;
    //grid[6][2] = true;
}

// 周囲8近傍の生存数を数える(盤面の外はラップせず死とみなす)
static int CountNeighbors(const bool grid[GRID_ROWS][GRID_COLS], int row, int col)
{
    int n = 0;
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int rr = row + dr;
            int cc = col + dc;
            if (rr < 0 || rr >= GRID_ROWS || cc < 0 || cc >= GRID_COLS) continue;
            if (grid[rr][cc]) n++;
        }
    }
    return n;
}

// コンウェイのルールで次世代を求める
static void NextGeneration(const bool cur[GRID_ROWS][GRID_COLS], bool next[GRID_ROWS][GRID_COLS])
{
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            int n = CountNeighbors(cur, r, c);
            if (cur[r][c]) next[r][c] = (n == 2 || n == 3); // 生存: 2 or 3
            else           next[r][c] = (n == 3);           // 誕生: ちょうど3
        }
    }
}

// 世代テーブル・誕生死亡テーブルを構築する(パターン開始時に1度だけ実行)
static void BuildLifeTable()
{
    SetupInitialPattern(s_life[0]);
    for (int g = 1; g < GEN_COUNT; g++) {
        NextGeneration(s_life[g - 1], s_life[g]);
    }

    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            for (int g = 0; g <= GEN_COUNT; g++) s_isDeath[g][r][c] = false;

            for (int g = 0; g < GEN_COUNT; g++) {
                bool prevAlive = (g == 0) ? false : s_life[g - 1][r][c];
                bool nowAlive = s_life[g][r][c];
                s_isBirth[g][r][c] = (!prevAlive && nowAlive);
                if (prevAlive && !nowAlive) s_isDeath[g][r][c] = true;
            }
            // 最終世代でまだ生きているセルは、テーブル終端で強制的に飛散させて綺麗に終わらせる
            if (s_life[GEN_COUNT - 1][r][c]) s_isDeath[GEN_COUNT][r][c] = true;
        }
    }

    // 誕生時点から見た「生存し続けるフレーム数」(=飛散が始まるまでの猶予)を求める
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            for (int g = 0; g < GEN_COUNT; g++) {
                if (!s_isBirth[g][r][c]) continue;
                int deathGen = GEN_COUNT; // 見つからない場合の保険(理論上は必ず見つかる)
                for (int g2 = g + 1; g2 <= GEN_COUNT; g2++) {
                    if (s_isDeath[g2][r][c]) { deathGen = g2; break; }
                }
                s_holdFrames[g][r][c] = (deathGen - g) * FRAMES_PER_GEN;
            }
        }
    }

    s_tableReady = true;
}

// ------------------------------------------------------------
// 弾リンクリストへ1個追加する共通処理
// ------------------------------------------------------------
static void AppendShot(sEnemyShotSet* pSet, sEnemyShot* pShot)
{
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 生存セル本体を1個生成する(誕生時に呼ばれる)
static void SpawnCellShot(sEnemyShotSet* pSet, int row, int col, int holdFrames)
{
    // 弾の色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
    sEnemyShot* pShot = new sEnemyShot;
    double cx = CellCenterX(col);
    double cy = CellCenterY(row);
    pShot->x = cx;
    pShot->y = cy;
    pShot->kind = img_enemyShotMediumBall[2]; // 緑の菱形弾＝生きているセル
    pShot->param_i[0] = ROLE_CELL;
    pShot->param_i[1] = holdFrames;
    pShot->param_d[0] = cx;
    pShot->param_d[1] = cy;
    // 飛散方向はグリッド中心から見た外向きに固定(乱数不使用でリプレイ安全)
    double gridCenterX = GRID_ORIGIN_X + CELL_SIZE * GRID_COLS * 0.5;
    double gridCenterY = GRID_ORIGIN_Y + CELL_SIZE * GRID_ROWS * 0.5;
    pShot->param_d[2] = atan2(cy - gridCenterY, cx - gridCenterX);
    pShot->param_d[3] = 1.6; // 飛散速度
    AppendShot(pSet, pShot);
}

// 誕生時の自機狙い弾(産声)を1個生成する
static void SpawnBirthShot(sEnemyShotSet* pSet, int row, int col)
{
    // 弾の種類一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、
    //              鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0/hitbox過大のため不使用)
    sEnemyShot* pShot = new sEnemyShot;
    double startX = CellCenterX(col);
    double startY = CellCenterY(row);
    pShot->x = startX;
    pShot->y = startY;
    pShot->kind = img_enemyShotSmallBall[6]; // 白い小玉＝産声
    pShot->param_i[0] = ROLE_BIRTH;
    pShot->param_d[0] = startX;
    pShot->param_d[1] = startY;
    pShot->param_d[2] = atan2(player.y - startY, player.x - startX);
    pShot->param_d[3] = 2.2; // 弾速
    AppendShot(pSet, pShot);
}

// 死亡時の飛散弾(爆散の破片)を複数生成する
static void SpawnDeathBurst(sEnemyShotSet* pSet, int row, int col)
{
    static const int FRAGMENTS = 6;
    double cx = CellCenterX(col);
    double cy = CellCenterY(row);
    for (int i = 0; i < FRAGMENTS; i++) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = cx;
        pShot->y = cy;
        pShot->kind = img_enemyShotSmallBall[8]; // 橙の小玉＝過疎/過密による爆散
        pShot->param_i[0] = ROLE_BURST;
        pShot->param_d[0] = cx;
        pShot->param_d[1] = cy;
        pShot->param_d[2] = (2.0 * DX_PI / FRAGMENTS) * i; // 均等に6方向へ飛散
        pShot->param_d[3] = 1.3; // 飛散速度
        AppendShot(pSet, pShot);
    }
}

// 毎フレーム: セットが抱える弾すべての座標をcountの式から再計算する(速度積分は行わない)
static void UpdateShotPositions(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case ROLE_CELL: {
            int holdFrames = pShot->param_i[1];
            if (pShot->count < holdFrames) {
                // 寿命が尽きるまでは静止(=待機弾として壁になる)
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];
            }
            else {
                int t = pShot->count - holdFrames;
                pShot->x = pShot->param_d[0] + pShot->param_d[3] * t * cos(pShot->param_d[2]);
                pShot->y = pShot->param_d[1] + pShot->param_d[3] * t * sin(pShot->param_d[2]);
            }
            break;
        }
        case ROLE_BIRTH:
        case ROLE_BURST: {
            int t = pShot->count;
            pShot->x = pShot->param_d[0] + pShot->param_d[3] * t * cos(pShot->param_d[2]);
            pShot->y = pShot->param_d[1] + pShot->param_d[3] * t * sin(pShot->param_d[2]);
            break;
        }
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 弾幕パターン本体(1つのShotSetがグリッド全体を管理する)
// ------------------------------------------------------------
static void PatternLifeGame(sEnemyShotSet* pSet)
{
    if (pSet->count == 0 && !s_tableReady) {
        BuildLifeTable();
    }

    // 世代の境界(FRAMES_PER_GENごと)で誕生・死亡イベントを処理する
    if (pSet->count % FRAMES_PER_GEN == 0) {
        int g = pSet->count / FRAMES_PER_GEN;
        if (g <= GEN_COUNT) {
            bool anyBirth = false;
            bool anyDeath = false;

            for (int r = 0; r < GRID_ROWS; r++) {
                for (int c = 0; c < GRID_COLS; c++) {
                    if (g < GEN_COUNT && s_isBirth[g][r][c]) {
                        SpawnCellShot(pSet, r, c, s_holdFrames[g][r][c]);
                        // 初期配置(g==0)は「世界の始まり」として静かに表示するだけにし、
                        // 産声弾はg>=1、つまり実際に生まれたセルのみ発射する
                        if (g > 0) {
                            SpawnBirthShot(pSet, r, c);
                            anyBirth = true;
                        }
                    }
                    if (s_isDeath[g][r][c]) {
                        SpawnDeathBurst(pSet, r, c);
                        anyDeath = true;
                    }
                }
            }

            // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium,
            //                   sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
            if (anyBirth) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }
            if (anyDeath) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
        }
    }

    UpdateShotPositions(pSet);
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_LifeGame_Claude()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 35.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        s_tableReady = false; // このパターン開始時に世代テーブルを作り直す(乱数不使用のため毎回同じ結果)

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = PatternLifeGame;
        pEnemyShotSet->x = GRID_ORIGIN_X + CELL_SIZE * GRID_COLS * 0.5;
        pEnemyShotSet->y = GRID_ORIGIN_Y + CELL_SIZE * GRID_ROWS * 0.5;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // グリッドの真上でゆったり左右に往復し、盤面を見守るような動き
        enemy.x += 0.4 * (double)muki;
        if (count % 200 == 100) muki *= -1;
    }
}