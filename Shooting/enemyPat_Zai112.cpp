// enemyPat_tmp.cpp
// 弾幕：QRコード「データ崩壊パターン」
// フェーズ1: QRコード展開(静止弾で21x21のQRコードを形成)
// フェーズ2: スキャンライン走査(通過した列の弾が自機狙いで発射)
// フェーズ3: データ崩壊(残弾が一斉飛散+自機狙い大玉)
// フェーズ4: 誤認識弾(読み取り失敗ノイズの追尾弾)

#include "gv.h"
#include <cmath>
#include "DxLib.h"

#ifndef DX_PI
#define DX_PI 3.14159265358979
#endif

// ============================================================
//  QRコード風ビットマップの定義
// ============================================================
static const int    QR_N = 21;     // 1辺21セル(実際のQRバージョン1と同じ)
static const double CELL = 15.0;   // セル間隔
static const double QR_X0 = 240.0 - (QR_N - 1) * CELL / 2.0;  // 中央寄せ(90.0)
static const double QR_Y0 = 60.0;

// 3つの位置検出パターン(左上・右上・左下)の左上座標
static const int FINDER_POS[3][2] = { { 0, 0 }, { 14, 0 }, { 0, 14 } };

// フェーズ切替タイミング
static const int SCAN_START = 150;   // スキャンライン開始
static const int SCAN_END = 330;   // スキャン完了
static const int BURST = 331;   // 残弾一斉飛散

// (cx,cy)が位置検出パターン内ならtrue(dx,dyにパターン内相対座標を返す)
static bool InFinder(int cx, int cy, int& dx, int& dy)
{
    for (int i = 0; i < 3; i++) {
        dx = cx - FINDER_POS[i][0];
        dy = cy - FINDER_POS[i][1];
        if (dx >= 0 && dx <= 6 && dy >= 0 && dy <= 6) return true;
    }
    return false;
}

// セルが黒(弾を置く)かどうかを返す
static bool QRCellDark(int cx, int cy)
{
    int dx, dy;
    if (InFinder(cx, cy, dx, dy)) {
        // 位置検出パターン: 外周の枠 + 中心3x3 が黒
        bool ring = (dx == 0 || dx == 6 || dy == 0 || dy == 6);
        bool core = (dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4);
        return ring || core;
    }
    // タイミングパターン(6行目/6列目の市松模様)
    if (cx == 6 || cy == 6) return ((cx + cy) % 2) == 0;
    // データ領域: 固定式の疑似ランダムで約4割を黒に
    int h = (cx * 73 + cy * 151 + cx * cy * 7) % 97;
    return h < 40;
}

// 弾をリストに追加するヘルパー
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind, int type)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = 0;    // 発射済みフラグ(0:未発射 1:発射済み)
    p->param_i[1] = type; // 0:QRセル 1:検出パターン中心(大玉) 2:誤認識追尾弾
    p->param_i[3] = 0;    // 蛇行フラグ(フェーズ3で立つ)

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// ============================================================
//  QRコード弾幕の本体
// ============================================================
static void ShotQRCode(sEnemyShotSet* pEnemyShotSet)
{
    const int c = pEnemyShotSet->count;

    // ---- フェーズ1: QRコード展開(1列ずつ出現) ----
    if (c <= 61 && (c - 1) % 3 == 0) {
        if (c == 1) {
            // 展開の予告音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        int row = (c - 1) / 3;
        for (int col = 0; col < QR_N; col++) {
            if (!QRCellDark(col, row)) continue;

            int dx, dy;
            bool core = InFinder(col, row, dx, dy) &&
                dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4;

            if (core) {
                // 検出パターンの中心3x3はマゼンタの大玉1個で表現
                if (dx == 3 && dy == 3) {
                    AddShot(pEnemyShotSet,
                        QR_X0 + col * CELL, QR_Y0 + row * CELL,
                        DX_PI / 2, 0.0, img_enemyShotLargeBall[5], 1);
                }
                // 中心の残りセルは大玉がカバーするため置かない
            }
            else {
                // 通常セルは白の中玉(黒背景に白セル = 反転QRコード風)
                AddShot(pEnemyShotSet,
                    QR_X0 + col * CELL, QR_Y0 + row * CELL,
                    DX_PI / 2, 0.0, img_enemyShotMediumBall[6], 0);
            }
        }
    }

    // ---- フェーズ2: スキャンライン走査 ----
    if (c >= SCAN_START && c <= SCAN_END) {
        if (c == SCAN_START) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        pEnemyShotSet->param_d[0] += 2.0;   // スキャンラインが右へ移動
        double scanX = pEnemyShotSet->param_d[0];

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // スキャンラインに到達した未発射の弾を読み取られたように発射
            if (pShot->param_i[1] != 2 && pShot->param_i[0] == 0 &&
                pShot->x <= scanX) {
                pShot->param_i[0] = 1;
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->speed = 1.4;   // ゆっくり自機狙い
            }
            pShot = pShot->next;
        }
    }

    // ---- フェーズ3: データ崩壊(残弾の一斉飛散) ----
    if (c == BURST) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                pShot->param_i[0] = 1;
                if (pShot->param_i[1] == 1) {
                    // 検出パターンの大玉は高速自機狙いで直進
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->speed = 4.0;
                }
                else {
                    // QRセルはランダム方向へ蛇行飛散
                    pShot->param_i[3] = 1;
                    pShot->param_d[1] = GetRand(360) / 180.0 * DX_PI; // 基準角度
                    pShot->param_d[2] = GetRand(628) / 100.0;         // 蛇行位相
                    pShot->speed = 2.5 + GetRand(100) / 100.0;
                }
            }
            pShot = pShot->next;
        }
    }

    // ---- フェーズ4: 誤認識弾(追尾するノイズ) ----
    if (c > BURST && c % 40 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int n = 1 + GetRand(1);   // 1〜2個
        for (int i = 0; i < n; i++) {
            sEnemyShot* p = AddShot(pEnemyShotSet,
                20.0 + GetRand(440), -20.0,
                DX_PI / 2, 2.2, img_enemyShotSmallBall[0], 2);
            p->param_i[2] = 0;    // 追尾残り時間カウンタ
        }
    }

    // ---- 毎フレームの弾移動 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[1]) {
        case 0: // QRセル
            if (pShot->param_i[3]) {
                // 蛇行(フェーズ3の飛散弾)
                pShot->muki = pShot->param_d[1] +
                    0.7 * sin(pShot->count * 0.15 + pShot->param_d[2]);
            }
            if (pShot->param_i[0]) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            break;

        case 1: // 検出パターンの大玉(直進)
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;

        case 2: // 誤認識弾(しばらく自機を緩く追尾)
            if (pShot->param_i[2] < 180) {
                double want = atan2(player.y - pShot->y, player.x - pShot->x);
                double diff = want - pShot->muki;
                while (diff > DX_PI) diff -= 2.0 * DX_PI;
                while (diff < -DX_PI) diff += 2.0 * DX_PI;
                if (diff > 0.025) diff = 0.025;
                if (diff < -0.025) diff = -0.025;
                pShot->muki += diff;
                pShot->param_i[2]++;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_QRCode_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    else {
        // ゆっくりと揺れながらQRコードを展開する位置で待機
        enemy.x = 240.0 + 50.0 * sin(count * 0.02);
        enemy.y = 40.0 + 8.0 * sin(count * 0.05);
    }

    // QRコード弾幕のセットは最初に1回だけ生成
    if (count % 500 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotQRCode;
        pEnemyShotSet->x = 0.0;
        pEnemyShotSet->y = 0.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_d[0] = 70.0;   // スキャンラインの初期X座標

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}