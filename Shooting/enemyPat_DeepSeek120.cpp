// ============================================================
//  仲間割れ弾幕：背信の十字火（クロスファイア・ディスコード）
// ============================================================

// 弾幕セットを新規生成してリストに繋ぐ共通ヘルパ
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// 弾幕：相手の現在位置を狙って撃つ（弾は相手をすり抜ける）
static void ShotCrossFire(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 発射時に相手の座標を記録済み（param_d[0], param_d[1]）
        double targetX = pEnemyShotSet->param_d[0];
        double targetY = pEnemyShotSet->param_d[1];
        double baseAngle = atan2(targetY - pEnemyShotSet->y,
            targetX - pEnemyShotSet->x);

        int    phase = pEnemyShotSet->param_i[0];
        int    way = (phase >= 2) ? 5 : 3;
        double speed = (phase == 0) ? 2.0 : (phase == 1) ? 3.0 : 4.5;
        double spread = 6.0 / 180.0 * DX_PI;

        for (int i = -(way / 2); i <= (way / 2); i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + i * spread;
            pEnemyShot->speed = speed;
            pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白（仲間弾）

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

// 弾幕：各ボス中心の円弾
static void ShotRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int    phase = pEnemyShotSet->param_i[0];
        int    way = (phase >= 2) ? 18 : 12;
        double speed = (phase == 0) ? 1.2 : (phase == 1) ? 1.6 : 2.0;

        for (int i = 0; i < way; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * 2.0 * DX_PI / way;
            pEnemyShot->speed = speed;
            pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄

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

// 敵本体のパターン
void EnemyPat_FallOut_DeepSeek()
{
    const double CX = 240.0, CY = 240.0; // 画面中心（480x480）
    const double ORBIT_R = 130.0;        // 軌道半径

    if (count == 1) {
        enemy.maxHp = enemy.hp = 200;
    }

    // HP共有なのでフェーズは1本
    int phase;
    if (enemy.hp > 140) phase = 0; // 睨み合い
    else if (enemy.hp > 80)  phase = 1; // 亀裂
    else                     phase = 2; // 共倒れ

    // ===== ボス移動：画面中心を挟んだ点対称の円運動 =====
    double angSpeed = 0.008 + phase * 0.004; // フェーズで加速
    double angle = count * angSpeed;
    enemy.x = CX + ORBIT_R * cos(angle);
    enemy.y = CY + ORBIT_R * sin(angle);
    enemy.x2 = CX - ORBIT_R * cos(angle);
    enemy.y2 = CY - ORBIT_R * sin(angle);

    // ===== 相手狙い弾の発射 =====
    int crossInterval = (phase == 0) ? 30 : (phase == 1) ? 20 : 15;
    if (count % crossInterval == 0) {
        // 2体それぞれが「相手の現在位置」を狙う
        for (int b = 0; b < 2; b++) {
            sEnemyShotSet* pSet = CreateShotSet(
                ShotCrossFire,
                (b == 0) ? enemy.x : enemy.x2,
                (b == 0) ? enemy.y : enemy.y2);
            pSet->muki = 0;
            pSet->param_i[0] = phase;
            // 相手の座標を発射時に記録（これが回廊の軸になる）
            pSet->param_d[0] = (b == 0) ? enemy.x2 : enemy.x;
            pSet->param_d[1] = (b == 0) ? enemy.y2 : enemy.y;
        }
    }

    // ===== 各ボス中心の円弾 =====
    int ringInterval = (phase == 0) ? 120 : (phase == 1) ? 90 : 60;
    if (count % ringInterval == 0) {
        for (int b = 0; b < 2; b++) {
            sEnemyShotSet* pSet = CreateShotSet(
                ShotRing,
                (b == 0) ? enemy.x : enemy.x2,
                (b == 0) ? enemy.y : enemy.y2);
            pSet->muki = count * 0.02; // リングが回転
            pSet->param_i[0] = phase;
        }
    }
}