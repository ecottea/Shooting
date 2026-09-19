//----------------------------------------------------------
// enemyPat_levatein.cpp
// 弾幕：禁忌「レーヴァテイン」風
//  ・赤短レーザー(64x4)を4本連結し全長256pxの1本のレーザーとして扱う
//  ・レーザーの根元(一方の端)は常にボスの位置と一致する
//  ・偶数周期：速めの平行移動
//    奇数周期：遅めの平行移動＋速めの回転移動
//  ・移動中、レーザーを等間隔に内分する点から「レーザーの向きと垂直かつ
//    内分点の移動方向との内積が正」の方向へ赤菱形弾を射出
//  ・菱形弾は初速0→毎フレーム加速→終端速度到達後は等速直線運動
//  ・1周期(240フレーム)ごとに動きを切り替えて繰り返す
//----------------------------------------------------------

// ---- 設定定数 ----
static const int    LASER_NUM = 4;     // 連結する短レーザーの本数
static const double SEG_LEN = 64.0;  // 短レーザー1本の長さ(画像サイズ)
static const double MOVE_FAST = 4.5;   // タイプA: 速めの平行移動速度
static const double MOVE_SLOW = 1.2;   // タイプB: 遅めの平行移動速度
static const double SPIN = 0.05;  // タイプB: 角速度[rad/frame](約2.9度/frame)
static const int    CYCLE = 240;   // 1周期のフレーム数
static const int    SHOT_INTERVAL = 8;     // 菱形弾の射出間隔[フレーム]
static const double SHOT_ACC = 0.16;  // 菱形弾の加速度
static const double SHOT_VEL = 3.4;   // 菱形弾の終端速度
static const double MARGIN = 68.0;  // レーザー根元(ボス)の壁反射マージン

// ============================================================
//  レーザーと菱形弾の挙動 (sEnemyShotSet に登録される)
// ============================================================
static void ShotLevatein(sEnemyShotSet* pEnemyShotSet)
{
    // param_i[0] : 動きのタイプ (0:速めの平行移動 / 1:遅めの平行移動＋速めの回転)
    // param_i[1] : 進行方向に対してレーザーを伸ばす側 (+1 / -1)…タイプA用
    // param_d[0],[1] : 平行移動速度 vx, vy
    // param_d[2] : 角速度 omega [rad/frame]…タイプB用
    // x, y   : レーザーの根元(一方の端)＝ボスの位置
    // muki   : レーザーの向き
    int    type = pEnemyShotSet->param_i[0];
    int    side = pEnemyShotSet->param_i[1];
    double vx = pEnemyShotSet->param_d[0];
    double vy = pEnemyShotSet->param_d[1];
    double omega = pEnemyShotSet->param_d[2];

    // ---- 根元の平行移動 ----
    pEnemyShotSet->x += vx;
    pEnemyShotSet->y += vy;

    // ---- 壁で反射させてボスを画面内に留める ----
    bool reflect = false;
    if (pEnemyShotSet->x < MARGIN && vx < 0.0) { pEnemyShotSet->param_d[0] = vx = -vx; reflect = true; }
    if (pEnemyShotSet->x > 480.0 - MARGIN && vx > 0.0) { pEnemyShotSet->param_d[0] = vx = -vx; reflect = true; }
    if (pEnemyShotSet->y < MARGIN && vy < 0.0) { pEnemyShotSet->param_d[1] = vy = -vy; reflect = true; }
    if (pEnemyShotSet->y > 480.0 - MARGIN && vy > 0.0) { pEnemyShotSet->param_d[1] = vy = -vy; reflect = true; }

    // ---- レーザーの向き ----
    double m = pEnemyShotSet->muki;
    if (type == 0) {
        // 速めの平行移動：レーザーは進行方向に対して垂直な向きを保つ
        if (reflect) {
            m = atan2(pEnemyShotSet->param_d[1], pEnemyShotSet->param_d[0])
                + side * DX_PI / 2.0;
            pEnemyShotSet->muki = m;
        }
    }
    else {
        // 速めの回転を加える
        m += omega;
        pEnemyShotSet->muki = m;
    }

    // ---- 赤菱形弾の射出 ----
    if (pEnemyShotSet->count % SHOT_INTERVAL == 0) {
        // 射出音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < LASER_NUM; i++) {
            double d = SEG_LEN * (i + 0.5);   // 根元から内分点までの距離(等間隔)
            double px = pEnemyShotSet->x + d * cos(m);
            double py = pEnemyShotSet->y + d * sin(m);

            // 内分点の移動速度 ＝ 平行移動 ＋ 回転による接線速度
            double vpx = vx - omega * d * sin(m);
            double vpy = vy + omega * d * cos(m);

            // レーザーに垂直な2方向のうち、移動方向との内積が正の方向を選ぶ
            double dot = vpx * (-sin(m)) + vpy * cos(m);   // muki+90°方向との内積
            double dir = m + (dot >= 0.0 ? DX_PI / 2.0 : -DX_PI / 2.0);

            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = px;
            pEnemyShot->y = py;
            pEnemyShot->muki = dir;
            pEnemyShot->speed = 0.0;                     // 初速0
            pEnemyShot->kind = img_enemyShotDiamond[0]; // 赤菱形弾
            pEnemyShot->param_i[0] = 1;                  // 1: 加速する菱形弾
            pEnemyShot->param_d[0] = SHOT_ACC;           // 加速度
            pEnemyShot->param_d[1] = SHOT_VEL;           // 終端速度
            pEnemyShot->margin = 480;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---- 短レーザーの再配置 ＆ 菱形弾の加速・移動 ----
    int found = 0; // 生存している短レーザーのインデックス(ビット管理)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 短レーザー：根元から SEG_LEN 間隔で再配置(1本の長いレーザーを形成)
            int i = pShot->param_i[1];
            double d = SEG_LEN * (i + 0.5);
            pShot->x = pEnemyShotSet->x + d * cos(m);
            pShot->y = pEnemyShotSet->y + d * sin(m);
            pShot->muki = m;
            found |= 1 << i;
        }
        else {
            // 赤菱形弾：初速0から加速し、終端速度に達したら等速直線運動
            pShot->speed += pShot->param_d[0];
            if (pShot->speed > pShot->param_d[1]) pShot->speed = pShot->param_d[1];
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // メインルーチンの画面外消去で短レーザーが欠けていたら補充する
    for (int i = 0; i < LASER_NUM; i++) {
        if (found & (1 << i)) continue;
        double d = SEG_LEN * (i + 0.5);
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x + d * cos(m);
        pEnemyShot->y = pEnemyShotSet->y + d * sin(m);
        pEnemyShot->muki = m;
        pEnemyShot->speed = 0.0;
        pEnemyShot->kind = img_enemyShotLaser[0]; // 赤の短レーザー
        pEnemyShot->param_i[0] = 0;
        pEnemyShot->param_i[1] = i;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }
}

// ============================================================
//  敵本体のパターン：禁忌「レーヴァテイン」
// ============================================================
void EnemyPat_Levatain_Zai()
{
    static sEnemyShotSet* pSet = nullptr; // レーザー管理用セット
    static int cycleFrame = 0;            // 周期内の経過フレーム

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        cycleFrame = 0;

        // ---- レーザー管理セットの生成 ----
        pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotLevatein;
        pSet->x = enemy.x;  // レーザーの根元(一方の端)＝ボスの位置
        pSet->y = enemy.y;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        // ---- 短レーザー(LASER_NUM本)の生成 ----
        for (int i = 0; i < LASER_NUM; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pSet->x;
            pEnemyShot->y = pSet->y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[0]; // 赤の短レーザー
            pEnemyShot->param_i[0] = 0; // 0: レーザー本体
            pEnemyShot->param_i[1] = i; // 根元から何本目か

            pEnemyShot->prev = pSet->pEnemyShotHead->prev;
            pEnemyShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pEnemyShot;
            pSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ---- 1周期目：速めの平行移動 ----
        double mv = atan2(player.y - enemy.y, player.x - enemy.x);
        pSet->param_d[0] = MOVE_FAST * cos(mv);
        pSet->param_d[1] = MOVE_FAST * sin(mv);
        pSet->param_d[2] = 0.0;
        pSet->param_i[0] = 0;
        // GetRand(1) は 0 か 1 を返すので ±1 を作る
        pSet->param_i[1] = GetRand(1) ? 1 : -1;
        pSet->muki = mv + pSet->param_i[1] * DX_PI / 2.0; // レーザーは進行方向に垂直

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    else {
        cycleFrame++;

        // 次の周期開始の予告音
        if (cycleFrame == CYCLE - 30) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // ---- 周期ごとにレーザーの動きを切り替える ----
        if (cycleFrame >= CYCLE) {
            cycleFrame = 0;
            pSet->count = 0; // 射出タイミングを周期の頭に合わせる

            if (pSet->param_i[0] == 0) {
                // 平行移動 → 遅めの平行移動＋速めの回転
                double mv = GetRand(360) / 180.0 * DX_PI;
                pSet->param_d[0] = MOVE_SLOW * cos(mv);
                pSet->param_d[1] = MOVE_SLOW * sin(mv);
                pSet->param_d[2] = (GetRand(1) ? 1 : -1) * SPIN; // 回転方向はランダム
                pSet->param_i[0] = 1;
            }
            else {
                // 回転 → 速めの平行移動(プレイヤー方向±30度)
                // GetRand(61)-30 で -30〜30 の61種類(偏りなし)
                double mv = atan2(player.y - pSet->y, player.x - pSet->x)
                    + (GetRand(61) - 30) / 180.0 * DX_PI;
                pSet->param_d[0] = MOVE_FAST * cos(mv);
                pSet->param_d[1] = MOVE_FAST * sin(mv);
                pSet->param_d[2] = 0.0;
                pSet->param_i[0] = 0;
                pSet->param_i[1] = GetRand(1) ? 1 : -1;
                pSet->muki = mv + pSet->param_i[1] * DX_PI / 2.0;
            }

            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        // ボスの位置を常にレーザーの根元(一方の端)に一致させる
        enemy.x = pSet->x;
        enemy.y = pSet->y;
    }
}