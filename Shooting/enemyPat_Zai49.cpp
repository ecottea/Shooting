// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <string.h>

// ============================================================
// ライフゲーム弾幕の設定値
// ============================================================
#define GRID_W 20           // グリッドの横マス数
#define GRID_H 20           // グリッドの縦マス数
#define CELL_SIZE 20.0      // 1マスのサイズ(ピクセル)
#define BASE_X 40.0         // グリッドの左端X座標 (480 - 20*20 = 80を均等割り)
#define BASE_Y 65.0         // グリッドの上端Y座標
#define GEN_INTERVAL 5     // 世代交代の間隔(フレーム数)

// ライフゲームの状態を保持する静的配列
static int grid[GRID_W][GRID_H];
static int nextGrid[GRID_W][GRID_H];

// ============================================================
// 弾幕パターン：細胞律動弾幕 (ライフゲーム)
// ============================================================
static void ShotLifeGame(sEnemyShotSet* pEnemyShotSet)
{
    // --- 初期化処理 (Setの生成直後のみ) ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // グリッドをすべてクリア
        memset(grid, 0, sizeof(grid));
        memset(nextGrid, 0, sizeof(nextGrid));

        // 初期パターンの配置 (R-ペントミノを中心に)
        int cx = GRID_W / 2;
        int cy = GRID_H / 2;
        grid[cx][cy - 1] = 1; grid[cx + 1][cy - 1] = 1;
        grid[cx - 1][cy] = 1; grid[cx][cy] = 1;
        grid[cx][cy + 1] = 1;

        // 動きのあるパターンを追加 (グライダー x 2)
        grid[2][2] = 1; grid[3][2] = 1; grid[4][2] = 1;
        grid[4][1] = 1; grid[3][0] = 1;

        grid[15][17] = 1; grid[16][17] = 1; grid[17][17] = 1;
        grid[15][16] = 1; grid[16][18] = 1;

        // 初期状態の弾丸を生成
        for (int y = 0; y < GRID_H; y++) {
            for (int x = 0; x < GRID_W; x++) {
                if (grid[x][y] == 1) {
                    sEnemyShot* pShot = new sEnemyShot;
                    // グリッド座標から実座標へ変換
                    pShot->x = BASE_X + x * CELL_SIZE + CELL_SIZE / 2.0;
                    pShot->y = BASE_Y + y * CELL_SIZE + CELL_SIZE / 2.0;
                    pShot->speed = 0.0; // グリッド上は静止
                    pShot->muki = 0.0;
                    // サイバーテイストで統一：シアン色(3)の中玉(7.0x7.0)
                    pShot->kind = img_enemyShotMediumBall[3];
                    // どのマスに属するかを記録
                    pShot->param_i[0] = x;
                    pShot->param_i[1] = y;

                    // 双方向リストに追加
                    pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pShot;
                }
            }
        }
        return; // 初期化フレームはここで終了
    }

    // --- 世代交代処理 ---
    if (pEnemyShotSet->count % GEN_INTERVAL == 0) {

        // 1. 次世代の計算
        memset(nextGrid, 0, sizeof(nextGrid));
        for (int y = 0; y < GRID_H; y++) {
            for (int x = 0; x < GRID_W; x++) {
                int alive = 0;
                // 周囲8マスをカウント (トーラス構造でループさせる)
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = (x + dx + GRID_W) % GRID_W;
                        int ny = (y + dy + GRID_H) % GRID_H;
                        if (grid[nx][ny] == 1) alive++;
                    }
                }
                // ライフゲームのルール適用
                if (grid[x][y] == 1) {
                    if (alive == 2 || alive == 3) nextGrid[x][y] = 1; // 生存
                }
                else {
                    if (alive == 3) nextGrid[x][y] = 1; // 誕生
                }
            }
        }

        // 2. 既存の弾の「死滅」判定と削除
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next; // 消去前に次のポインタを保持
            int gx = pShot->param_i[0];
            int gy = pShot->param_i[1];

            if (nextGrid[gx][gy] == 0) {
                // 過疎・過密により死滅：リストから外して削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
            else {
                // 生存：重複生成を防ぐために印をつける
                nextGrid[gx][gy] = 2;
            }
            pShot = pNext;
        }

        // 3. 新規「誕生」の弾の追加
        for (int y = 0; y < GRID_H; y++) {
            for (int x = 0; x < GRID_W; x++) {
                if (nextGrid[x][y] == 1) { // 印がついていない＝新規誕生
                    sEnemyShot* pNewShot = new sEnemyShot;
                    pNewShot->x = BASE_X + x * CELL_SIZE + CELL_SIZE / 2.0;
                    pNewShot->y = BASE_Y + y * CELL_SIZE + CELL_SIZE / 2.0;
                    pNewShot->speed = 0.0;
                    pNewShot->muki = 0.0;
                    pNewShot->kind = img_enemyShotMediumBall[3];
                    pNewShot->param_i[0] = x;
                    pNewShot->param_i[1] = y;

                    // 双方向リストの末尾に追加
                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }
                else if (nextGrid[x][y] == 2) {
                    nextGrid[x][y] = 1;
                }
            }
        }

        // 4. グリッド状態を次世代へ更新
        memcpy(grid, nextGrid, sizeof(grid));
    }

    // ※ speed = 0 なので、弾の移動処理(pShot->x += ...)は不要です。
    // ※ count のインクリメントや画面外消去はメインルーチンが行います。
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_LifeGame_Zai()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // ライフゲーム用のShotSetを1つだけ生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLifeGame;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵はゆっくり左右に動く
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }
}