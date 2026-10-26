#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================
// マスゲーム弾幕 : ShotMassGame
// 小弾(人1人)512発で人文字「HUMAN」を作り、崩落させる
// ============================================
static const int MASS_ROWS = 16;
static const int MASS_COLS = 32;
static const int MASS_PITCH = 13; // 人と人の間隔
static const int MASS_NUM = MASS_ROWS * MASS_COLS; // 512

static bool g_humanPattern[MASS_ROWS][MASS_COLS];
static bool g_patternInited = false;

static void InitHumanPattern()
{
    if (g_patternInited) return;
    for (int r = 0; r < MASS_ROWS; r++)
        for (int c = 0; c < MASS_COLS; c++)
            g_humanPattern[r][c] = false;

    // 7行 x 29列の HUMAN ドットフォント
    const char* font[7] = {
        "#   # #   # #   #  ###  #   #",
        "#   # #   # ## ## #   # ##  #",
        "#   # #   # # # # #   # # # #",
        "##### #   # #   # ##### #  ##",
        "#   # #   # #   # #   # #   #",
        "#   # #   # #   # #   # #   #",
        "#   #  ###  #   # #   # #   #"
    };
    const int fontH = 7;
    const int fontW = 29;
    const int rowOff = (MASS_ROWS - fontH) / 2; // 4
    const int colOff = (MASS_COLS - fontW) / 2; // 1

    for (int r = 0; r < fontH; r++) {
        for (int c = 0; c < fontW; c++) {
            if (font[r][c] == '#') {
                g_humanPattern[rowOff + r][colOff + c] = true;
            }
        }
    }
    g_patternInited = true;
}

static void ShotMassGame(sEnemyShotSet* pEnemyShotSet)
{
    InitHumanPattern();

    sEnemyShot* pEnemyShot;
    // Phase 0 : 生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const double baseX = pEnemyShotSet->x; // 240を想定
        const double baseY = pEnemyShotSet->y; // 60を想定
        const double startX = baseX - (MASS_COLS * MASS_PITCH) / 2.0 + MASS_PITCH / 2.0;
        const double startY = baseY;

        for (int r = 0; r < MASS_ROWS; r++) {
            for (int c = 0; c < MASS_COLS; c++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = startX + c * MASS_PITCH;
                pEnemyShot->y = startY + r * MASS_PITCH;
                pEnemyShot->muki = DX_PI / 2.0; // 下向きに落とす時の向き
                pEnemyShot->speed = 0.0; // 最初は静止 (無害演出)

                // param_i 使い方:
                // [0]=行, [1]=列, [2]=落下タイプ(0直進/1sinカーブ/2直角曲がり), [3]=現在色番号
                pEnemyShot->param_i[0] = r;
                pEnemyShot->param_i[1] = c;
                pEnemyShot->param_i[2] = c % 3; // 列で役割分担してバラけさせる
                pEnemyShot->param_i[3] = 6; // 白

                // 素材選定: 小玉のみで人を表現。色だけ使い分ける
                // 小玉(2.5x2.5) / 色 0:赤 1:黄 4:青 6:白
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 最初は全員白

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    // Phase 1 : 人文字出現 (60F)
    else if (pEnemyShotSet->count == 60) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            int r = pShot->param_i[0];
            int c = pShot->param_i[1];
            int col = g_humanPattern[r][c] ? 0 : 6; // 文字部は赤(0)、背景は白(6)
            pShot->param_i[3] = col;
            pShot->kind = img_enemyShotSmallBall[col];
            pShot = pShot->next;
        }
    }
    // Phase 2 : ウェーブ演出 (120F～299F) 色替えだけで波を打つ
    else if (pEnemyShotSet->count >= 120 && pEnemyShotSet->count < 300) {
        if (pEnemyShotSet->count % 8 == 0) {
            int waveRow = (pEnemyShotSet->count / 8) % MASS_ROWS;
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                int r = pShot->param_i[0];
                int c = pShot->param_i[1];
                int newCol;
                if (r == waveRow) {
                    newCol = 1; // 黄でウェーブ強調
                }
                else {
                    newCol = g_humanPattern[r][c] ? 0 : 6;
                }
                pShot->param_i[3] = newCol;
                pShot->kind = img_enemyShotSmallBall[newCol];
                pShot = pShot->next;
            }
        }
    }
    // Phase 3 : 崩落開始 (300F)
    else if (pEnemyShotSet->count == 300) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // GetRand(x)は0～xの x+1通り。注意して-20～20を作る
            pShot->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI;
            pShot->speed = 1.2 + GetRand(80) / 100.0; // 1.2～2.0
            // 落下タイプ2(白)は後で直角曲がりするので初速少し遅く
            if (pShot->param_i[2] == 2) {
                pShot->speed *= 0.85;
            }
            pShot = pShot->next;
        }
    }

    // 移動処理 : countのインクリメントはメイン側、ここでは座標更新のみ
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->count < 300) {
            // 集合中は静止、少しだけ浮遊感
            // pShot->y += sin(pEnemyShotSet->count * 0.05 + pShot->param_i[1]) * 0.1;
        }
        else {
            // 落下中
            // タイプ1(青系担当)はsinカーブ
            if (pShot->param_i[2] == 1) {
                pShot->x += sin(pEnemyShotSet->count * 0.08/2 + pShot->param_i[0]) * 0.6/3;
            }
            // タイプ2(白)は60F後に横へ曲がる = 列崩れ
            if (pShot->param_i[2] == 2 && pEnemyShotSet->count - 300 == 60) {
                // 左右ランダムに直角曲がり
                if (GetRand(1) == 0) {
                    pShot->muki += DX_PI / 2.0;
                }
                else {
                    pShot->muki -= DX_PI / 2.0;
                }
                // 曲がったら色を青(4)に変えて視覚的に区別
                pShot->kind = img_enemyShotSmallBall[4];
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体
void EnemyPat_MassGame_MetaAI()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        g_patternInited = false;
    }
    else {
        // サンプルと同じく左右にゆっくり移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 60Fでマスゲームを1回だけ召喚
    if (count % 500 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMassGame;
        pEnemyShotSet->x = 240.0; // スタジアム中心
        pEnemyShotSet->y = 70.0;  // 上部
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}