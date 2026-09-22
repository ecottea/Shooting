// EnemyPat_KochSnowflake_MetaAI.cpp - Koch雪片弾幕「フラクタル・ブリザード」
// 使用素材: 小玉(シアン), 中玉(白)

#include <functional>

struct Vec2 {
    double x, y;
};

// コッホ雪片の点列を生成
static std::vector<Vec2> GenerateKochPoints(double circumRadius, int depth)
{
    std::vector<Vec2> points;
    points.reserve(3 * (int)std::pow(4, depth) + 3);

    // 外向きが右法線になるようにCCWで初期正三角形を作成
    // -90度を上とする
    Vec2 verts[3];
    for (int i = 0; i < 3; i++) {
        double ang = -DX_PI * 0.5 + i * (2.0 * DX_PI / 3.0);
        verts[i].x = circumRadius * cos(ang);
        verts[i].y = circumRadius * sin(ang);
    }

    // 再帰ラムダ
    std::function<void(Vec2, Vec2, int, std::vector<Vec2>&)> kochEdge =
        [&](Vec2 a, Vec2 b, int d, std::vector<Vec2>& out) {
        if (d == 0) {
            out.push_back(a);
            return;
        }
        Vec2 ab{ b.x - a.x, b.y - a.y };
        Vec2 p2{ a.x + ab.x / 3.0, a.y + ab.y / 3.0 };
        Vec2 p4{ a.x + ab.x * 2.0 / 3.0, a.y + ab.y * 2.0 / 3.0 };
        Vec2 mid{ (p2.x + p4.x) * 0.5, (p2.y + p4.y) * 0.5 };

        double len = std::sqrt(ab.x * ab.x + ab.y * ab.y);
        if (len < 1e-6) { out.push_back(a); return; }
        double h = len / 3.0 * std::sqrt(3.0) * 0.5;

        // CCWなら内部が左、外側が右法線 (dy, -dx)
        double nx = ab.y / len;
        double ny = -ab.x / len;
        Vec2 p3{ mid.x + nx * h, mid.y + ny * h };

        kochEdge(a, p2, d - 1, out);
        kochEdge(p2, p3, d - 1, out);
        kochEdge(p3, p4, d - 1, out);
        kochEdge(p4, b, d - 1, out);
    };

    kochEdge(verts[0], verts[1], depth, points);
    kochEdge(verts[1], verts[2], depth, points);
    kochEdge(verts[2], verts[0], depth, points);

    return points;
}

// 弾幕本体
static void ShotKoch(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // param_i[0]=depth, param_d[0]=baseRadius, param_d[2]=累積回転, param_d[3]=拡大速度
        int depth = pSet->param_i[0];
        double baseR = pSet->param_d[0];

        // 効果音
        if (depth == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        else {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        pSet->param_d[2] = 0.0; // 累積回転
        pSet->param_d[3] = 0.12 + depth * 0.02 + 0.3; // 拡大速度
        pSet->param_d[4] = (depth % 2 == 0) ? 0.006 : -0.006; // 回転方向

        std::vector<Vec2> pts = GenerateKochPoints(baseR, depth);
        int N = (int)pts.size();
        int pow4 = 1;
        for (int i = 0; i < depth; i++) pow4 *= 4;

        // 線の密度調整: depthが低い時は辺の間に中間弾を追加して三角形を視認しやすくする
        int interp = 0;
        if (depth == 0) interp = 4;
        else if (depth == 1) interp = 2;
        else if (depth == 2) interp = 1;
        else interp = 0;

        for (int i = 0; i < N; i++) {
            Vec2 p1 = pts[i];
            Vec2 p2 = pts[(i + 1) % N];

            // 頂点弾
            {
                sEnemyShot* pShot = new sEnemyShot;
                double ang = std::atan2(p1.y, p1.x);
                double rad = std::sqrt(p1.x * p1.x + p1.y * p1.y);
                pShot->x = pSet->x + p1.x;
                pShot->y = pSet->y + p1.y;
                pShot->param_d[0] = ang; // 中心からの角度を保存
                pShot->param_d[1] = rad; // 中心からの距離を保存
                pShot->muki = 0.0;
                pShot->speed = 0.0;

                // pow4ごとに元々の頂点 = 中玉(白)で強調、それ以外は小玉(シアン)
                bool isMainVertex = (pow4 > 0 && (i % pow4 == 0));
                if (isMainVertex || depth == 0) {
                    pShot->kind = img_enemyShotMediumBall[6]; // 白
                }
                else {
                    pShot->kind = img_enemyShotSmallBall[3]; // シアン
                }

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }

            // 辺の中間を埋める小玉
            for (int k = 1; k <= interp; k++) {
                double t = (double)k / (interp + 1);
                Vec2 pm{ p1.x + (p2.x - p1.x) * t, p1.y + (p2.y - p1.y) * t };
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x + pm.x;
                pShot->y = pSet->y + pm.y;
                pShot->param_d[0] = std::atan2(pm.y, pm.x);
                pShot->param_d[1] = std::sqrt(pm.x * pm.x + pm.y * pm.y);
                pShot->muki = 0.0;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotSmallBall[3]; // シアンは輪郭用

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 回転・拡大更新
    // countはメインでインクリメントされる
    pSet->param_d[2] += pSet->param_d[4]; // 回転加算

    double addRot = pSet->param_d[2];
    double addRad = pSet->param_d[3] * pSet->count;

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double baseAng = pShot->param_d[0];
        double baseR = pShot->param_d[1];
        double ang = baseAng + addRot;
        double rad = baseR + addRad;
        pShot->x = pSet->x + rad * std::cos(ang);
        pShot->y = pSet->y + rad * std::sin(ang);
        pShot = pShot->next;
    }
}

// 敵本体
void EnemyPat_KochSnowflake_MetaAI()
{
    static int muki;
    static int depth_cycle;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        depth_cycle = 0;
    }
    else {
        // ゆっくり横移動して雪片を見やすく
        enemy.x += 0.35 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
    }

    // 120F(2秒)ごとに深度0->1->2->3と成長させる
    if (count % 120 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotKoch;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);

        int depth = depth_cycle % 4; // 0,1,2,3
        pSet->param_i[0] = depth;
        // depthが上がるほど少し大きく
        pSet->param_d[0] = 45.0 + depth * 18.0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        depth_cycle++;

        // 3週したら予告音
        if (depth == 0) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }
}