// enemyPat_KochSnowflake.cpp

struct KochPoint {
    double x;
    double y;
};

static void AddKochShot(sEnemyShotSet* pEnemyShotSet,
    double x, double y,
    double muki, double speed,
    int kind,
    int mode = 0)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[0] = mode;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static std::vector<KochPoint> MakeKochPolyline(int iteration, double radius)
{
    const double sin60 = std::sqrt(3.0) * 0.5;
    const double cos30 = std::sqrt(3.0) * 0.5;

    // 上向きの正三角形。頂点は反時計回り。
    std::vector<KochPoint> polyline = {
        { 0.0, -radius },
        { radius * cos30, radius * 0.5 },
        {-radius * cos30, radius * 0.5 }
    };

    for (int n = 0; n < iteration; ++n) {
        std::vector<KochPoint> next;
        next.reserve(polyline.size() * 4);

        for (size_t i = 0; i < polyline.size(); ++i) {
            const KochPoint a = polyline[i];
            const KochPoint b = polyline[(i + 1) % polyline.size()];

            const double dx = (b.x - a.x) / 3.0;
            const double dy = (b.y - a.y) / 3.0;

            const KochPoint p0 = a;
            const KochPoint p1 = { a.x + dx, a.y + dy };
            const KochPoint p2 = {
                p1.x + dx * 0.5 + dy * sin60,
                p1.y - dx * sin60 + dy * 0.5
            };
            const KochPoint p3 = { a.x + dx * 2.0, a.y + dy * 2.0 };

            next.push_back(p0);
            next.push_back(p1);
            next.push_back(p2);
            next.push_back(p3);
        }

        polyline.swap(next);
    }

    return polyline;
}

static std::vector<KochPoint> MakeKochSamples(int iteration, double radius, int samplesPerSegment)
{
    const std::vector<KochPoint> polyline = MakeKochPolyline(iteration, radius);
    std::vector<KochPoint> result;
    result.reserve(polyline.size() * samplesPerSegment);

    for (size_t i = 0; i < polyline.size(); ++i) {
        const KochPoint a = polyline[i];
        const KochPoint b = polyline[(i + 1) % polyline.size()];

        for (int j = 0; j < samplesPerSegment; ++j) {
            const double t = (double)j / (double)samplesPerSegment;
            result.push_back({
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t
                });
        }
    }

    return result;
}

// 弾幕：氷晶「コッホ・スノーフレーク」
static void ShotKochSnowflake(sEnemyShotSet* pEnemyShotSet)
{
    const double radius = 105.0;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 第一段階：三角形を短レーザーで描く。
        const double cos30 = std::sqrt(3.0) * 0.5;
        const KochPoint vertex[3] = {
            { 0.0, -radius },
            { radius * cos30, radius * 0.5 },
            {-radius * cos30, radius * 0.5 }
        };

        for (int side = 0; side < 3; ++side) {
            const KochPoint a = vertex[side];
            const KochPoint b = vertex[(side + 1) % 3];
            const double dx = (b.x - a.x) / 3.0;
            const double dy = (b.y - a.y) / 3.0;
            const double sideMuki = std::atan2(dy, dx);

            for (int part = 0; part < 3; ++part) {
                const double mx = a.x + dx * (part + 0.5);
                const double my = a.y + dy * (part + 0.5);

                AddKochShot(
                    pEnemyShotSet,
                    pEnemyShotSet->x + mx,
                    pEnemyShotSet->y + my,
                    sideMuki,
                    0.0,
                    img_enemyShotLaser[6]
                );
            }
        }
    }

    // 第一段階の三角形を維持。
    if (pEnemyShotSet->count < 90-30) {
        return;
    }

    // 第二段階：一段階目のコッホ曲線を重ねる。
    if (pEnemyShotSet->count == 90-30) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const std::vector<KochPoint> points = MakeKochSamples(1, radius, 4);
        for (const KochPoint& p : points) {
            AddKochShot(
                pEnemyShotSet,
                pEnemyShotSet->x + p.x,
                pEnemyShotSet->y + p.y,
                0.0,
                0.0,
                img_enemyShotSmallBall[4]
            );
        }
    }

    // 第三段階：さらに細かな突起を大量追加して、雪片を完成させる。
    if (pEnemyShotSet->count == 180-60) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const std::vector<KochPoint> points = MakeKochSamples(2, radius, 4);
        for (const KochPoint& p : points) {
            AddKochShot(
                pEnemyShotSet,
                pEnemyShotSet->x + p.x,
                pEnemyShotSet->y + p.y,
                0.0,
                0.0,
                img_enemyShotSmallBall[3]
            );
        }
    }

    // 完成した輪郭が一拍静止したあと、すべての弾が中心へ吸い込まれる。
    if (pEnemyShotSet->count == 300-90) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            const double dx = pEnemyShotSet->x - pShot->x;
            const double dy = pEnemyShotSet->y - pShot->y;

            pShot->muki = std::atan2(dy, dx);
            pShot->speed = 1.8 + std::sqrt(dx * dx + dy * dy) * 0.004;
            pShot->kind = img_enemyShotDiamond[4];
            pShot->param_i[0] = 1;

            pShot = pShot->next;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            pShot->x += pShot->speed * std::cos(pShot->muki);
            pShot->y += pShot->speed * std::sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_KochSnowflake_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 152.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.85 * (double)muki;
        if (count % 180 == 90) muki *= -1;

        enemy.y = 152.0 + 18.0 * std::sin((double)count * 0.018);
    }

    // 雪片が前のサイクルの形を崩し切ったあとに次の雪片を生成する。
    if (count % 280 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKochSnowflake;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
