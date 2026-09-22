// enemyPat_KochSnowflake.cpp

// 2次元座標用構造体
struct Point2D {
    double x, y;
};

// コッホ曲線の頂点を再帰的に計算する関数
// p1からp2への線分に対し、中央1/3を外側に突き出させた第 depth 世代の頂点列を生成
static void BuildKochCurve(Point2D p1, Point2D p2, int depth, std::vector<Point2D>& outPoints)
{
    if (depth == 0) {
        outPoints.push_back(p1);
        return;
    }

    Point2D v = { (p2.x - p1.x) / 3.0, (p2.y - p1.y) / 3.0 };
    Point2D a = p1;
    Point2D b = { p1.x + v.x, p1.y + v.y };
    Point2D d = { p1.x + v.x * 2.0, p1.y + v.y * 2.0 };

    // ベクトルvを外側(-60度)に回転させて三角形の頂点cを作る
    // cos(-60 deg) = 0.5, sin(-60 deg) = -sqrt(3)/2
    const double cos60 = 0.5;
    const double sin60 = -0.8660254037844386;

    Point2D c = {
        b.x + (v.x * cos60 - v.y * sin60),
        b.y + (v.x * sin60 + v.y * cos60)
    };

    BuildKochCurve(a, b, depth - 1, outPoints);
    BuildKochCurve(b, c, depth - 1, outPoints);
    BuildKochCurve(c, d, depth - 1, outPoints);
    BuildKochCurve(d, p2, depth - 1, outPoints);
}

// 弾幕：氷結晶『コッホ・スノーフレイク』
static void ShotKochSnowflake(sEnemyShotSet* pEnemyShotSet)
{
    // param_i[0] : 弾の役割 (0: 辺の小玉, 1: 頂点の中玉, 2: 直線移動弾/拡散弾)

    // ----------------------------------------------------
    // 1. 発生フェーズ (count == 0)
    // ----------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 正三角形の基本骨格を作成 (時計回り)
        const double radius = 100.0; // 雪片の基本サイズ
        Point2D tri[3];
        for (int i = 0; i < 3; i++) {
            double angle = -DX_PI / 2.0 + i * (2.0 * DX_PI / 3.0);
            tri[i].x = radius * cos(angle);
            tri[i].y = radius * sin(angle);
        }

        // コッホ雪片の頂点列を計算 (第2世代)
        std::vector<Point2D> kochPoints;
        BuildKochCurve(tri[0], tri[1], 2, kochPoints);
        BuildKochCurve(tri[1], tri[2], 2, kochPoints);
        BuildKochCurve(tri[2], tri[0], 2, kochPoints);

        const size_t numVertices = kochPoints.size();
        const int subSteps = 3; // 頂点間に配置する小玉の数

        for (size_t i = 0; i < numVertices; i++) {
            Point2D pStart = kochPoints[i];
            Point2D pEnd = kochPoints[(i + 1) % numVertices];

            // --- A. 頂点弾 (中玉: 青) ---
            {
                sEnemyShot* pShot = new sEnemyShot;
                double r = sqrt(pStart.x * pStart.x + pStart.y * pStart.y);
                double theta = atan2(pStart.y, pStart.x);

                pShot->x = pEnemyShotSet->x + pStart.x;
                pShot->y = pEnemyShotSet->y + pStart.y;
                pShot->muki = theta;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotMediumBall[4]; // 青の中玉
                pShot->param_i[0] = 1;                    // 頂点属性
                pShot->param_d[0] = r;                    // 中心からの距離
                pShot->param_d[1] = theta;                // 初期相対角度

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }

            // --- B. 辺の弾 (小玉: シアン / 白) ---
            for (int s = 1; s < subSteps; s++) {
                double t = (double)s / (double)subSteps;
                double px = pStart.x + (pEnd.x - pStart.x) * t;
                double py = pStart.y + (pEnd.y - pStart.y) * t;

                sEnemyShot* pShot = new sEnemyShot;
                double r = sqrt(px * px + py * py);
                double theta = atan2(py, px);

                pShot->x = pEnemyShotSet->x + px;
                pShot->y = pEnemyShotSet->y + py;
                pShot->muki = theta;
                pShot->speed = 0.0;
                // シアンと白を交互に配置して輝きを表現
                pShot->kind = (s % 2 == 0) ? img_enemyShotSmallBall[3] : img_enemyShotSmallBall[6];
                pShot->param_i[0] = 0; // 辺属性
                pShot->param_d[0] = r;
                pShot->param_d[1] = theta;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // ----------------------------------------------------
    // 2. 毎フレームの挙動・更新フェーズ
    // ----------------------------------------------------
    const double rotSpeed = 0.008; // 全体の回転速度 (時計回り)
    bool shootNeedle = false;

    // 形成〜回転フェーズ中で一定間隔ごとに頂点から針弾を発射
    if (pEnemyShotSet->count >= 40 && pEnemyShotSet->count <= 160 && (pEnemyShotSet->count % 10 == 0)) {
        shootNeedle = true;
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 崩壊・拡散タイマー (count == 180 で崩壊開始)
    if (pEnemyShotSet->count == 181) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    std::vector<sEnemyShot*> newNeedleShots; // 安全に新規追加するためのバッファ

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // --- A. 雪片の固定形状維持 & 回転 (count <= 180) ---
        if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
            if (pEnemyShotSet->count <= 180) {
                double currentTheta = pShot->param_d[1] + pEnemyShotSet->count * rotSpeed;
                pShot->x = pEnemyShotSet->x + pShot->param_d[0] * cos(currentTheta);
                pShot->y = pEnemyShotSet->y + pShot->param_d[0] * sin(currentTheta);
                pShot->muki = currentTheta; // 外向きの角度として更新

                // 頂点弾から針弾を射出
                if (pShot->param_i[0] == 1 && shootNeedle) {
                    sEnemyShot* pNeedle = new sEnemyShot;
                    pNeedle->x = pShot->x;
                    pNeedle->y = pShot->y;
                    pNeedle->muki = pShot->muki; // 頂点の外向き方向へ飛ばす
                    pNeedle->speed = 2.2;
                    pNeedle->kind = img_enemyShotBullet[6]; // 白の銃弾
                    pNeedle->param_i[0] = 2;               // 直線移動属性
                    newNeedleShots.push_back(pNeedle);
                }
            }
            else {
                // --- B. 崩壊フェーズへ移行 (count > 180) ---
                // 中心から外側に向かう角度とランダムな速度を設定して拡散弾へ属性変更
                pShot->muki = atan2(pShot->y - pEnemyShotSet->y, pShot->x - pEnemyShotSet->x);
                // GetRand(x) は 0〜x を返すため 0.0〜0.5 のゆらぎを加える
                pShot->speed = 1.2 + (GetRand(50) / 100.0);
                pShot->param_i[0] = 2; // 移動属性へ移行
            }
        }

        // --- C. 直線移動弾 (針弾 & 拡散後の弾) ---
        if (pShot->param_i[0] == 2) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }

    // バッファに溜めた針弾をリストに追加
    for (auto* pNeedle : newNeedleShots) {
        pNeedle->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pNeedle->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pNeedle;
        pEnemyShotSet->pEnemyShotHead->prev = pNeedle;
    }
}

// 敵本体のパターン関数
void EnemyPat_KochSnowflake_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面: 480x480
        // 雪片が美しく展開できるよう、敵を画面上部中央やや低めに配置
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200; // HP 200固定
        muki = 1;
    }
    else {
        // 敵本体の移動（左右にゆるやかに移動）
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 240フレーム（4秒）周期でコッホ雪片弾幕を生成
    if (count % 240 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKochSnowflake;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;

        // 双方向リスト頭ノードの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体リストへ挿入
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}