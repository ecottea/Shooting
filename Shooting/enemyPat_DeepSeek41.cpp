// enemyPat_Tmp.cpp
// バウンド・カタストロフ パターン実装（ブロック・バー表示対応）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// ファイルスコープの静的変数（ブロック、バー、オーブ管理）
// ============================================================
static const int   BLOCK_COLS = 10;
static const int   BLOCK_ROWS = 4;
static const double BLOCK_W = 40.0;
static const double BLOCK_H = 20.0;
static const double BLOCK_START_X = (480.0 - BLOCK_COLS * BLOCK_W) / 2.0;
static const double BLOCK_START_Y = 60.0;

static int    blockDur[BLOCK_ROWS][BLOCK_COLS];   // 0:破壊済み, 1-3:耐久値
static double blockX[BLOCK_ROWS][BLOCK_COLS];
static double blockY[BLOCK_ROWS][BLOCK_COLS];
static int    blocksRemaining = 0;
static bool   breakMode = false;

static double bar_y = 440.0;         // バーのY座標（固定）
static double bar_x = 240.0;         // バーのX座標（プレイヤー追従）
static const double BAR_HALF_W = 30.0;     // バー半幅（反射判定用）
static const double BAR_THICKNESS = 8.0;   // バーの太さ（見た目用）

static double prevPlayerX = 240.0;         // 前フレームのプレイヤーX
static double playerDX = 0.0;           // プレイヤーの横移動量

static sEnemyShotSet* orbSet1 = nullptr;
static sEnemyShotSet* orbSet2 = nullptr;

// ブロックとバーの表示用ショットセット
static sEnemyShotSet* blockBarSet = nullptr;

// 小物用関数プロトタイプ
static void SpawnBlockFragments(double x, double y, int blockColor, bool redPanic);
static void SpawnBarAttack();
static sEnemyShotSet* CreateOrb(double x, double y, double angle, double speed);
static void UpdateBlockBarDisplay();
static void ClearBlockBarDisplay();

// ============================================================
// ブロック・バー表示用の静止パターン関数（速度0、単に保持）
// ============================================================
static void BlockBarDrawFunc(sEnemyShotSet* pSet)
{
    // 弾は移動しない（速度0のまま）
    // 描画は本体の描画システムが kind に応じて行う
}

// ============================================================
// ブロックとバーの表示を最新の状態に再構築する
// ============================================================
static void UpdateBlockBarDisplay()
{
    // 既存の表示用弾を全削除
    ClearBlockBarDisplay();

    if (!blockBarSet) {
        blockBarSet = new sEnemyShotSet;
        blockBarSet->count = 0;
        blockBarSet->patternFunc = BlockBarDrawFunc;
        blockBarSet->pEnemyShotHead = new sEnemyShot;
        blockBarSet->pEnemyShotHead->prev = blockBarSet->pEnemyShotHead;
        blockBarSet->pEnemyShotHead->next = blockBarSet->pEnemyShotHead;
        // グローバルリストに追加
        blockBarSet->prev = enemyShotSetHead.prev;
        blockBarSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = blockBarSet;
        enemyShotSetHead.prev = blockBarSet;
    }

    sEnemyShot* head = blockBarSet->pEnemyShotHead;

    // ブロックを弾として追加
    for (int r = 0; r < BLOCK_ROWS; ++r) {
        for (int c = 0; c < BLOCK_COLS; ++c) {
            if (blockDur[r][c] <= 0) continue;

            int color;
            if (blockDur[r][c] == 1)      color = 4; // 青
            else if (blockDur[r][c] == 2) color = 1; // 黄
            else                          color = 0; // 赤

            // ブロックの見た目には大玉の画像を使う（矩形に近い大きさ）
            int img = img_enemyShotLargeBall[color];

            sEnemyShot* p = new sEnemyShot;
            p->x = blockX[r][c] + BLOCK_W / 2.0;
            p->y = blockY[r][c] + BLOCK_H / 2.0;
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = img;
            p->margin = 0.0;  // 画面内固定なので消去判定不要（0にすると安全）
            p->prev = head->prev;
            p->next = head;
            head->prev->next = p;
            head->prev = p;
        }
    }

    // バーを弾として追加（横長のレーザー画像で代用）
    {
        int img = img_enemyShotLaser[6]; // 白レーザー（横長）
        sEnemyShot* p = new sEnemyShot;
        p->x = bar_x;
        p->y = bar_y;
        p->muki = 0.0;
        p->speed = 0.0;
        p->kind = img;
        p->margin = 0.0;
        p->prev = head->prev;
        p->next = head;
        head->prev->next = p;
        head->prev = p;
    }
}

// ============================================================
// ブロック・バー表示用の弾を全削除
// ============================================================
static void ClearBlockBarDisplay()
{
    if (!blockBarSet) return;
    sEnemyShot* head = blockBarSet->pEnemyShotHead;
    sEnemyShot* p = head->next;
    while (p != head) {
        sEnemyShot* next = p->next;
        delete p;
        p = next;
    }
    head->prev = head;
    head->next = head;
}

static void GameOver(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 360; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i / (2.0 * DX_PI);
            pEnemyShot->speed = 10.0;
            pEnemyShot->kind = img_enemyShotLargeBall[0];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// オーブの移動・反射・ブロック破壊を行うパターン関数
// ============================================================
static void OrbMove(sEnemyShotSet* pSet)
{
    sEnemyShot* pOrb = pSet->pEnemyShotHead->next;
    if (pOrb == pSet->pEnemyShotHead) return;

    double prevX = pOrb->param_d[0];
    double prevY = pOrb->param_d[1];
    pOrb->param_d[0] = pOrb->x;
    pOrb->param_d[1] = pOrb->y;

    const double initialSpeed = pOrb->param_d[2];
    if (pOrb->speed < initialSpeed * 2.0) {
        pOrb->speed += 0.002;
        if (pOrb->speed > initialSpeed * 2.0) pOrb->speed = initialSpeed * 2.0;
    }

    pOrb->x += pOrb->speed * cos(pOrb->muki);
    pOrb->y += pOrb->speed * sin(pOrb->muki);

    // 壁反射
    if (pOrb->x < 0.0) {
        pOrb->x = 0.0;
        pOrb->muki = DX_PI - pOrb->muki;
    }
    else if (pOrb->x > 480.0) {
        pOrb->x = 480.0;
        pOrb->muki = DX_PI - pOrb->muki;
    }
    if (pOrb->y < 0.0) {
        pOrb->y = 0.0;
        pOrb->muki = -pOrb->muki;
    }
    else if (pOrb->y > 480.0 && count % 3 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = GameOver;
        pEnemyShotSet->x = pOrb->x;
        pEnemyShotSet->y = pOrb->y;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // バー反射
    if (prevY < bar_y && pOrb->y >= bar_y) {
        if (fabs(pOrb->x - bar_x) <= BAR_HALF_W) {
            pOrb->muki = -pOrb->muki + playerDX * 0.015;
            pOrb->y = bar_y;
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // ブロック衝突
    for (int r = 0; r < BLOCK_ROWS; ++r) {
        for (int c = 0; c < BLOCK_COLS; ++c) {
            if (blockDur[r][c] <= 0) continue;

            double bx = blockX[r][c];
            double by = blockY[r][c];
            if (pOrb->x >= bx && pOrb->x <= bx + BLOCK_W &&
                pOrb->y >= by && pOrb->y <= by + BLOCK_H) {

                bool fromTop = (prevY + 2.5 <= by);
                bool fromBottom = (prevY - 2.5 >= by + BLOCK_H);
                bool fromLeft = (prevX + 2.5 <= bx);
                bool fromRight = (prevX - 2.5 >= bx + BLOCK_W);

                if (fromTop || fromBottom) {
                    pOrb->muki = -pOrb->muki;
                    pOrb->y = fromTop ? by - 0.01 : by + BLOCK_H + 0.01;
                }
                else if (fromLeft || fromRight) {
                    pOrb->muki = DX_PI - pOrb->muki;
                    pOrb->x = fromLeft ? bx - 0.01 : bx + BLOCK_W + 0.01;
                }
                else {
                    pOrb->muki = -pOrb->muki;
                }

                // 耐久値処理
                int originalDur = blockDur[r][c];
                blockDur[r][c]-=999;
                if (blockDur[r][c] <= 0) blocksRemaining--;

                // ブロック表示更新
                UpdateBlockBarDisplay();

                // 破壊時エフェクト
                if (blockDur[r][c] <= 0) {
                    int blockColor;
                    if (originalDur == 1)      blockColor = 4; // 青
                    else if (originalDur == 2) blockColor = 1; // 黄
                    else                       blockColor = 0; // 赤

                    SpawnBlockFragments(bx + BLOCK_W / 2, by + BLOCK_H / 2, blockColor, (originalDur == 3));
                    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                }
                break; // 1フレームに1ブロックのみ処理
            }
        }
    }
}

// ============================================================
// ブロック破壊時の弾幕生成（変更なし）
// ============================================================
static void SpawnBlockFragments(double x, double y, int blockColor, bool redPanic)
{
    // 8方向針弾（小玉）
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = [](sEnemyShotSet* p) {
            sEnemyShot* pShot = p->pEnemyShotHead->next;
            while (pShot != p->pEnemyShotHead) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                pShot = pShot->next;
            }
        };
        pSet->x = x;
        pSet->y = y;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        for (int i = 0; i < 8; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = x;
            p->y = y;
            p->muki = i * DX_PI / 4.0;
            p->speed = 2.0;
            p->kind = img_enemyShotSmallBall[blockColor];
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 青（レーザー）
    if (blockColor == 4) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = [](sEnemyShotSet* p) {
            sEnemyShot* pShot = p->pEnemyShotHead->next;
            while (pShot != p->pEnemyShotHead) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                pShot = pShot->next;
            }
        };
        pSet->x = x;
        pSet->y = y;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        sEnemyShot* p = new sEnemyShot;
        p->x = x;
        p->y = y;
        p->muki = atan2(player.y - y, player.x - x);
        p->speed = 5.0;
        p->kind = img_enemyShotLaser[6];
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // 黄（3way）
    else if (blockColor == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = [](sEnemyShotSet* p) {
            sEnemyShot* pShot = p->pEnemyShotHead->next;
            while (pShot != p->pEnemyShotHead) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                pShot = pShot->next;
            }
        };
        pSet->x = x;
        pSet->y = y;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        double base = atan2(player.y - y, player.x - x);
        for (int i = -1; i <= 1; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->x = x;
            p->y = y;
            p->muki = base + i * (10.0 * DX_PI / 180.0);
            p->speed = 3.5;
            p->kind = img_enemyShotSmallBall[1];
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // 赤（全体パニック）
    else if (blockColor == 0 && redPanic) {
        for (int r = 0; r < BLOCK_ROWS; ++r) {
            for (int c = 0; c < BLOCK_COLS; ++c) {
                if (blockDur[r][c] > 0) {
                    sEnemyShotSet* pSet = new sEnemyShotSet;
                    pSet->count = 0;
                    pSet->patternFunc = [](sEnemyShotSet* p) {
                        sEnemyShot* pShot = p->pEnemyShotHead->next;
                        while (pShot != p->pEnemyShotHead) {
                            pShot->x += pShot->speed * cos(pShot->muki);
                            pShot->y += pShot->speed * sin(pShot->muki);
                            pShot = pShot->next;
                        }
                    };
                    pSet->x = blockX[r][c] + BLOCK_W / 2;
                    pSet->y = blockY[r][c] + BLOCK_H / 2;
                    pSet->pEnemyShotHead = new sEnemyShot;
                    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                    sEnemyShot* p = new sEnemyShot;
                    p->x = pSet->x;
                    p->y = pSet->y;
                    p->muki = GetRand(360) * DX_PI / 180.0;
                    p->speed = 1.0 + GetRand(100) / 100.0;
                    p->kind = img_enemyShotSmallBall[0];
                    p->prev = pSet->pEnemyShotHead->prev;
                    p->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = p;
                    pSet->pEnemyShotHead->prev = p;
                    pSet->prev = enemyShotSetHead.prev;
                    pSet->next = &enemyShotSetHead;
                    enemyShotSetHead.prev->next = pSet;
                    enemyShotSetHead.prev = pSet;
                }
            }
        }
    }
}

// ============================================================
// バーからの定時攻撃
// ============================================================
static void SpawnBarAttack()
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = [](sEnemyShotSet* p) {
        sEnemyShot* pShot = p->pEnemyShotHead->next;
        while (pShot != p->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    };
    pSet->x = bar_x;
    pSet->y = bar_y;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    double base = atan2(player.y - bar_y, player.x - bar_x);
    for (int i = -1; i <= 1; i += 2) {
        sEnemyShot* p = new sEnemyShot;
        p->x = bar_x + i * 5.0;
        p->y = bar_y;
        p->muki = base;
        p->speed = 3.0;
        p->kind = img_enemyShotBullet[6];
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
}

// ============================================================
// オーブ生成（変更なし）
// ============================================================
static sEnemyShotSet* CreateOrb(double x, double y, double angle, double speed)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = OrbMove;
    pSet->x = x;
    pSet->y = y;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    sEnemyShot* pOrb = new sEnemyShot;
    pOrb->x = x;
    pOrb->y = y;
    pOrb->muki = angle;
    pOrb->speed = speed;
    pOrb->kind = img_enemyShotMediumBall[8]; // オレンジ中玉
    pOrb->param_d[0] = x; // prevX
    pOrb->param_d[1] = y; // prevY
    pOrb->param_d[2] = speed; // 初速保存
    pOrb->prev = pSet->pEnemyShotHead->prev;
    pOrb->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pOrb;
    pSet->pEnemyShotHead->prev = pOrb;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

// ============================================================
// 敵本体パターン
// ============================================================
void EnemyPat_BlockBreak_DeepSeek()
{
    if (count == 1) {
        // 静的変数のリセット（重要！）
        blockBarSet = nullptr;
        orbSet1 = nullptr;
        orbSet2 = nullptr;

        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        blocksRemaining = BLOCK_COLS * BLOCK_ROWS;
        for (int r = 0; r < BLOCK_ROWS; ++r) {
            for (int c = 0; c < BLOCK_COLS; ++c) {
                blockX[r][c] = BLOCK_START_X + c * BLOCK_W;
                blockY[r][c] = BLOCK_START_Y + r * BLOCK_H;
                blockDur[r][c] = GetRand(2) + 1; // 1～3
            }
        }
        breakMode = false;

        prevPlayerX = player.x;
        playerDX = 0.0;
        bar_x = player.x;
        if (bar_x < BAR_HALF_W) bar_x = BAR_HALF_W;
        if (bar_x > 480.0 - BAR_HALF_W) bar_x = 480.0 - BAR_HALF_W;

        // ブロック・バー表示の初期構築
        UpdateBlockBarDisplay();

        // 最初のオーブ
        orbSet1 = CreateOrb(240.0, 250.0, DX_PI / 4.0, 2.5);
        orbSet2 = nullptr;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // プレイヤー移動量更新
        playerDX = player.x - prevPlayerX;
        prevPlayerX = player.x;

        // バー追従
        double oldBarX = bar_x;
        bar_x = player.x;
        if (bar_x < BAR_HALF_W) bar_x = BAR_HALF_W;
        if (bar_x > 480.0 - BAR_HALF_W) bar_x = 480.0 - BAR_HALF_W;

        // バーの位置が変わったら表示更新
        if (fabs(bar_x - oldBarX) > 1.0) {
            UpdateBlockBarDisplay();
        }

        // バー攻撃（2秒間隔）
        if (count % 120 == 1) {
            SpawnBarAttack();
        }

        // ブレイクモード移行チェック
        if (!breakMode && blocksRemaining <= (BLOCK_COLS * BLOCK_ROWS) / 3) {
            breakMode = true;
            // 全ブロックを赤（耐久3）に強化
            for (int r = 0; r < BLOCK_ROWS; ++r)
                for (int c = 0; c < BLOCK_COLS; ++c)
                    if (blockDur[r][c] > 0)
                        blockDur[r][c] = 3;
            UpdateBlockBarDisplay();

            // 二つ目のオーブ生成
            if (orbSet1 && !orbSet2) {
                sEnemyShot* orb = orbSet1->pEnemyShotHead->next;
                if (orb != orbSet1->pEnemyShotHead) {
                    double ang = orb->muki + (GetRand(40) - 20) * DX_PI / 180.0;
                    orbSet2 = CreateOrb(orb->x, orb->y, ang, orb->speed);
                }
            }
        }

        // ブレイクモード中はブロック降下
        if (breakMode) {
            for (int r = 0; r < BLOCK_ROWS; ++r)
                for (int c = 0; c < BLOCK_COLS; ++c)
                    if (blockDur[r][c] > 0)
                        blockY[r][c] += 0.5;
            // 降下したので表示更新（毎フレーム呼ぶと重いなら間引いても可）
            UpdateBlockBarDisplay();
        }

        // 全ブロック破壊時の後処理（必要あれば）
        if (blocksRemaining == 0) {
            // クリア
        }
    }
}