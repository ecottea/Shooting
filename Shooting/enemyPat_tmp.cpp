#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：シュレッダー「裁断の刻」
//  フェーズ1: 白い紙(短レーザー)が4枚降りてきて刃の位置で停止
//  フェーズ2: シアンの刃(短レーザー)が縦に走り抜ける
//  フェーズ3: 紙が短冊(銃弾)に分裂し、正弦波で揺れながら舞い落ちる
// ============================================================

// param_i[0] に格納する弾の状態
#define SHRED_STATE_PAPER_MOVE   0  // 投入中の紙(下降中)
#define SHRED_STATE_PAPER_STOP   1  // 刃の位置で停止した紙
#define SHRED_STATE_SHRED_LASER  2  // 断裁後の長い紙片(短レーザー)
#define SHRED_STATE_BLADE        3  // 刃(直進のみ)
#define SHRED_STATE_SHRED_BULLET 4  // 断裁後の短冊(銃弾)

// 弾をリストに追加してポインタを返す
static sEnemyShot* ShredAddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->prev = pSet->pEnemyShotHead->prev;
    pEnemyShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pEnemyShot;
    pSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// 紙片の共通更新：正弦波で揺れながら落下(位相は個体ごとにバラバラ)
static void ShredFall(sEnemyShot* pShot)
{
    double sway = sin(pShot->count * pShot->param_d[2] + pShot->param_d[0]);
    pShot->x += sway * pShot->param_d[1];
    pShot->y += pShot->param_d[3];
    // 揺れに合わせて紙片が傾く
    pShot->muki = DX_PI / 2.0
        + cos(pShot->count * pShot->param_d[2] + pShot->param_d[0]) * 0.5;
}

// シュレッダー本体
static void ShotShredder(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;

    if (t == 0) {
        // フェーズ1: 予告音を鳴らして紙を4枚投入
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 4; i++) {
            // 紙の投入位置(敵の中心を基準に左右に展開、少しランダム)
            double px = pEnemyShotSet->x - 135.0 + i * 90.0 + (GetRand(40) - 20);
            if (px < 40.0)  px = 40.0;   // 画面内にクランプ
            if (px > 440.0) px = 440.0;
            // 白い紙(短レーザー白)を横方向にして上から降ろす
            sEnemyShot* pPaper = ShredAddShot(pEnemyShotSet, px, 70.0, 0.0, 2.5,
                img_enemyShotLaser[6]);
            pPaper->param_i[0] = SHRED_STATE_PAPER_MOVE;
            pPaper->param_d[0] = 160.0;  // 停止位置=刃のライン
        }
    }
    else if (t == 50) {
        // フェーズ2: 刃(シアンの短レーザー)が縦に一瞬走り抜ける
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            pShot != pEnemyShotSet->pEnemyShotHead; pShot = pShot->next) {
            if (pShot->param_i[0] == SHRED_STATE_PAPER_STOP) {
                sEnemyShot* pBlade = ShredAddShot(pEnemyShotSet, pShot->x,
                    pShot->y - 20.0,
                    DX_PI / 2.0, 16.0,
                    img_enemyShotLaser[3]);
                pBlade->param_i[0] = SHRED_STATE_BLADE;
            }
        }
    }
    else if (t == 56) {
        // フェーズ3: 停止中の紙を断裁して短冊にする
        // ※ループ中にリストへ追加するため、nextを先に退避してから処理
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;
            if (pShot->param_i[0] == SHRED_STATE_PAPER_STOP) {
                // 紙そのものを「長い紙片」に変化させて落下させる
                pShot->param_i[0] = SHRED_STATE_SHRED_LASER;
                pShot->param_d[0] = GetRand(628) / 100.0;        // 揺れの位相 0.00〜6.28
                pShot->param_d[1] = 0.6 + GetRand(60) / 100.0;   // 揺れ幅   0.60〜1.20
                pShot->param_d[2] = 0.05 + GetRand(6) / 100.0;   // 揺れ周期 0.05〜0.11
                pShot->param_d[3] = 1.0 + GetRand(50) / 100.0;   // 落下速度 1.00〜1.50

                // 1枚の紙から短冊(銃弾)を4本切り出す
                for (int i = 0; i < 4; i++) {
                    double sx = pShot->x + GetRand(64) - 32;     // 紙の幅(64px)内に散らす
                    // 2割ほど橙味の紙片を混ぜる
                    int color = (GetRand(4) == 0) ? 8 : 6;
                    sEnemyShot* pShred = ShredAddShot(pEnemyShotSet,
                        sx, pShot->y + GetRand(8) - 4,
                        DX_PI / 2.0, 0.0,
                        img_enemyShotBullet[color]);
                    pShred->param_i[0] = SHRED_STATE_SHRED_BULLET;
                    pShred->param_d[0] = GetRand(628) / 100.0;       // 位相
                    pShred->param_d[1] = 0.8 + GetRand(100) / 100.0; // 揺れ幅   0.80〜1.80
                    pShred->param_d[2] = 0.06 + GetRand(8) / 100.0;  // 揺れ周期 0.06〜0.14
                    pShred->param_d[3] = 1.3 + GetRand(80) / 100.0;  // 落下速度 1.30〜2.10
                }
            }
            pShot = pNext;
        }
    }

    // 毎フレームの移動更新
    for (sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        pShot != pEnemyShotSet->pEnemyShotHead; pShot = pShot->next) {
        switch (pShot->param_i[0]) {
        case SHRED_STATE_PAPER_MOVE:
            // 紙が降りてきて刃の位置で停止する
            pShot->y += pShot->speed;
            if (pShot->y >= pShot->param_d[0]) {
                pShot->y = pShot->param_d[0];
                pShot->speed = 0.0;
                pShot->param_i[0] = SHRED_STATE_PAPER_STOP;
            }
            break;
        case SHRED_STATE_SHRED_LASER:
        case SHRED_STATE_SHRED_BULLET:
            // 揺れながら舞い落ちる紙片
            ShredFall(pShot);
            break;
        case SHRED_STATE_BLADE:
        default:
            // 刃などは直進(メインルーチンと同じ更新式)
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Tmp()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // 300フレームごとにシュレッダー攻撃を1サイクル起こす
    // (前サイクルの紙片が画面外に出て消える頃合いを見計らった間隔)
    if (count % 200 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShredder;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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