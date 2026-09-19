// 弾幕：禁忌「レーヴァテイン」
//
// 短レーザーを連結して長いレーザーとして見せ、レーザーそのものを移動させる。
// レーザー上の内分点からは、レーザーの向きと垂直かつレーザーの平行移動方向との
// 内積が正になる側へ、赤菱形弾を連続射出する。

static constexpr int    LEVATIN_CYCLE_FRAMES = 120;
static constexpr int    LEVATIN_LASER_COUNT = 8;
static constexpr int    LEVATIN_EMIT_INTERVAL = 3;
static constexpr int    LEVATIN_DIVISION_COUNT = 6;
static constexpr double LEVATIN_LASER_LENGTH = 64.0;
static constexpr double LEVATIN_LASER_START_Y = 56.0;
static constexpr double LEVATIN_FAST_TARGET_LEFT = 60.0;
static constexpr double LEVATIN_FAST_TARGET_RIGHT = 420.0;
static constexpr double LEVATIN_SLOW_SHIFT = 90.0;
static constexpr double LEVATIN_DIAMOND_MAX_SPEED = 5.8;
static constexpr double LEVATIN_DIAMOND_ACCEL = 0.16;
static constexpr double LEVATIN_OFFSCREEN = 10000.0;

// レーヴァテインのレーザーを1本構成する短レーザー群＋放射される菱形弾を管理する。
static void ShotLevatain(sEnemyShotSet* pEnemyShotSet)
{
    const int age = pEnemyShotSet->count;

    const int mode = pEnemyShotSet->param_i[0];
    const double startX = pEnemyShotSet->param_d[0];
    const double startY = pEnemyShotSet->param_d[1];
    const double moveDeltaX = pEnemyShotSet->param_d[2];
    const double startAngle = pEnemyShotSet->param_d[3];
    const double rotateSpeed = pEnemyShotSet->param_d[4];

    double laserX = startX;
    double laserY = startY;
    double laserAngle = startAngle;

    if (age < LEVATIN_CYCLE_FRAMES) {
        const double ratio = (double)age / (double)(LEVATIN_CYCLE_FRAMES - 1);
        laserX = startX + moveDeltaX * ratio;

        if (mode == 1) {
            laserAngle += rotateSpeed * age;
        }

        pEnemyShotSet->x = laserX;
        pEnemyShotSet->y = laserY;
        pEnemyShotSet->muki = laserAngle;
    }

    // まず既存のレーザーと菱形弾を更新する。
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 0) {
            // レーザー片。
            // 周期終了後はレーザーだけ画面外へ追い出し、菱形弾はそのまま飛ばす。
            if (age >= LEVATIN_CYCLE_FRAMES) {
                pShot->x = LEVATIN_OFFSCREEN;
                pShot->y = LEVATIN_OFFSCREEN;
            }
            else {
                const int segmentIndex = pShot->param_i[1];
                const double distance = (segmentIndex + 0.5) * LEVATIN_LASER_LENGTH;
                pShot->x = laserX + distance * cos(laserAngle);
                pShot->y = laserY + distance * sin(laserAngle);
                pShot->muki = laserAngle;
                pShot->speed = 0.0;
            }
        }
        else {
            // 菱形弾。
            // 初速度は0。加速区間も含めて毎回原点からの距離を数式で求めることで、
            // フレームごとの速度積算による誤差を避ける。
            const double terminalSpeed = pShot->param_d[2];
            const double accel = pShot->param_d[3];
            const double t = (double)pShot->count;
            const double accelFrames = terminalSpeed / accel;

            double distance;
            if (t < accelFrames) {
                distance = 0.5 * accel * t * t;
                pShot->speed = accel * t;
            }
            else {
                distance = 0.5 * accel * accelFrames * accelFrames
                    + terminalSpeed * (t - accelFrames);
                pShot->speed = terminalSpeed;
            }

            pShot->x = pShot->param_d[0] + distance * cos(pShot->muki);
            pShot->y = pShot->param_d[1] + distance * sin(pShot->muki);
        }

        pShot = pNext;
    }

    // レーザーが動いている間だけ、等間隔の内分点から菱形弾を射出する。
    if (age < LEVATIN_CYCLE_FRAMES && age % LEVATIN_EMIT_INTERVAL == 0) {
        const double laserDirX = cos(laserAngle);
        const double laserDirY = sin(laserAngle);

        // 平行移動成分の向き。今回のレーザーは水平移動する。
        const double moveDirX = (moveDeltaX >= 0.0) ? 1.0 : -1.0;
        const double moveDirY = 0.0;

        // レーザー方向に垂直な2方向から、移動方向との内積が正になる側を選ぶ。
        double normalX = -laserDirY;
        double normalY = laserDirX;
        const double dot = normalX * moveDirX + normalY * moveDirY;
        if (dot < 0.0) {
            normalX = -normalX;
            normalY = -normalY;
        }

        // レーザーを等間隔に7分割したときの6個の内分点。
        for (int i = 1; i <= LEVATIN_DIVISION_COUNT; i++) {
            const double ratio = (double)i / (double)(LEVATIN_DIVISION_COUNT + 1);
            const double distance = ratio * LEVATIN_LASER_COUNT * LEVATIN_LASER_LENGTH;

            sEnemyShot* pNewShot = new sEnemyShot;
            pNewShot->x = laserX + distance * laserDirX;
            pNewShot->y = laserY + distance * laserDirY;
            pNewShot->muki = atan2(normalY, normalX);
            pNewShot->speed = 0.0;
            pNewShot->kind = img_enemyShotDiamond[0];
            pNewShot->param_i[0] = 1;
            pNewShot->param_d[0] = pNewShot->x;
            pNewShot->param_d[1] = pNewShot->y;
            pNewShot->param_d[2] = LEVATIN_DIAMOND_MAX_SPEED;
            pNewShot->param_d[3] = LEVATIN_DIAMOND_ACCEL;
            pNewShot->margin = 480;

            pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNewShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
            pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Levatain_ChatGPT()
{
    static bool initialized;
    static int cycleIndex;
    static int cycleMode;
    static double cycleStartX;
    static double cycleStartY;
    static double cycleStartAngle;
    static double moveDeltaX;
    static double rotateSpeed;

    if (count == 1) {
        enemy.x = 240;
        enemy.y = 50;
        enemy.maxHp = enemy.hp = 200;
        initialized = true;
        cycleIndex = 0;
        cycleMode = 0;
        cycleStartX = 240.0;
        cycleStartY = LEVATIN_LASER_START_Y;
        cycleStartAngle = DX_PI * 0.5;
        moveDeltaX = LEVATIN_FAST_TARGET_RIGHT - cycleStartX;
        rotateSpeed = 0.0;
    }

    if (count == 30) {
        initialized = true;
        cycleIndex = 0;
        cycleMode = 0;
        cycleStartX = 240.0;
        cycleStartY = LEVATIN_LASER_START_Y;
        cycleStartAngle = DX_PI * 0.5;
        moveDeltaX = LEVATIN_FAST_TARGET_RIGHT - cycleStartX;
        rotateSpeed = 0.0;
        enemy.x = cycleStartX;
        enemy.y = cycleStartY;
    }

    if (!initialized) {
        return;
    }

    const int cycleAge = (count - 30) % LEVATIN_CYCLE_FRAMES;
    const bool cycleStart = (count == 30) || (cycleAge == 0 && count > 30);

    if (cycleStart && count > 30) {
        cycleIndex++;

        // 直線移動と回転＋遅い平行移動を交互に切り替える。
        cycleMode = cycleIndex % 2;

        cycleStartX = enemy.x;
        cycleStartY = LEVATIN_LASER_START_Y;
        cycleStartAngle = cycleStartAngle + rotateSpeed * (LEVATIN_CYCLE_FRAMES - 1);

        if (cycleMode == 0) {
            // 速い平行移動。画面内を左右に大きく往復する。
            const double targetX = (cycleStartX < 240.0)
                ? LEVATIN_FAST_TARGET_RIGHT
                : LEVATIN_FAST_TARGET_LEFT;
            moveDeltaX = targetX - cycleStartX;
            rotateSpeed = 0.0;
        }
        else {
            // 遅い平行移動＋速い回転。角度が画面に対して極端に浅くならない範囲で回す。
            const double targetX = (cycleStartX < 240.0)
                ? cycleStartX + LEVATIN_SLOW_SHIFT
                : cycleStartX - LEVATIN_SLOW_SHIFT;
            moveDeltaX = targetX - cycleStartX;

            int rotateSign;
            if (cycleStartAngle < DX_PI * 30.0 / 180.0) {
                rotateSign = 1;
            }
            else if (cycleStartAngle > DX_PI * 150.0 / 180.0) {
                rotateSign = -1;
            }
            else {
                rotateSign = (GetRand(1) == 0) ? -1 : 1;
            }

            double desiredTotal = DX_PI * (60.0 + GetRand(20)) / 180.0;
            const double lowerLimit = DX_PI * 30.0 / 180.0;
            const double upperLimit = DX_PI * 150.0 / 180.0;

            if (rotateSign > 0) {
                desiredTotal = (desiredTotal < upperLimit - cycleStartAngle)
                    ? desiredTotal
                    : upperLimit - cycleStartAngle;
            }
            else {
                desiredTotal = (desiredTotal < cycleStartAngle - lowerLimit)
                    ? desiredTotal
                    : cycleStartAngle - lowerLimit;
            }

            rotateSpeed = rotateSign * desiredTotal / (double)(LEVATIN_CYCLE_FRAMES - 1);
        }

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 敵本体は常にレーザーの一方の端に一致させる。
    const double moveRatio = (double)cycleAge / (double)(LEVATIN_CYCLE_FRAMES - 1);
    enemy.x = cycleStartX + moveDeltaX * moveRatio;
    enemy.y = cycleStartY;

    double currentAngle = cycleStartAngle;
    if (cycleMode == 1) {
        currentAngle += rotateSpeed * cycleAge;
    }

    // レーザーが現れる少し後に発射音を鳴らす。
    if (cycleAge == 15) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    if (cycleStart) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLevatain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = currentAngle;
        pEnemyShotSet->kind = cycleIndex;

        pEnemyShotSet->param_i[0] = cycleMode;
        pEnemyShotSet->param_d[0] = cycleStartX;
        pEnemyShotSet->param_d[1] = cycleStartY;
        pEnemyShotSet->param_d[2] = moveDeltaX;
        pEnemyShotSet->param_d[3] = cycleStartAngle;
        pEnemyShotSet->param_d[4] = rotateSpeed;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 8本の短レーザーを端から端まで連結して1本の長レーザーにする。
        for (int i = 0; i < LEVATIN_LASER_COUNT; i++) {
            sEnemyShot* pNewShot = new sEnemyShot;
            pNewShot->kind = img_enemyShotLaser[0];
            pNewShot->muki = currentAngle;
            pNewShot->speed = 0.0;
            pNewShot->param_i[0] = 0;
            pNewShot->param_i[1] = i;
            pNewShot->margin = 999;

            pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNewShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
            pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
        }

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
