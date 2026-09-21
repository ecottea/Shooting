// 弾幕：ゴルトンボード（ガルトンの審判）
static void ShotGaltonBoard(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 発射時の効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 種弾の生成（白い中玉）
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki;
        pShot->speed = 2.5;
        pShot->kind = img_enemyShotMediumBall[6]; // 6:白
        pShot->param_i[0] = 0; // param_i[0] を「分裂世代」として使用

        // リストに追加
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    // 収束フェーズ：セット生成から一定時間（約3秒）経過後、画面下部の弾を自機狙いに変化させる
    bool isConvergence = (pEnemyShotSet->count == 180);

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next; // 削除・変更される可能性があるので退避

        // 分裂処理：世代が5未満で、かつ20フレーム経過ごとに左右に分裂
        if (pShot->param_i[0] < 5 && pShot->count > 0 && pShot->count % 20 == 0) {
            // 分裂時の軽い効果音
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            // 左に分裂する弾（青い小玉）
            sEnemyShot* pLeft = new sEnemyShot;
            pLeft->x = pShot->x;
            pLeft->y = pShot->y;
            // 基本角度 -0.3 に、GetRandを使ったわずかなばらつき(±0.05)を与えて確率分布を表現
            pLeft->muki = pShot->muki - 0.3 + (GetRand(10) - 5) / 100.0;
            pLeft->speed = pShot->speed;
            pLeft->kind = img_enemyShotSmallBall[4]; // 4:青
            pLeft->param_i[0] = pShot->param_i[0] + 1;

            pLeft->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pLeft->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pLeft;
            pEnemyShotSet->pEnemyShotHead->prev = pLeft;

            // 右に分裂する弾（青い小玉）
            sEnemyShot* pRight = new sEnemyShot;
            pRight->x = pShot->x;
            pRight->y = pShot->y;
            // 基本角度 +0.3 に、GetRandを使ったわずかなばらつき(±0.05)を与えて確率分布を表現
            pRight->muki = pShot->muki + 0.3 + (GetRand(10) - 5) / 100.0;
            pRight->speed = pShot->speed;
            pRight->kind = img_enemyShotSmallBall[4]; // 4:青
            pRight->param_i[0] = pShot->param_i[0] + 1;

            pRight->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRight->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRight;
            pEnemyShotSet->pEnemyShotHead->prev = pRight;

            // 親弾は「ペグ」としての役割を終えるため、画面外へ飛ばしてメインルーチンに消去させる
            pShot->y = -1000.0;
            pShot->speed = 0.0;
        }
        else {
            // 収束フェーズかつ最終世代(世代5)の弾は、自機狙いに変化して加速する（審判の収束）
            if (isConvergence && pShot->param_i[0] == 5 && pShot->speed > 0.0) {
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->speed = 4.5; // 加速
                pShot->kind = img_enemyShotScale[0]; // 0:赤（危険な鱗弾に変化して警告）
            }

            // 通常の移動処理
            if (pShot->speed > 0.0) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }

        pShot = nextShot;
    }
}

// 敵本体のパターン（ゴルトンボード実装版）
void EnemyPat_GaltonBoard_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0; // やや上めに配置し、落下距離を確保
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右への緩やかな移動
        enemy.x += 1.2 * (double)muki;
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            muki *= -1;
        }
    }

    // 90フレーム（約1.5秒）ごとにゴルトンボード弾幕を発射
    if (count % 90 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGaltonBoard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;

        // 種弾は「真下」に向かって発射。分裂によって左右に広がり、結果的に正規分布を形成する
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}