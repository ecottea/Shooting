// enemyPat_tmp.cpp
// 弾幕:「エアホッケー・ラリー」
//  大玉=パック / 中玉=マレット / 小玉=アイスのスパーク として表現する
//
//  [流れ]
//  1. サーブ     : 予告音の後、敵がパック(大玉)を自機めがけて発射
//  2. ラリー     : パックは画面の壁で反射し続ける
//                  敵はマレット(中玉)を飛ばしてパックに追突させ、
//                  パックを自機方向へ打ち返しつつ加速させる(最大4回)
//                  パックが壁に当たるたびにスパーク(小玉)が散る
//  3. フィニッシュ: 5回目の打撃で敵はミスショット。
//                  パックは画面下のゴールへ抜けていき、周囲に小玉が一斉放出される
//  4. 休憩       : 2秒後、再びサーブから繰り返し

// 弾の役割を param_i[0] に保存する(1:パック 2:マレット 3:スパーク)
static const double PACK_R = 12.0; // パック(大玉)の反射判定半径
static const double WALL = 8.0;  // 壁(画面端)の内側座標
static const int    MAX_RALLY = 4;    // 自機を狙って打ち返す回数(次の打撃でミスショット)

// 弾をリストの末尾に追加して返す
static sEnemyShot* AddShot(sEnemyShotSet* pSet, int type, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->param_i[0] = type;
    pEnemyShot->kind = kind;
    pEnemyShot->prev = pSet->pEnemyShotHead->prev;
    pEnemyShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pEnemyShot;
    pSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// パックが壁に当たった瞬間に散るアイスのスパーク
static void SpawnSpark(sEnemyShotSet* pSet, double x, double y, double baseMuki, int num)
{
    for (int i = 0; i < num; i++) {
        sEnemyShot* pEnemyShot = AddShot(pSet, 3, img_enemyShotSmallBall[3]); // 小玉:シアン
        pEnemyShot->x = x;
        pEnemyShot->y = y;
        pEnemyShot->muki = baseMuki + (GetRand(60) - 30) / 180.0 * DX_PI; // 反射方向±30度
        pEnemyShot->speed = 2.0 + GetRand(250) / 100.0;
    }
}

// フィニッシュ時にパックの周囲へ一斉放出
static void SpawnBurst(sEnemyShotSet* pSet, double x, double y, int num)
{
    for (int i = 0; i < num; i++) {
        sEnemyShot* pEnemyShot = AddShot(pSet, 3, img_enemyShotSmallBall[1]); // 小玉:黄
        pEnemyShot->x = x;
        pEnemyShot->y = y;
        pEnemyShot->muki = DX_PI * 2.0 * i / num + (GetRand(10) - 5) / 180.0 * DX_PI;
        pEnemyShot->speed = 3.0 + GetRand(100) / 100.0;
    }
}

// エアホッケー・ラリーの弾幕セット
static void ShotAirHockey(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot;
    sEnemyShot* pPack = nullptr;
    bool malletAlive = false;

    // ---- フェーズ0:サーブ待ち ----
    if (pEnemyShotSet->param_i[0] == 0) {
        pEnemyShotSet->param_i[4]++;
        if (pEnemyShotSet->param_i[4] == 1) {
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 予告音
        }
        if (pEnemyShotSet->param_i[4] >= 20) {
            // パック(大玉・白)をサーブ:自機めがけて発射
            pPack = AddShot(pEnemyShotSet, 1, img_enemyShotLargeBall[6]);
            pPack->x = 240.0;
            pPack->y = 60.0;
            pPack->muki = atan2(player.y - pPack->y, player.x - pPack->x);
            pPack->speed = 3.0;
            pPack->margin = 40;

            pEnemyShotSet->param_i[0] = 1;   // ラリーへ
            pEnemyShotSet->param_i[1] = 0;   // ラリー回数
            pEnemyShotSet->param_i[2] = 45;  // マレット再発射タイマー
            pEnemyShotSet->param_i[3] = 100 + GetRand(280); // ミスショットのゴールx(100〜380)
            pEnemyShotSet->param_i[4] = 0;
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }

    // ---- パックの移動と壁反射 ----
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            pPack = pShot;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // ラリー中は壁で反射する(フィニッシュ後はゴールへそのまま抜ける)
            if (pEnemyShotSet->param_i[0] == 1) {
                double newMuki = pShot->muki;
                bool bounced = false;

                if (pShot->x < WALL) {              // 左壁
                    pShot->x = WALL; newMuki = DX_PI - newMuki; bounced = true;
                }
                else if (pShot->x > 480.0 - WALL) { // 右壁
                    pShot->x = 480.0 - WALL; newMuki = DX_PI - newMuki; bounced = true;
                }
                if (pShot->y < WALL) {              // 上壁
                    pShot->y = WALL; newMuki = -newMuki; bounced = true;
                }
                else if (pShot->y > 480.0 - WALL) { // 下壁
                    pShot->y = 480.0 - WALL; newMuki = -newMuki; bounced = true;
                }
                pShot->muki = newMuki;

                if (bounced) {
                    if (!CheckSoundMem(sound_enemyShot_light))
                        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                    // ラリーが続くほどスパークが増える(最大8発)
                    int n = 4 + pEnemyShotSet->param_i[1];
                    if (n > 8) n = 8;
                    SpawnSpark(pEnemyShotSet, pShot->x, pShot->y, newMuki, n*10);
                }
            }
        }
        pShot = pShot->next;
    }

    // ---- マレットとスパークの移動 ----
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 2) { // マレット(中玉)
            malletAlive = true;
            if (pPack != nullptr) {
                double dx = pPack->x - pShot->x;
                double dy = pPack->y - pShot->y;
                // パックを追尾(必ず追いつける速度)
                pShot->muki = atan2(dy, dx);
                pShot->speed = pPack->speed + 1.5;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);

                // パックに追突したら打ち返す
                if (dx * dx + dy * dy < 24.0 * 24.0) {
                    pEnemyShotSet->param_i[1]++;

                    if (pEnemyShotSet->param_i[1] > MAX_RALLY) {
                        // 敵のミスショット:画面下のゴールへ抜けていく
                        pEnemyShotSet->param_i[0] = 2;
                        pPack->muki = atan2(520.0 - pPack->y, pEnemyShotSet->param_i[3] - pPack->x);
                        SpawnBurst(pEnemyShotSet, pPack->x, pPack->y, 16*10);
                    }
                    else {
                        // パックを自機の現在地めがけて打ち返し、加速する
                        pPack->muki = atan2(player.y - pPack->y, player.x - pPack->x);
                        if (pPack->speed < 8.0) pPack->speed += 0.8;
                    }
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                    // 打ったマレットは消す
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                    pEnemyShotSet->param_i[2] = 50;
                }
            }
            else {
                // パックが無い場合はそのまま直進(画面外でメインルーチンが消去)
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else if (pShot->param_i[0] == 3) { // スパーク(小玉)
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pNext;
    }

    // ---- フェーズ管理 ----
    if (pEnemyShotSet->param_i[0] == 1) {
        // マレットの再発射
        if (pEnemyShotSet->param_i[2] > 0) pEnemyShotSet->param_i[2]--;
        if (pEnemyShotSet->param_i[2] == 0 && !malletAlive && pPack != nullptr) {
            sEnemyShot* pMallet = AddShot(pEnemyShotSet, 2, img_enemyShotMediumBall[8]); // 中玉:橙
            pMallet->x = enemy.x;           // 敵の現在位置から発射
            pMallet->y = enemy.y + 15.0;
            pMallet->muki = atan2(pPack->y - pMallet->y, pPack->x - pMallet->x);
            pMallet->speed = 4.0;
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }
    else if (pEnemyShotSet->param_i[0] == 2) {
        // パックが画面外へ抜けたら休憩へ
        if (pPack == nullptr) {
            pEnemyShotSet->param_i[0] = 3;
            pEnemyShotSet->param_i[4] = 0;
        }
    }
    else if (pEnemyShotSet->param_i[0] == 3) {
        pEnemyShotSet->param_i[4]++;
        if (pEnemyShotSet->param_i[4] >= 10) {
            // サーブ待ちへ戻って無限ループ
            pEnemyShotSet->param_i[0] = 0;
            pEnemyShotSet->param_i[4] = 0;
        }
    }
}

// 敵本体のパターン
void EnemyPat_AirHockey_Zai()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;

        // エアホッケー・ラリー用の弾幕セットを1つだけ生成する
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAirHockey;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->kind = 0;
        // param_i[0]:フェーズ(0:サーブ待ち 1:ラリー中 2:フィニッシュ 3:休憩)
        // param_i[1]:ラリー回数  param_i[2]:マレット再発射タイマー
        // param_i[3]:ゴールx座標  param_i[4]:フェーズ内タイマー
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[4] = 0;
        pEnemyShotSet->alive = 99999;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵は台の上を左右にゆっくり移動する(マレットの発射位置が変わる)
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 60.0)  muki = 1;
        if (enemy.x > 420.0) muki = -1;
    }
}