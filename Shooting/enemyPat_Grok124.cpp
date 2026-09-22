// enemyPat_Tmp.cpp
// 弾幕：コッホ雪片（Koch Snowflake）
// 既存の敵弾（小玉・中玉）を組み合わせてコッホ雪片の輪郭を段階的に形成し、外側へ拡大させる

// コッホ雪片の頂点を生成する（order: 0=三角形, 1=1次, 2=2次）
// outPoints[i][0]=x, outPoints[i][1]=y  （中心相対）
// 戻り値は頂点数（閉じた多角形）
static int GenerateKochSnowflake(int order, double radius, double outPoints[][2])
{
    const double PI = DX_PI;
    const double COS60 = 0.5;
    const double SIN60 = 0.8660254037844386; // sqrt(3)/2

    // 作業用バッファ（order=2 で最大 48 頂点）
    double points[64][2];
    int n = 3;

    // 初期の正三角形（反時計回り、頂点が上）
    double ang = -PI / 2.0;
    for (int i = 0; i < 3; i++) {
        points[i][0] = radius * cos(ang);
        points[i][1] = radius * sin(ang);
        ang += 2.0 * PI / 3.0;
    }

    // 指定次数まで細分化
    for (int o = 0; o < order; o++) {
        double newPoints[256][2];
        int newN = 0;
        for (int i = 0; i < n; i++) {
            double x1 = points[i][0];
            double y1 = points[i][1];
            double x2 = points[(i + 1) % n][0];
            double y2 = points[(i + 1) % n][1];
            double dx = x2 - x1;
            double dy = y2 - y1;

            // 始点
            newPoints[newN][0] = x1;
            newPoints[newN][1] = y1;
            newN++;

            // 1/3 点
            double x13 = x1 + dx / 3.0;
            double y13 = y1 + dy / 3.0;
            newPoints[newN][0] = x13;
            newPoints[newN][1] = y13;
            newN++;

            // 突起の頂点（外向き = 時計回りに60度回転）
            // ベクトル (dx/3, dy/3) を -60度回転
            double vx = dx / 3.0;
            double vy = dy / 3.0;
            double rx = vx * COS60 + vy * SIN60;   // cos(-60)=0.5, sin(-60)=-sin60 → 符号調整済
            double ry = -vx * SIN60 + vy * COS60;
            newPoints[newN][0] = x13 + rx;
            newPoints[newN][1] = y13 + ry;
            newN++;

            // 2/3 点
            double x23 = x1 + 2.0 * dx / 3.0;
            double y23 = y1 + 2.0 * dy / 3.0;
            newPoints[newN][0] = x23;
            newPoints[newN][1] = y23;
            newN++;
        }
        // コピーバック
        for (int i = 0; i < newN; i++) {
            points[i][0] = newPoints[i][0];
            points[i][1] = newPoints[i][1];
        }
        n = newN;
    }

    // 結果を出力
    for (int i = 0; i < n; i++) {
        outPoints[i][0] = points[i][0];
        outPoints[i][1] = points[i][1];
    }
    return n;
}

// 指定次数のコッホ雪片の辺上に弾を配置する
static void SpawnKochShots(sEnemyShotSet* pEnemyShotSet, int order, double radius, int colorIndex, int bulletsPerEdge, double speed)
{
    double points[64][2];
    int numPoints = GenerateKochSnowflake(order, radius, points);
    if (numPoints < 3) return;

    sEnemyShot* pEnemyShot;
    for (int i = 0; i < numPoints; i++) {
        double x1 = points[i][0];
        double y1 = points[i][1];
        double x2 = points[(i + 1) % numPoints][0];
        double y2 = points[(i + 1) % numPoints][1];

        // 辺上に等間隔で弾を置く（両端は次の辺と共有するので少し内側から）
        for (int k = 1; k <= bulletsPerEdge; k++) {
            double t = (double)k / (bulletsPerEdge + 1);
            double bx = x1 + t * (x2 - x1);
            double by = y1 + t * (y2 - y1);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + bx;
            pEnemyShot->y = pEnemyShotSet->y + by;
            // 中心から外側へ放射状に飛ばす
            pEnemyShot->muki = atan2(by, bx);
            pEnemyShot->speed = speed;
            // 弾種：小玉を基本に、低次は中玉で存在感を出す
            if (order <= 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[colorIndex];
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[colorIndex];
            }
            // リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
}

// 弾幕パターン本体
static void ShotKochSnowflake(sEnemyShotSet* pEnemyShotSet)
{
    // count==0 のとき初期化＆効果音
    if (pEnemyShotSet->count == 0) {
        // 使える効果音: sound_enemyShot_light, medium, heavy, extreme, sound_enemyCharge
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // パラメータ初期化（必要なら）
        // pEnemyShotSet->param_i[0] などを自由に使ってよい
    }

    // 段階的にコッホ雪片を形成（成長演出）
    // 第0次：大きな正三角形（白・中玉）
    if (pEnemyShotSet->count == 5) {
        SpawnKochShots(pEnemyShotSet, 0, 90.0, 6 /*白*/, 5, 1.8);
    }
    // 第1次：各辺に突起追加（シアン・小玉）
    if (pEnemyShotSet->count == 45) {
        SpawnKochShots(pEnemyShotSet, 1, 105.0, 3 /*シアン*/, 3, 1.9);
    }
    // 第2次：さらに細かい突起（青・小玉）
    if (pEnemyShotSet->count == 85) {
        SpawnKochShots(pEnemyShotSet, 2, 120.0, 4 /*青*/, 2, 2.0);
    }

    // 全弾を移動（メインルーチンで count や画面外消去は行われる）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_KochSnowflake_Grok()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.7 * (double)muki;
        if (count % 160 == 80) muki *= -1;
        // 上下にもわずかに揺らす
        enemy.y = 160.0 + 8.0 * sin(count * 0.03);
    }

    // 一定間隔でコッホ雪片弾幕を発生
    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKochSnowflake;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 5.0;
        pEnemyShotSet->muki = 0.0; // このパターンでは未使用
        pEnemyShotSet->kind = shot_count++; // 色やバリエーション用に残しておく
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}