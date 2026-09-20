// 弾幕：直線弾（相手orプレイヤー狙い）
static void ShotStraight(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // param_i[0] : 弾数
        // param_i[1] : 色（4=青, 0=赤 など）
        // param_d[0] : 速度
        int num = pEnemyShotSet->param_i[0] > 0 ? pEnemyShotSet->param_i[0] : 1;
        int color = pEnemyShotSet->param_i[1];
        double spd = pEnemyShotSet->param_d[0] > 0.0 ? pEnemyShotSet->param_d[0] : 3.0;

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 少しだけばらけさせる
            double angleOffset = (num > 1) ? ((i - (num - 1) / 2.0) * 0.08) : 0.0;
            pEnemyShot->muki = pEnemyShotSet->muki + angleOffset;
            pEnemyShot->speed = spd;
            pEnemyShot->kind = img_enemyShotMediumBall[color];  // 中玉
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 弾幕：扇状弾（相手狙い用）
static void ShotFan(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int num = pEnemyShotSet->param_i[0] > 0 ? pEnemyShotSet->param_i[0] : 5;
        int color = pEnemyShotSet->param_i[1];
        double spd = pEnemyShotSet->param_d[0] > 0.0 ? pEnemyShotSet->param_d[0] : 2.8;
        double spread = pEnemyShotSet->param_d[1] > 0.0 ? pEnemyShotSet->param_d[1] : 0.35; // 扇の広がり

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            double angleOffset = (i - (num - 1) / 2.0) * spread;
            pEnemyShot->muki = pEnemyShotSet->muki + angleOffset;
            pEnemyShot->speed = spd;
            pEnemyShot->kind = img_enemyShotSmallBall[color];  // 小玉
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

// 弾幕：環状弾（よろめき反撃用）
static void ShotRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int num = pEnemyShotSet->param_i[0] > 0 ? pEnemyShotSet->param_i[0] : 12;
        int color = pEnemyShotSet->param_i[1];
        double spd = pEnemyShotSet->param_d[0] > 0.0 ? pEnemyShotSet->param_d[0] : 2.2;

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (DX_PI * 2.0 * i / num);
            pEnemyShot->speed = spd;
            pEnemyShot->kind = img_enemyShotScale[color];  // 鱗弾
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

// 敵本体のパターン（仲間割れ）
void EnemyPat_FallOut_Grok()
{
    // 移動用パラメータ
    static double phase1 = 0.0;
    static double phase2 = 0.0;
    static int phase = 0;          // 0:序盤 1:中盤 2:終盤
    static int lastHitFrame = -999; // よろめき演出用

    if (count == 1) {
        // 初期位置
        enemy.x = 120.0;
        enemy.y = 80.0;
        enemy.x2 = 360.0;
        enemy.y2 = 80.0;
        enemy.maxHp = enemy.hp = 200;
        phase1 = 0.0;
        phase2 = DX_PI;           // 位相をずらす
        phase = 0;
        lastHitFrame = -999;
    }
    else {
        // ===== 移動パターン =====
        // お互いに近づいたり離れたりしながら画面を大きく動く
        phase1 += 0.025;
        phase2 += 0.028;

        double baseY = 100.0 + 40.0 * sin(count * 0.02);

        if (phase == 0) {
            // 序盤：左右に分かれて上下移動
            enemy.x = 140.0 + 30.0 * sin(phase1);
            enemy.y = baseY + 50.0 * sin(phase1 * 1.3);
            enemy.x2 = 340.0 + 30.0 * sin(phase2);
            enemy.y2 = baseY + 50.0 * sin(phase2 * 1.3);
        }
        else if (phase == 1) {
            // 中盤：斜めに交差するように動く
            enemy.x = 240.0 + 160.0 * sin(phase1);
            enemy.y = 120.0 + 80.0 * cos(phase1 * 0.8);
            enemy.x2 = 240.0 + 160.0 * sin(phase2);
            enemy.y2 = 120.0 + 80.0 * cos(phase2 * 0.8);
        }
        else {
            // 終盤：激しく接近・離反
            double dist = 80.0 + 60.0 * sin(count * 0.04);
            double angle = count * 0.03;
            enemy.x = 240.0 + dist * cos(angle);
            enemy.y = 140.0 + dist * 0.6 * sin(angle);
            enemy.x2 = 240.0 + dist * cos(angle + DX_PI);
            enemy.y2 = 140.0 + dist * 0.6 * sin(angle + DX_PI);
        }

        // フェーズ遷移
        if (count == 300) phase = 1;      // 約5秒後
        if (count == 720) phase = 2;      // 約12秒後
    }

    // ===== 弾発射 =====
    // 二人の距離を計算（よろめき判定の擬似）
    double dx = enemy.x2 - enemy.x;
    double dy = enemy.y2 - enemy.y;
    double dist = sqrt(dx * dx + dy * dy);

    // お互いに向かって撃つ基本弾（高頻度）
    if (count % 12 == 1) {
        // ボス1 → ボス2 方向
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotStraight;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = atan2(enemy.y2 - enemy.y, enemy.x2 - enemy.x);
            pSet->param_i[0] = 1;               // 弾数
            pSet->param_i[1] = 0;               // 赤
            pSet->param_d[0] = 3.5;             // 速度
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        // ボス2 → ボス1 方向
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotStraight;
            pSet->x = enemy.x2;
            pSet->y = enemy.y2;
            pSet->muki = atan2(enemy.y - enemy.y2, enemy.x - enemy.x2);
            pSet->param_i[0] = 1;
            pSet->param_i[1] = 4;               // 青
            pSet->param_d[0] = 3.5;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }

    // 中盤以降：扇状弾で相手を狙い始める
    if (phase >= 1 && count % 45 == 5) {
        // ボス1が扇を撃つ
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotFan;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = atan2(enemy.y2 - enemy.y, enemy.x2 - enemy.x);
            pSet->param_i[0] = 7;               // 弾数
            pSet->param_i[1] = 0;               // 赤
            pSet->param_d[0] = 2.6;
            pSet->param_d[1] = 0.22;            // 広がり
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    if (phase >= 1 && count % 45 == 25) {
        // ボス2が扇を撃つ
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotFan;
            pSet->x = enemy.x2;
            pSet->y = enemy.y2;
            pSet->muki = atan2(enemy.y - enemy.y2, enemy.x - enemy.x2);
            pSet->param_i[0] = 7;
            pSet->param_i[1] = 4;               // 青
            pSet->param_d[0] = 2.6;
            pSet->param_d[1] = 0.22;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }

    // 距離が近づいたときに「当たった」とみなして環状弾を出す（よろめき反撃）
    if (dist < 90.0 && count - lastHitFrame > 40) {
        lastHitFrame = count;
        // ボス1から環状
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotRing;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->param_i[0] = 14;
            pSet->param_i[1] = 0;               // 赤
            pSet->param_d[0] = 2.0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        // ボス2から環状
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotRing;
            pSet->x = enemy.x2;
            pSet->y = enemy.y2;
            pSet->muki = 0.0;
            pSet->param_i[0] = 14;
            pSet->param_i[1] = 4;               // 青
            pSet->param_d[0] = 2.0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }

    // 終盤：プレイヤー方向への牽制弾も追加
    if (phase == 2 && count % 30 == 10) {
        // ボス1からプレイヤーへ
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotStraight;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
            pSet->param_i[0] = 3;
            pSet->param_i[1] = 8;               // 橙
            pSet->param_d[0] = 4.0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        // ボス2からプレイヤーへ
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotStraight;
            pSet->x = enemy.x2;
            pSet->y = enemy.y2;
            pSet->muki = atan2(player.y - enemy.y2, player.x - enemy.x2);
            pSet->param_i[0] = 3;
            pSet->param_i[1] = 3;               // シアン寄り
            pSet->param_d[0] = 4.0;
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