// enemyPat_qrScan.cpp
// 「符号走査陣」 - QRコードモチーフの弾幕パターン
//
// フェーズ構成:
//   1. 網目形成   … 9x9格子状のモジュール弾が上から順に走査線のように色を変えて現れる
//   2. 隅部発射   … 三隅(左上/右上/左下)のファインダーマーカーが正方形リングバーストを放つ
//   3. 走査弾幕   … モジュール弾がQR実データの格納順(蛇行スキャン)で自機狙いになって発射される
//   4. 帯状締め   … タイミングパターンを模した点線状の壁が画面端から掃引し、中央で最終バーストへ

// ============================================================
//  定数・格子ジオメトリ
// ============================================================
static const int    GRID_N = 9;    // 格子は9x9マス
static const double CELL = 20.0; // 1マスのサイズ(px)
static const double GRID_ORIGIN_X = 240.0 - (GRID_N - 1) * CELL / 2.0; // = 160.0
static const double GRID_ORIGIN_Y = 150.0;

static const int LOOP_LEN = 500; // このパターン全体の周期(フレーム数。以後繰り返す)

// ファインダーパターン(切り出しシンボル)が占める3x3セルの判定
static bool IsFinderCell(int r, int c)
{
    if (r <= 2 && c <= 2) return true;              // 左上
    if (r <= 2 && c >= GRID_N - 3) return true;      // 右上
    if (r >= GRID_N - 3 && c <= 2) return true;      // 左下
    return false;                                    // 右下は実際のQRコードと同様に空
}

// セル座標→ワールド座標
static double CellX(int c) { return GRID_ORIGIN_X + c * CELL; }
static double CellY(int r) { return GRID_ORIGIN_Y + r * CELL; }

// フェーズタイミング(グリッド生成からの相対フレーム数)
static const int    ROW_INTERVAL = 18;                        // 網目スキャン:1行ごとの間隔
static const int    TELEGRAPH_HOLD = 9 * ROW_INTERVAL + 40;      // 全行スキャン後、発射を開始するフレーム
static const int    ACTIVATE_INTERVAL = 3;                         // モジュール弾の発射間隔(蛇行順)
static const double DATA_SPEED = 2.3;                       // 発射後の弾速

// 三隅ファインダー中心座標 (0:左上 1:右上 2:左下)
static double FinderCenterX(int corner)
{
    switch (corner) {
    case 1:  return CellX(GRID_N - 2);
    default: return CellX(1);
    }
}
static double FinderCenterY(int corner)
{
    switch (corner) {
    case 2:  return CellY(GRID_N - 2);
    default: return CellY(1);
    }
}

// ファインダー発射トリガー(グリッド生成からの相対フレーム。左上→右上→左下の順)
static const int BURST_TRIGGER[3] = { 214, 234, 254 };

// ============================================================
//  格子モジュール弾を1個追加するヘルパー(フェーズ1+3で使用)
// ============================================================
static void AddModuleShot(sEnemyShotSet* pEnemyShotSet, int r, int c, int order)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = pShot->param_d[0] = CellX(c);
    pShot->y = pShot->param_d[1] = CellY(r);
    // 未発射の間は「敵本体から読み取られている」印象を出すため、敵本体の方を向かせておく
    pShot->muki = atan2(enemy.y - pShot->y, enemy.x - pShot->x);
    // 弾の種類一覧より小玉(2.5x2.5)を採用: 密集した格子でも視認・回避しやすい
    pShot->kind = img_enemyShotSmallBall[6]; // 色一覧: 6=白(未走査)
    pShot->param_i[0] = r;
    pShot->param_i[1] = c;
    pShot->param_i[2] = r * ROW_INTERVAL;                          // appearFrame(走査線が通過する時刻)
    pShot->param_i[3] = TELEGRAPH_HOLD + order * ACTIVATE_INTERVAL; // activateFrame(発射時刻)
    pShot->param_i[4] = 0;                                          // 照準確定フラグ

    pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
    pEnemyShotSet->pEnemyShotHead->prev = pShot;
}

// ============================================================
//  フェーズ1+3: 格子モジュール弾(網目形成→走査弾幕)
// ============================================================
static void ShotGridModule(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int order = 0;
        for (int r = 0; r < GRID_N; r++) {
            // QRコードの実データ格納順(蛇行スキャン)を再現: 偶数行は左→右、奇数行は右→左
            if (r % 2 == 0) {
                for (int c = 0; c < GRID_N; c++) {
                    if (IsFinderCell(r, c)) continue;
                    AddModuleShot(pEnemyShotSet, r, c, order++);
                }
            }
            else {
                for (int c = GRID_N - 1; c >= 0; c--) {
                    if (IsFinderCell(r, c)) continue;
                    AddModuleShot(pEnemyShotSet, r, c, order++);
                }
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int    appearFrame = pShot->param_i[2];
        int    activateFrame = pShot->param_i[3];
        double baseX = pShot->param_d[0];
        double baseY = pShot->param_d[1];

        if (pShot->count < appearFrame) {
            // 未走査: 白い小玉のまま格子上に静止
            pShot->kind = img_enemyShotSmallBall[6];
            pShot->x = baseX;
            pShot->y = baseY;
        }
        else if (pShot->count < activateFrame) {
            // 走査済み・発射待機: シアンの小玉に変化して強調
            pShot->kind = img_enemyShotSmallBall[3];
            pShot->x = baseX;
            pShot->y = baseY;
        }
        else {
            // 発射: 照準は発射の瞬間に1度だけ確定させ、以後は数式(count基準)で直進させる
            if (pShot->param_i[4] == 0) {
                pShot->param_d[2] = atan2(player.y - baseY, player.x - baseX);
                pShot->param_i[4] = 1;
                pShot->kind = img_enemyShotDiamond[5]; // 発射: マゼンタの菱形弾
                pShot->muki = pShot->param_d[2];       // 発射の瞬間、進行方向へ向きを切り替える
            }
            double muki = pShot->param_d[2];
            int    t = pShot->count - activateFrame;
            pShot->x = baseX + DATA_SPEED * cos(muki) * t;
            pShot->y = baseY + DATA_SPEED * sin(muki) * t;
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ1: ファインダーマーカー(点滅する警告目印)
// ============================================================
static void ShotFinderMarker(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int corner = 0; corner < 3; corner++) {
            sEnemyShot* pShot = new sEnemyShot;
            double cx = FinderCenterX(corner);
            double cy = FinderCenterY(corner);
            pShot->x = pShot->param_d[0] = cx;
            pShot->y = pShot->param_d[1] = cy;
            pShot->kind = img_enemyShotLargeBall[8]; // 大玉(20x20)・橙で目立たせる
            pShot->param_i[0] = BURST_TRIGGER[corner]; // 離脱(発射後の自然消去)を始めるフレーム
            // 離脱方向 = 画面中心から見て外向き
            double ex = cx - 240.0, ey = cy - 240.0;
            double len = sqrt(ex * ex + ey * ey);
            pShot->param_d[2] = ex / len;
            pShot->param_d[3] = ey / len;
            // 警戒中は格子(画面中心側)を睨むように、中心方向を向かせる
            pShot->muki = atan2(240.0 - cy, 240.0 - cx);

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int    escapeFrame = pShot->param_i[0];
        double baseX = pShot->param_d[0];
        double baseY = pShot->param_d[1];

        if (pShot->count < escapeFrame) {
            // 橙/赤で点滅しながら定位置で警告
            pShot->kind = ((pShot->count / 10) % 2 == 0) ? img_enemyShotLargeBall[8] : img_enemyShotLargeBall[0];
            pShot->x = baseX;
            pShot->y = baseY;
        }
        else {
            // バースト発射後、画面外へ素早く離脱してメインルーチンに自然消去させる
            int    t = pShot->count - escapeFrame;
            double ex = pShot->param_d[2], ey = pShot->param_d[3];
            pShot->x = baseX + 7.0 * t * ex;
            pShot->y = baseY + 7.0 * t * ey;
            pShot->muki = atan2(ey, ex); // 離脱の瞬間、向きも進行方向(外向き)へ切り替える
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ2: 正方形リングバースト(ファインダーパターンの入れ子構造を再現)
// ============================================================
static void ShotSquareRingBurst(sEnemyShotSet* pEnemyShotSet)
{
    static const int RING_NUM = 3+2;
    static const int POINTS_PER_RING = 16*2;

    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧より、迫力を出すため heavy を採用
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int ring = 0; ring < RING_NUM; ring++) {
            for (int k = 0; k < POINTS_PER_RING; k++) {
                double s = 4.0 * k / (double)POINTS_PER_RING; // 0〜4 (正方形の周を一周)
                double ux, uy;
                if (s < 1.0) { ux = -1.0 + 2.0 * s;        uy = -1.0; }
                else if (s < 2.0) { ux = 1.0;                   uy = -1.0 + 2.0 * (s - 1.0); }
                else if (s < 3.0) { ux = 1.0 - 2.0 * (s - 2.0); uy = 1.0; }
                else { ux = -1.0;                  uy = 1.0 - 2.0 * (s - 3.0); }

                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                // 赤/白を交互にしてファインダーパターンの黒白入れ子構造を表現(黒は視認性の都合上避ける)
                pShot->kind = img_enemyShotDiamond[(ring % 2 == 0) ? 0 : 6];
                pShot->param_d[0] = 14.0 + ring * 16.0; // 初期半径(リングごとにずらす)
                pShot->param_d[1] = 1.6 + ring * 0.3;   // 拡大速度
                pShot->param_d[2] = ux;
                pShot->param_d[3] = uy;
                pShot->muki = atan2(uy, ux); // 拡大していく方向(外向き)を向かせる

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 正方形の頂点座標(ux,uy)を半径rでスケールするだけで、相似形のまま拡大する
        double r = pShot->param_d[0] + pShot->param_d[1] * pShot->count;
        pShot->x = pEnemyShotSet->x + r * pShot->param_d[2];
        pShot->y = pEnemyShotSet->y + r * pShot->param_d[3];

        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ4: タイミングパターン掃引(点線状の壁)
// ============================================================
static void ShotTimingSweepH(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int idx = 0;
        for (double x = 0.0; x <= 480.0; x += 20.0) {
            if (idx % 5 != 0) { // 3個に1個を歯抜けにして点線(ダッシュ)状にする
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pShot->param_d[0] = x;
                pShot->y = -20.0;
                pShot->kind = img_enemyShotBullet[4]; // 銃弾(5.0x2.0)・青
                pShot->muki = DX_PI / 2.0; // 進行方向(下向き)を向かせる

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
            idx++;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0];
        pShot->y = -20.0 + 5.5 * pShot->count; // 上から下へ掃引
        pShot = pShot->next;
    }
}

static void ShotTimingSweepV(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int idx = 0;
        for (double y = 0.0; y <= 480.0; y += 20.0) {
            if (idx % 5 != 0) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = 500.0;
                pShot->y = pShot->param_d[0] = y;
                pShot->kind = img_enemyShotBullet[4];
                pShot->muki = DX_PI; // 進行方向(左向き)を向かせる

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
            idx++;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->y = pShot->param_d[0];
        pShot->x = 500.0 - 5.5 * pShot->count; // 右から左へ掃引
        pShot = pShot->next;
    }
}

// ============================================================
//  フェーズ4終盤: 中央からの最終放射バースト(「デコード完了」演出)
// ============================================================
static void ShotFinalRadialBurst(sEnemyShotSet* pEnemyShotSet)
{
    static const int SHOT_NUM = 24*2;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < SHOT_NUM; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double muki = 2.0 * DX_PI * i / SHOT_NUM;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->kind = img_enemyShotMediumBall[(i % 2 == 0) ? 3 : 6]; // シアン/白交互
            pShot->param_d[0] = cos(muki);
            pShot->param_d[1] = sin(muki);
            pShot->muki = muki; // 放射方向をそのまま向きとして設定

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double r = 3.2 * pShot->count;
        pShot->x = pEnemyShotSet->x + r * pShot->param_d[0];
        pShot->y = pEnemyShotSet->y + r * pShot->param_d[1];
        pShot = pShot->next;
    }
}

// ============================================================
//  ショットセット生成ヘルパー
// ============================================================
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    return pEnemyShotSet;
}

// ============================================================
//  敵本体パターン:「符号走査陣」
// ============================================================
void EnemyPat_QRCode_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    else {
        // 小さく上下にバウンドするだけの簡易モーション(count基準の数式駆動、速度積分は行わない)
        enemy.y = 60.0 + 5.0 * sin(count * 0.02);
    }

    int localCount = ((count - 1) % LOOP_LEN) + 1;

    // フェーズ1: 網目形成 + ファインダーマーカー出現
    if (localCount == 1) {
        // 使える効果音一覧より、予告音として enemyCharge を採用
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        CreateShotSet(ShotGridModule, 0.0, 0.0);
        CreateShotSet(ShotFinderMarker, 0.0, 0.0);
    }

    // フェーズ3開始の合図(データモジュールの発射が始まるタイミング)
    if (localCount == TELEGRAPH_HOLD + 1) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // フェーズ2: 隅部発射(左上→右上→左下の順に正方形リングバースト)
    if (localCount == BURST_TRIGGER[0] + 1) CreateShotSet(ShotSquareRingBurst, FinderCenterX(0), FinderCenterY(0));
    if (localCount == BURST_TRIGGER[1] + 1) CreateShotSet(ShotSquareRingBurst, FinderCenterX(1), FinderCenterY(1));
    if (localCount == BURST_TRIGGER[2] + 1) CreateShotSet(ShotSquareRingBurst, FinderCenterX(2), FinderCenterY(2));

    // フェーズ4: タイミングパターン掃引 + 最終放射バースト(デコード完了演出)
    if (localCount == 480-100) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        CreateShotSet(ShotTimingSweepH, 0.0, 0.0);
        CreateShotSet(ShotTimingSweepV, 0.0, 0.0);
    }
    if (localCount == 580-150) {
        CreateShotSet(ShotFinalRadialBurst, 240.0, 240.0);
    }
}