// enemyPat_nakamawari.cpp
// 「仲間割れ」モチーフ：ボス1とボス2が画面内を別々の軌道で移動しながら、
// 周期的にお互いへ向けて扇状の詰問弾を撃ち合い、時折どちらかが激高して
// 相手方向への密集バラマキを浴びせ、もう片方がすぐ報復する。

// カラーインデックス（弾の色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙）
static const int kColorBoss1 = 0; // 赤：ボス1
static const int kColorBoss2 = 4; // 青：ボス2

// ---------------------------------------------------------
// 弾幕：詰問の弾（相手へ向けた扇状ショット）
// pEnemyShotSet->muki      … 相手方向の角度（呼び出し側で atan2 により設定）
// pEnemyShotSet->param_i[0] … 発射元の色（kColorBoss1 / kColorBoss2）
// ---------------------------------------------------------
static void ShotConfront(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int wayCount = 7;
        const double spreadRad = 40.0 / 180.0 * DX_PI; // 扇の全開き角

        for (int i = 0; i < wayCount; i++) {
            pEnemyShot = new sEnemyShot;

            double offset = spreadRad * (i - (wayCount - 1) / 2.0) / (wayCount - 1);
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + offset; // 相手方向＝進行方向
            pEnemyShot->speed = 3.2;
            pEnemyShot->kind = img_enemyShotDiamond[pEnemyShotSet->param_i[0]];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 弾幕：激高の密集バラマキ（相手方向を中心にした広範囲の報復弾）
// pEnemyShotSet->muki      … 相手方向の角度（バラマキの中心）
// pEnemyShotSet->param_i[0] … 発射元の色
// ---------------------------------------------------------
static void ShotFury(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        const int shotCount = 240*2; // 激高＝数百発規模の密集弾幕
        const double halfSpread = 100.0 / 180.0 * DX_PI; // 相手方向を中心に±100度の広い扇

        for (int i = 0; i < shotCount; i++) {
            pEnemyShot = new sEnemyShot;

            double offset = (GetRand(2000) - 1000) / 1000.0 * halfSpread;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + offset;
            pEnemyShot->speed = (150 + GetRand(250)) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[pEnemyShotSet->param_i[0]];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------
// 発射ヘルパー：ShotSetを1つ生成してキューへつなぐ（連結リスト規約に準拠）
// ---------------------------------------------------------
static void SpawnShotSet(double x, double y, double muki, sEnemyShotSet::PatternFunc func, int colorIndex)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->param_i[0] = colorIndex;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ---------------------------------------------------------
// 敵本体のパターン：「仲間割れ」
// ボス1(enemy.x, enemy.y) とボス2(enemy.x2, enemy.y2) が
// 独立したsin波軌道で移動しつつ、時折互いに引き寄せ合う。
// 90フレーム周期で扇状の詰問弾を撃ち合い、420フレーム周期で
// どちらかが激高して密集バラマキを浴びせ、90フレーム後に
// もう片方が報復する（次周期は先手が入れ替わる）。
// ---------------------------------------------------------
void EnemyPat_FallOut_Claude()
{
    static int furyTurn;     // 0:ボス1が先に激高, 1:ボス2が先に激高
    static bool furyPending; // 報復待ちフラグ

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 120.0;
        enemy.y = 120.0;
        enemy.x2 = 360.0;
        enemy.y2 = 120.0;
        enemy.maxHp = enemy.hp = 200; // 2体で共用
        furyTurn = 0;
        furyPending = false;
    }

    // --- 移動：独立したsin波軌道 ---
    double baseX1 = 120.0 + 80.0 * sin(count * 0.010);
    double baseY1 = 120.0 + 25.0 * sin(count * 0.017);
    double baseX2 = 360.0 + 80.0 * sin(count * 0.010 + DX_PI / 2.0);
    double baseY2 = 120.0 + 25.0 * sin(count * 0.013 + DX_PI);

    // 600フレーム周期のうち120フレームだけ、言い争うように互いへ引き寄せ合う
    if (count % 600 < 120) {
        double t = count % 600;
        double pull = (60.0 - abs(t - 60.0)) / 60.0 * 0.5; // 0→0.5→0 の山形補間
        double centerX = (baseX1 + baseX2) / 2.0;
        baseX1 = baseX1 * (1.0 - pull) + centerX * pull;
        baseX2 = baseX2 * (1.0 - pull) + centerX * pull;
    }

    enemy.x = baseX1;
    enemy.y = baseY1;
    enemy.x2 = baseX2;
    enemy.y2 = baseY2;

    // --- 詰問の弾：90フレーム周期で互いへ撃ち合う ---
    if (count % 90 == 1) {
        double mukiToBoss2 = atan2(enemy.y2 - enemy.y, enemy.x2 - enemy.x);
        double mukiToBoss1 = atan2(enemy.y - enemy.y2, enemy.x - enemy.x2);
        SpawnShotSet(enemy.x, enemy.y, mukiToBoss2, ShotConfront, kColorBoss1);
        SpawnShotSet(enemy.x2, enemy.y2, mukiToBoss1, ShotConfront, kColorBoss2);
    }

    // --- 激高フェーズ：420フレーム周期でどちらかが先に激高 ---
    if (count % 420 == 1) {
        double srcX = (furyTurn == 0) ? enemy.x : enemy.x2;
        double srcY = (furyTurn == 0) ? enemy.y : enemy.y2;
        double dstX = (furyTurn == 0) ? enemy.x2 : enemy.x;
        double dstY = (furyTurn == 0) ? enemy.y2 : enemy.y;
        int color = (furyTurn == 0) ? kColorBoss1 : kColorBoss2;

        double mukiToTarget = atan2(dstY - srcY, dstX - srcX);
        SpawnShotSet(srcX, srcY, mukiToTarget, ShotFury, color);

        furyPending = true;
    }

    // --- 報復：激高の90フレーム後、もう片方が撃ち返す ---
    if (furyPending && count % 420 == 91) {
        double srcX = (furyTurn == 0) ? enemy.x2 : enemy.x;
        double srcY = (furyTurn == 0) ? enemy.y2 : enemy.y;
        double dstX = (furyTurn == 0) ? enemy.x : enemy.x2;
        double dstY = (furyTurn == 0) ? enemy.y : enemy.y2;
        int color = (furyTurn == 0) ? kColorBoss2 : kColorBoss1;

        double mukiToTarget = atan2(dstY - srcY, dstX - srcX);
        SpawnShotSet(srcX, srcY, mukiToTarget, ShotFury, color);

        furyPending = false;
        furyTurn = 1 - furyTurn; // 次周期は逆側が先に激高する
    }
}