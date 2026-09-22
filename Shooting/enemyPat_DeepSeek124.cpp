// ============================================================
// 敵弾幕：コッホ雪片 -Koch Snowflake-
// ------------------------------------------------------------
// 6本のコッホ曲線（フラクタル図形）を放射状に配置し、
// ゆっくり回転・膨張させて雪の結晶のように見せる弾幕。
//
// 使用素材：
//   img_enemyShotMediumBall[3] : シアン中玉（世代0/1の頂点）
//   img_enemyShotSmallBall[3]  : シアン小玉（世代2の頂点）
//   img_enemyShotSmallBall[6]  : 白小玉（辺の中間点）
//   img_enemyShotLargeBall[6]  : 白大玉（中心核）
//   sound_enemyShot_heavy      : 発射音
//   sound_enemyCharge          : 予告音
//
// 弾は「中心からの極座標(r, θ)」と角速度・膨張速度を
// param_d[] に保持し、毎フレーム位置を再計算する。
// これにより、プールの外側で座標をいじらなくても
// 雪片全体が剛体回転・膨張しているように見せられる。
// ============================================================

// ------------------------------------------------------------
// コッホ曲線の再帰分割
//   線分 A-B を 4 本に置換し、leaf の辺を edges に格納する。
//   peakLeft = true なら山を進行方向左側に倒す。
// ------------------------------------------------------------
struct KochEdge { double x1, y1, x2, y2; };

static void KochSubdivide(
    double ax, double ay, double bx, double by, int level,
    std::vector<KochEdge>& edges, bool peakLeft)
{
    if (level <= 0) {
        edges.push_back({ ax, ay, bx, by });
        return;
    }

    const double dx = bx - ax;
    const double dy = by - ay;
    const double x1 = ax + dx / 3.0;
    const double y1 = ay + dy / 3.0;
    const double x2 = ax + 2.0 * dx / 3.0;
    const double y2 = ay + 2.0 * dy / 3.0;

    // 中央 1/3 を正三角形の山に置換
    const double ex = x2 - x1;
    const double ey = y2 - y1;
    const double ang = peakLeft ? -DX_PI / 3.0 : DX_PI / 3.0;
    const double rx = ex * cos(ang) - ey * sin(ang);
    const double ry = ex * sin(ang) + ey * cos(ang);
    const double px = x1 + rx;
    const double py = y1 + ry;

    KochSubdivide(ax, ay, x1, y1, level - 1, edges, peakLeft);
    KochSubdivide(x1, y1, px, py, level - 1, edges, peakLeft);
    KochSubdivide(px, py, x2, y2, level - 1, edges, peakLeft);
    KochSubdivide(x2, y2, bx, by, level - 1, edges, peakLeft);
}

// ------------------------------------------------------------
// 弾を1つ追加するヘルパ
// ------------------------------------------------------------
static void AddKochShot(
    sEnemyShotSet* set,
    double x, double y,
    double r, double ang,
    double angSpeed, double expSpeed,
    int kind, int typeFlag)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = ang;
    p->speed = 0.0;
    p->kind = kind;
    p->margin = 480;

    // param_d[0] = 初期半径
    // param_d[1] = 初期角度
    // param_d[2] = 角速度(rad/frame)
    // param_d[3] = 膨張速度(px/frame)
    p->param_d[0] = r;
    p->param_d[1] = ang;
    p->param_d[2] = angSpeed;
    p->param_d[3] = expSpeed;
    p->param_i[0] = typeFlag; // 0:頂点, 1:辺, 2:核

    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;
}

// ------------------------------------------------------------
// 弾幕：コッホ雪片
// ------------------------------------------------------------
static void ShotKochSnowflake(sEnemyShotSet* pEnemyShotSet)
{
    const int    ARMS = 6;
    const double R0 = 10.0*5;  // アーム内側半径
    const double R1 = 80.0*5;  // アーム外側半径
    const int    HOLD = 50;    // 静止保持フレーム
    const int    COLLAPSE_T = 180;   // 崩壊（加速）開始フレーム

    // 発射フレーム：雪片を一気に構築
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int gen = pEnemyShotSet->param_i[0];
        if (gen < 0) gen = 0;
        if (gen > 3) gen = 3;

        const double baseRot = pEnemyShotSet->param_d[0];
        const double rotDir = pEnemyShotSet->param_d[1];
        const double angSpeed = rotDir * 0.0035*2;  // 約 0.2°/frame
        const double expSpeed = 0.15;             // px/frame

        // 世代で見た目を変える
        int vertexKind, edgeKind;
        if (gen >= 2) {
            vertexKind = img_enemyShotSmallBall[3];   // シアン小玉
            edgeKind = img_enemyShotSmallBall[6];   // 白小玉
        }
        else if (gen == 1) {
            vertexKind = img_enemyShotMediumBall[3];  // シアン中玉
            edgeKind = img_enemyShotSmallBall[6];
        }
        else {
            vertexKind = img_enemyShotMediumBall[3];
            edgeKind = img_enemyShotMediumBall[6];  // 白中玉
        }

        // 1本のアーム（x軸上）のコッホ曲線を生成
        std::vector<KochEdge> localEdges;
        KochSubdivide(R0, 0.0, R1, 0.0, gen, localEdges, true);

        // 6本のアームを回転配置
        for (int arm = 0; arm < ARMS; arm++) {
            const double armAng = baseRot + arm * DX_PI / 3.0;
            const double ca = cos(armAng);
            const double sa = sin(armAng);

            for (const auto& e : localEdges) {
                // ローカル → ワールド（回転のみ、中心は後で加算）
                const double wx1 = e.x1 * ca - e.y1 * sa;
                const double wy1 = e.x1 * sa + e.y1 * ca;
                const double wx2 = e.x2 * ca - e.y2 * sa;
                const double wy2 = e.x2 * sa + e.y2 * ca;

                // --- 辺の中間点（白小玉で線を描く） ---
                const double mx = (wx1 + wx2) * 0.5;
                const double my = (wy1 + wy2) * 0.5;
                const double mr = sqrt(mx * mx + my * my);
                const double mang = atan2(my, mx);
                if (mr > 0.5) {
                    AddKochShot(pEnemyShotSet,
                        pEnemyShotSet->x + mx, pEnemyShotSet->y + my,
                        mr, mang, angSpeed, expSpeed,
                        edgeKind, 1);
                }

                // --- 辺の始点（頂点） ---
                const double r1 = sqrt(wx1 * wx1 + wy1 * wy1);
                const double a1 = atan2(wy1, wx1);
                if (r1 > 0.5) {
                    AddKochShot(pEnemyShotSet,
                        pEnemyShotSet->x + wx1, pEnemyShotSet->y + wy1,
                        r1, a1, angSpeed, expSpeed,
                        vertexKind, 0);
                }
            }
        }

        // --- 中心核（大玉） ---
        AddKochShot(pEnemyShotSet,
            pEnemyShotSet->x, pEnemyShotSet->y,
            0.0, 0.0, 0.0, 0.0,
            img_enemyShotLargeBall[6], 2);
    }

    // 更新フェーズ
    const int    t = pEnemyShotSet->count;
    const int    dt = (t > HOLD) ? (t - HOLD) : 0;
    const double accel = (t > COLLAPSE_T) ? (t - COLLAPSE_T) * 0.05 : 0.0;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 2) {
            // 核は中心に固定
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
        }
        else {
            const double r = pShot->param_d[0] + (pShot->param_d[3] + accel) * dt;
            const double ang = pShot->param_d[1] + pShot->param_d[2] * dt;
            pShot->x = pEnemyShotSet->x + r * cos(ang);
            pShot->y = pEnemyShotSet->y + r * sin(ang);
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_KochSnowflake_DeepSeek()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
    }

    // 予告音（発射30フレーム前）
    if (count >= 90 && (count - 90) % 180 == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 発射（周期 300 フレーム）
    if (count >= 120 && (count - 120) % 180 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKochSnowflake;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count % 8;

        // 世代：0 → 1 → 2 → 以降 1/2 を交互
        int gen;
        if (shot_count == 0)      gen = 0;
        else if (shot_count == 1) gen = 1;
        else if (shot_count == 2) gen = 2;
        else                      gen = 2;
        pEnemyShotSet->param_i[0] = gen;

        // 回転オフセットと回転方向（毎回少しずらす）
        pEnemyShotSet->param_d[0] = shot_count * 0.20;
        pEnemyShotSet->param_d[1] = (shot_count % 2 == 0) ? 1.0 : -1.0;

        // 弾リストのダミーヘッド
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットをリストに接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        shot_count++;
    }
}