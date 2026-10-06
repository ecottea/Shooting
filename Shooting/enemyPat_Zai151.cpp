#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：サブタレイニアンローズ風
// ============================================================

#define ROSE_WAY_NUM 55   // 全方位弾のway数(N)

// HPが1/4削れる毎にランダムに変化するパラメータ
static int    g_fireIntervalT;   // T: 撃つ間隔(フレーム)
static double g_speedV;          // V: 弾速
static double g_rotateA;         // A: 大玉帯域の回転速度(rad/frame)
static double g_widthB;          // B: 大玉になる偏角帯域の幅(rad)

// 角度を 0〜2π に正規化する
static double NormalizeAngle(double a)
{
    a = fmod(a, 2.0 * DX_PI);
    if (a < 0.0) a += 2.0 * DX_PI;
    return a;
}

// 青(color==0)なら +(count*A)〜+(count*A+B)、
// 赤(color==1)なら -(count*A)〜-(count*A+B) の帯域内か判定
static bool IsInLargeBand(double th, double band, double width, int color)
{
    double d;
    if (color == 0) d = NormalizeAngle(th - band);   // +方向の帯域
    else            d = NormalizeAngle(-th - band);  // -方向の帯域(反転して同じ式になる)
    return (d <= width);
}

// 1回分のN way全方位弾(青 or 赤)を管理する弾セットのパターン関数
static void ShotSubterraneanRose(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int    color = pEnemyShotSet->param_i[0]; // 0:青 1:赤
        double baseMuki = pEnemyShotSet->param_d[0]; // ランダムに決めた基準角

        for (int i = 0; i < ROSE_WAY_NUM; i++) {
            pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = baseMuki + i * (2.0 * DX_PI / ROSE_WAY_NUM); // 全方位に等間隔
            pShot->speed = pEnemyShotSet->param_d[1];                   // 弾速V
            // とりあえず中玉で生成（直後の毎フレーム分類処理で帯域内なら大玉に変化する）
            if (color == 0) pShot->kind = img_enemyShotMediumBall[4];   // 青中玉
            else            pShot->kind = img_enemyShotMediumBall[0];   // 赤中玉
            // 弾の種類と半径一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、...
            // 弾の色一覧:   0:赤、4:青

            // リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 毎フレーム：移動 + 大玉/中玉の分類・変化
    // (pEnemyShotSet->count / pShot->count のインクリメントと画面外の弾の消去はメインルーチンで行われる)
    int    color = pEnemyShotSet->param_i[0];
    double A = pEnemyShotSet->param_d[2];
    double B = pEnemyShotSet->param_d[3];
    double band = A * count; // グローバルcountで帯域を回転させる(青赤でずれないように同期)

    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 発射中心から見た偏角
        double th = NormalizeAngle(atan2(pShot->y - pEnemyShotSet->y, pShot->x - pEnemyShotSet->x));

        if (IsInLargeBand(th, band, B, color)) {
            pShot->kind = (color == 0) ? img_enemyShotLargeBall[4]   // 青大玉
                : img_enemyShotLargeBall[0];  // 赤大玉
        }
        else {
            pShot->kind = (color == 0) ? img_enemyShotMediumBall[4]  // 青中玉
                : img_enemyShotMediumBall[0]; // 赤中玉
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SubterraneanRose_Zai()
{
    static int volleyColor;   // 次に撃つ色(0:青 1:赤) 青赤交互に撃つ
    static int fireTimer;     // 次に撃つまでのカウンタ
    static int pauseTimer;    // HP削り後の一時的な射撃停止時間
    static int phase;         // 通過済みのHP区切りの数(0〜3)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 400から変更しないこと

        // 初期パラメータ
        g_fireIntervalT = 20;    // T
        g_speedV = 2.5;   // V
        g_rotateA = 0.010; // A
        g_widthB = 0.9;   // B

        volleyColor = 0;
        fireTimer = 30;
        pauseTimer = 0;
        phase = 0;
    }
    else {
        // ゆっくり揺れながら移動
        enemy.x = 240.0 + 60.0 * sin(count * 0.02);
        enemy.y = 50.0 + 16.0 * sin(count * 0.031);

        // HPが1/4削れる毎にフェーズ移行(maxHp=400なので300/200/100が区切り)
        if (phase < 3 && enemy.hp <= enemy.maxHp * (3 - phase) / 4) {
            phase++;
            pauseTimer = 60; // 少しだけ撃つのを止める
            // T,V,A,Bをランダムに変化(GetRand(x)は0〜xのx+1種類の整数を返すので注意)
            g_fireIntervalT = 12 + GetRand(18);               // T: 12〜30フレーム間隔
            g_speedV = 2.0 + GetRand(200) / 100.0;     // V: 2.0〜4.0
            g_rotateA = (GetRand(400) - 200) / 10000.0; // A: -0.020〜+0.020 rad/frame(回転方向もランダム)
            if (fabs(g_rotateA) < 0.004) g_rotateA = 0.004;   // 回転が止まりすぎないように
            g_widthB = 0.6 + GetRand(80) / 100.0-0.4;      // B: 0.6〜1.4 rad
        }
    }

    if (pauseTimer > 0) {
        pauseTimer--; // 射撃停止中
    }
    else {
        fireTimer--;
        if (fireTimer <= 0) {
            fireTimer = g_fireIntervalT;

            // 1回分のN way全方位弾セットを生成(青赤交互、基準角は毎回ランダム)
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSubterraneanRose;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y;
            pEnemyShotSet->muki = 0;
            pEnemyShotSet->kind = volleyColor;
            pEnemyShotSet->param_i[0] = volleyColor;                     // 0:青 1:赤
            pEnemyShotSet->param_d[0] = GetRand(360) / 180.0 * DX_PI;    // ランダムな基準角
            pEnemyShotSet->param_d[1] = g_speedV;                        // 弾速V
            pEnemyShotSet->param_d[2] = g_rotateA;                       // A
            pEnemyShotSet->param_d[3] = g_widthB;                        // B
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;

            volleyColor ^= 1; // 青→赤→青→…と交互に
        }
    }
}