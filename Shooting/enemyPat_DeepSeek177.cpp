// ============================================================
// 弾幕：スペルカード「勾股転置」
//
// 三平方の定理の証明をモチーフにしたパターン。
// 四つの合同な 3-4-5 直角三角形を既存の敵弾で描き、
// 配置A（中央に斜辺の平方 c^2 の空白正方形）から
// 配置B（左上に a^2、右下に b^2 の二つの空白正方形）
// へ、四つの三角形を平行移動で転置する。
// 1サイクル＝描画→転置→保持→解散 を繰り返す。
//
// 使用素材:
//   短辺 a: img_enemyShotBullet[0]     (赤銃弾)
//   長辺 b: img_enemyShotBullet[4]     (青銃弾)
//   斜辺 c: img_enemyShotSmallBall[6]  (白小玉)
//   援護弾: img_enemyShotSmallBall[0,1,3], img_enemyShotDiamond[5]
// ============================================================

static void ShotPythagoras(sEnemyShotSet* pEnemyShotSet)
{
    // ---- サイクル定数 ----
    const int T_DRAW = 60;   // 配置A：描画
    const int T_TRANSPOSE = 90;   // 転置 A→B
    const int T_HOLD = 60;   // 配置B：保持
    const int T_DISSOLVE = 40+40;   // 解散
    const int CYCLE_LEN = T_DRAW + T_TRANSPOSE + T_HOLD + T_DISSOLVE; // 250

    int frame = pEnemyShotSet->count;
    int fc = frame % CYCLE_LEN;
    int cycle = frame / CYCLE_LEN;

    // ---- 図形定数 ----
    const double U = 40.0/280*460;   // 1単位＝40px
    const double OX = 100.0-90;  // 図形原点
    const double OY = 100.0-90;
    const double SPACING = 6.0*2.5;    // 弾間隔（線に見える密度）

    // ============================================================
    // サイクル開始：三角形弾の生成
    // ============================================================
    if (fc == 0) {
        if (cycle == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 三角形の頂点（A配置）と（B配置）
        const double triAX[4][3] = {
            {0,3,0}, {3,7,7}, {7,7,4}, {0,4,0}
        };
        const double triAY[4][3] = {
            {0,0,4}, {0,0,3}, {3,7,7}, {4,7,7}
        };
        const double triBX[4][3] = {
            {0,3,0}, {3,7,7}, {3,3,0}, {3,7,3}
        };
        const double triBY[4][3] = {
            {3,3,7}, {0,0,3}, {3,7,7}, {0,3,3}
        };
        // 各辺の頂点ペア（頂点番号）: {短辺a, 長辺b, 斜辺c}
        const int edges[4][3][2] = {
            { {0,1}, {0,2}, {1,2} }, // T0: 右角 (0,0)
            { {1,2}, {0,1}, {0,2} }, // T1: 右角 (7,0)
            { {1,2}, {0,1}, {0,2} }, // T2: 右角 (7,7)
            { {0,2}, {1,2}, {0,1} }, // T3: 右角 (0,7)
        };
        const int edgeKind[3] = {
            img_enemyShotBullet[0],    // 短辺a
            img_enemyShotBullet[4],    // 長辺b
            img_enemyShotSmallBall[6], // 斜辺c
        };

        for (int t = 0; t < 4; t++) {
            for (int e = 0; e < 3; e++) {
                int i0 = edges[t][e][0];
                int i1 = edges[t][e][1];
                double ax1 = OX + triAX[t][i0] * U;
                double ay1 = OY + triAY[t][i0] * U;
                double ax2 = OX + triAX[t][i1] * U;
                double ay2 = OY + triAY[t][i1] * U;
                double bx1 = OX + triBX[t][i0] * U;
                double by1 = OY + triBY[t][i0] * U;
                double bx2 = OX + triBX[t][i1] * U;
                double by2 = OY + triBY[t][i1] * U;

                double ddx = ax2 - ax1;
                double ddy = ay2 - ay1;
                double len = sqrt(ddx * ddx + ddy * ddy);
                double muki = atan2(ddy, ddx); // ← 辺の向きに合わせる

                int n = (int)(len / SPACING) + 1;
                if (n < 2) n = 2;

                for (int i = 0; i < n; i++) {
                    double s = (double)i / (double)(n - 1);
                    sEnemyShot* p = new sEnemyShot;
                    p->kind = edgeKind[e];
                    p->speed = 0.0;
                    p->muki = muki;   // 辺の向き
                    p->x = ax1 + (ax2 - ax1) * s;
                    p->y = ay1 + (ay2 - ay1) * s;
                    p->param_i[0] = 0;      // 三角形弾マーカー
                    p->param_i[1] = t;      // 三角形番号
                    p->param_i[2] = e;      // 辺番号
                    p->param_i[3] = cycle;  // サイクルID
                    p->param_d[0] = p->x;   // A位置X
                    p->param_d[1] = p->y;   // A位置Y
                    p->param_d[2] = bx1 + (bx2 - bx1) * s; // B位置X
                    p->param_d[3] = by1 + (by2 - by1) * s; // B位置Y
                    p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    p->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = p;
                    pEnemyShotSet->pEnemyShotHead->prev = p;
                }
            }
        }
    }

    // ============================================================
    // フェーズ境界イベント
    // ============================================================
    if (fc == T_DRAW) {
        // 転置開始
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    if (fc == T_DRAW + T_TRANSPOSE + T_HOLD) {
        // 解散開始：三角形弾に外向きの速度と向きを与える
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 0 && p->param_i[3] == cycle) {
                //double dx = p->x - 240.0;
                //double dy = p->y - 240.0;
                //if (dx * dx + dy * dy < 100.0) {
                //    p->muki = GetRand(359) / 180.0 * DX_PI;
                //}
                //else {
                //    p->muki = atan2(dy, dx);
                //}
                p->muki = DX_PI / 2 + (GetRand(360) - 180) / 3600.0 * DX_PI;
                p->speed = 3.0 + GetRand(150) / 100.0-1; // 3.0 ～ 4.5
                p->param_i[0] = 2; // 以降は自由飛翔
            }
            p = p->next;
        }
    }

    // ============================================================
    // 位置更新
    // ============================================================
    {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0 && pShot->param_i[3] == cycle) {
                // 現在サイクルの制御対象
                double ax = pShot->param_d[0];
                double ay = pShot->param_d[1];
                double bx = pShot->param_d[2];
                double by = pShot->param_d[3];

                if (fc < T_DRAW) {
                    pShot->x = ax;
                    pShot->y = ay;
                }
                else if (fc < T_DRAW + T_TRANSPOSE) {
                    double t = (double)(fc - T_DRAW) / (double)T_TRANSPOSE;
                    double s = t * t * (3.0 - 2.0 * t);
                    pShot->x = ax + (bx - ax) * s;
                    pShot->y = ay + (by - ay) * s;
                }
                else if (fc < T_DRAW + T_TRANSPOSE + T_HOLD) {
                    pShot->x = bx;
                    pShot->y = by;
                }
                else {
                    pShot->x += pShot->speed * cos(pShot->muki);
                    pShot->y += pShot->speed * sin(pShot->muki);
                }
            }
            else {
                // 援護弾、または前サイクルの飛翔中弾
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }

    // ============================================================
    // 援護弾・リング弾
    // ============================================================
    // 描画中：3-way 弱い自機狙い
    if (fc > 0 && fc < T_DRAW && fc % 25 == 0) {
        int base = (int)(atan2(player.y - enemy.y, player.x - enemy.x) / DX_PI * 180.0);
        for (int k = -1; k <= 1; k++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = enemy.x;
            p->y = enemy.y + 10.0;
            p->muki = (base + k * 20) / 180.0 * DX_PI;
            p->speed = 1.8;
            p->kind = img_enemyShotSmallBall[1];
            p->param_i[0] = 1;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }
    // 転置中：3-way 自機狙い
    if (fc >= T_DRAW && fc < T_DRAW + T_TRANSPOSE
        && (fc - T_DRAW) % 15 == 0) {
        int base = (int)(atan2(player.y - enemy.y, player.x - enemy.x) / DX_PI * 180.0);
        for (int k = -1; k <= 1; k++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = enemy.x;
            p->y = enemy.y + 10.0;
            p->muki = (base + k * 12) / 180.0 * DX_PI;
            p->speed = 2.0;
            p->kind = img_enemyShotSmallBall[0];
            p->param_i[0] = 1;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }
    // 転置完了：菱形リング
    if (fc == T_DRAW + T_TRANSPOSE) {
        int n = 16;
        for (int i = 0; i < n; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = 240.0;
            p->y = 240.0;
            p->muki = i * 2.0 * DX_PI / n;
            p->speed = 1.6;
            p->kind = img_enemyShotDiamond[5];
            p->param_i[0] = 1;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }
    // 保持中：リング
    if (fc > T_DRAW + T_TRANSPOSE && fc < T_DRAW + T_TRANSPOSE + T_HOLD
        && (fc - T_DRAW - T_TRANSPOSE) % 40 == 0) {
        int n = 12;
        double offset = GetRand(359) / 180.0 * DX_PI;
        for (int i = 0; i < n; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = 240.0;
            p->y = 240.0;
            p->muki = offset + i * 2.0 * DX_PI / n;
            p->speed = 1.4;
            p->kind = img_enemyShotSmallBall[3];
            p->param_i[0] = 1;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_PythagoreanTheorem_DeepSeek()
{
    static int  muki;
    static bool setCreated;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        setCreated = false;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x += 0.3 * (double)muki;
        if (enemy.x > 270.0) muki = -1;
        if (enemy.x < 210.0) muki = 1;
    }

    // パターン開始時に一度だけセットを生成。
    // セット内でサイクルを繰り返すため、alive を十分大きくする。
    if (!setCreated) {
        setCreated = true;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPythagoras;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->alive = 7200; // 約120秒ぶん反復

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}