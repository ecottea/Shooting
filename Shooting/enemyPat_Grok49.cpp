// enemyPat_lifeDanmaku.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// ライフゲームをモチーフにした弾幕パターン
// =============================================

static const int GRID_W = 12;
static const int GRID_H = 10;
static const double GRID_CELL_SIZE = 28.0;   // セル1つあたりの画面上間隔

// 簡易2Dグリッド（静的で1敵につき1つ）
static int lifeGrid[GRID_H][GRID_W] = { 0 };
static int nextGrid[GRID_H][GRID_W] = { 0 };

// 弾の種類・色を固定（必要に応じて変更可）
static int lifeBulletKind = 0;   // 後でランダム化も可能

// ライフゲーム更新関数
static void UpdateLifeGrid() {
    // 次の世代を計算
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            int neighbors = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < GRID_W && ny >= 0 && ny < GRID_H) {
                        neighbors += lifeGrid[ny][nx];
                    }
                }
            }
            if (lifeGrid[y][x] == 1) {
                nextGrid[y][x] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
            }
            else {
                nextGrid[y][x] = (neighbors == 3) ? 1 : 0;
            }
        }
    }
    // コピー
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            lifeGrid[y][x] = nextGrid[y][x];
        }
    }
}

// グリッドの初期パターン（Glider + ブリンカー + ランダム要素）
static void InitLifeGrid() {
    memset(lifeGrid, 0, sizeof(lifeGrid));
    // Glider
    lifeGrid[2][3] = 1;
    lifeGrid[3][4] = 1;
    lifeGrid[4][2] = 1; lifeGrid[4][3] = 1; lifeGrid[4][4] = 1;

    // Blinker
    lifeGrid[2][8] = 1; lifeGrid[3][8] = 1; lifeGrid[4][8] = 1;

    // 軽くランダム追加
    for (int i = 0; i < 8; i++) {
        int rx = GetRand(GRID_W - 1);
        int ry = GetRand(GRID_H - 1);
        if (GetRand(1) == 1) lifeGrid[ry][rx] = 1;
    }
}

// 生きているセル位置に弾を生成・更新する関数
static void ShotLifeGrid(sEnemyShotSet* pEnemyShotSet) {
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 初回のみ効果音
        //if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        //PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 現在のグリッド状態に基づいて弾を配置（毎回再配置ではなく、更新時に新規生成）
    if (pEnemyShotSet->count % 24 == 0) {   // 世代更新間隔（調整可）
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        UpdateLifeGrid();

        // 生きているセルごとに弾を生成
        for (int gy = 0; gy < GRID_H; gy++) {
            for (int gx = 0; gx < GRID_W; gx++) {
                if (lifeGrid[gy][gx] == 0) continue;

                pEnemyShot = new sEnemyShot;
                // グリッド中心からの相対位置
                double baseX = pEnemyShotSet->x - (GRID_W * GRID_CELL_SIZE) * 0.5 + GRID_CELL_SIZE * 0.5;
                double baseY = pEnemyShotSet->y - (GRID_H * GRID_CELL_SIZE) * 0.5 + GRID_CELL_SIZE * 0.5;

                pEnemyShot->x = baseX + gx * GRID_CELL_SIZE + GetRand(8) - 4;
                pEnemyShot->y = baseY + gy * GRID_CELL_SIZE + GetRand(8) - 4;

                // ゆっくり下方向＋軽いランダム移動
                pEnemyShot->muki = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI;
                pEnemyShot->speed = 0.8 + GetRand(60) / 100.0;

                // 弾の見た目（中玉をメインに）
                int rnd = GetRand(3);
                switch (rnd) {
                case 0:
                    pEnemyShot->kind = img_enemyShotSmallBall[4];   // 青系
                    break;
                case 1:
                    pEnemyShot->kind = img_enemyShotDiamond[3];   // シアン
                    break;
                default:
                    pEnemyShot->kind = img_enemyShotSmallBall[5];   // マゼンタ
                    break;
                }

                // リンク
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

                {
                    sEnemyShot* pEnemyShot2 = new sEnemyShot;
                    pEnemyShot2->x = baseX + gx * GRID_CELL_SIZE;
                    pEnemyShot2->y = baseY + gy * GRID_CELL_SIZE;
                    pEnemyShot2->muki = pEnemyShot->muki;
                    pEnemyShot2->speed = 0.0;
                    switch (rnd) {
                    case 0:
                        pEnemyShot2->kind = img_enemyShotMediumBall[4];   // 青系
                        break;
                    case 1:
                        pEnemyShot2->kind = img_enemyShotMediumOval[3];   // シアン
                        break;
                    default:
                        pEnemyShot2->kind = img_enemyShotMediumBall[5];   // マゼンタ
                        break;
                    }
                    pEnemyShot2->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot2->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot2;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot2;
                }
            }
        }
    }

    // 全弾移動（メインループでcount++される）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        // 軽い減速＋重力風
        pShot->speed /= 0.985;
        
        if (pShot->count >= 24 && pShot->speed == 0.0) pShot->margin = -9999;

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_LifeGame_Grok() {
    static int muki = 1;
    static int phase = 0;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;

        InitLifeGrid();
    }
    else {
        // 左右ゆっくり移動
        enemy.x += 1.1 * (double)muki;
        if (count % 140 == 70) muki *= -1;

        // 軽く上下移動
        enemy.y = 60.0 + sin(count / 45.0) * 18.0;
    }

    // ライフグリッド弾幕のShotSetを定期的に生成・更新
    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLifeGrid;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 25.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リンク
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 補助：プレイヤー方向への軽い散弾（ライフ弾幕の合間に）
    if (count % 55 == 20) {
        sEnemyShotSet* pScatter = new sEnemyShotSet;
        pScatter->count = 0;
        pScatter->patternFunc = [](sEnemyShotSet* set) {  // 簡易ラムダで散弾
            if (set->count == 0) {
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                for (int i = 0; i < 7; i++) {
                    sEnemyShot* s = new sEnemyShot;
                    s->x = set->x + GetRand(30) - 15;
                    s->y = set->y;
                    s->muki = atan2(player.y - set->y, player.x - set->x) + (GetRand(80) - 40) / 180.0 * DX_PI;
                    s->speed = 2.8 + GetRand(70) / 100.0;
                    s->kind = img_enemyShotSmallBall[1];
                    s->prev = set->pEnemyShotHead->prev;
                    s->next = set->pEnemyShotHead;
                    set->pEnemyShotHead->prev->next = s;
                    set->pEnemyShotHead->prev = s;
                }
            }
            // 移動
            sEnemyShot* s = set->pEnemyShotHead->next;
            while (s != set->pEnemyShotHead) {
                s->x += s->speed * cos(s->muki);
                s->y += s->speed * sin(s->muki);
                s = s->next;
            }
        };
        pScatter->x = enemy.x;
        pScatter->y = enemy.y + 30.0;
        pScatter->pEnemyShotHead = new sEnemyShot;
        pScatter->pEnemyShotHead->prev = pScatter->pEnemyShotHead;
        pScatter->pEnemyShotHead->next = pScatter->pEnemyShotHead;

        pScatter->prev = enemyShotSetHead.prev;
        pScatter->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pScatter;
        enemyShotSetHead.prev = pScatter;
    }
}