// enemyPat_Tmp.cpp
// クラドニ図形（節線）をモチーフにした弾幕パターン
// 敵本体関数名: void EnemyPat_Chladni_Grok()
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 使える効果音: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy,
//               sound_enemyShot_extreme, sound_enemyCharge
// 弾種: SmallBall / MediumBall / LargeBall / Bullet / Scale / Diamond / MediumOval / Laser
// 色index: 0赤 1黄 2緑 3シアン 4青 5マゼンタ 6白 7黒 8橙
// GetRand(x) は 0〜x の x+1 種類を返す

// ------------------------------------------------------------
// 弾を1発生成してリストに追加する共通処理
// ------------------------------------------------------------
static void AddChladniShot(sEnemyShotSet* pEnemyShotSet,
    double px, double py,
    double muki, double spd,
    int colorIdx)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = px;
    pEnemyShot->y = py;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = spd;

    // 節線らしいシャープさを優先して菱形弾を多用、たまに小玉で密度を出す
    if (GetRand(3) == 0) {
        pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];
    }
    else {
        pEnemyShot->kind = img_enemyShotDiamond[colorIdx];
    }

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// ------------------------------------------------------------
// 弾幕パターン本体：クラドニ節線
// 節線上に弾を配置し、ゆっくり放射状に広がる。
// mode によって十字・X字・放射・花弁風の節線パターンを切り替える。
// ------------------------------------------------------------
static void ShotChladni(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 発生音
        if (CheckSoundMem(sound_enemyShot_medium)) {
            StopSoundMem(sound_enemyShot_medium);
        }
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // パターンモード（0〜3）と回転オフセット
        const int mode = pEnemyShotSet->kind % 4;
        const double rot = (GetRand(359) / 180.0) * DX_PI;  // 0〜約2π
        const double cx = pEnemyShotSet->x;
        const double cy = pEnemyShotSet->y;

        // ---- モード別節線生成 ----
        if (mode == 0) {
            // 十字（水平・垂直） + 2重同心円
            for (int a = 0; a < 4; a++) {
                double th = a * (DX_PI * 0.5) + rot;
                for (int j = 2; j <= 10*2; j++) {          // 中心付近は空けて視認性向上
                    double r = j * 14.0;
                    double px = cx + r * cos(th);
                    double py = cy + r * sin(th);
                    AddChladniShot(pEnemyShotSet, px, py, th,
                        1.35 + GetRand(40) / 100.0, 3); // シアン
                }
            }
            // 同心円 2本
            for (int ring = 0; ring < 2; ring++) {
                double r = 55.0 + ring * 45.0;
                int num = 20 + ring * 8 + 50;
                for (int i = 0; i < num; i++) {
                    double th = i * (2.0 * DX_PI / num) + rot * 0.5;
                    double px = cx + r * cos(th);
                    double py = cy + r * sin(th);
                    AddChladniShot(pEnemyShotSet, px, py, th,
                        1.1 + GetRand(30) / 100.0, 6); // 白
                }
            }
        }
        else if (mode == 1) {
            // X字（対角） + 同心円
            for (int a = 0; a < 4; a++) {
                double th = a * (DX_PI * 0.5) + DX_PI * 0.25 + rot;
                for (int j = 2; j <= 10*2; j++) {
                    double r = j * 14.0;
                    double px = cx + r * cos(th);
                    double py = cy + r * sin(th);
                    AddChladniShot(pEnemyShotSet, px, py, th,
                        1.4 + GetRand(35) / 100.0, 4); // 青
                }
            }
            for (int ring = 0; ring < 2; ring++) {
                double r = 50.0 + ring * 50.0;
                int num = 18 + ring * 10 + 50;
                for (int i = 0; i < num; i++) {
                    double th = i * (2.0 * DX_PI / num) + rot;
                    double px = cx + r * cos(th);
                    double py = cy + r * sin(th);
                    AddChladniShot(pEnemyShotSet, px, py, th,
                        1.15 + GetRand(25) / 100.0, 3);
                }
            }
        }
        else if (mode == 2) {
            // 6本放射（星型に近い） + 外側円
            for (int a = 0; a < 6*2; a++) {
                double th = a * (DX_PI / 3.0/2) + rot;
                for (int j = 2; j <= 11; j++) {
                    double r = j * 13.0;
                    double px = cx + r * cos(th);
                    double py = cy + r * sin(th);
                    AddChladniShot(pEnemyShotSet, px, py, th,
                        1.5 + GetRand(40) / 100.0, 3);
                }
            }
            {
                double r = 120.0;
                int num = 28 + 50;
                for (int i = 0; i < num; i++) {
                    double th = i * (2.0 * DX_PI / num) + rot * 0.3;
                    double px = cx + r * cos(th);
                    double py = cy + r * sin(th);
                    AddChladniShot(pEnemyShotSet, px, py, th,
                        1.0 + GetRand(20) / 100.0, 6);
                }
            }
        }
        else { // mode == 3
            // 花弁風（ローズ曲線近似の節線）
            const int k = 3;          // 3弁
            const double a = 110.0;
            for (int i = 0; i < 48*2; i++) {
                double th = i * (2.0 * DX_PI / 48.0/2) + rot;
                double r = a * fabs(cos(k * th));
                if (r < 25.0) continue;   // 中心密集回避
                double px = cx + r * cos(th);
                double py = cy + r * sin(th);
                double muki = th + (GetRand(20) - 10) / 180.0 * DX_PI;
                AddChladniShot(pEnemyShotSet, px, py, muki,
                    1.25 + GetRand(50) / 100.0,
                    (i % 2 == 0) ? 3 : 4);
            }
            // 補助の細い放射線
            for (int a = 0; a < 6*2; a++) {
                double th = a * (DX_PI / 3.0/2) + rot + 0.2;
                for (int j = 3; j <= 8*2; j++) {
                    double r = j * 16.0;
                    double px = cx + r * cos(th);
                    double py = cy + r * sin(th);
                    AddChladniShot(pEnemyShotSet, px, py, th, 1.6, 6);
                }
            }
        }
    }

    // 毎フレームの移動
    // （count のインクリメントと画面外消去はメインルーチン側で実施）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Chladni_Grok()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;   // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 緩やかな左右移動
        enemy.x += 0.85 * (double)muki;
        if (count % 150 == 75) muki *= -1;
        // 画面端保険
        if (enemy.x < 80.0) { enemy.x = 80.0;  muki = 1; }
        if (enemy.x > 400.0) { enemy.x = 400.0; muki = -1; }
    }

    // 一定間隔でクラドニ弾幕セットを生成
    // 間隔をやや長めにしてパターンが視認できるようにする
    if (count % 95 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotChladni;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 12.0;
        pEnemyShotSet->muki = 0.0;          // 本パターンでは未使用
        pEnemyShotSet->kind = shot_count++; // モード切り替え用
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}