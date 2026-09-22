// enemyPat_airHockey.cpp
//
// 「センターライン・ホッケー」— エアホッケーをモチーフにした弾幕
// 専用素材は使わず、既存の弾種の組み合わせだけで卓・パック・マレット・ゴールを表現する。
//
// 素材の対応:
//   壁(上下)           → 短レーザー(白)
//   センターライン       → 鱗弾(シアン)
//   センターサークル     → 中楕円弾(内周シアン／外周青、二重リングで反対向きに回転)
//   パック              → 大玉(赤)
//   パックの着弾スパーク → 小玉(白)
//   マレット本体         → 菱形弾(左:黄／右:緑)
//   マレットの迎撃弾     → 中玉(マレットと同色)
//   シュートライン       → 銃弾(橙)
//   ゴール演出の収束弾   → 中玉(左ゴール:黄／右ゴール:緑)
//
// 画面は 480x480 を前提とする。

// ============================================================
//  壁(短レーザー)：上下に一時的に出現し、しばらく留まってから
//  画面外へ退場する「パルス壁」
// ============================================================
static void ShotWallPulse(sEnemyShotSet* pEnemyShotSet)
{
    const int HOLD = 50;      // 壁が留まるフレーム数
    const int ROW_COUNT = 22; // 横一列に並べるレーザーの数
    int side = pEnemyShotSet->param_i[0]; // 0:上壁 1:下壁
    double exitMuki = (side == 0) ? -DX_PI / 2.0 : DX_PI / 2.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < ROW_COUNT; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 15.0 + i * (450.0 / (ROW_COUNT - 1));
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 0.0; // 静止中は水平な壁として見せる
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[6]; // 白
            pEnemyShot->margin = 80;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    bool retreat = (pEnemyShotSet->count == HOLD);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (retreat) {
            pShot->muki = exitMuki; // 退場する向きへ切り替える
            pShot->speed = 7.0;
            pShot->y += 15 * sin(pShot->muki);
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  センターライン(鱗弾)：中央に静止する点線
// ============================================================
static void ShotCenterLine(sEnemyShotSet* pEnemyShotSet)
{
    const int DOT_COUNT = 26;

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < DOT_COUNT; i++) {
            if (i % 3 == 2) continue; // 3個に1個は間隔を空けて点線に見せる

            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 10.0 + i * (460.0 / (DOT_COUNT - 1));
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 0.0; // センターラインとして水平に並べる
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotScale[3]; // シアン

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 静止したまま維持するため移動処理は不要
}

// ============================================================
//  センターサークル(中楕円弾)：中心を軸にゆっくり回転するリング。
//  param_i[0]==1 になるとゴール演出として外側へ拡散して消える。
// ============================================================
static void ShotCenterCircle(sEnemyShotSet* pEnemyShotSet)
{
    int    ringCount = pEnemyShotSet->param_i[1];
    double radius = pEnemyShotSet->param_d[1];
    double rotSpeed = pEnemyShotSet->param_d[2];
    int    color = pEnemyShotSet->param_i[2];

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < ringCount; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double angle = DX_PI * 2.0 * i / ringCount;
            pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);
            pEnemyShot->muki = angle; // 中心から外向きに整列させておく
            pEnemyShot->speed = 0.0;
            pEnemyShot->param_d[0] = angle; // 現在の回転角を保持
            pEnemyShot->kind = img_enemyShotMediumOval[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    bool burst = (pEnemyShotSet->param_i[0] == 1);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (burst) {
            pShot->speed = 4.5; // ゴール演出：外側へ拡散して画面外へ
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            pShot->param_d[0] += rotSpeed;
            pShot->x = pEnemyShotSet->x + radius * cos(pShot->param_d[0]);
            pShot->y = pEnemyShotSet->y + radius * sin(pShot->param_d[0]);
            pShot->muki = pShot->param_d[0]; // 向きも回転に合わせる
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  マレット(菱形弾)：左右の縁を上下にスライドし、定期的に
//  自機狙いの扇状弾(中玉)を放つ
// ============================================================
static void ShotMallet(sEnemyShotSet* pEnemyShotSet)
{
    const double AMPLITUDE = 150.0;
    const double CENTER_Y = 240.0;
    int color = pEnemyShotSet->param_i[1];

    sEnemyShot* pMallet;

    if (pEnemyShotSet->count == 0) {
        pMallet = new sEnemyShot;
        pMallet->x = pEnemyShotSet->x;
        pMallet->y = pEnemyShotSet->y;
        pMallet->muki = pEnemyShotSet->muki; // 卓の中央を向かせておく
        pMallet->speed = 0.0;
        pMallet->kind = img_enemyShotLargeBall[color];

        pMallet->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pMallet->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pMallet;
        pEnemyShotSet->pEnemyShotHead->prev = pMallet;
    }

    // マレット本体は常にリストの先頭(1発目)
    pMallet = pEnemyShotSet->pEnemyShotHead->next;
    pMallet->y = CENTER_Y + AMPLITUDE * sin((pEnemyShotSet->count + pEnemyShotSet->param_i[0]) * 0.02);

    if (pEnemyShotSet->count % 85 == 30) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pMallet->y, player.x - pMallet->x);
        for (int i = -4; i <= 4; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pMallet->x;
            pEnemyShot->y = pMallet->y;
            pEnemyShot->muki = baseAngle + i * (DX_PI / 12.0);
            pEnemyShot->speed = 3.2;
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // マレット本体以外(迎撃弾)だけを移動させる
    sEnemyShot* pShot = pMallet->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  パック(大玉)：上下の壁で反射しながらジグザグに自機へ迫る。
//  反射の瞬間に小玉のスパークをまき散らす
// ============================================================
static void ShotPuck(sEnemyShotSet* pEnemyShotSet)
{
    const double WALL_TOP = 60.0;
    const double WALL_BOTTOM = 420.0;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki; // 自機を狙う初速方向(縦成分を誇張済み)
        pEnemyShot->speed = 3.6;
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤(パック)

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 上下の壁に到達したら反射(縦成分だけ反転)
        if (((pShot->y <= WALL_TOP && sin(pShot->muki) < 0.0) ||
            (pShot->y >= WALL_BOTTOM && sin(pShot->muki) > 0.0))
            && pShot->param_i[0] == 0) {
            pShot->muki = -pShot->muki;

            for (int i = 0; i < 10; i++) {
                sEnemyShot* pSpark = new sEnemyShot;
                pSpark->x = pShot->x;
                pSpark->y = pShot->y;
                pSpark->muki = DX_PI * 2.0 * i / 10.0;
                pSpark->speed = 2.0 + GetRand(100) / 100.0;
                pSpark->kind = img_enemyShotSmallBall[6]; // 白
                pSpark->param_i[0] = 1;

                pSpark->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSpark->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSpark;
                pEnemyShotSet->pEnemyShotHead->prev = pSpark;
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  シュートライン(銃弾)：左右の縁から水平に高速連射する
// ============================================================
static void ShotSweepLine(sEnemyShotSet* pEnemyShotSet)
{
    const int BULLET_COUNT = 20;
    const int INTERVAL = 2; // 発射間隔(フレーム)

    if (pEnemyShotSet->count % INTERVAL == 0 && pEnemyShotSet->count / INTERVAL < BULLET_COUNT) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y + (GetRand(40) - 20); // 発射高さを少しばらけさせる
        pEnemyShot->muki = pEnemyShotSet->muki; // 左向き/右向き
        pEnemyShot->speed = 8.0;
        pEnemyShot->kind = img_enemyShotBullet[8]; // 橙

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  ゴール演出(中玉)：中央から左右の縁へ向けて一気に撃ち出す収束弾
// ============================================================
static void ShotGoalBurst(sEnemyShotSet* pEnemyShotSet)
{
    int color = pEnemyShotSet->param_i[0];

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 40; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            double spread = (GetRand(200) - 100) / 100.0 * (DX_PI / 6.0);
            pEnemyShot->muki = pEnemyShotSet->muki + spread; // ゴールへ向かう方向を中心に広がる
            pEnemyShot->speed = 5.0 + GetRand(200) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumBall[color];

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

// ============================================================
//  共通のセット登録処理(重複コードをまとめたもの)
// ============================================================
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    return pEnemyShotSet;
}

// センターサークルの内周/外周リングを新しく張り直す
static void SpawnCenterCircleRings(sEnemyShotSet** pInner, sEnemyShotSet** pOuter)
{
    *pInner = CreateShotSet(ShotCenterCircle, 240.0, 240.0, 0.0);
    (*pInner)->param_i[0] = 0;    // 0:回転中 1:ゴール演出で拡散
    (*pInner)->param_i[1] = 14;   // リングを構成する弾数
    (*pInner)->param_i[2] = 3;    // シアン
    (*pInner)->param_d[1] = 45.0; // 半径
    (*pInner)->param_d[2] = 0.015;// 回転速度(時計回り)

    *pOuter = CreateShotSet(ShotCenterCircle, 240.0, 240.0, 0.0);
    (*pOuter)->param_i[0] = 0;
    (*pOuter)->param_i[1] = 20;
    (*pOuter)->param_i[2] = 4;     // 青
    (*pOuter)->param_d[1] = 85.0;
    (*pOuter)->param_d[2] = -0.009; // 反時計回り(内周と逆回転)
}

// ============================================================
//  敵本体のパターン:「センターライン・ホッケー」
// ============================================================
void EnemyPat_AirHockey_Claude()
{
    static sEnemyShotSet* pCenterRingInner;
    static sEnemyShotSet* pCenterRingOuter;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 30.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        // --- センターライン(鱗弾) ---
        CreateShotSet(ShotCenterLine, 240.0, 240.0, 0.0);

        // --- センターサークル(中楕円弾、二重リング) ---
        SpawnCenterCircleRings(&pCenterRingInner, &pCenterRingOuter);

        // --- マレット(菱形弾、左右1体ずつ) ---
        for (int side = 0; side < 2; side++) {
            double x = (side == 0) ? 25.0 : 455.0;
            double muki = (side == 0) ? 0.0 : DX_PI; // 卓の中央を向く
            sEnemyShotSet* pMalletSet = CreateShotSet(ShotMallet, x, 240.0, muki);
            pMalletSet->param_i[0] = side * 157; // 2体の位相をずらす
            pMalletSet->param_i[1] = (side == 0) ? 1 : 2; // 黄/緑
        }
    }

    // --- 壁のパルス(上下の短レーザー壁が定期的に出現→退場) ---
    if (count % 180 == 1) {
        for (int side = 0; side < 2; side++) {
            double y = (side == 0) ? 45.0 : 435.0;
            sEnemyShotSet* pWallSet = CreateShotSet(ShotWallPulse, 240.0, y, 0.0);
            pWallSet->param_i[0] = side; // 0:上壁 1:下壁
        }
    }

    // --- パック投入(上下交互に、自機を狙ってジグザグに発射) ---
    if (count % 400 == 360) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (count % 400 == 20) {
        for (int side = 0; side < 2; side++) {
            double y = (side == 0) ? 70.0 : 410.0;
            double aimMuki = atan2(player.y - y, player.x - 240.0);
            // 縦方向の角度を誇張して反射のジグザグを目立たせる
            double exaggerated = atan2(sin(aimMuki) * 1.6, cos(aimMuki));
            CreateShotSet(ShotPuck, 240.0, y, exaggerated);
        }
    }

    // --- シュートライン(銃弾、左右3高さから水平連射) ---
    if (count % 220 == 110 || count % 220 == 190) {
        bool fromLeft = (count % 220 == 110);
        double muki = fromLeft ? 0.0 : DX_PI;
        double x = fromLeft ? 0.0 : 480.0;
        double ys[3] = { 140.0, 240.0, 340.0 };
        for (int i = 0; i < 3; i++) {
            CreateShotSet(ShotSweepLine, x, ys[i], muki);
        }
    }

    // --- ゴール演出(センターサークルを拡散させつつ左右へ収束弾) ---
    if (count % 600 == 300) {
        pCenterRingInner->param_i[0] = 1;
        pCenterRingOuter->param_i[0] = 1;

        for (int side = 0; side < 2; side++) {
            double muki = (side == 0) ? DX_PI : 0.0; // 左/右のゴールへ
            sEnemyShotSet* pGoalSet = CreateShotSet(ShotGoalBurst, 240.0, 240.0, muki);
            pGoalSet->param_i[0] = (side == 0) ? 1 : 2; // 黄/緑
        }
    }

    // ゴール演出の少し後、センターサークルを新しく張り直して再開する
    if (count % 600 == 360) {
        SpawnCenterCircleRings(&pCenterRingInner, &pCenterRingOuter);
    }

    // enemy本体は卓全体の存在感として左右にゆっくり揺れる
    enemy.x = 240.0 + 40.0 * sin(count * 0.01);
}