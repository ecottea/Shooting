// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

constexpr int GRID = 7;
constexpr double CELL = 24.0;

static bool board[GRID][GRID];
static bool nextBoard[GRID][GRID];

static void AddShot(
    sEnemyShotSet* set,
    double x,
    double y,
    double angle,
    double speed,
    int kind)
{
    sEnemyShot* s = new sEnemyShot;

    s->x = x;
    s->y = y;
    s->muki = angle;
    s->speed = speed;
    s->kind = kind;

    s->prev = set->pEnemyShotHead->prev;
    s->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = s;
    set->pEnemyShotHead->prev = s;
}

static void InitBoard()
{
    for (int y = 0; y < GRID; y++)
        for (int x = 0; x < GRID; x++)
            board[y][x] = false;

    // グライダー
    board[1][2] = true;
    board[2][3] = true;
    board[3][1] = true;
    board[3][2] = true;
    board[3][3] = true;

    // 小さな島
    board[1][5] = true;
    board[2][5] = true;
    board[3][5] = true;
}

static void StepBoard()
{
    for (int y = 0; y < GRID; y++) {

        for (int x = 0; x < GRID; x++) {

            int cnt = 0;

            for (int dy = -1; dy <= 1; dy++) {

                for (int dx = -1; dx <= 1; dx++) {

                    if (dx == 0 && dy == 0)
                        continue;

                    int xx = x + dx;
                    int yy = y + dy;

                    if (xx < 0 || xx >= GRID)
                        continue;

                    if (yy < 0 || yy >= GRID)
                        continue;

                    if (board[yy][xx])
                        cnt++;
                }
            }

            if (board[y][x]) {

                nextBoard[y][x] =
                    (cnt == 2 || cnt == 3);

            }
            else {

                nextBoard[y][x] =
                    (cnt == 3);
            }
        }
    }

    for (int y = 0; y < GRID; y++)
        for (int x = 0; x < GRID; x++)
            board[y][x] = nextBoard[y][x];
}

static void ShotLifeGame(sEnemyShotSet* set)
{
    if (set->count == 0) {

        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);

        PlaySoundMem(
            sound_enemyShot_medium,
            DX_PLAYTYPE_BACK);

        for (int y = 0; y < GRID; y++) {

            for (int x = 0; x < GRID; x++) {

                if (!board[y][x])
                    continue;

                double px =
                    set->x +
                    (x - GRID / 2) * CELL;

                double py =
                    set->y +
                    (y - GRID / 2) * CELL;

                AddShot(
                    set,
                    px,
                    py,
                    0.0,
                    0.0,
                    img_enemyShotSmallBall[2]);

            }
        }
    }

    if (set->count % 20 == 0) {
        if (CheckSoundMem(sound_enemyShot_light))
            StopSoundMem(sound_enemyShot_light);

        PlaySoundMem(
            sound_enemyShot_light,
            DX_PLAYTYPE_BACK);

        StepBoard();

        sEnemyShot* shot =
            set->pEnemyShotHead->next;

        while (shot != set->pEnemyShotHead) {
            sEnemyShot* next = shot->next;
            //delete shot;
            if (shot->speed == 0.0) {
                shot->margin = -9999;
            }
            shot = next;
        }

        //set->pEnemyShotHead->next =
        //    set->pEnemyShotHead;

        //set->pEnemyShotHead->prev =
        //    set->pEnemyShotHead;

        for (int y = 0; y < GRID; y++) {

            for (int x = 0; x < GRID; x++) {

                if (!board[y][x])
                    continue;

                double px =
                    set->x +
                    (x - GRID / 2) * CELL;

                double py =
                    set->y +
                    (y - GRID / 2) * CELL;

                AddShot(
                    set,
                    px,
                    py,
                    0.0,
                    0.0,
                    img_enemyShotSmallBall[2]);
            }
        }
    }

    // セルの位置を毎フレーム補正し、一定間隔で弾を撃つ
    int idx = 0;

    sEnemyShot* shot = set->pEnemyShotHead->next;

    for (int y = 0; y < GRID; y++) {

        for (int x = 0; x < GRID; x++) {

            if (!board[y][x])
                continue;

            if (shot == set->pEnemyShotHead)
                break;

            double tx =
                set->x +
                (x - GRID / 2) * CELL;

            double ty =
                set->y +
                (y - GRID / 2) * CELL;

            // 少し滑らかに追従させる
            shot->x += (tx - shot->x) * 0.25;
            shot->y += (ty - shot->y) * 0.25;

            // 脈動
            shot->x += cos((set->count + idx * 9) * 0.08) * 0.5;
            shot->y += sin((set->count + idx * 9) * 0.08) * 0.5;

            // 一定間隔でセルから弾を発射
            if ((set->count + idx * 3) % 45 == 0) {

                double ang =
                    atan2(
                        player.y - shot->y,
                        player.x - shot->x);

                AddShot(
                    set,
                    shot->x,
                    shot->y,
                    ang,
                    2.2,
                    img_enemyShotMediumBall[6]);
            }

            idx++;
            shot = shot->next;
        }
    }

    // 一定時間ごとに新しいグライダーを投入
    if (set->count % 120 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);

        PlaySoundMem(
            sound_enemyShot_medium,
            DX_PLAYTYPE_BACK);

        board[0][1] = true;
        board[1][2] = true;
        board[2][0] = true;
        board[2][1] = true;
        board[2][2] = true;
    }

    // 発射された弾だけを移動させる
    shot = set->pEnemyShotHead->next;

    while (shot != set->pEnemyShotHead) {

        if (shot->speed > 0.0) {

            shot->x += shot->speed * cos(shot->muki);
            shot->y += shot->speed * sin(shot->muki);
        }

        shot = shot->next;
    }
}

//============================================================
// 敵本体
//============================================================
void EnemyPat_LifeGame_ChatGPT()
{
    static int muki;

    if (count == 1) {

        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        muki = 1;

        InitBoard();
    }
    else {

        enemy.x += muki * 0.8;

        if (enemy.x < 120.0)
            muki = 1;

        if (enemy.x > 360.0)
            muki = -1;
    }

    // ライフゲーム盤面を1セット生成
    if (count == 1) {

        sEnemyShotSet* set = new sEnemyShotSet;

        set->count = 0;
        set->patternFunc = ShotLifeGame;
        set->x = enemy.x;
        set->y = enemy.y + 80.0;
        set->muki = 0.0;
        set->kind = 0;

        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }

    // 盤面全体をボスの移動に追従させる
    sEnemyShotSet* set = enemyShotSetHead.next;

    while (set != &enemyShotSetHead) {

        set->x += (enemy.x - set->x) * 0.15;
        set->y += ((enemy.y + 80.0) - set->y) * 0.15;

        set = set->next;
    }
}