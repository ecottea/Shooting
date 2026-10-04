// ============================================================
//  三節棍弾幕「三連棍・螺旋」
//    enemyPat_sampleForAI.cpp から派生
//
//  使用素材:
//    棍   … img_enemyShotMediumBall[色]   (第1節=シアン, 第2節=黄, 第3節=マゼンタ)
//    鎖   … img_enemyShotSmallBall[6]     (白)
//    関節 … img_enemyShotDiamond[6]       (白)
//    仕上げの自機狙い扇弾 … img_enemyShotSmallBall[0] (赤)
//
//  動き:
//    敵を基点に3本の棍が関節(鎖)を介して接続されており、
//    第1節 → 第2節 → 第3節 の順に遅延して追従することで
//    「しなる三節棍」を表現する。
//    周期的に「打ち込み」フェーズ(角速度2.4倍)が発生し、
//    鞭のようにしなり、終了時に先端から自機狙い扇弾を放つ。
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾を1発追加するヘルパ
// ------------------------------------------------------------
static void AddEnemyShot(sEnemyShotSet* pSet,
    double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->margin = 120;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// ------------------------------------------------------------
// 線分上に等間隔で弾を並べる（棍・鎖の表現用）
// 両端は少し内側に寄せて、関節弾と重ならないようにする
// ------------------------------------------------------------
static void AddLine(sEnemyShotSet* pSet,
    double x0, double y0, double x1, double y1,
    int num, double speed, int kind)
{
    double dx = x1 - x0;
    double dy = y1 - y0;
    for (int i = 0; i < num; i++) {
        double r = (double)(i + 1) / (double)(num + 1);
        double bx = x0 + dx * r;
        double by = y0 + dy * r;
        // 進行方向は「敵から外向き」の放射方向
        double muki = atan2(by - pSet->y, bx - pSet->x);
        AddEnemyShot(pSet, bx, by, muki, speed, kind);
    }
}

// ------------------------------------------------------------
// 三節棍パターン本体
// ------------------------------------------------------------
static void ShotSansetsukon(sEnemyShotSet* pEnemyShotSet)
{
    // --- 棍・鎖の寸法と関節の曲がり角 ---
    const double L1 = 55.0;   // 第1節(棍)の長さ
    const double L2 = 50.0;   // 第2節(棍)の長さ
    const double L3 = 45.0;   // 第3節(棍)の長さ
    const double Lc = 14.0;   // 鎖の長さ
    const double PHI = 0.30;   // 関節の基準曲がり角(rad)

    // --- 初期化 ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_d[0] = 0.0;        // 基点角度(第1節)
        pEnemyShotSet->param_d[1] = -PHI;       // 第2節の追従角
        pEnemyShotSet->param_d[2] = -2.0 * PHI; // 第3節の追従角
        pEnemyShotSet->param_i[0] = 0;
    }

    // --- 敵の位置に追従 ---
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    double ex = pEnemyShotSet->x;
    double ey = pEnemyShotSet->y;

    double t = (double)pEnemyShotSet->count;
    double phase = fmod(t, 300.0);

    // --- 打ち込み開始の効果音 ---
    if (t > 0.0) {
        double prevPhase = fmod(t - 1.0, 300.0);
        if (prevPhase < 240.0 && phase >= 240.0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }

    // --- 打ち込み中は角速度を上げる ---
    double omegaMul = (phase > 240.0) ? 2.4 : 1.0;

    // --- 基点角度の積分 ---
    double baseAngle = pEnemyShotSet->param_d[0];
    double dBase = (0.020 + 0.008 * sin(t * 0.05)) * omegaMul;
    baseAngle += dBase;
    pEnemyShotSet->param_d[0] = baseAngle;

    // --- 第2節・第3節の遅延追従(しなり) ---
    double lag1 = pEnemyShotSet->param_d[1];
    double lag2 = pEnemyShotSet->param_d[2];
    {
        double target1 = baseAngle - PHI;
        double d1 = target1 - lag1;
        while (d1 > DX_PI) d1 -= 2.0 * DX_PI;
        while (d1 < -DX_PI) d1 += 2.0 * DX_PI;
        lag1 += d1 * 0.12;

        double target2 = lag1 - PHI;
        double d2 = target2 - lag2;
        while (d2 > DX_PI) d2 -= 2.0 * DX_PI;
        while (d2 < -DX_PI) d2 += 2.0 * DX_PI;
        lag2 += d2 * 0.12;
    }
    pEnemyShotSet->param_d[1] = lag1;
    pEnemyShotSet->param_d[2] = lag2;

    // --- 各節点の座標 ---
    //   P0 = 敵(基点)
    //   P1 = 第1節先端
    //   P2 = 鎖1終端(= 第2節開始)
    //   P3 = 第2節先端
    //   P4 = 鎖2終端(= 第3節開始)
    //   P5 = 第3節先端
    double x1 = ex + L1 * cos(baseAngle), y1 = ey + L1 * sin(baseAngle);
    double x2 = x1 + Lc * cos(lag1), y2 = y1 + Lc * sin(lag1);
    double x3 = x2 + L2 * cos(lag1), y3 = y2 + L2 * sin(lag1);
    double x4 = x3 + Lc * cos(lag2), y4 = y3 + Lc * sin(lag2);
    double x5 = x4 + L3 * cos(lag2), y5 = y4 + L3 * sin(lag2);

    // --- 弾の発射(7フレーム間隔) ---
    if (pEnemyShotSet->count % 7 == 0) {
        // 棍1(中弾・シアン)  鎖1(小弾・白)
        AddLine(pEnemyShotSet, ex, ey, x1, y1, 5, 1.8, img_enemyShotMediumBall[3]);
        AddLine(pEnemyShotSet, x1, y1, x2, y2, 2, 1.2, img_enemyShotSmallBall[6]);
        // 棍2(中弾・黄)      鎖2(小弾・白)
        AddLine(pEnemyShotSet, x2, y2, x3, y3, 5, 1.8, img_enemyShotMediumBall[1]);
        AddLine(pEnemyShotSet, x3, y3, x4, y4, 2, 1.2, img_enemyShotSmallBall[6]);
        // 棍3(中弾・マゼンタ)
        AddLine(pEnemyShotSet, x4, y4, x5, y5, 5, 1.8, img_enemyShotMediumBall[5]);

        // 関節(菱形弾・白) を4か所に配置
        AddEnemyShot(pEnemyShotSet, x1, y1, atan2(y1 - ey, x1 - ex), 1.5, img_enemyShotDiamond[6]);
        AddEnemyShot(pEnemyShotSet, x2, y2, atan2(y2 - ey, x2 - ex), 1.5, img_enemyShotDiamond[6]);
        AddEnemyShot(pEnemyShotSet, x3, y3, atan2(y3 - ey, x3 - ex), 1.5, img_enemyShotDiamond[6]);
        AddEnemyShot(pEnemyShotSet, x4, y4, atan2(y4 - ey, x4 - ex), 1.5, img_enemyShotDiamond[6]);
    }

    // --- 打ち込み終了時、先端から自機狙い扇弾 ---
    if (t > 0.0) {
        double prevPhase = fmod(t - 1.0, 300.0);
        if (prevPhase >= 240.0 && phase < 240.0) {
            double aim = atan2(player.y - y5, player.x - x5);
            for (int i = -2; i <= 2; i++) {
                AddEnemyShot(pEnemyShotSet, x5, y5,
                    aim + i * 0.15, 2.5, img_enemyShotLargeBall[0]);
            }
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }

    // --- 弾の移動 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン(サンプル踏襲: 左右往復)
// ------------------------------------------------------------
void EnemyPat_ThreeSectionStaff_DeepSeek()
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

    // --- 三節棍弾幕を1つだけ発生させる ---
    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSansetsukon;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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