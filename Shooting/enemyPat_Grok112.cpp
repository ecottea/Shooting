// enemyPat_Tmp.cpp
// QRコードをモチーフにした弾幕パターン
// 既存弾（小玉・中玉・大玉など）を組み合わせてファインダーパターン＋タイミング＋データ格子を表現
// 弾の間隔を広めに取り、重なりを軽減

#include "gv.h"  // 必要な宣言はこちらを想定（実際のプロジェクトのインクルードに合わせて調整）

// 外部で定義されている想定の画像・サウンド（サンプルに合わせて使用）
// img_enemyShotSmallBall[9], img_enemyShotMediumBall[9], img_enemyShotLargeBall[9] など
// sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyCharge など

// ------------------------------------------------------------
// ヘルパー：弾をリストに追加
// ------------------------------------------------------------
static void AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->count = 0;
    pEnemyShot->margin = 20.0;
    // paramは未使用（必要なら後で拡張）
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// ------------------------------------------------------------
// ファインダーパターン（3つの角の大きな四角）を生成
// 大玉で外枠、中玉で中枠、小玉で中心 → 既存弾の組み合わせで表現
// 間隔を広げて重なりを軽減
// ------------------------------------------------------------
static void ShotFinderPatterns(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const double baseX = pEnemyShotSet->x;
        const double baseY = pEnemyShotSet->y;
        const double down = DX_PI / 2.0;  // 下方向
        const double speedBase = 1.15;

        // 3つのファインダー位置（相対）— 広めに配置
        struct FinderPos { double ox, oy; };
        FinderPos finders[3] = {
            { -110.0, -15.0 },  // 左上
            {  110.0, -15.0 },  // 右上
            { -110.0, 110.0 }   // 左下
        };

        for (int f = 0; f < 3; ++f) {
            double fx = baseX + finders[f].ox;
            double fy = baseY + finders[f].oy;

            // --- 外枠（大玉・黒） 間隔を広げた四角 ---
            const double outer = 40.0;
            // 上辺
            for (int i = -2; i <= 2; ++i) {
                AddShot(pEnemyShotSet, fx + i * 20.0, fy - outer, down, speedBase + GetRand(20) / 100.0, img_enemyShotLargeBall[7]);
            }
            // 下辺
            for (int i = -2; i <= 2; ++i) {
                AddShot(pEnemyShotSet, fx + i * 20.0, fy + outer, down, speedBase + GetRand(20) / 100.0, img_enemyShotLargeBall[7]);
            }
            // 左辺（角重複を避ける）
            for (int i = -1; i <= 1; ++i) {
                AddShot(pEnemyShotSet, fx - outer, fy + i * 20.0, down, speedBase + GetRand(20) / 100.0, img_enemyShotLargeBall[7]);
            }
            // 右辺
            for (int i = -1; i <= 1; ++i) {
                AddShot(pEnemyShotSet, fx + outer, fy + i * 20.0, down, speedBase + GetRand(20) / 100.0, img_enemyShotLargeBall[7]);
            }

            // --- 中枠（中玉・黒） 間隔広め ---
            const double mid = 24.0;
            for (int i = -1; i <= 1; ++i) {
                AddShot(pEnemyShotSet, fx + i * 24.0, fy - mid, down, speedBase + 0.05, img_enemyShotMediumBall[7]);
                AddShot(pEnemyShotSet, fx + i * 24.0, fy + mid, down, speedBase + 0.05, img_enemyShotMediumBall[7]);
            }
            AddShot(pEnemyShotSet, fx - mid, fy, down, speedBase + 0.05, img_enemyShotMediumBall[7]);
            AddShot(pEnemyShotSet, fx + mid, fy, down, speedBase + 0.05, img_enemyShotMediumBall[7]);

            // --- 中心（小玉・黒） 密集度を下げて間隔確保 ---
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    AddShot(pEnemyShotSet,
                        fx + dx * 10.0,
                        fy + dy * 10.0,
                        down,
                        speedBase + GetRand(15) / 100.0,
                        img_enemyShotSmallBall[7]);
                }
            }
        }
    }

    // 毎フレーム移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// タイミングパターン＋データ領域の格子弾幕
// 小玉を格子状に配置。黒いモジュールは密度高め、白いモジュールは疎
// セル間隔を広げて重なりを大幅に軽減
// ------------------------------------------------------------
static void ShotTimingAndData(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const double baseX = pEnemyShotSet->x;
        const double baseY = pEnemyShotSet->y + 30.0;
        const double down = DX_PI / 2.0;
        const double cell = 18.0;  // モジュール間隔を広げた
        const int gridSize = 11;   // サイズを少し抑えて全体の密度を調整

        // 簡易的な「黒いモジュール」判定（固定パターンでQRっぽく見せる）
        auto isDark = [](int gx, int gy) -> bool {
            // タイミングパターン風（中央付近の交互）
            if (gy == 5) return (gx % 2 == 0);
            if (gx == 5) return (gy % 2 == 0);

            // データ領域っぽい固定パターン（中央にアラインメント風）
            if (gx >= 4 && gx <= 6 && gy >= 4 && gy <= 6) {
                if (gx == 5 && gy == 5) return true;  // 中心黒
                if (gx == 4 || gx == 6 || gy == 4 || gy == 6) return true; // 枠
                return false;
            }
            // その他決定的パターン
            int h = (gx * 17 + gy * 31) % 7;
            return (h == 0 || h == 3 || h == 5);
        };

        for (int gy = 0; gy < gridSize; ++gy) {
            for (int gx = 0; gx < gridSize; ++gx) {
                // ファインダーが占める角はデータから除外
                if ((gx < 2 && gy < 2) || (gx > gridSize - 3 && gy < 2) || (gx < 2 && gy > gridSize - 3))
                    continue;

                double sx = baseX + (gx - gridSize / 2.0) * cell;
                double sy = baseY + (gy - 1) * cell;

                if (isDark(gx, gy)) {
                    // 黒いモジュール：密集を控えめに（1〜2発）し、オフセットを大きめに
                    int num = 1 + ((gx + gy) % 2);  // 1 or 2
                    for (int n = 0; n < num; ++n) {
                        double ox = (GetRand(8) - 4) * 0.8;
                        double oy = (GetRand(8) - 4) * 0.8;
                        AddShot(pEnemyShotSet,
                            sx + ox, sy + oy,
                            down + (GetRand(8) - 4) / 180.0 * DX_PI,
                            1.05 + GetRand(20) / 100.0,
                            img_enemyShotSmallBall[7]);  // 黒
                    }
                }
                else {
                    // 白いモジュール：さらに疎に
                    if ((gx + gy) % 6 == 0) {
                        AddShot(pEnemyShotSet,
                            sx, sy,
                            down,
                            0.95 + GetRand(15) / 100.0,
                            img_enemyShotSmallBall[6]);  // 白
                    }
                }
            }
        }

        // タイミングパターン強調（鱗弾・菱形弾）も間隔を考慮して薄く
        for (int i = 0; i < gridSize; ++i) {
            if (i % 2 == 0) {
                double tx = baseX + (i - gridSize / 2.0) * cell;
                AddShot(pEnemyShotSet, tx, baseY + 5 * cell, down, 1.1, img_enemyShotScale[7]);
                AddShot(pEnemyShotSet, baseX + 5 * cell, baseY + (i - 1) * cell, down, 1.1, img_enemyShotDiamond[7]);
            }
        }
    }

    // 移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        // 終盤で少し加速
        if (pEnemyShotSet->count > 180) {
            pShot->speed += 0.008;
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// アラインメントパターン風の小四角を追加で放つ（中盤補強）
// 間隔を広げて配置
// ------------------------------------------------------------
static void ShotAlignment(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const double baseX = pEnemyShotSet->x;
        const double baseY = pEnemyShotSet->y + 50.0;
        const double down = DX_PI / 2.0;

        // 小さめのアラインメントを2箇所、位置を離す
        struct AlignPos { double ox, oy; };
        AlignPos aligns[2] = {
            {  55.0,  60.0 },
            { -45.0, 100.0 }
        };

        for (int a = 0; a < 2; ++a) {
            double ax = baseX + aligns[a].ox;
            double ay = baseY + aligns[a].oy;

            // 外枠（中玉）間隔広め
            for (int i = -1; i <= 1; ++i) {
                AddShot(pEnemyShotSet, ax + i * 16.0, ay - 16.0, down, 1.2, img_enemyShotMediumBall[7]);
                AddShot(pEnemyShotSet, ax + i * 16.0, ay + 16.0, down, 1.2, img_enemyShotMediumBall[7]);
            }
            AddShot(pEnemyShotSet, ax - 16.0, ay, down, 1.2, img_enemyShotMediumBall[7]);
            AddShot(pEnemyShotSet, ax + 16.0, ay, down, 1.2, img_enemyShotMediumBall[7]);

            // 中心（小玉黒）
            AddShot(pEnemyShotSet, ax, ay, down, 1.25, img_enemyShotSmallBall[7]);
        }
    }

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
void EnemyPat_QRCode_Grok()
{
    static int moveDir = 1;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;  // 200で固定
        moveDir = 1;
    }
    else {
        // 緩やかに左右移動
        enemy.x += 0.7 * (double)moveDir;
        if (enemy.x < 120.0) moveDir = 1;
        if (enemy.x > 360.0) moveDir = -1;
    }

    const int T = 250;

    // フェーズ1：ファインダーパターン出現（序盤）
    if (count%T == 90-60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFinderPatterns;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // フェーズ2：タイミング＋データ格子（中盤）
    if (count % T == 200 - 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTimingAndData;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 1;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // フェーズ3：アラインメント追加（中盤後半）
    if (count % T == 280 - 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAlignment;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 2;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 以降は必要に応じて追加のばら撒きや通常弾を入れても良いが、
    // QRモチーフを崩さないようここでは抑える
}