// 弾幕：線香花火のライフサイクル（点火・激発・終息）
static void ShotSenshokuHanabi(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // フェーズ1：点火・蕾の形成（count == 0 時点）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 中心で熱を帯びる小さな赤・オレンジの丸弾を複数生成（蕾）
        for (int i = 0; i < 6*2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(10) - 5);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(10) - 5);
            pEnemyShot->muki = (GetRand(60) / 60.0) * 2.0 * DX_PI;
            pEnemyShot->speed = 0.2 + GetRand(20) / 100.0; // ほとんど動かずその場で揺らぐ

            // 色8:橙、色0:赤 を活用
            int colorIndex = (i % 2 == 0) ? 0 : 8;
            pEnemyShot->kind = img_enemyShotSmallBall[colorIndex];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // フェーズ2：激発・火花が不規則に散る（count が 10 ～ 90 の間）
    if (pEnemyShotSet->count >= 10 && pEnemyShotSet->count <= 90 && pEnemyShotSet->count % 6 == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 鋭く飛び散る黄色(1)・白色(6)の針弾（銃弾素材を利用）
        for (int i = 0; i < 3*2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // プレイヤー方向を基準にランダムな広がりを持たせる
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(80) - 40) / 180.0 * DX_PI;
            pEnemyShot->speed = 3.5 + GetRand(250) / 100.0; // 初速が速い

            int colorIndex = (GetRand(1) == 0) ? 1 : 6; // 黄 or 白
            pEnemyShot->kind = img_enemyShotBullet[colorIndex]; // 銃弾を火花に見立てる

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // フェーズ3：終息・残り火が重力で垂れて消える（count が 100 以降）
    if (pEnemyShotSet->count >= 100 && pEnemyShotSet->count == 100) {
        // パタパタと落ちるオレンジの小玉（残り火）
        for (int i = 0; i < 8*2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(40) - 20);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(20) - 10);
            pEnemyShot->muki = (GetRand(60) / 60.0) * DX_PI; // 下方向へ広がる角度 (0 ～ PI)
            pEnemyShot->speed = 0.5 + GetRand(100) / 100.0;

            pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙の小玉

            // param_d[0] を縦方向の加速度（重力）として利用
            pEnemyShot->param_d[0] = 0.05;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾ごとの個別移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 終息フェーズの残り火には重力を加算
        if (pShot->param_d[0] != 0.0) {
            pShot->speed += pShot->param_d[0];
            pShot->y += 0.8; // 下への垂れ込み
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Sparkler_Gemini()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 画面上部をゆっくり左右に移動
        enemy.x += 0.8 * (double)muki;
        if (enemy.x < 100.0 || enemy.x > 380.0) muki *= -1;
    }

    // 一定周期で線香花火の弾幕セットを発生させる
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSenshokuHanabi;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;
        // 弾幕が存在し続けるフレーム数（終息まで余裕を持たせる）
        pEnemyShotSet->alive = 220;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}