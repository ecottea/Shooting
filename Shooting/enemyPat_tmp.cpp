// 弾幕：線香花火（フェーズ1：火球形成 → フェーズ2：火花飛散 → フェーズ3：消滅）
static void ShotSenkoHanabi(sEnemyShotSet* pEnemyShotSet)
{
    const int PHASE_DURATION = 60; // 各フェーズの持続時間（フレーム）
    int phase = pEnemyShotSet->count / PHASE_DURATION;

    if (phase == 0) {
        // フェーズ1：火球形成（低速誘導弾）→ 赤/橙系
        if (pEnemyShotSet->count == 0) {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK); // 効果音
            for (int i = 0; i < 5; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + GetRand(48) - 24;
                pEnemyShot->y = pEnemyShotSet->y + GetRand(40) - 20;
                pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(120) - 60) / 180.0 * DX_PI;
                pEnemyShot->speed = 1.0 + GetRand(100) / 100.0; // 低速
                pEnemyShot->kind = img_enemyShotSmallBall[GetRand(2)]; // 小玉（赤=0, 橙=8を想定）
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    else if (phase == 1) {
        // フェーズ2：火花飛散（放射状高速弾＋螺旋弾）→ 黄/橙系
        if (pEnemyShotSet->count % 30 == 0) { // 30フレームごとに発射
            // 放射状高速弾（銃弾）→ 黄=1, 橙=8
            for (int i = 0; i < 12; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = (DX_PI * 2 * i) / 12.0;
                pEnemyShot->speed = 5.0 + GetRand(100) / 10.0; // 高速
                pEnemyShot->kind = img_enemyShotBullet[GetRand(2) + 1]; // 銃弾（黄=1, 橙=8を想定）
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // 螺旋弾（鱗弾）→ 橙=8
            for (int dir = -1; dir <= 1; dir += 2) {
                for (int i = 0; i < 8; i++) {
                    sEnemyShot* pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = pEnemyShotSet->x;
                    pEnemyShot->y = pEnemyShotSet->y;
                    pEnemyShot->muki = (DX_PI * 2 * i) / 8.0 + dir * DX_PI / 4.0;
                    pEnemyShot->speed = 2.0 + GetRand(100) / 50.0; // 中速
                    pEnemyShot->kind = img_enemyShotScale[8]; // 鱗弾（橙=8）
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
    }
    else if (phase == 2) {
        // フェーズ3：消滅の余韻（拡散弾）→ 白=6, 黄=1
        if (pEnemyShotSet->count % 15 == 0) {
            for (int i = 0; i < 10; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + GetRand(240) - 120;
                pEnemyShot->y = pEnemyShotSet->y + GetRand(200) - 100;
                pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
                pEnemyShot->speed = 3.0 - (pEnemyShotSet->count % 30) / 10.0; // 徐々に減速
                pEnemyShot->kind = img_enemyShotMediumOval[GetRand(2) + 6]; // 中楕円弾（白=6, 黄=1）
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 全弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Tmp()
{
    static int shot_count = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)((count % 240) < 120 ? 1 : -1); // 往復移動
    }

    if (count % 60 == 1) { // 60フレームごとに弾幕発射
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSenkoHanabi;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->alive = 180; // 3フェーズ分（60×3）

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}