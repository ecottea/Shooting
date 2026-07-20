// enemyPat_Tmp.cpp
// ライフゲーム・グライダーガン弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 定数
// ------------------------------------------------------------
static const int GRID_W = 46;          // グリッド横幅
static const int GRID_H = 46;          // グリッド縦幅
static const double CELL_SIZE = 480.0 / GRID_W; // セルサイズ (15.0)
static const int GEN_INTERVAL = 18;         // 世代更新間隔 (0.6秒@60fps)

// ゴスパー・グライダーガンの初期パターン (dx,dy) オフセット
// パターンの左上を基準とする
static const int GUN_PATTERN[][2] = {
    {24,0}, {22,1}, {24,1}, {12,2}, {13,2}, {20,2}, {21,2}, {34,2}, {35,2},
    {11,3}, {15,3}, {20,3}, {21,3}, {34,3}, {35,3}, { 0,4}, { 1,4}, {10,4},
    {16,4}, {20,4}, {21,4}, { 0,5}, { 1,5}, {10,5}, {14,5}, {16,5}, {17,5},
    {22,5}, {24,5}, {10,6}, {16,6}, {24,6}, {11,7}, {15,7}, {12,8}, {13,8}
};
static const int GUN_PATTERN_SIZE = sizeof(GUN_PATTERN) / sizeof(GUN_PATTERN[0]);
static const int GUN_OFFSET_X = 5;  // 画面左上に配置するためのオフセット
static const int GUN_OFFSET_Y = 5;

// ------------------------------------------------------------
// ライフゲームグリッドの弾幕パターン関数
// ------------------------------------------------------------
static void LifeGridPattern(sEnemyShotSet* pSet)
{
    static int grid[GRID_W][GRID_H] = {};   // 現在のセル状態 (0:死 1:生)
    static int age[GRID_W][GRID_H] = {};   // 連続生存世代数
    
    // 初回初期化（ゴスパーガンの配置）
    if (pSet->count == 0) {
        for (int x = 0; x < GRID_W; ++x) {
            for (int y = 0; y < GRID_H; ++y) {
                grid[x][y] = 0;
                age[x][y] = 0;
            }
        }

        for (int i = 0; i < GUN_PATTERN_SIZE; ++i) {
            int gx = GUN_OFFSET_X + GUN_PATTERN[i][0];
            int gy = GUN_OFFSET_Y + GUN_PATTERN[i][1];
            if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H) {
                grid[gx][gy] = 1;
                age[gx][gy] = 0;
            }
        }

        for (int x = 0; x < GRID_W; ++x) {
            for (int y = 0; y < GRID_H; ++y) {
                if (grid[x][y] == 1) {
                    double cx = (x + 0.5) * CELL_SIZE;
                    double cy = (y + 0.5) * CELL_SIZE;
                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->x = cx;
                    pShot->y = cy;
                    pShot->muki = 0.0;
                    pShot->speed = 0.0;
                    pShot->kind = img_enemyShotSmallBall[6];
                    pShot->prev = pSet->pEnemyShotHead->prev;
                    pShot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pShot;
                    pSet->pEnemyShotHead->prev = pShot;
                }
            }
        }
    }

    // 世代更新（pSet->count はメインルーチンが毎フレームインクリメント）
    if (pSet->count % GEN_INTERVAL == 0 && pSet->count > 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 次世代の状態を計算
        int nextGrid[GRID_W][GRID_H] = {};
        for (int x = 0; x < GRID_W; ++x) {
            for (int y = 0; y < GRID_H; ++y) {
                int neighbors = 0;
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx, ny = y + dy;
                        if (nx >= 0 && nx < GRID_W && ny >= 0 && ny < GRID_H)
                            neighbors += grid[nx][ny];
                    }
                }
                if (grid[x][y] == 1)
                    nextGrid[x][y] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
                else
                    nextGrid[x][y] = (neighbors == 3) ? 1 : 0;
            }
        }

        // 遷移に応じた弾の生成
        for (int x = 0; x < GRID_W; ++x) {
            for (int y = 0; y < GRID_H; ++y) {
                double cx = (x + 0.5) * CELL_SIZE;
                double cy = (y + 0.5) * CELL_SIZE;

                // 誕生：低速自機狙い弾（小玉・青）
                if (nextGrid[x][y] == 1 && grid[x][y] == 0) {
                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->x = cx;
                    pShot->y = cy;
                    pShot->muki = atan2(player.y - cy, player.x - cx);
                    pShot->speed = 2.0;
                    pShot->kind = img_enemyShotSmallBall[4]; // 青 (index 4)
                    pShot->prev = pSet->pEnemyShotHead->prev;
                    pShot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pShot;
                    pSet->pEnemyShotHead->prev = pShot;
                }
                // 死滅：高速自機狙い弾（銃弾・赤）
                else if (nextGrid[x][y] == 0 && grid[x][y] == 1) {
                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->x = cx;
                    pShot->y = cy;
                    pShot->muki = atan2(player.y - cy, player.x - cx);
                    pShot->speed = 4.0;
                    pShot->kind = img_enemyShotBullet[0]; // 赤 (index 0)
                    pShot->prev = pSet->pEnemyShotHead->prev;
                    pShot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pShot;
                    pSet->pEnemyShotHead->prev = pShot;
                }
            }
        }

        // 世代の進行と「老化」による爆発処理
        for (int x = 0; x < GRID_W; ++x) {
            for (int y = 0; y < GRID_H; ++y) {
                if (nextGrid[x][y] == 1) {
                    age[x][y] = (grid[x][y] == 1) ? age[x][y] + 1 : 0;
                    // 10世代生存で8方向低速炸裂弾（鱗弾・マゼンタ）
                    if (age[x][y] >= 10) {
                        double cx = (x + 0.5) * CELL_SIZE;
                        double cy = (y + 0.5) * CELL_SIZE;
                        for (int dir = 0; dir < 8; ++dir) {
                            double angle = dir * DX_PI / 4.0;
                            sEnemyShot* pShot = new sEnemyShot;
                            pShot->x = cx;
                            pShot->y = cy;
                            pShot->muki = angle;
                            pShot->speed = 1.5;
                            pShot->kind = img_enemyShotScale[5]; // マゼンタ (index 5)
                            pShot->prev = pSet->pEnemyShotHead->prev;
                            pShot->next = pSet->pEnemyShotHead;
                            pSet->pEnemyShotHead->prev->next = pShot;
                            pSet->pEnemyShotHead->prev = pShot;
                        }
                    }
                }
                else {
                    age[x][y] = 0;
                }
            }
        }

        // グリッドの更新
        for (int x = 0; x < GRID_W; ++x) {
            for (int y = 0; y < GRID_H; ++y) {
                grid[x][y] = nextGrid[x][y];

                if (grid[x][y] == 1) {
                    double cx = (x + 0.5) * CELL_SIZE;
                    double cy = (y + 0.5) * CELL_SIZE; 
                    sEnemyShot* pShot = new sEnemyShot;
                    pShot->x = cx;
                    pShot->y = cy;
                    pShot->muki = 0.0;
                    pShot->speed = 0.0;
                    pShot->kind = img_enemyShotSmallBall[6];
                    pShot->prev = pSet->pEnemyShotHead->prev;
                    pShot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pShot;
                    pSet->pEnemyShotHead->prev = pShot;
                }
            }
        }
    }

    // このセットが持つ全ての弾を移動（メインルーチンが代行していないため）
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        if (pShot->count >= GEN_INTERVAL && pShot->speed == 0.0) pShot->margin = -9999;

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_LifeGame_DeepSeek()
{
    static int  muki;
    static bool gridSetCreated = false;

    // 初回フレーム
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        gridSetCreated = false;
    }
    else {
        // 水平往復移動
        enemy.x += 0.98 * muki;
        if (count % 120 == 60) muki *= -1;
    }

    // ライフゲームグリッドの弾幕セットを一度だけ生成
    if (!gridSetCreated) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;          // メインルーチンが自動でインクリメント
        pSet->patternFunc = LifeGridPattern;
        pSet->x = 0.0;
        pSet->y = 0.0;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // メインルーチンの二重連結リストに登録
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        gridSetCreated = true;
    }
}