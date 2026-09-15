// enemyPat_Tmp.cpp
// 花実符「ケント・アップルフォール」
// ケントの花が咲いて散り、赤い実(リンゴ)が重力落下する弾幕

//------------------------------------------------------------
// 素材選定メモ
// 小玉(2.5x2.5) : 花粉、蜜の雫 -> 小さくてフワフワさせたい
// 中玉(7.0x7.0) : 花の中心(黄色)
// 大玉(20.0x20.0) : リンゴ本体(赤) -> 落下する重さを表現
// 鱗弾(4.0x3.0) : 葉っぱ -> 葉の形に最も近い
// 菱形弾(4.5x2.5) : 種 -> とがった種に見える
// 中楕円弾(10.5x7.0) : 花びら -> 楕円で花弁の形を表現できる唯一の素材
// 色: 0:赤 1:黄 2:緑 6:白 7:黒 8:橙
//------------------------------------------------------------

// 花パターン
static void ShotKentFlower(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 5枚の花びら : 中楕円弾[白]
        for (int i = 0; i < 5; i++) {
            pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = (DX_PI * 2.0 / 5.0) * i;
            pShot->speed = 0;
            pShot->kind = img_enemyShotMediumOval[6]; // 白
            pShot->param_i[0] = 0; // 0=花びら
            pShot->param_d[0] = (DX_PI * 2.0 / 5.0) * i; // 基準角度
            pShot->param_d[1] = 0.0; // 半径
            pShot->margin = 480;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        // 中心 : 中玉[黄]
        pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->kind = img_enemyShotMediumBall[1]; // 黄
        pShot->param_i[0] = 1; // 1=中心
        pShot->margin = 480;
        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;

        // 葉っぱ2枚 : 鱗弾[緑]
        for (int i = 0; i < 2; i++) {
            pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->kind = img_enemyShotScale[2]; // 緑
            pShot->param_i[0] = 2; // 2=葉
            pShot->param_d[0] = (i == 0 ? -0.8 : 0.8); // 左右オフセット角度
            pShot->margin = 480;
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 花粉生成: 30f〜150fの間、8f毎に2発
    if (pSet->count >= 30 && pSet->count <= 150 && pSet->count % 8 == 0) {
        for (int k = 0; k < 2*2; k++) {
            pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            // GetRand(x)は0〜xまでなので注意
            pShot->muki = (GetRand(360) / 360.0) * DX_PI * 2.0;
            pShot->speed = 0.8 + GetRand(120) / 100.0; // 0.8〜2.0
            pShot->kind = img_enemyShotSmallBall[1]; // 黄 小玉
            pShot->param_i[0] = 3; // 3=花粉
            pShot->param_d[5] = 0; // wobble用
            pShot->margin = 480;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 更新
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p->param_i[0] == 0) { // 花びら
            double baseAng = p->param_d[0];
            double rot = pSet->count * 0.008; // ゆっくり回転
            double ang = baseAng + rot;

            // 蕾から開花: 半径を0→26まで広げる
            if (pSet->count < 60) {
                p->param_d[1] = 8.0 + pSet->count * 0.30;
            }
            else {
                p->param_d[1] = 26.0;
            }
            double r = p->param_d[1];

            // 散華フェーズ
            double extra = 0;
            if (pSet->count > 180) {
                extra = (pSet->count - 180) * 0.85;
            }

            p->x = pSet->x + (r + extra) * cos(ang);
            p->y = pSet->y + (r + extra) * sin(ang);
            p->muki = ang; // 楕円が外を向くように

        }
        else if (p->param_i[0] == 1) { // 中心
            p->x = pSet->x + sin(pSet->count * 0.12) * 1.5;
            p->y = pSet->y + cos(pSet->count * 0.10) * 1.5;
        }
        else if (p->param_i[0] == 2) { // 葉
            double side = p->param_d[0];
            p->x = pSet->x + side * 28.0 + sin(pSet->count * 0.02) * 2.0;
            p->y = pSet->y + 18.0;
            p->muki = side * 0.6 + DX_PI * 0.5;
        }
        else if (p->param_i[0] == 3) { // 花粉
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki) + 0.03; // 少し重力
            p->muki += sin(pSet->count * 0.05 + p->param_d[5]) * 0.02;
        }

        p = p->next;
    }
}

// リンゴ落下パターン
static void ShotKentApple(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pShot = new sEnemyShot;
        pShot->x = pSet->x;
        pShot->y = pSet->y;
        pShot->kind = img_enemyShotLargeBall[0]; // 赤 大玉 = リンゴ
        pShot->param_i[0] = 0; // 0=リンゴ本体
        // 初速: 自機方向に少し振る + 上に少し跳ねる
        double toPlayer = atan2(player.y - pSet->y, player.x - pSet->x);
        // GetRandで左右にブレ
        double spread = (GetRand(60) - 30) / 180.0 * DX_PI; // -30〜+30度
        pShot->param_d[0] = cos(toPlayer + spread) * 0.8; // vx
        pShot->param_d[1] = -2.5; // vy 初期は少し上に
        pShot->param_d[2] = GetRand(100) / 100.0 * DX_PI * 2.0; // 揺れ位相
        pShot->margin = 40.0; // 大玉なので余裕を持たせる

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = p->next; // 生成中にnextが変わるので保存

        if (p->param_i[0] == 0) { // リンゴ本体
            // 重力落下
            p->param_d[1] += 0.12; // vy += g
            p->x += p->param_d[0] + sin(pSet->count * 0.08 + p->param_d[2]) * 0.4;
            p->y += p->param_d[1];

            // 蜜の雫をこぼす
            if (pSet->count % 3 == 0) {
                sEnemyShot* drop = new sEnemyShot;
                drop->x = p->x;
                drop->y = p->y + 8.0;
                drop->muki = DX_PI * 0.5; // 真下
                drop->speed = 1.8 + GetRand(40) / 100.0;
                drop->kind = img_enemyShotSmallBall[0]; // 赤 小玉
                drop->param_i[0] = 1; // 1=蜜
                drop->margin = 40.0;

                drop->prev = pSet->pEnemyShotHead->prev;
                drop->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = drop;
                pSet->pEnemyShotHead->prev = drop;
            }

            // 地面で弾ける
            if (p->y > 460.0) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                // 種を8方向に
                for (int i = 0; i < 8*2; i++) {
                    sEnemyShot* seed = new sEnemyShot;
                    seed->x = p->x;
                    seed->y = p->y;
                    seed->muki = (DX_PI * 2.0 / 8.0/2) * i + (GetRand(20) - 10) / 180.0 * DX_PI;
                    seed->speed = 2.2 + GetRand(80) / 100.0;
                    seed->kind = img_enemyShotDiamond[7]; // 黒 菱形 = 種
                    // 1発だけ自機狙いを混ぜる
                    if (i == 0) seed->muki = atan2(player.y - seed->y, player.x - seed->x);
                    seed->param_i[0] = 2; // 2=種
                    seed->margin = 40.0;

                    seed->prev = pSet->pEnemyShotHead->prev;
                    seed->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = seed;
                    pSet->pEnemyShotHead->prev = seed;
                }
                // リンゴ本体は画面外に飛ばしてメイン側で消去させる
                p->y = 1000.0;
            }
        }
        else if (p->param_i[0] == 1) { // 蜜
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        else if (p->param_i[0] == 2) { // 種
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }

        p = pNext;
    }
}

// 敵本体
void EnemyPat_FlowerOfKent_MetaAI()
{
    static int muki;
    static int shotId;
    static double lastFlowerX;
    static double lastFlowerY;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotId = 0;
        lastFlowerX = 240.0;
        lastFlowerY = 120.0;
    }
    else {
        // ゆっくり左右移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x > 360) muki = -1;
        if (enemy.x < 120) muki = 1;
    }

    // 240Fサイクルで花→実→落下
    if (count % 180 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotKentFlower;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 30.0;
        pSet->muki = 0;
        pSet->kind = shotId++;

        lastFlowerX = pSet->x;
        lastFlowerY = pSet->y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    if (count % 180 == 90) {
        // 前回咲いた花の位置からリンゴを落とす
        // 2回に1回は2個同時に落として難易度UP
        int numApple = (count % 480 == 90) ? 1 : 2;

        for (int a = 0; a < numApple; a++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotKentApple;
            pSet->x = lastFlowerX + (a * 80 - 40) + (GetRand(40) - 20);
            pSet->y = lastFlowerY;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->kind = shotId++;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}