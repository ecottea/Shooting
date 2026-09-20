// 弾幕：仲間割れ - 裏切り
// ボス1(赤)がボス2を、ボス2(青)がボス1を狙う。プレイヤーはその交差点に立つ。

// ボス1 -> ボス2への処刑弾。速くて太い。避けられた残骸がプレイヤーに飛ぶ
static void Shot_Boss1_to_Boss2(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 中心の極太弾1発 + 随伴の中弾2発
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            // 少しバラけさせる
            pShot->muki = pSet->muki + (GetRand(40) - 20) / 180.0 * DX_PI;
            pShot->speed = (i == 0 ? 500 : 380 + GetRand(60)) / 100.0;
            pShot->margin = 60.0; // 画面外まで飛ばす

            if (i == 0) {
                pShot->kind = img_enemyShotLargeBall[0]; // 赤・大玉
            }
            else {
                pShot->kind = img_enemyShotMediumBall[0]; // 赤・中玉
            }

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        // 大玉は徐々に減速して居座る地雷に変化 = 裏切りの執念
        if (p->count > 50 && p->speed > 0.8) p->speed *= 0.97;
        p = p->next;
    }
}

// ボス2 -> ボス1への言い訳弾。拡散して爆発、分裂してプレイヤーを狙う
static void Shot_Boss2_to_Boss1(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 菱形弾を扇状に7発
        for (int i = 0; i < 7; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = pSet->muki + (i - 3) * (8.0 / 180.0 * DX_PI) + (GetRand(20) - 10) / 180.0 * DX_PI;
            pShot->speed = (280 + GetRand(80)) / 100.0;
            pShot->kind = img_enemyShotDiamond[4]; // 青・菱形
            pShot->margin = 50.0;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        pSet->param_i[0] = 0; // 分裂済みフラグ
    }

    // 28F後に分裂。元標的(ボス1)に当たらなかった弾がプレイヤーへ向かう = 八つ当たり
    if (pSet->count == 28 && pSet->param_i[0] == 0) {
        pSet->param_i[0] = 1;
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            sEnemyShot* next = p->next; // 追加中にnextが変わるので保存
            if (p->param_i[0] == 1) {
                p = next;
                continue;
            }
            for (int j = 0; j < 2; j++) {
                sEnemyShot* pChild = new sEnemyShot;
                pChild->x = p->x;
                pChild->y = p->y;
                // プレイヤー方向へ誘導 + 少し散らす
                double toPlayer = atan2(player.y - p->y, player.x - p->x);
                pChild->muki = toPlayer + (GetRand(60) - 30) / 180.0 * DX_PI;
                pChild->speed = (150 + GetRand(100)) / 100.0;
                pChild->kind = img_enemyShotSmallBall[4]; // 青・小玉
                pChild->margin = 20.0;
                pChild->param_i[0] = 1;

                pChild->prev = pSet->pEnemyShotHead->prev;
                pChild->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pChild;
                pSet->pEnemyShotHead->prev = pChild;
            }
            p = next;
        }
        // 親は消さずにそのまま残して弾幕の壁にする
    }

    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

void EnemyPat_FallOut_MetaAI()
{
    if (count == 1) {
        enemy.x = 120.0;
        enemy.y = 80.0;
        enemy.x2 = 360.0;
        enemy.y2 = 120.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 2体が常に反対側にいるようにリサージュで移動 = 決別した2人のダンス
        enemy.x = 240.0 + 150.0 * cos(count * 0.008);
        enemy.y = 110.0 + 45.0 * sin(count * 0.019);

        enemy.x2 = 240.0 + 140.0 * cos(count * 0.011 + DX_PI);
        enemy.y2 = 130.0 + 70.0 * sin(count * 0.013 + DX_PI);

        // 画面外に出ないようにクランプ
        if (enemy.x < 40) enemy.x = 40; if (enemy.x > 440) enemy.x = 440;
        if (enemy.x2 < 40) enemy.x2 = 40; if (enemy.x2 > 440) enemy.x2 = 440;
        if (enemy.y < 40) enemy.y = 40; if (enemy.y > 200) enemy.y = 200;
        if (enemy.y2 < 40) enemy.y2 = 40; if (enemy.y2 > 200) enemy.y2 = 200;
    }

    // 互いに撃ち合う。プレイヤーは狙わない
    // ボス1のターン
    if (count % 16 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Shot_Boss1_to_Boss2;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        // ボス2の現在位置へ
        pSet->muki = atan2(enemy.y2 - pSet->y, enemy.x2 - pSet->x);

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // ボス2のターン
    if (count % 16 == 8) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Shot_Boss2_to_Boss1;
        pSet->x = enemy.x2;
        pSet->y = enemy.y2 + 10.0;
        // ボス1の現在位置へ
        pSet->muki = atan2(enemy.y - pSet->y, enemy.x - pSet->x);

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}