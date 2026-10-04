#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕:三節棍「三節連鎖・逆鱗打」
//  大玉(根元)→中玉(中央)→小玉(先端)の3連リンクが、
//  根元ほど遅く先端ほど速い遅延追従回転で鞭のようにしなり、
//  関節の反動で散弾を放った後、一直線に揃って突きを放つ。
//
//  param_i[0] : 初期化済みフラグ
//  param_i[1] : 現サイクルの開始count
//  param_i[2] : 回転方向(+1/-1、サイクル毎に反転)
//  param_d[0〜2]  : 第1/2/3節の向き(絶対角度)
//  param_d[6]  : 突きの角度
//  param_d[7〜9]: 突き前の各節角度(補間用)
//  弾側 param_i[0]: 節番号(0=自由飛行,1〜3=リンク構成弾)
//  弾側 param_i[1]: 節内の位置(1〜)
//  弾側 param_i[2]: 0=リンク制御中 / 1=自由飛行
// ============================================================

// 弾をセットのリストに追加するヘルパ
static sEnemyShot* AddStaffShot(sEnemyShotSet* pSet, int kind,
    double x, double y, double muki, double speed,
    int section, int index, int freeFlag)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = section;
    p->param_i[1] = index;
    p->param_i[2] = freeFlag;
    p->margin = 480;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 三節棍の弾幕パターン
static void ShotSansetsukon(sEnemyShotSet* pEnemyShotSet)
{
    const int    SEC_BULLETS = 4*2;                 // 1節あたりの弾数
    const double SPACING = 22.0/2;              // 弾の間隔
    const double SEC_LEN = SEC_BULLETS * SPACING; // 1節の長さ
    const double W1 = 0.030;                      // 第1節の角速度(rad/frame)
    const double W2 = 0.048;                      // 第2節(親の1.6倍)
    const double W3 = 0.075;                      // 第3節(親の2.5倍)
    const int    PHASE_A = 300;                   // 振り回し終了
    const int    PHASE_B = 420;                   // 食い止め終了
    const int    PHASE_C = 510;                   // 突き終了(1サイクル)

    // 各節の関節(付け根)位置を計算
    auto calcJoints = [&](double a1, double a2, double a3,
        double* j1, double* j2, double* j3)
    {
        j1[0] = pEnemyShotSet->x + SEC_LEN * cos(a1);
        j1[1] = pEnemyShotSet->y + SEC_LEN * sin(a1);
        j2[0] = j1[0] + SEC_LEN * cos(a2);
        j2[1] = j1[1] + SEC_LEN * sin(a2);
        j3[0] = j2[0] + SEC_LEN * cos(a3);
        j3[1] = j2[1] + SEC_LEN * sin(a3);
    };

    // 新規サイクル開始:3節分のリンク弾を作り直してプレイヤー方向へ構える
    auto startCycle = [&](int playSound)
    {
        pEnemyShotSet->param_i[1] = pEnemyShotSet->count;
        double aim = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
        pEnemyShotSet->param_d[0] = aim;
        pEnemyShotSet->param_d[1] = aim;
        pEnemyShotSet->param_d[2] = aim;
        for (int s = 1; s <= 3; s++) {
            for (int k = 1; k <= SEC_BULLETS; k++) {
                int kind = 0;
                int color = (s == 1) ? 8 : (s == 2) ? 1 : 4; // 橙/黄/青
                switch (s) {
                case 1: kind = img_enemyShotLargeBall[color];  break;
                case 2: kind = img_enemyShotMediumBall[color]; break;
                case 3: kind = img_enemyShotSmallBall[color];  break;
                }
                AddStaffShot(pEnemyShotSet, kind,
                    pEnemyShotSet->x, pEnemyShotSet->y,
                    aim, 0.0, s, k, 0);
            }
        }
        if (playSound) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    };

    // 初回のみ初期化
    if (pEnemyShotSet->param_i[0] == 0) {
        pEnemyShotSet->param_i[0] = 1;
        pEnemyShotSet->param_i[2] = GetRand(1) * 2 - 1; // 回転方向(±1)
        startCycle(1);
    }

    // 根元は敵の手元に追従
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;

    int    t = pEnemyShotSet->count - pEnemyShotSet->param_i[1];
    int    spin = pEnemyShotSet->param_i[2];
    double a1 = pEnemyShotSet->param_d[0];
    double a2 = pEnemyShotSet->param_d[1];
    double a3 = pEnemyShotSet->param_d[2];

    double j1[2], j2[2], j3[2];
    calcJoints(a1, a2, a3, j1, j2, j3);

    // ---- 各節の回転(フェーズBは角速度が反転→復帰して鞭がしなる) ----
    double scale = 1.0;
    if (t >= PHASE_A && t < PHASE_B) {
        int tB = t - PHASE_A;
        double u = (tB < 60) ? tB : 60;
        scale = -cos(DX_PI * u / 60.0);   // -1 → +1 に滑らかに反転
    }
    if (t >= PHASE_B && t < PHASE_C) {
        scale = 0.0;                       // 突き中は回転停止(角度は補間で制御)
    }
    a1 += W1 * spin * scale;
    a2 += W2 * spin * scale;
    a3 += W3 * spin * scale;

    pEnemyShotSet->param_d[0] = a1;
    pEnemyShotSet->param_d[1] = a2;
    pEnemyShotSet->param_d[2] = a3;

    // ---- フェーズB:食い止め(関節から散弾を8発ずつ放射) ----
    if (t >= PHASE_A && t < PHASE_B) {
        int tB = t - PHASE_A;
        if (tB % 30 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            double* joints[3] = { j1, j2, j3 };
            for (int j = 0; j < 3; j++) {
                double base_angle = GetRand(360) / 360.0 * DX_PI * 2;
                for (int i = 0; i < 8; i++) for (int k = 0; k < 5; k++) {
                    double a = base_angle + i / 8.0 * 2.0 * DX_PI
                        + k * 0.04;
                    double sp = 2.0 + k * 0.2;
                    AddStaffShot(pEnemyShotSet, img_enemyShotSmallBall[0],
                        joints[j][0], joints[j][1],
                        a, sp, 0, 0, 1);
                }
            }
        }
    }

    // ---- フェーズC:突き(3節を一直線に揃えてから根元から順に解き放つ) ----
    if (t >= PHASE_B && t < PHASE_C) {
        int tC = t - PHASE_B;
        if (tC == 0) {
            // 先端からプレイヤーへ突く角度を決定(角度差は最短方向に正規化)
            double thrust = atan2(player.y - j3[1], player.x - j3[0]);
            double d = thrust - a3;
            while (d > DX_PI) d -= 2.0 * DX_PI;
            while (d < -DX_PI) d += 2.0 * DX_PI;
            thrust = a3 + d;
            pEnemyShotSet->param_d[6] = thrust;
            pEnemyShotSet->param_d[7] = a1;
            pEnemyShotSet->param_d[8] = a2;
            pEnemyShotSet->param_d[9] = a3;
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        // 30フレームかけて3節を一直線へ補間
        double k = (tC >= 30) ? 1.0 : tC / 30.0;
        double target = pEnemyShotSet->param_d[6];
        pEnemyShotSet->param_d[0] = pEnemyShotSet->param_d[7] + (target - pEnemyShotSet->param_d[7]) * k;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->param_d[8] + (target - pEnemyShotSet->param_d[8]) * k;
        pEnemyShotSet->param_d[2] = pEnemyShotSet->param_d[9] + (target - pEnemyShotSet->param_d[9]) * k;

        a1 = pEnemyShotSet->param_d[0];
        a2 = pEnemyShotSet->param_d[1];
        a3 = pEnemyShotSet->param_d[2];

        // 根元側の節から順に自由飛行へ切り替えて突き出す
        for (int s = 1; s <= 3; s++) {
            if (tC == 30 + (s - 1) * 6) {
                if (s == 1) {
                    if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
                }
                sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
                while (p != pEnemyShotSet->pEnemyShotHead) {
                    if (p->param_i[2] == 0 && p->param_i[0] == s) {
                        p->param_i[2] = 1;
                        p->muki = target;
                        p->speed = 7.0;
                    }
                    p = p->next;
                }
            }
        }
    }

    // ---- サイクル終了→次のサイクルへ(回転方向を反転) ----
    if (t >= PHASE_C) {
        pEnemyShotSet->param_i[2] *= -1;
        startCycle(1);
        a1 = pEnemyShotSet->param_d[0];
        a2 = pEnemyShotSet->param_d[1];
        a3 = pEnemyShotSet->param_d[2];
    }

    // ---- 弾の配置/移動 ----
    calcJoints(a1, a2, a3, j1, j2, j3);
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[2] == 0) {
            // リンク構成弾:親節の付け根から極座標で再配置
            int s = pShot->param_i[0];
            int k = pShot->param_i[1];
            double bx, by, ang;
            if (s == 1) { bx = pEnemyShotSet->x; by = pEnemyShotSet->y; ang = a1; }
            else if (s == 2) { bx = j1[0];            by = j1[1];            ang = a2; }
            else { bx = j2[0];            by = j2[1];            ang = a3; }
            pShot->x = bx + k * SPACING * cos(ang);
            pShot->y = by + k * SPACING * sin(ang);
            pShot->muki = ang;
        }
        else {
            // 自由飛行弾(散弾・突き):等速直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Tmp()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;

        // 三節棍のセットを1本だけ生成(以降はセット内でサイクルループ)
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSansetsukon;
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
    else {
        // 左右にゆっくり揺れて棍を振り回す
        enemy.x += 0.7 * muki;
        if (enemy.x > 400.0) muki = -1;
        if (enemy.x < 80.0)  muki = 1;
    }
}