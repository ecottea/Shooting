#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ===================================================================
//  弾幕：マスゲーム「シンクロ・パレード」
//  - 小弾をマスゲームの選手に見立て、等間隔の隊形と発射タイミングの
//    ズレで「整列 → ウェーブ → ピラミッド → 交差」を表現する。
//  - 人型素材は使わず、既存の小弾・大弾のみで構成する。
//  - 1つの sEnemyShotSet を生成し、count を時間軸として全幕を進行。
// ===================================================================

// 弾をセットに追加するヘルパー（直進用：behavior=0）
static void AddMassGameShot(sEnemyShotSet* set,
    double x, double y, double muki, double speed,
    int kind, int behavior)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = behavior;
    p->margin = 120;
    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;
}

// 弾幕本体：マスゲーム「シンクロ・パレード」
static void ShotMassGame(sEnemyShotSet* pEnemyShotSet)
{
    const double PI = 3.14159265358979323846;
    const double DOWN = PI / 2.0;  // 下方向
    int t = pEnemyShotSet->count;

    // ========== 第1幕：整列入場（t=0） ==========
    if (t == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        int kindSmall = img_enemyShotSmallBall[0];  // 赤小玉（選手）
        int kindLarge = img_enemyShotLargeBall[1];  // 黄大玉（旗手）

        // 7列×5行のグリッド（x:60,120,...,420 / y:60,80,100,120,140）
        for (int r = 0; r < 5; r++) {
            for (int c = 0; c < 7*2-1; c++) {
                double x = 60.0 + c * 60.0/2;
                double y = 60.0 + r * 20.0;
                AddMassGameShot(pEnemyShotSet, x, y, DOWN, 2.0, kindSmall, 0);
            }
        }
        // 四隅に旗手（大玉）。少し遅い速度で降下し、隊列の後ろに残る。
        double corners[4][2] = {
            { 60.0, 60.0 }, { 420.0, 60.0 },
            { 60.0, 140.0 }, { 420.0, 140.0 }
        };
        for (int i = 0; i < 4; i++) {
            AddMassGameShot(pEnemyShotSet, corners[i][0], corners[i][1],
                DOWN, 0.9, kindLarge, 0);
        }
    }

    // ========== 第2幕：ウェーブ（t=90〜96で列ごとに時間差出現） ==========
    // 左の列から1フレームずつずらして出現。位相も列ごとにずらし、
    // 正弦振動で列が前後に波打つように見せる。
    if (t >= 90 && t < 97+6) {
        int col = t - 90;
        if (col == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        double bx = 60.0 + col * 60.0/2;
        int kindSmall = img_enemyShotSmallBall[3];  // シアン小玉

        for (int r = 0; r < 5; r++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = bx;
            p->y = 40.0 + r * 20.0;
            p->muki = DOWN;
            p->speed = 0.0;
            p->kind = kindSmall;
            p->param_i[0] = 1;                  // ウェーブ挙動
            p->param_d[0] = bx;                 // 振動中心X
            p->param_d[1] = 2.0;                // 降下速度（Y）
            p->param_d[2] = 25.0;               // 振幅
            p->param_d[3] = 0.10;               // 周波数(rad/frame)
            p->param_d[4] = (double)col * 0.6;  // 列ごとの位相
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // ウェーブに合わせて左右から旗（大玉）が横移動
    if (t == 90) {
        int kindLarge = img_enemyShotLargeBall[0];  // 赤大玉
        AddMassGameShot(pEnemyShotSet, 20.0, 100.0, 0.0, 0.6, kindLarge, 0); // 左→右
        AddMassGameShot(pEnemyShotSet, 460.0, 100.0, PI, 0.6, kindLarge, 0); // 右→左
    }

    // ========== 第3幕：ピラミッドを繰り返し → 交差 ==========
    // ピラミッド隊形を一定間隔で3回出現
    if (t == 180 || t == 210 || t == 240) {
        if (t == 180) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        int kindSmall = img_enemyShotSmallBall[2];  // 緑小玉
        int kindLarge = img_enemyShotLargeBall[1];  // 黄大玉

        // 頂点に大玉（旗手）
        AddMassGameShot(pEnemyShotSet, 240.0, 40.0, DOWN, 1.5, kindLarge, 0);
        // 両斜辺に小玉を配置
        for (int i = 1; i <= 5*2; i++) {
            double y = 40.0 + i * 24.0/2;
            AddMassGameShot(pEnemyShotSet, 240.0 - i * 34.0/2, y, DOWN, 1.5, kindSmall, 0);
            AddMassGameShot(pEnemyShotSet, 240.0 + i * 34.0/2, y, DOWN, 1.5, kindSmall, 0);
        }
    }

    // 交差フォーメーション（X字）
    if (t == 300) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int kindSmall = img_enemyShotSmallBall[5];  // マゼンタ小玉
        int kindLarge = img_enemyShotLargeBall[3];  // シアン大玉

        // 中央交点に大玉
        AddMassGameShot(pEnemyShotSet, 240.0, 200.0, DOWN, 1.0, kindLarge, 0);

        // 左上→右下の斜め列（中央は大玉があるのでスキップ）
        for (int i = 0; i < 5*2-1; i++) {
            if (i == 2*2) continue;
            AddMassGameShot(pEnemyShotSet, 140.0 + i * 50.0/2, 100.0 + i * 50.0/2,
                DOWN, 1.0, kindSmall, 0);
        }
        // 右上→左下の斜め列（中央はスキップ）
        for (int i = 0; i < 5*2-1; i++) {
            if (i == 2*2) continue;
            AddMassGameShot(pEnemyShotSet, 340.0 - i * 50.0/2, 100.0 + i * 50.0/2,
                DOWN, 1.0, kindSmall, 0);
        }
    }

    // ========== 弾の移動 ==========
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // ウェーブ挙動：Y軸は等速、X軸は正弦振動
            pShot->y += pShot->param_d[1];
            pShot->x = pShot->param_d[0]
                + pShot->param_d[2] * sin(pShot->count * pShot->param_d[3]
                    + pShot->param_d[4]);
        }
        else {
            // 通常直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_MassGame_DeepSeek()
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
        if (count % 120 == 60) muki *= -1;
    }

    // 全体を1つのセットで演出（count を時間軸として幕が進行）
    if (count % 360 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMassGame;
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