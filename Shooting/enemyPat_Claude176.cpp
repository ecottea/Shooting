// ============================================================
// 弾幕「線香花火」
//   敵弾（小玉・中玉・大玉・銃弾・鱗弾・菱形弾・中楕円弾・短レーザー）だけで線香花火を表現する。
//   ① 蕾 → ② 牡丹 → ③ 松葉 → ④ 柳 → ⑤ 散り菊 の5フェーズ構成（約21秒）で、終わると繰り返す。
//
//   線香本体 : 敵本体（左上）から火球へ伸びる銃弾の列（白と黒が交互、先端は橙）
//   火球     : 大玉（橙→黄→赤→黒）＋ その周りを回る小玉（赤）
//   火花     : 小玉・中楕円弾・菱形弾・銃弾・鱗弾・短レーザー
//
//   ShotSetは1周期に1つだけ作り、その中でフェーズを管理する。
//   フェーズの切れ目で弾が0になっても消えないよう、alive を大きな値にしておき、最後に0へ戻す。
//   弾の role は param_i[0] に入れる（役割ごとに挙動を切り替える）。
// ============================================================

// ---- フェーズの開始フレーム（ShotSetのcount基準 / 60fps想定）----
constexpr int SENKO_T_PEONY = 180;   // ② 牡丹
constexpr int SENKO_T_PINE = 480;   // ③ 松葉
constexpr int SENKO_T_WILLOW = 840;   // ④ 柳
constexpr int SENKO_T_CHRYS = 1080;  // ⑤ 散り菊
constexpr int SENKO_T_END = 1260;  // 終了
constexpr int SENKO_CYCLE = 1320;  // 繰り返し周期（終了後に60フレームの間をあける）

constexpr int SENKO_FALL_START = 20;  // ⑤開始からこのフレーム後に火球が落ち始める
constexpr int SENKO_BURST = 100; // ⑤開始からこのフレームで最後の牡丹が開く

// ---- 位置 ----
constexpr double SENKO_ENEMY_X = 90.0;   // 敵本体（線香を持つ側）
constexpr double SENKO_ENEMY_Y = 40.0;
constexpr double SENKO_TIP_X = 240.0;  // 線香の先端（火球の位置）
constexpr double SENKO_TIP_Y = 150.0+100;

constexpr int SENKO_STICK_NUM = 28;      // 線香本体を作る弾の数
constexpr int SENKO_ORBIT_NUM = 8;       // 火球の周りを回る小玉の数

// ---- 弾の色 ----
constexpr int SENKO_RED = 0, SENKO_YELLOW = 1, SENKO_WHITE = 6, SENKO_BLACK = 7, SENKO_ORANGE = 8;

// ---- 弾の役割 ----
enum SenkoRole {
    SENKO_ROLE_STICK,        // 線香本体
    SENKO_ROLE_CORE,         // 火球の中心
    SENKO_ROLE_ORBIT,        // 火球の周りを回る小玉
    SENKO_ROLE_BUD,          // ① 火球に吸い込まれる弾
    SENKO_ROLE_PEONY,        // ② 牡丹の火花（減速して寿命で消える）
    SENKO_ROLE_STRAIGHT,     // 等速直進
    SENKO_ROLE_PINE_PARENT,  // ③ 松葉の親
    SENKO_ROLE_PINE_CHILD,   // ③ 松葉の子
    SENKO_ROLE_PINE_GRAND,   // ③ 松葉の孫
    SENKO_ROLE_FALL          // ④⑤ 空気抵抗と重力で垂れ落ちる弾
};

// ============================================================
//  ヘルパー
// ============================================================

// 効果音（連続再生で音が重ならないよう止めてから鳴らす）
static void SenkoPlaySound(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// 弾を1発追加してリストの末尾につなぐ
static sEnemyShot* SenkoAddShot(sEnemyShotSet* pSet, int role, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = role;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 重力で垂れ落ちる弾を追加（param_d: 0=vx, 1=vy, 2=横の抵抗, 3=縦の抵抗, 4=重力）
static sEnemyShot* SenkoAddFall(sEnemyShotSet* pSet, double x, double y, double vx, double vy, int kind,
    double dragX, double dragY, double gravity)
{
    sEnemyShot* p = SenkoAddShot(pSet, SENKO_ROLE_FALL, x, y, atan2(vy, vx), sqrt(vx * vx + vy * vy), kind);
    p->param_d[0] = vx;
    p->param_d[1] = vy;
    p->param_d[2] = dragX;
    p->param_d[3] = dragY;
    p->param_d[4] = gravity;
    return p;
}

// 弾をリストから外して解放する（呼び出し側で next を控えてから呼ぶこと）
static void SenkoRemoveShot(sEnemyShot* p)
{
    p->prev->next = p->next;
    p->next->prev = p->prev;
    delete p;
}

// 火球の色：橙（蕾・牡丹）→ 黄（松葉）→ 赤（柳・落下）→ 黒（燃え尽きる直前）
static int SenkoBallColor(int t)
{
    if (t < SENKO_T_PINE)   return SENKO_ORANGE;
    if (t < SENKO_T_WILLOW) return SENKO_YELLOW;
    if (t < SENKO_T_CHRYS + 70) return SENKO_RED;
    return SENKO_BLACK;
}

// 火球の中心位置（微かに震え、最後は揺れながら落ちる）。毎フレーム1回だけ呼ぶこと。
static void SenkoBallPos(int t, double* px, double* py)
{
    double amp;  // 震えの大きさ
    if (t < SENKO_T_PEONY)       amp = 0.3 + 1.2 * t / (double)SENKO_T_PEONY;  // 蕾：だんだん激しく
    else if (t < SENKO_T_PINE)   amp = 1.0;
    else if (t < SENKO_T_WILLOW) amp = 1.8;                                   // 松葉：最も激しい
    else if (t < SENKO_T_CHRYS)  amp = 1.0;
    else                         amp = 0.6;

    double x = SENKO_TIP_X + (GetRand(200) - 100) / 100.0 * amp;
    double y = SENKO_TIP_Y + (GetRand(200) - 100) / 100.0 * amp;

    int u = t - SENKO_T_CHRYS;
    if (u > SENKO_FALL_START) {
        double s = u - SENKO_FALL_START;
        x += sin(s * 0.12) * 5.0;       // ゆらゆら
        y += 0.5 * 0.05 * s * s;        // 重力でだんだん加速
    }
    *px = x;
    *py = y;
}

// ============================================================
//  各フェーズの弾の生成
// ============================================================

// ① 蕾：火球の周囲から小玉が渦を巻いて吸い込まれる
static void SenkoEmitBud(sEnemyShotSet* pSet, double bx, double by, int t)
{
    int ringNo = (t - 30) / 6;
    int color = (ringNo % 2 == 0) ? SENKO_RED : SENKO_ORANGE;
    double base = ringNo * 0.21;
    for (int i = 0; i < 24; i++) {
        double a = base + DX_PI * 2.0 * i / 24;
        SenkoAddShot(pSet, SENKO_ROLE_BUD, bx + 130.0 * cos(a), by + 130.0 * sin(a), a + DX_PI, 1.0,
            img_enemyShotSmallBall[color]);
    }
}

// ② 牡丹：丸く広がる小玉のリング（だんだん密になる）
static void SenkoEmitPeony(sEnemyShotSet* pSet, double bx, double by, int t)
{
    int n = t - SENKO_T_PEONY;
    int ways = 24 + n * 24 / (SENKO_T_PINE - SENKO_T_PEONY);  // 24 → 48方向
    int ringNo = n / 8;
    double base = ringNo * 0.37 + (ringNo % 2) * DX_PI / ways;
    int color = (ringNo % 2 == 0) ? SENKO_ORANGE : SENKO_YELLOW;
    for (int i = 0; i < ways; i++) {
        double a = base + DX_PI * 2.0 * i / ways;
        sEnemyShot* p = SenkoAddShot(pSet, SENKO_ROLE_PEONY, bx, by, a, 3.2, img_enemyShotSmallBall[color]);
        p->param_i[1] = 170 + GetRand(30) + GetRand(2)*100;  // 寿命（火花がぱちぱち消える）
    }
}

// ② 牡丹：ランダムに弾ける銃弾の火花
static void SenkoEmitSparks(sEnemyShotSet* pSet, double bx, double by)
{
    for (int i = 0; i < 3; i++) {
        double a = GetRand(3599) / 3600.0 * DX_PI * 2.0;
        double sp = 2.0 + GetRand(150) / 100.0;
        SenkoAddShot(pSet, SENKO_ROLE_STRAIGHT, bx, by, a, sp, img_enemyShotBullet[SENKO_ORANGE]);
    }
}

// ③ 松葉：親の中楕円弾を16方向へ。親は飛びながら枝分かれする
static void SenkoEmitPine(sEnemyShotSet* pSet, double bx, double by)
{
    SenkoPlaySound(sound_enemyShot_medium);
    double base = GetRand(3599) / 3600.0 * DX_PI * 2.0;
    for (int i = 0; i < 16; i++) {
        double a = base + DX_PI * 2.0 * i / 16 + (GetRand(10) - 5) / 100.0;
        SenkoAddShot(pSet, SENKO_ROLE_PINE_PARENT, bx, by, a, 2.4, img_enemyShotMediumOval[SENKO_YELLOW]);
    }
}

// ③ 松葉：弾pの進行方向の左右に2発の枝を出す
static void SenkoBranch(sEnemyShotSet* pSet, const sEnemyShot* p, int role, double speed, int kind)
{
    for (int side = -1; side <= 1; side += 2) {
        double a = p->muki + side * 0.44 + (GetRand(10) - 5) / 100.0;
        SenkoAddShot(pSet, role, p->x, p->y, a, speed, kind);
    }
}

// ④ 柳：全方向に飛んだ火花が減速し、しだれ柳のように垂れ落ちる
static void SenkoEmitWillow(sEnemyShotSet* pSet, double bx, double by, int t)
{
    int n = t - SENKO_T_WILLOW;
    if (n % 20 == 0) SenkoPlaySound(sound_enemyShot_light);

    const int ways = 30;
    double base = GetRand(3599) / 3600.0 * DX_PI * 2.0;
    for (int i = 0; i < ways; i++) {
        double a = base + DX_PI * 2.0 * i / ways;
        double sp = 2.5 + GetRand(200) / 100.0;
        int kind;
        switch (i % 3) {
        case 0:  kind = img_enemyShotScale[SENKO_ORANGE];     break;
        case 1:  kind = img_enemyShotSmallBall[SENKO_WHITE];  break;
        default: kind = img_enemyShotSmallBall[SENKO_YELLOW]; break;
        }
        SenkoAddFall(pSet, bx, by, sp * cos(a), sp * sin(a), kind, 0.965, 0.975, 0.05);
    }

    // 枝先に垂れる光の筋（短レーザー）
    if (n % 30 == 0) {
        for (int j = 0; j < 8; j++) {
            double a = base + DX_PI * 2.0 * j / 8 + 0.2;
            SenkoAddFall(pSet, bx, by, 3.0 * cos(a), 3.0 * sin(a), img_enemyShotLaser[SENKO_YELLOW],
                0.965, 0.975, 0.05);
        }
    }
}

// ⑤ 散り菊：火球が落ちる直前に開く小さな牡丹
static void SenkoEmitFinal(sEnemyShotSet* pSet, double bx, double by)
{
    SenkoPlaySound(sound_enemyShot_heavy);

    // 小玉の2重リング
    for (int ring = 0; ring < 2; ring++) {
        const int ways = 32;
        double sp = 1.6 + ring * 0.9;
        int color = (ring == 0) ? SENKO_ORANGE : SENKO_YELLOW;
        for (int i = 0; i < ways; i++) {
            double a = 0.1 + ring * DX_PI / ways + DX_PI * 2.0 * i / ways;
            SenkoAddShot(pSet, SENKO_ROLE_STRAIGHT, bx, by, a, sp, img_enemyShotSmallBall[color]);
        }
    }
    // 白い銃弾の放射
    for (int i = 0; i < 16; i++) {
        double a = DX_PI * 2.0 * i / 16;
        SenkoAddShot(pSet, SENKO_ROLE_STRAIGHT, bx, by, a, 3.2, img_enemyShotBullet[SENKO_WHITE]);
    }
    // 燃え残りの赤い中玉
    for (int i = 0; i < 8; i++) {
        double a = DX_PI / 8 + DX_PI * 2.0 * i / 8;
        SenkoAddShot(pSet, SENKO_ROLE_STRAIGHT, bx, by, a, 1.2, img_enemyShotMediumBall[SENKO_RED]);
    }
}

// ============================================================
//  ShotSetのパターン本体
// ============================================================
static void SenkoShotMain(sEnemyShotSet* pSet)
{
    const int t = pSet->count;

    double bx, by;
    SenkoBallPos(t, &bx, &by);

    const double ex = enemy.x, ey = enemy.y;
    const double stickMuki = atan2(SENKO_TIP_Y - ey, SENKO_TIP_X - ex);

    // 火球の見た目（大きさは蕾の間に 小→中→大 と膨らむ）
    const int ballColor = SenkoBallColor(t);
    int ballKind;
    if (t < 60)       ballKind = img_enemyShotSmallBall[ballColor];
    else if (t < 120) ballKind = img_enemyShotMediumBall[ballColor];
    else              ballKind = img_enemyShotLargeBall[ballColor];

    // 周りを回る小玉の半径と色
    const double orbitR = (t < SENKO_T_PEONY) ? 6.0 + 10.0 * t / (double)SENKO_T_PEONY
        : 16.0 + 3.0 * sin(t * 0.15);
    const int orbitColor = (t >= SENKO_T_PINE && t < SENKO_T_WILLOW) ? SENKO_ORANGE : SENKO_RED;

    // ---------- 弾の生成 ----------
    if (t == 0) {
        // 線香本体（位置は毎フレーム更新するので、ここでは仮置き）
        for (int i = 0; i < SENKO_STICK_NUM; i++) {
            int color = (i >= SENKO_STICK_NUM - 4) ? SENKO_ORANGE : ((i % 2 == 0) ? SENKO_WHITE : SENKO_BLACK);
            sEnemyShot* p = SenkoAddShot(pSet, SENKO_ROLE_STICK, ex, ey, stickMuki, 0.0, img_enemyShotBullet[color]);
            p->param_i[1] = i;
        }
        // 火球
        SenkoAddShot(pSet, SENKO_ROLE_CORE, bx, by, DX_PI / 2.0, 0.0, ballKind);
        for (int k = 0; k < SENKO_ORBIT_NUM; k++) {
            sEnemyShot* p = SenkoAddShot(pSet, SENKO_ROLE_ORBIT, bx, by, 0.0, 0.0, img_enemyShotSmallBall[orbitColor]);
            p->param_i[1] = k;
        }
        SenkoPlaySound(sound_enemyCharge);
    }
    if (t == 90) SenkoPlaySound(sound_enemyCharge);

    // ① 蕾
    if (t >= 30 && t < SENKO_T_PEONY - 5 && t % 6 == 0) SenkoEmitBud(pSet, bx, by, t);

    // ② 牡丹
    if (t >= SENKO_T_PEONY && t < SENKO_T_PINE) {
        int n = t - SENKO_T_PEONY;
        if (n % 8 == 0) {
            if (n % 16 == 0) SenkoPlaySound(sound_enemyShot_light);
            SenkoEmitPeony(pSet, bx, by, t);
        }
        if (n % 5 == 0) SenkoEmitSparks(pSet, bx, by);
    }

    // ③ 松葉
    if (t >= SENKO_T_PINE && t < SENKO_T_WILLOW && (t - SENKO_T_PINE) % 20 == 0) SenkoEmitPine(pSet, bx, by);

    // ④ 柳
    if (t >= SENKO_T_WILLOW && t < SENKO_T_CHRYS && (t - SENKO_T_WILLOW) % 10 == 0) SenkoEmitWillow(pSet, bx, by, t);

    // ⑤ 散り菊
    if (t >= SENKO_T_CHRYS) {
        int u = t - SENKO_T_CHRYS;
        // 落ちながら赤い火の粉をこぼす
        if (u >= SENKO_FALL_START && u < SENKO_BURST && u % 4 == 0) {
            for (int j = 0; j < 3; j++) {
                double vx = (GetRand(80) - 40) / 100.0;
                SenkoAddFall(pSet, bx, by, vx, 0.2, img_enemyShotSmallBall[SENKO_RED], 0.98, 1.0, 0.04);
            }
        }
        if (u == SENKO_BURST) SenkoEmitFinal(pSet, bx, by);
    }

    // ---------- 弾の更新 ----------
    const int burstFrame = SENKO_T_CHRYS + SENKO_BURST;
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        bool del = false;

        switch (pShot->param_i[0]) {
        case SENKO_ROLE_STICK: {
            // 火球が弾けたあと、先端から順に燃え尽きて消える
            int i = pShot->param_i[1];
            if (t >= SENKO_T_END || (t >= burstFrame && (t - burstFrame) >= (SENKO_STICK_NUM - 1 - i) * 2)) {
                del = true;
                break;
            }
            double r = (i + 1) / (SENKO_STICK_NUM + 1.0);
            pShot->x = ex + (SENKO_TIP_X - ex) * r;
            pShot->y = ey + (SENKO_TIP_Y - ey) * r;
            pShot->muki = stickMuki;
            break;
        }
        case SENKO_ROLE_CORE:
            if (t >= burstFrame) { del = true; break; }
            pShot->x = bx;
            pShot->y = by;
            pShot->kind = ballKind;
            break;

        case SENKO_ROLE_ORBIT: {
            if (t >= burstFrame) { del = true; break; }
            double a = t * 0.09 + pShot->param_i[1] * DX_PI * 2.0 / SENKO_ORBIT_NUM;
            pShot->x = bx + orbitR * cos(a);
            pShot->y = by + orbitR * sin(a);
            pShot->muki = a + DX_PI / 2.0;  // 進行方向（接線方向）
            pShot->speed = orbitR * 0.09;
            pShot->kind = img_enemyShotSmallBall[orbitColor];
            break;
        }
        case SENKO_ROLE_BUD: {
            double dx = bx - pShot->x, dy = by - pShot->y;
            if (sqrt(dx * dx + dy * dy) < 12.0) { del = true; break; }
            double sp = 1.0 + pShot->count * 0.03;
            if (sp > 2.8) sp = 2.8;
            pShot->speed = sp;
            pShot->muki = atan2(dy, dx) + 0.5;  // 少しずらして渦を巻くように吸い込む
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;
        }
        case SENKO_ROLE_PEONY:
            pShot->speed *= 0.96;
            if (pShot->speed < 0.9) pShot->speed = 0.9;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->count >= pShot->param_i[1]) del = true;
            break;

        case SENKO_ROLE_STRAIGHT:
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;

        case SENKO_ROLE_PINE_PARENT:
            pShot->speed *= 0.992;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->count > 0 && pShot->count % 16 == 0) {
                SenkoBranch(pSet, pShot, SENKO_ROLE_PINE_CHILD, 2.0, img_enemyShotDiamond[SENKO_ORANGE]);
            }
            if (pShot->count >= 48) del = true;
            break;

        case SENKO_ROLE_PINE_CHILD:
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->count == 12) {
                SenkoBranch(pSet, pShot, SENKO_ROLE_PINE_GRAND, 2.2, img_enemyShotBullet[SENKO_WHITE]);
            }
            if (pShot->count >= 36) del = true;
            break;

        case SENKO_ROLE_PINE_GRAND:
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->count == 30 && GetRand(2)) del = true;
            break;

        case SENKO_ROLE_FALL: {
            double vx = pShot->param_d[0] * pShot->param_d[2];
            double vy = pShot->param_d[1] * pShot->param_d[3] + pShot->param_d[4];
            pShot->param_d[0] = vx;
            pShot->param_d[1] = vy;
            pShot->x += vx;
            pShot->y += vy;
            pShot->muki = atan2(vy, vx);  // 見た目の向きは常に実際の進行方向
            pShot->speed = sqrt(vx * vx + vy * vy);
            break;
        }
        }

        // 次の弾を先に取得してから消す（この更新中に生成された弾も同フレームで更新される）
        sEnemyShot* pNext = pShot->next;
        if (del) SenkoRemoveShot(pShot);
        pShot = pNext;
    }

    // 終了：以降は残った弾が画面外に出れば、このShotSetも回収される
    if (t == SENKO_T_END) pSet->alive = 0;
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Sparkler_Claude()
{
    if (count == 1) {
        enemy.x = SENKO_ENEMY_X;
        enemy.y = SENKO_ENEMY_Y;
        enemy.maxHp = enemy.hp = 200;  // 200で固定
    }

    // 線香を持つ手が、ほんの少し揺れる
    enemy.x = SENKO_ENEMY_X + sin(count * 0.04) * 3.0;
    enemy.y = SENKO_ENEMY_Y + cos(count * 0.03) * 2.0;

    // 周期の頭で、線香花火1本分のShotSetを作る
    if ((count - 1) % SENKO_CYCLE == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = SenkoShotMain;
        pSet->x = SENKO_TIP_X;
        pSet->y = SENKO_TIP_Y;
        pSet->muki = DX_PI / 2.0;
        pSet->kind = 0;
        pSet->alive = 99999;  // フェーズの切れ目で弾が0になっても消えないようにする

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}