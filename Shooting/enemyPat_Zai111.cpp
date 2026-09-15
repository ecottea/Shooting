// enemyPat_tmp.cpp
// 弾幕パターン:「ケント・オーチャード」
//  ケントの花(開花→散華→種まき)とリンゴ(落下・バウンド・破裂)が交互に襲う弾幕
//
//  1ループ = 900フレーム(15秒)
//   [GardenSet] 開花(0-180) → 散華(180-420) → 待機 → 種まき(780-900)
//   [AppleSet ] 実り(420-450:予告点滅) → 収穫期の嵐(450-750:リンゴ落下+風)
//
//  ※弾の役割は param_i[0] で管理
//    0:花びら 1:花芯 2:直進弾(花粉・種・風) 3:リンゴ予告 4:リンゴ 5:リンゴの茎

static const int BLOOM_END = 180;   // 開花終了(花びらが停止するフレーム)
static const int SCATTER_END = 320;   // 散華終了
static const int SEED_BEGIN = 380;   // 種まき開始
static const int SEED_END = 600;   // 種まき終了(=1ループ)
static const int LOOP_FRAMES = 600;

// ------------------------------------------------------------------
// 生成ヘルパ
// ------------------------------------------------------------------
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int kind, int role)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->param_i[0] = role;
    p->margin = 240;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func,
    int kind, double x, double y)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = kind; // ループ回数 = 難易度

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// ------------------------------------------------------------------
// 花のセット:開花 → 散華 → (待機) → 種まき
// ------------------------------------------------------------------
static void ShotGarden(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    const int kind = pSet->kind; // 難易度(ループ回数)

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 花びら(大弾・白)を放射状にゆっくり発射
        int petalNum = 12 + kind * 2;
        if (petalNum > 16) petalNum = 16;
        for (int i = 0; i < petalNum; i++) {
            pShot = AddShot(pSet, pSet->x, pSet->y,
                i * 2.0 * DX_PI / petalNum, 1.5,
                img_enemyShotLargeBall[6], 0);
            pShot->param_d[0] = 1.5;                  // 初速
            pShot->param_d[1] = GetRand(628) / 100.0; // 散華時の揺れの位相
        }
        // 花芯(小弾・黄)をまとめて噴き出し、減速して花の中心になる
        // ※GetRand(360) は 0〜360 を返すので 180 で割って角度へ
        for (int i = 0; i < 10; i++) {
            AddShot(pSet, pSet->x, pSet->y,
                GetRand(360) / 180.0 * DX_PI, 1.0,
                img_enemyShotSmallBall[1], 1);
        }
    }

    // 散華フェーズ:花芯から花粉(自機狙い3-way)を連射
    if (pSet->count >= BLOOM_END && pSet->count < SCATTER_END && pSet->count % 30 == 0) {
        double base = atan2(player.y - pSet->y, player.x - pSet->x);
        for (int j = -1; j <= 1; j++) {
            AddShot(pSet, pSet->x, pSet->y, base + j * 0.25, 3.0,
                img_enemyShotSmallBall[1], 2);
        }
    }

    // 種まきフェーズ:画面内のランダム位置から種(小弾)が弾ける
    if (pSet->count >= SEED_BEGIN && pSet->count < SEED_END && pSet->count % 8 == 0) {
        AddShot(pSet, 20.0 + GetRand(440), 40.0 + GetRand(200),
            GetRand(360) / 180.0 * DX_PI, 2.8,
            img_enemyShotSmallBall[2], 2);
    }

    // ---- 弾の移動 ----
    sEnemyShot* pList = pSet->pEnemyShotHead->next;
    while (pList != pSet->pEnemyShotHead) {
        switch (pList->param_i[0]) {
        case 0: // 花びら
            if (pList->count < BLOOM_END) {
                // 開花:減速しながら進み、一定距離で停止して「一輪の花」になる
                pList->speed = pList->param_d[0] * (1.0 - (double)pList->count / BLOOM_END);
                if (pList->speed < 0.0) pList->speed = 0.0;
                pList->x += pList->speed * cos(pList->muki);
                pList->y += pList->speed * sin(pList->muki);
            }
            else {
                // 散華:自機方向へ、風に揺られながら漂う
                if (pList->count == BLOOM_END) {
                    pList->muki = atan2(player.y - pList->y, player.x - pList->x);
                    pList->speed = 1.2;
                }
                double sway = sin(pList->count * 0.05 + pList->param_d[1])
                    * (1.0 + 0.5 * kind);
                if (sway > 2.0) sway = 2.0;
                double cx = cos(pList->muki), cy = sin(pList->muki);
                // 進行方向 + それに垂直な成分で左右に揺らす
                pList->x += pList->speed * cx - sway * 0.5 * cy;
                pList->y += pList->speed * cy + sway * 0.5 * cx;
            }
            break;

        case 1: // 花芯:急減速してその場に留まる
            if (pList->speed > 0.05) {
                pList->speed *= 0.92;
                pList->x += pList->speed * cos(pList->muki);
                pList->y += pList->speed * sin(pList->muki);
            }
            break;

        case 2: // 直進弾(花粉・種)
        default:
            pList->x += pList->speed * cos(pList->muki);
            pList->y += pList->speed * sin(pList->muki);
            break;
        }
        pList = pList->next;
    }
}

// ------------------------------------------------------------------
// リンゴのセット:実り(予告) → 収穫期の嵐(落下+風)
// ------------------------------------------------------------------
static void ShotApple(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;
    const int kind = pSet->kind; // 難易度(ループ回数)

    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 落下予告音
    }
    if (pSet->count == 30) { // 最初のリンゴが落ちるタイミング
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // リンゴ投下(予告→落下)
    int interval = 36 - kind * 6 - 10;
    if (interval < 16) interval = 16;
    if (pSet->count < 330 && pSet->count % interval == 0) {
        double ax = 30.0 + GetRand(420); // 落下位置 x = 30〜450

        // 落下予告(中弾)を上端に置く。30フレーム後にリンゴへ変化する
        pShot = AddShot(pSet, ax, 24.0, DX_PI / 2.0, 0.0,
            img_enemyShotMediumBall[8], 3);

        pShot->param_i[1] = (GetRand(9) < 5) ? 1 : 0;  // 30%が「熟れたリンゴ」
        // バウンドするか(難易度2以上は50%、それ未満は20%)
        pShot->param_i[2] = (GetRand(9) < ((kind >= 2) ? 8 : 4)) ? 1 : 0;
        pShot->param_d[2] = 400.0 + GetRand(40);       // バウンドする高さ
    }

    // 風(銃弾が左右から横切る)
    if (pSet->count >= 30 && pSet->count < 330 && pSet->count % 40 == 0) {
        double speed = 1.5 + 0.4 * (kind > 3 ? 3 : kind);
        if (GetRand(1) == 0) {
            AddShot(pSet, -10.0, 60.0 + GetRand(320), 0.0, speed,
                img_enemyShotBullet[3], 2);
        }
        else {
            AddShot(pSet, 490.0, 60.0 + GetRand(320), DX_PI, speed,
                img_enemyShotBullet[3], 2);
        }
    }

    // ---- 弾の移動 ----
    sEnemyShot* pList = pSet->pEnemyShotHead->next;
    while (pList != pSet->pEnemyShotHead) {
        switch (pList->param_i[0]) {
        case 3: // 予告:点滅表示、30フレーム後にリンゴ(大弾)へ変化して落下開始
            pList->kind = ((pList->count / 5) % 2 == 0) ? img_enemyShotMediumBall[8]
                : img_enemyShotMediumBall[6];
            if (pList->count == 30) {
                pList->param_i[0] = 4;
                pList->kind = (pList->param_i[1] == 1) ? img_enemyShotLargeBall[8]  // 熟れたリンゴ
                    : img_enemyShotLargeBall[0]; // 普通のリンゴ
                pList->param_d[1] = 0.5; // 落下初速
                // 茎(緑の銃弾)をリンゴの上に追加 →「茎の付いた実」に見せる
                pShot = AddShot(pSet, pList->x, pList->y - 20.0, DX_PI / 2.0, 0.0,
                    img_enemyShotBullet[2], 5);
                pShot->param_d[1] = 0.5;
            }
            break;

        case 4: // リンゴ:重力で加速しながら落ちる
            pList->param_d[1] += 0.06;
            pList->y += pList->param_d[1];

            // 熟れたリンゴ:一定ラインで割れて花粉が5-wayに飛散(本体はそのまま落下)
            if (pList->param_i[1] == 1 && pList->y > 280.0) {
                pList->param_i[1] = 2; // 爆発済み
                double base = atan2(player.y - pList->y, player.x - pList->x);
                for (int j = -2; j <= 2; j++) {
                    AddShot(pSet, pList->x, pList->y, base + j * 0.2, 3.0,
                        img_enemyShotSmallBall[1], 2);
                }
            }
            // バウンド:一度だけ跳ねて小玉(赤)に変化する
            if (pList->param_i[2] == 1 && pList->y > pList->param_d[2]
                && pList->param_d[1] > 0.0) {
                pList->param_i[2] = 2;
                pList->y = pList->param_d[2];
                pList->param_d[1] *= -0.5;
                pList->kind = img_enemyShotSmallBall[0];
            }
            break;

        case 5: // 茎:リンゴに追従して落ちる(同じ重力なので位置がずれない)
            pList->param_d[1] += 0.06;
            pList->y += pList->param_d[1];
            break;

        case 2: // 直進弾(風)
        default:
            pList->x += pList->speed * cos(pList->muki);
            pList->y += pList->speed * sin(pList->muki);
            break;
        }
        pList = pList->next;
    }
}

// ------------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------------
void EnemyPat_FlowerOfKent_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 15秒(900フレーム)で1ループ。ループ回数をそのまま難易度に使う
    int phase = (count - 1) % LOOP_FRAMES;
    int loopNum = (count - 1) / LOOP_FRAMES;

    if (phase == 0) {
        // フェーズ1〜2,5:花のセット
        CreateShotSet(ShotGarden, loopNum, enemy.x, enemy.y + 10.0);
    }
    if (phase == 300) {
        // フェーズ3〜4:リンゴのセット
        CreateShotSet(ShotApple, loopNum, enemy.x, enemy.y);
    }
}