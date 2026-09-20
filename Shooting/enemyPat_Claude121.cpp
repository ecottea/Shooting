// ============================================================
// 弾幕：カルマン渦
//
// ボスが左右に往復し、反転するたびに「予告」を挟んでから
// 反対側の後方(下側)へ渦を1つ剥がして放つ。
// 渦は中心から二重螺旋を描きながら広がりつつ、
// ゆっくり下流(自機側)へ流されていく。
// 渦の回転方向(渦度の符号)は放出のたびに反転し、
// 実際のカルマン渦列と同じく、交互に逆回転する渦が
// 左右千鳥状に並んで下流へ連なっていく。
// ============================================================

// 弾幕：渦放出の予告（次にどちら側から渦が剥がれるかを示す小玉の広がり）
static void ShotVortexTelegraph(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(360) - 30) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.3 + GetRand(40) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄色 = 予告

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

// 弾幕：渦本体（二重螺旋を描いて広がりながら下流へ流れる渦弾群）
static void ShotVortex(sEnemyShotSet* pEnemyShotSet)
{
    const int    SPAWN_INTERVAL = 4;      // 何フレームおきに螺旋弾を追加するか
    const int    SPAWN_END = 160;    // このフレーム数まで追加を続ける
    const double ANGLE_STEP = DX_PI / 12.0; // 追加ごとに進める角度(15度)
    const double ORBIT_OMEGA = 0.028/3;  // 弾自身が飛びながら曲がる角速度
    const double RADIAL_SPEED = 1.05;   // 中心から広がる速さ
    const double R0 = 6.0;    // 生成直後の初期半径(中心での重なり回避)
    const double TRANSPORT_SPEED = 0.55;   // 渦本体が下流(自機側)へ流される速さ

    int dir = pEnemyShotSet->param_i[0]; // +1:時計回り、-1:反時計回り = 渦度の符号

    // 渦本体を下流へゆっくり移送する
    pEnemyShotSet->y += TRANSPORT_SPEED;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 螺旋弾を一定間隔で追加していく(2本の腕を持つ二重螺旋として展開)
    if (pEnemyShotSet->count % SPAWN_INTERVAL == 0 && pEnemyShotSet->count <= SPAWN_END) {
        double baseTheta = (pEnemyShotSet->count / SPAWN_INTERVAL) * ANGLE_STEP * dir;

        for (int arm = 0; arm < 2; arm++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->speed = 0.0; // 速度は使わず、毎フレーム座標を直接計算する
            // 渦度の符号を色で表現：時計回り=赤系、反時計回り=青系
            pEnemyShot->kind = (dir > 0) ? img_enemyShotScale[0] : img_enemyShotScale[4];
            pEnemyShot->param_d[0] = baseTheta + arm * DX_PI; // 初期角度(腕ごとにπずらす)
            pEnemyShot->param_i[0] = dir;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存の螺旋弾を更新：中心からの半径と角度を経過フレーム数から直接計算する
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double theta = pShot->param_d[0] + ORBIT_OMEGA * t * pShot->param_i[0];
        double r = R0 + RADIAL_SPEED * t;

        pShot->x = pEnemyShotSet->x + r * cos(theta);
        pShot->y = pEnemyShotSet->y + r * sin(theta);
        // muki は速度ベクトルではなく、渦の接線方向(見た目の流れ)に合わせる
        pShot->muki = theta + pShot->param_i[0] * DX_PI / 2.0;

        pShot = pShot->next;
    }
}

// 敵本体のパターン：カルマン渦
void EnemyPat_KarmanVortex_Claude()
{
    const int    OSC_PERIOD = 75;   // ボスの半周期(フレーム)。渦の放出周期でもある
    const double OSC_SPEED = 0.9;  // ボスの左右移動速度
    const double LATERAL_OFFSET = 70.0; // 渦を放つ左右オフセット
    const double TRAIL_OFFSET_Y = 26.0; // ボスより後方(下側)に渦を発生させる距離
    const int    TELEGRAPH_DELAY = 18;   // 予告から実際の渦放出までの遅延フレーム

    static int  oscMuki;      // ボスの移動方向 ±1
    static int  vortexParity; // 渦の回転方向/放出サイド ±1 (放出のたびに反転)
    static bool pendingShed;  // 予告済みで渦放出待ちかどうか
    static int  shedAtFrame;  // 渦を実際に放出するフレーム

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        oscMuki = 1;
        vortexParity = 1;
        pendingShed = false;
        shedAtFrame = 0;
    }
    else {
        enemy.x += OSC_SPEED * (double)oscMuki;
    }

    // 反転タイミングで、次にどちら側から渦が剥がれるかの予告を出す
    if (count > 1 && count % OSC_PERIOD == 1) {
        oscMuki *= -1;
        vortexParity *= -1;

        sEnemyShotSet* pTelegraph = new sEnemyShotSet;
        pTelegraph->count = 0;
        pTelegraph->patternFunc = ShotVortexTelegraph;
        pTelegraph->x = enemy.x + vortexParity * LATERAL_OFFSET;
        pTelegraph->y = enemy.y + TRAIL_OFFSET_Y;
        pTelegraph->muki = DX_PI / 2.0 + vortexParity * 0.3; // ほぼ下方向、少し外側へ偏らせる

        pTelegraph->pEnemyShotHead = new sEnemyShot;
        pTelegraph->pEnemyShotHead->prev = pTelegraph->pEnemyShotHead;
        pTelegraph->pEnemyShotHead->next = pTelegraph->pEnemyShotHead;

        pTelegraph->prev = enemyShotSetHead.prev;
        pTelegraph->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pTelegraph;
        enemyShotSetHead.prev = pTelegraph;

        pendingShed = true;
        shedAtFrame = count + TELEGRAPH_DELAY;
    }

    // 予告から少し遅れて、実際に渦を1つ放出する
    if (pendingShed && count == shedAtFrame) {
        sEnemyShotSet* pVortex = new sEnemyShotSet;
        pVortex->count = 0;
        pVortex->patternFunc = ShotVortex;
        pVortex->x = enemy.x + vortexParity * LATERAL_OFFSET;
        pVortex->y = enemy.y + TRAIL_OFFSET_Y;
        pVortex->param_i[0] = vortexParity; // 回転方向をそのまま渦度の符号として使う

        pVortex->pEnemyShotHead = new sEnemyShot;
        pVortex->pEnemyShotHead->prev = pVortex->pEnemyShotHead;
        pVortex->pEnemyShotHead->next = pVortex->pEnemyShotHead;

        pVortex->prev = enemyShotSetHead.prev;
        pVortex->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pVortex;
        enemyShotSetHead.prev = pVortex;

        pendingShed = false;
    }
}