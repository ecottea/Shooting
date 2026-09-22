// enemyPat_Tmp.cpp (または enemyPat_sampleForAI.cpp に追記)

// 弾幕：壁反射とヒットエフェクト
static void ShotWallBounce(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 通常の移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        if (pShot->param_i[0] == 1) {
            pShot = pShot->next;
            continue;
        }

        // 壁反射判定 (ゲーム画面幅 480)
        bool reflected = false;
        if (pShot->x <= 15.0) {
            pShot->x = 15.0;
            pShot->muki = DX_PI - pShot->muki; // 入射角＝反射角
            reflected = true;
        }
        else if (pShot->x > 465.0) {
            pShot->x = 465.0;
            pShot->muki = DX_PI - pShot->muki;
            reflected = true;
        }

        // 反射時のヒットエフェクト生成
        if (reflected) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            // 壁に当たった位置から火花を散らす
            for (int i = 0; i < 8*5; i++) {
                sEnemyShot* pEff = new sEnemyShot;
                pEff->x = pShot->x;
                pEff->y = pShot->y;
                pEff->muki = (GetRand(360) / 180.0) * DX_PI; // 全方位にばら撒く
                pEff->speed = 1.0 + GetRand(150) / 100.0;
                pEff->kind = img_enemyShotSmallBall[1]; // 1:黄 (火花イメージ)
                pEff->param_i[0] = 1;

                pEff->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEff->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEff;
                pEnemyShotSet->pEnemyShotHead->prev = pEff;
            }
        }

        pShot = pShot->next;
    }
}

// 弾幕：ストライカーの軌跡（風圧）用
static void ShotTrail(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_AirHockey_Qwen()
{
    static int muki;
    static int wait_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0; // やや下げて攻撃範囲を確保
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        wait_count = 0;
    }
    else {
        // 端で少し「溜める」動作を入れることで、打つ瞬間のリズムを表現
        if (wait_count > 0) {
            wait_count--;
        }
        else {
            enemy.x += 1.5 * (double)muki * 3;

            // 左端到達
            if (enemy.x <= 50.0) {
                enemy.x = 50.0;
                muki = 1;
                wait_count = 15; // 右へ打つ前に15フレーム溜める
            }
            // 右端到達
            else if (enemy.x >= 430.0) {
                enemy.x = 430.0;
                muki = -1;
                wait_count = 15; // 左へ打つ前に15フレーム溜める
            }
        }
    }

    // 【1】ストライカーの軌跡（風圧）
    // 移動中かつ溜め中でない場合に、進行方向と逆向きに鱗弾を撒き散らす
    if (wait_count == 0 && count % 4 == 0) {
        sEnemyShotSet* pTrailSet = new sEnemyShotSet;
        pTrailSet->count = 0;
        pTrailSet->patternFunc = ShotTrail;
        pTrailSet->x = enemy.x;
        pTrailSet->y = enemy.y + 15.0;
        pTrailSet->muki = (muki > 0) ? DX_PI : 0; // 進行方向と逆向き
        pTrailSet->kind = 0;

        pTrailSet->pEnemyShotHead = new sEnemyShot;
        pTrailSet->pEnemyShotHead->prev = pTrailSet->pEnemyShotHead;
        pTrailSet->pEnemyShotHead->next = pTrailSet->pEnemyShotHead;

        for (int i = 0; i < 2; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pTrailSet->x + GetRand(16) - 8;
            pShot->y = pTrailSet->y + GetRand(16) - 8;
            pShot->muki = pTrailSet->muki + (GetRand(60) - 30) / 180.0 * DX_PI;
            pShot->speed = 1.0 + GetRand(100) / 100.0;
            pShot->kind = img_enemyShotScale[3]; // 3:シアン (風圧・軌跡イメージ)

            pShot->prev = pTrailSet->pEnemyShotHead->prev;
            pShot->next = pTrailSet->pEnemyShotHead;
            pTrailSet->pEnemyShotHead->prev->next = pShot;
            pTrailSet->pEnemyShotHead->prev = pShot;
        }

        pTrailSet->prev = enemyShotSetHead.prev;
        pTrailSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pTrailSet;
        enemyShotSetHead.prev = pTrailSet;
    }

    // 【2】パック弾の発射
    // 溜め動作の頂点（動きが止まった瞬間）で自機を狙って発射
    if (wait_count == 10) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWallBounce;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x) + muki * DX_PI / 2.5;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = pEnemyShotSet->x;
        pShot->y = pEnemyShotSet->y;
        pShot->muki = pEnemyShotSet->muki;
        pShot->speed = 4.0; // パックは速く飛ばす
        pShot->kind = img_enemyShotLargeBall[6]; // 6:白 (視認性の高いパックイメージ)

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 発射音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
}