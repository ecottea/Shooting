// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --- 定数定義 ---
static const int GRID_SIZE = 15;            // 15x15の仮想グリッド
static const double GRID_OFFSET_X = 70.0;   // グリッド全体の左端座標
static const double GRID_OFFSET_Y = 70.0;   // グリッド全体の上端座標
static const double GRID_CELL_SIZE = 24.0;  // 1マスのピクセルサイズ (15x24 = 360px)
static const int GENERATION_INTERVAL = 40;  // 世代交代の周期（フレーム数。約0.66秒ごと）

// --- ユーティリティ関数 ---
static double GetGridX(int col) {
    return GRID_OFFSET_X + col * GRID_CELL_SIZE + GRID_CELL_SIZE / 2.0;
}

static double GetGridY(int row) {
    return GRID_OFFSET_Y + row * GRID_CELL_SIZE + GRID_CELL_SIZE / 2.0;
}

// 新しい弾をセットに追加するヘルパー関数
static void AddEnemyShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind, int type, int r = 0, int c = 0, int target_r = 0, int target_c = 0) {
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;

    // ライフゲーム制御用の各種パラメータを割り当てる
    pEnemyShot->param_i[0] = r;        // セルの行位置 (0 〜 14)
    pEnemyShot->param_i[1] = c;        // セルの列位置 (0 〜 14)
    pEnemyShot->param_i[2] = type;     // 役割分類 (0:固定セル, 1:消滅狙撃弾, 2:誕生予告弾, 3:種火弾)
    pEnemyShot->param_i[3] = target_r; // 【種火弾専用】目標の行
    pEnemyShot->param_i[4] = target_c; // 【種火弾専用】目標の列

    // リストの末尾に追加
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// 弾幕メインルーチン：ジェネレーション・グリッド
static void ShotGenerationGrid(sEnemyShotSet* pEnemyShotSet)
{
    // 【1. 初期配置】
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        bool initialGrid[GRID_SIZE][GRID_SIZE] = { false };

        // パターンA：ペンタデカスロン (周期15の振動型) を中央に配置
        for (int c = 3; c <= 12; c++) {
            initialGrid[7][c] = true;
        }

        // パターンB：グライダー (右下へ進む) を左上に配置
        initialGrid[1][2] = true;
        initialGrid[2][3] = true;
        initialGrid[3][1] = true;
        initialGrid[3][2] = true;
        initialGrid[3][3] = true;

        // パターンC：逆グライダー (左下へ進む) を右上に配置
        initialGrid[1][12] = true;
        initialGrid[2][11] = true;
        initialGrid[3][11] = true;
        initialGrid[3][12] = true;
        initialGrid[3][13] = true;

        // 実際のゲーム世界へ固定セル弾として生成
        for (int r = 0; r < GRID_SIZE; r++) {
            for (int c = 0; c < GRID_SIZE; c++) {
                if (initialGrid[r][c]) {
                    AddEnemyShot(pEnemyShotSet, GetGridX(c), GetGridY(r), 0.0, 0.0, img_enemyShotMediumBall[2], 0, r, c);
                }
            }
        }
    }

    // 【2. アクティブ・ハック判定】
    sPlayerShot* pPlShot = playerShotHead.next;
    while (pPlShot != &playerShotHead) {
        sPlayerShot* pPlNext = pPlShot->next;
        bool isHit = false;

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;

            if (pShot->param_i[2] == 0 || pShot->param_i[2] == 2) {
                double dx = pShot->x - pPlShot->x;
                double dy = pShot->y - pPlShot->y;
                if (dx * dx + dy * dy < 12.0 * 12.0) {
                    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;

                    isHit = true;
                    break;
                }
            }
            pShot = pNext;
        }

        if (isHit) {
            pPlShot->prev->next = pPlShot->next;
            pPlShot->next->prev = pPlShot->prev;
            delete pPlShot;
        }
        pPlShot = pPlNext;
    }

    // 【3. 誕生予告の展開】
    if (pEnemyShotSet->count % GENERATION_INTERVAL == GENERATION_INTERVAL - 15) {
        bool currentGrid[GRID_SIZE][GRID_SIZE] = { false };
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[2] == 0) {
                int r = pShot->param_i[0];
                int c = pShot->param_i[1];
                if (r >= 0 && r < GRID_SIZE && c >= 0 && c < GRID_SIZE) {
                    currentGrid[r][c] = true;
                }
            }
            pShot = pShot->next;
        }

        for (int r = 0; r < GRID_SIZE; r++) {
            for (int c = 0; c < GRID_SIZE; c++) {
                int neighbors = 0;
                for (int dr = -1; dr <= 1; dr++) {
                    for (int dc = -1; dc <= 1; dc++) {
                        if (dr == 0 && dc == 0) continue;
                        int nr = r + dr;
                        int nc = c + dc;
                        if (nr >= 0 && nr < GRID_SIZE && nc >= 0 && nc < GRID_SIZE) {
                            if (currentGrid[nr][nc]) neighbors++;
                        }
                    }
                }
                if (!currentGrid[r][c] && neighbors == 3) {
                    AddEnemyShot(pEnemyShotSet, GetGridX(c), GetGridY(r), 0.0, 0.0, img_enemyShotDiamond[1], 2, r, c);
                }
            }
        }
    }

    // 【4. 世代交代の実行（メイン更新）】
    if (pEnemyShotSet->count % GENERATION_INTERVAL == 0 && pEnemyShotSet->count > 0) {
        // A. 予告弾をクリア
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            if (pShot->param_i[2] == 2) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
            pShot = pNext;
        }

        // B. 生存マップ構築
        bool currentGrid[GRID_SIZE][GRID_SIZE] = { false };
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[2] == 0) {
                int r = pShot->param_i[0];
                int c = pShot->param_i[1];
                if (r >= 0 && r < GRID_SIZE && c >= 0 && c < GRID_SIZE) {
                    currentGrid[r][c] = true;
                }
            }
            pShot = pShot->next;
        }

        // C. 次世代の判定
        bool nextGrid[GRID_SIZE][GRID_SIZE] = { false };
        for (int r = 0; r < GRID_SIZE; r++) {
            for (int c = 0; c < GRID_SIZE; c++) {
                int neighbors = 0;
                for (int dr = -1; dr <= 1; dr++) {
                    for (int dc = -1; dc <= 1; dc++) {
                        if (dr == 0 && dc == 0) continue;
                        int nr = r + dr;
                        int nc = c + dc;
                        if (nr >= 0 && nr < GRID_SIZE && nc >= 0 && nc < GRID_SIZE) {
                            if (currentGrid[nr][nc]) neighbors++;
                        }
                    }
                }

                if (currentGrid[r][c]) {
                    nextGrid[r][c] = (neighbors == 2 || neighbors == 3);
                }
                else {
                    nextGrid[r][c] = (neighbors == 3);
                }
            }
        }

        // D. 既存セルの消滅・狙撃弾の射出
        bool playedScatterSound = false;
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            if (pShot->param_i[2] == 0) {
                int r = pShot->param_i[0];
                int c = pShot->param_i[1];
                if (!nextGrid[r][c]) {
                    double muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    double speed = 2.0;
                    AddEnemyShot(pEnemyShotSet, pShot->x, pShot->y, muki, speed, img_enemyShotSmallBall[0], 1);

                    if (!playedScatterSound) {
                        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                        playedScatterSound = true;
                    }

                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                }
            }
            pShot = pNext;
        }

        // E. 新規セルの誕生
        bool playedBirthSound = false;
        for (int r = 0; r < GRID_SIZE; r++) {
            for (int c = 0; c < GRID_SIZE; c++) {
                if (!currentGrid[r][c] && nextGrid[r][c]) {
                    AddEnemyShot(pEnemyShotSet, GetGridX(c), GetGridY(r), 0.0, 0.0, img_enemyShotMediumBall[2], 0, r, c);

                    if (!playedBirthSound) {
                        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                        playedBirthSound = true;
                    }
                }
            }
        }
    }

    // 【5. 各弾の進行処理】
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[2] == 1) {
            // 消滅狙撃弾
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[2] == 3) {
            // ボスの種火弾
            int tr = pShot->param_i[3];
            int tc = pShot->param_i[4];
            double tx = GetGridX(tc);
            double ty = GetGridY(tr);

            double dx = tx - pShot->x;
            double dy = ty - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);

            // 着弾した瞬間、その場所を中心に「長生きするパターン」を配置する
            if (dist <= pShot->speed) {
                // GetRand(2) により 3種類の不老・振動パターンからランダムに選択
                // 0: 横ウインカー, 1: 縦ウインカー, 2: 固定ブロック
                int patternType = GetRand(2);
                int offsets[4][2] = { 0 };
                int cellCount = 0;

                if (patternType == 0) { // 横ウインカー (横3マス)
                    offsets[0][0] = 0;  offsets[0][1] = -1;
                    offsets[1][0] = 0;  offsets[1][1] = 0;
                    offsets[2][0] = 0;  offsets[2][1] = 1;
                    cellCount = 3;
                }
                else if (patternType == 1) { // 縦ウインカー (縦3マス)
                    offsets[0][0] = -1; offsets[0][1] = 0;
                    offsets[1][0] = 0;  offsets[1][1] = 0;
                    offsets[2][0] = 1;  offsets[2][1] = 0;
                    cellCount = 3;
                }
                else { // 固定ブロック (2x2正方形。永遠に形が変わらない安定パターン)
                    offsets[0][0] = 0;  offsets[0][1] = 0;
                    offsets[1][0] = 0;  offsets[1][1] = 1;
                    offsets[2][0] = 1;  offsets[2][1] = 0;
                    offsets[3][0] = 1;  offsets[3][1] = 1;
                    cellCount = 4;
                }

                bool placedAny = false;
                for (int i = 0; i < cellCount; i++) {
                    int r = tr + offsets[i][0];
                    int c = tc + offsets[i][1];

                    // 仮想グリッド外にはみ出さないよう境界チェック
                    if (r >= 0 && r < GRID_SIZE && c >= 0 && c < GRID_SIZE) {
                        // 重複生成を防ぐため、既にその座標に生きたセルが無いかチェック
                        bool alreadyExists = false;
                        sEnemyShot* pCheck = pEnemyShotSet->pEnemyShotHead->next;
                        while (pCheck != pEnemyShotSet->pEnemyShotHead) {
                            if (pCheck->param_i[2] == 0 && pCheck->param_i[0] == r && pCheck->param_i[1] == c) {
                                alreadyExists = true;
                                break;
                            }
                            pCheck = pCheck->next;
                        }

                        // なければそこに緑の固定セル(type=0)を生成
                        if (!alreadyExists) {
                            AddEnemyShot(pEnemyShotSet, GetGridX(c), GetGridY(r), 0.0, 0.0, img_enemyShotMediumBall[2], 0, r, c);
                            placedAny = true;
                        }
                    }
                }

                if (placedAny) {
                    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                }

                // 役目を終えた種火弾(type=3)を消去
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_LifeGame_Gemini()
{
    static int muki;
    static int shot_count;
    static sEnemyShotSet* pMainSet;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
        pMainSet = nullptr;

        pMainSet = new sEnemyShotSet;
        pMainSet->count = 0;
        pMainSet->patternFunc = ShotGenerationGrid;
        pMainSet->x = enemy.x;
        pMainSet->y = enemy.y;
        pMainSet->muki = 0.0;
        pMainSet->kind = shot_count++;

        pMainSet->pEnemyShotHead = new sEnemyShot;
        pMainSet->pEnemyShotHead->prev = pMainSet->pEnemyShotHead;
        pMainSet->pEnemyShotHead->next = pMainSet->pEnemyShotHead;

        pMainSet->prev = enemyShotSetHead.prev;
        pMainSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMainSet;
        enemyShotSetHead.prev = pMainSet;
    }
    else {
        enemy.x += 0.8 * (double)muki;
        if (count % 180 == 90) muki *= -1;

        // 120フレーム（2秒）ごとに、盤面内のランダムな位置（端過ぎてパターンがはみ出ないよう1〜13の範囲）を狙って種火弾を落とす
        if (count % 120 == 0 && pMainSet != nullptr) for (int i = 0; i < 4; i++) {
            // 端にはみ出るとパターンが削れて崩壊しやすいため、内側の 1 〜 13 マス目を狙わせる
            int target_r = 1 + GetRand(GRID_SIZE - 3); // 1 〜 13
            int target_c = 1 + GetRand(GRID_SIZE - 3); // 1 〜 13

            double tx = GetGridX(target_c);
            double ty = GetGridY(target_r);

            double muki_to_target = atan2(ty - enemy.y, tx - enemy.x);
            double seed_speed = 1.8;

            // 種火弾 (type=3, 橙の大玉)
            AddEnemyShot(pMainSet, enemy.x, enemy.y, muki_to_target, seed_speed, img_enemyShotLargeBall[8], 3, 0, 0, target_r, target_c);

            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }
}