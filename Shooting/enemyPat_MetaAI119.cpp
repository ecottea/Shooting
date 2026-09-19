// 定数
static const int LASER_SEG_COUNT = 12; // 短レーザー本数
static const double LASER_SPACING = 32.0; // 中心間隔
static const double LASER_HALF = 32.0; // 短レーザー半長(64/2)
static const double LASER_LENGTH = LASER_HALF * 2 + (LASER_SEG_COUNT - 1) * LASER_SPACING; // 416
static const int DIV_COUNT = 7; // 内分点数
static const int EMIT_INTERVAL = 6; // 何F毎に全点から射出
static const double DIAMOND_ACCEL = 0.06;
static const double DIAMOND_MAX = 2.8;
static const int PERIOD_FRAMES = 210;

// 周期共有状態 - EnemyPat_Levatain_MetaAIとレーザーパターンで共有
static double g_laserTheta = DX_PI * 0.5; // 剣角度
static double g_laserOmega = 0.0; // 角速度
static double g_enemyVx = 0.0;
static double g_enemyVy = 0.0;
static double g_prevEnemyX = 240.0;
static double g_prevEnemyY = 60.0;
static int g_period = 0;
static int g_periodTimer = 0;
static int g_motionMode = 0; // 0:高速平行 1:低速+回転

static void ShotLevateinDiamond(sEnemyShotSet* pSet);
static void ShotLevateinLaser(sEnemyShotSet* pSet);

// --------------------------------------------------
// 赤菱形: 0から加速→終端速度で等速
// --------------------------------------------------
static void ShotLevateinDiamond(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki;
        pShot->speed = 0.0;
        pShot->kind = img_enemyShotDiamond[0]; // 赤菱形 4.5x2.5
        pShot->param_d[0] = DIAMOND_ACCEL;
        pShot->param_d[1] = DIAMOND_MAX;
        pShot->margin = 480;

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        if (p->speed < p->param_d[1]) {
            p->speed += p->param_d[0];
            if (p->speed > p->param_d[1]) p->speed = p->param_d[1];
        }
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }

    // 弾が画面外で消えたらセット自体も解放(メインで消去される前提)
    if (pEnemyShotSet->pEnemyShotHead->next == pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->count > 60) {
            pEnemyShotSet->prev->next = pEnemyShotSet->next;
            pEnemyShotSet->next->prev = pEnemyShotSet->prev;
            delete pEnemyShotSet->pEnemyShotHead;
            delete pEnemyShotSet;
            return;
        }
    }
}

// --------------------------------------------------
// 長剣: 赤短レーザーを並べて1本として扱う
// --------------------------------------------------
static void ShotLevateinLaser(sEnemyShotSet* pSet)
{
    int segCount = pSet->param_i[0];
    int divCount = pSet->param_i[1];
    double length = pSet->param_d[0];

    // 初回: 剣本体生成
    if (pSet->count == 0) {
        for (int i = 0; i < segCount; ++i) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->kind = img_enemyShotLaser[0]; // 赤短レーザー 64x4
            pShot->muki = g_laserTheta;
            pShot->speed = 0.0;
            double s = LASER_HALF + i * LASER_SPACING;
            pShot->x = enemy.x + s * cos(g_laserTheta);
            pShot->y = enemy.y + s * sin(g_laserTheta);

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 画面外消去で欠けた分を補充
    int cur = 0;
    for (sEnemyShot* p = pSet->pEnemyShotHead->next; p != pSet->pEnemyShotHead; p = p->next) cur++;
    for (int i = cur; i < segCount; ++i) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->kind = img_enemyShotLaser[0];
        pShot->muki = g_laserTheta;
        pShot->speed = 0.0;
        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    // 剣の位置更新 - ボスを根本とする
    int idx = 0;
    for (sEnemyShot* p = pSet->pEnemyShotHead->next; p != pSet->pEnemyShotHead; p = p->next) {
        double s = LASER_HALF + idx * LASER_SPACING;
        p->x = enemy.x + s * cos(g_laserTheta);
        p->y = enemy.y + s * sin(g_laserTheta);
        p->muki = g_laserTheta;
        idx++;
        if (idx >= segCount) break;
    }

    // 菱形射出
    if (pSet->count % EMIT_INTERVAL == 0) {
        double evx = enemy.x - g_prevEnemyX;
        double evy = enemy.y - g_prevEnemyY;
        double perpX = -sin(g_laserTheta);
        double perpY = cos(g_laserTheta);

        for (int j = 0; j < divCount; ++j) {
            double sDiv = length * (double)(j + 1) / (double)(divCount + 1);
            double ex = enemy.x + sDiv * cos(g_laserTheta);
            double ey = enemy.y + sDiv * sin(g_laserTheta);

            double rotVx = sDiv * g_laserOmega * perpX;
            double rotVy = sDiv * g_laserOmega * perpY;
            double tvx = evx + rotVx;
            double tvy = evy + rotVy;
            double dot = tvx * perpX + tvy * perpY;

            double dirX = (dot >= 0.0) ? perpX : -perpX;
            double dirY = (dot >= 0.0) ? perpY : -perpY;
            double emitMuki = atan2(dirY, dirX);

            // 菱形用ShotSetを1発ごとに生成
            sEnemyShotSet* pDiamondSet = new sEnemyShotSet;
            pDiamondSet->count = 0;
            pDiamondSet->patternFunc = ShotLevateinDiamond;
            pDiamondSet->x = ex;
            pDiamondSet->y = ey;
            pDiamondSet->muki = emitMuki;
            pDiamondSet->kind = 0;

            pDiamondSet->pEnemyShotHead = new sEnemyShot;
            pDiamondSet->pEnemyShotHead->prev = pDiamondSet->pEnemyShotHead;
            pDiamondSet->pEnemyShotHead->next = pDiamondSet->pEnemyShotHead;

            pDiamondSet->prev = enemyShotSetHead.prev;
            pDiamondSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pDiamondSet;
            enemyShotSetHead.prev = pDiamondSet;
        }

        if (pSet->count % (EMIT_INTERVAL * 2)) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }
}

// --------------------------------------------------
// 敵本体: EnemyPat_Levatain_MetaAI
// --------------------------------------------------
void EnemyPat_Levatain_MetaAI()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        g_laserTheta = DX_PI * 0.5;
        g_laserOmega = 0.0;
        g_enemyVx = 2.7;
        g_enemyVy = 0.0;
        g_prevEnemyX = enemy.x;
        g_prevEnemyY = enemy.y;
        g_period = 0;
        g_periodTimer = 0;
        g_motionMode = 0; // 高速平行から開始
    }

    if (count == 30) {
        g_laserTheta = DX_PI * 0.5;
        g_laserOmega = 0.0;
        g_enemyVx = 2.7;
        g_enemyVy = 0.0;
        g_prevEnemyX = enemy.x;
        g_prevEnemyY = enemy.y;
        g_period = 0;
        g_periodTimer = 0;
        g_motionMode = 0; // 高速平行から開始

        // 長剣セット生成(1本だけ永続)
        sEnemyShotSet* pLaserSet = new sEnemyShotSet;
        pLaserSet->count = 0;
        pLaserSet->patternFunc = ShotLevateinLaser;
        pLaserSet->x = enemy.x;
        pLaserSet->y = enemy.y;
        pLaserSet->muki = g_laserTheta;
        pLaserSet->kind = 0;
        pLaserSet->param_i[0] = LASER_SEG_COUNT;
        pLaserSet->param_i[1] = DIV_COUNT;
        pLaserSet->param_d[0] = LASER_LENGTH;

        pLaserSet->pEnemyShotHead = new sEnemyShot;
        pLaserSet->pEnemyShotHead->prev = pLaserSet->pEnemyShotHead;
        pLaserSet->pEnemyShotHead->next = pLaserSet->pEnemyShotHead;

        pLaserSet->prev = enemyShotSetHead.prev;
        pLaserSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pLaserSet;
        enemyShotSetHead.prev = pLaserSet;
    }
    else {
        // 前フレーム位置を保存(射出方向判定用)
        g_prevEnemyX = enemy.x;
        g_prevEnemyY = enemy.y;

        // 移動
        enemy.x += g_enemyVx;
        enemy.y += g_enemyVy;
        g_laserTheta += g_laserOmega;

        // 角度正規化
        while (g_laserTheta > DX_PI) g_laserTheta -= DX_PI * 2;
        while (g_laserTheta < -DX_PI) g_laserTheta += DX_PI * 2;

        // 画面端バウンド
        if (enemy.x < 70.0) { enemy.x = 70.0; g_enemyVx = fabs(g_enemyVx); }
        if (enemy.x > 410.0) { enemy.x = 410.0; g_enemyVx = -fabs(g_enemyVx); }
        if (enemy.y < 40.0) { enemy.y = 40.0; g_enemyVy = fabs(g_enemyVy); }
        if (enemy.y > 140.0) { enemy.y = 140.0; g_enemyVy = -fabs(g_enemyVy); }

        g_periodTimer++;

        if (g_periodTimer >= PERIOD_FRAMES) {
            g_period++;
            g_periodTimer = 0;
            g_motionMode = g_period % 2; // 交互

            if (g_motionMode == 0) {
                // 速めの平行移動: 横スイープ
                g_laserOmega = 0.0;
                // 位置に応じて進行方向を決める
                g_enemyVx = (enemy.x < 240.0) ? 2.7 : -2.7;
                g_enemyVy = 0.0;
                // 真下基準に少しランダム傾き
                double jitter = (double)(GetRand(20) - 10) / 180.0 * DX_PI;
                g_laserTheta = DX_PI * 0.5 + jitter;
            }
            else {
                // 遅めの平行移動 + 速めの回転
                g_enemyVx = (GetRand(1) == 0 ? 0.7 : -0.7) + (double)(GetRand(20) - 10) / 100.0;
                g_enemyVy = (double)(GetRand(20) - 10) / 200.0 + 0.15;
                double baseOmega = 0.035 + (double)GetRand(10) / 1000.0;
                g_laserOmega = (GetRand(1) == 0) ? baseOmega : -baseOmega;
            }
        }
    }
}