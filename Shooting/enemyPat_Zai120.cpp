// ============================================================
// 仲間割れ弾幕「決裂！同根の対決」
//  ボス1(赤)とボス2(青)が中央で決別 → 互いに撃ち合い、
//  プレイヤーは交差する弾と「とばっちり」に巻き込まれる
// ============================================================

// 色インデックス: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
#define COL_RED    0
#define COL_BLUE   4

// ----- ヘルパー:ShotSet生成 -----
static sEnemyShotSet* CreateShotSet(void(*func)(sEnemyShotSet*), double x, double y, double muki)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = func;
    p->x = x;  p->y = y;  p->muki = muki;
    p->kind = 0; // 0:ボス1(赤)弾  1:ボス2(青)弾 の識別に流用

    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;

    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
    return p;
}

// ----- ヘルパー:弾追加 -----
static void AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;  p->y = y;  p->muki = muki;  p->speed = speed;  p->kind = kind;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// ----- ヘルパー:弾移動 -----
static void MoveShots(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

static double ClampY(double y)
{
    if (y < 40.0)  return 40.0;
    if (y > 440.0) return 440.0;
    return y;
}

// ============================================================
// 弾幕パターン群
// ============================================================

// フェーズ1:決別のリング(赤青交互の全方位弾が中央から炸裂)
static void ShotPartingRing(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 24*2; i++) {
            AddShot(pSet, pSet->x, pSet->y,
                (i / 24.0/2) * 2.0 * DX_PI,
                2.2 + (i % 2) * 0.6, // 2段階速度でリングに厚みを出す
                img_enemyShotMediumBall[(i % 2) ? COL_RED : COL_BLUE]);
        }
    }
    MoveShots(pSet);
}

// フェーズ2:撃ち合い(相手ボスへ向けた3way速弾。中央で交差する)
static void ShotDuel(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = -2; i <= 2; i++) {
            AddShot(pSet, pSet->x, pSet->y,
                pSet->muki + i * 0.12, // 3way
                4.5,
                pSet->kind ? img_enemyShotSmallBall[COL_BLUE] : img_enemyShotSmallBall[COL_RED]);
        }
    }
    MoveShots(pSet);
}

// フェーズ3:大型弾(相手へゆっくり飛ぶ。回避されるとそのままプレイヤー方向へ直進=とばっちり)
static void ShotBigSlow(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        AddShot(pSet, pSet->x, pSet->y, pSet->muki, 1.2,
            pSet->kind ? img_enemyShotLargeBall[COL_BLUE] : img_enemyShotLargeBall[COL_RED]);
    }
    MoveShots(pSet);
}

// フェーズ3:巻き添え(回避した側がプレイヤーへ一撃お見舞い)
static void ShotSplashAtPlayer(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        for (int i = -3; i <= 3; i += 2) {
            AddShot(pSet, pSet->x, pSet->y,
                atan2(player.y - pSet->y, player.x - pSet->x) + i * 0.2,
                3.0,
                pSet->kind ? img_enemyShotScale[COL_BLUE] : img_enemyShotScale[COL_RED]);
        }
    }
    MoveShots(pSet);
}

// フェーズ4:すれ違いざまの全方位拡散弾
static void ShotRadial(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 16*2; i++) {
            AddShot(pSet, pSet->x, pSet->y,
                (i / 16.0/2) * 2.0 * DX_PI + pSet->muki,
                3.2,
                pSet->kind ? img_enemyShotMediumBall[COL_BLUE] : img_enemyShotMediumBall[COL_RED]);
        }
    }
    MoveShots(pSet);
}

// フェーズ4:螺旋弾(下方向へうねりながら降り注ぐ。pSet->mukiに発射時のうねり角)
static void ShotSpiralDown(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        AddShot(pSet, pSet->x, pSet->y, pSet->muki, 2.6,
            pSet->kind ? img_enemyShotDiamond[COL_BLUE] : img_enemyShotDiamond[COL_RED]);
    }
    MoveShots(pSet);
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_FallOut_Zai()
{
    const int CYCLE = 660; // 1周のフレーム数

    // ボスの揺らぎ速度・回避タイマー
    static double vy1 = 0.0, vy2 = 0.0;
    static int    dodgeTimer = 0;

    if (count == 1) {
        enemy.x = 210.0;  enemy.y = 240.0; // ボス1(赤)
        enemy.x2 = 270.0;  enemy.y2 = 240.0; // ボス2(青) ※背中合わせ
        enemy.maxHp = enemy.hp = 200;
        vy1 = vy2 = 0.0;
        dodgeTimer = 0;
    }

    int t = (count - 1) % CYCLE;

    // ---------- 移動 ----------
    if (t < 60) {
        // フェーズ1:背中合わせで決別のときを待つ(静止)
    }
    else if (t < 90) {
        // フェーズ1:互いに反対方向へ離脱ダッシュ
        enemy.x += (120.0 - enemy.x) * 0.15; // → 左1/4へ
        enemy.x2 += (360.0 - enemy.x2) * 0.15; // → 右1/4へ
    }
    else if (t < 270) {
        // フェーズ2:撃ち合い中。上下にフラフラ(軌道を読みにくく)
        if (t == 90) { vy1 = 1.2; vy2 = -1.2; }
        if (t % 45 == 0) {
            vy1 = (GetRand(200) - 100) / 100.0 * 1.6;
            vy2 = (GetRand(200) - 100) / 100.0 * 1.6;
        }
        enemy.y = ClampY(enemy.y + vy1);
        enemy.y2 = ClampY(enemy.y2 + vy2);
    }
    else if (t < 430) {
        // フェーズ3:大型弾の撃ち合い。回避中は Dodge 時に設定した vy で一気に逃げる
        if (t == 270) { vy1 = 0.6; vy2 = -0.6; dodgeTimer = 0; }

        if (dodgeTimer > 0) {
            dodgeTimer--;
        }
        else if (t % 50 == 0) {
            vy1 = (GetRand(200) - 100) / 100.0 * 1.0;
            vy2 = (GetRand(200) - 100) / 100.0 * 1.0;
        }
        enemy.y = ClampY(enemy.y + vy1);
        enemy.y2 = ClampY(enemy.y2 + vy2);
    }
    else if (t < 430 + 0) {
        // (予備)
    }
    else if (t < 460) {
        // フェーズ4:中央へ突進(すれ違いざまに拡散弾)
        enemy.x += (228.0 - enemy.x) * 0.15;
        enemy.x2 += (252.0 - enemy.x2) * 0.15;
        enemy.y += (240.0 - enemy.y) * 0.10;
        enemy.y2 += (240.0 - enemy.y2) * 0.10;
    }
    else if (t < 620) {
        // フェーズ4:中央付近で上下に揺れながら螺旋弾
        enemy.x += (200.0 - enemy.x) * 0.01; // ゆっくり外側へ開く
        enemy.x2 += (280.0 - enemy.x2) * 0.01;
        enemy.y = 240.0 + sin(t * 0.03) * 70.0;
        enemy.y2 = 240.0 - sin(t * 0.03) * 70.0;
    }
    else {
        // 周回の redeemed:初期位置(背中合わせ)へ戻る
        enemy.x += (210.0 - enemy.x) * 0.08;
        enemy.x2 += (270.0 - enemy.x2) * 0.08;
        enemy.y += (240.0 - enemy.y) * 0.08;
        enemy.y2 += (240.0 - enemy.y2) * 0.08;
    }

    // ---------- 攻撃 ----------
    if (t == 30) {
        // 決別の予告音
        if (!CheckSoundMem(sound_enemyCharge)) PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (t == 60) {
        // 決別の狼煚:2体の間(中央)からリング弾が炸裂
        CreateShotSet(ShotPartingRing, 240.0, 240.0, 0.0);
    }

    // フェーズ2:互いに向かって3way速弾(中央で交差する)
    if (t >= 90 && t < 270 && t % 15 == 0) {
        sEnemyShotSet* p;
        p = CreateShotSet(ShotDuel, enemy.x, enemy.y + 10.0,
            atan2(enemy.y2 - enemy.y, enemy.x2 - enemy.x)); // ボス1→ボス2(赤)
        p->kind = 0;
        p = CreateShotSet(ShotDuel, enemy.x2, enemy.y2 + 10.0,
            atan2(enemy.y - enemy.y2, enemy.x - enemy.x2)); // ボス2→ボス1(青)
        p->kind = 1;
    }

    // フェーズ3:大型弾の撃ち合い。狙われた側はランダム方向に回避→弾は空振りしてプレイヤー方向へ(とばっちり)
    if (t >= 280 && t < 420 && (t - 280) % 40 == 0) {
        sEnemyShotSet* p;
        if (((t - 280) / 40) % 2 == 0) {
            // ボス1が撃つ→ボス2が回避
            CreateShotSet(ShotBigSlow, enemy.x, enemy.y + 10.0,
                atan2(enemy.y2 - enemy.y, enemy.x2 - enemy.x))->kind = 0;
            vy2 = GetRand(1) ? 3.0 : -3.0; // ランダム方向へ回避!
            dodgeTimer = 25;
            p = CreateShotSet(ShotSplashAtPlayer, enemy.x2, enemy.y2, 0.0); // 巻き添え
            p->kind = 1;
        }
        else {
            // ボス2が撃つ→ボス1が回避
            CreateShotSet(ShotBigSlow, enemy.x2, enemy.y2 + 10.0,
                atan2(enemy.y - enemy.y2, enemy.x - enemy.x2))->kind = 1;
            vy1 = GetRand(1) ? 3.0 : -3.0;
            dodgeTimer = 25;
            p = CreateShotSet(ShotSplashAtPlayer, enemy.x, enemy.y, 0.0);
            p->kind = 0;
        }
    }
    if (t == 420) {
        // フェーズ4の予告音
        if (!CheckSoundMem(sound_enemyCharge)) PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // フェーズ4:すれ違いざまに全方位拡散弾
    if (t == 445) {
        CreateShotSet(ShotRadial, enemy.x, enemy.y, 0.0)->kind = 0;
        CreateShotSet(ShotRadial, enemy.x2, enemy.y2, DX_PI / 16.0)->kind = 1; // 角度をずらして弾幕に厚み
    }

    // フェーズ4:下方向への螺旋弾(2体から交互にうねりながら降り注ぐ=板挟み)
    if (t >= 460 && t < 620 && t % 2 == 0) {
        double ang = sin(t * 0.07) * 0.9; // うねり角
        sEnemyShotSet* p;
        p = CreateShotSet(ShotSpiralDown, enemy.x, enemy.y + 10.0, DX_PI / 2 - ang);
        p->kind = 0;
        p = CreateShotSet(ShotSpiralDown, enemy.x2, enemy.y2 + 10.0, DX_PI / 2 + ang);
        p->kind = 1;
    }
}