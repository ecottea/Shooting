// ========================================================================
// 弾幕：仲間割れ（互いを狙い撃ち、すれ違いで干渉）
// ========================================================================
static void ShotBetrayal(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 効果音（必要に応じて調整）
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // param_i[0] で発射元を判定 (1:ボス1, 2:ボス2)
        int isBoss1 = (pEnemyShotSet->param_i[0] == 1);

        // ターゲットは「相手」の現在位置
        double targetX = isBoss1 ? enemy.x2 : enemy.x;
        double targetY = isBoss1 ? enemy.y2 : enemy.y;

        // 相手に向かう基本角度
        double baseMuki = atan2(targetY - pEnemyShotSet->y, targetX - pEnemyShotSet->x);

        // ボスごとの弾幕パラメータ
        int shotCount = isBoss1 ? 7 : 9;                  // 弾数
        int colorIndex = isBoss1 ? 0 : 4;                  // 0:赤, 4:青
        int shotKind = isBoss1 ? img_enemyShotBullet[colorIndex] : img_enemyShotMediumBall[colorIndex];
        double speedBase = isBoss1 ? 4.5 : 2.5;              // ボス1は速く鋭く、ボス2は遅く広く
        double spreadAngle = isBoss1 ? (20.0 / 180.0 * DX_PI) : (80.0 / 180.0 * DX_PI); // 拡散角

        for (int i = 0; i < shotCount; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 角度をばらつかせる（再現性のある GetRand を使用）
            double angleOffset = (GetRand(100) - 50) / 100.0 * spreadAngle;
            pEnemyShot->muki = baseMuki + angleOffset;
            pEnemyShot->speed = speedBase + (GetRand(100) - 50) / 100.0 * 1.5;
            pEnemyShot->kind = shotKind;

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ========================================================================
// 敵本体のパターン：仲間割れ（断絶の双星）
// ========================================================================
void EnemyPat_FallOut_Qwen() // 新しく作成する場合、名前は void EnemyPat_FallOut_Qwen() にすること。
{
    if (count == 1) {
        enemy.x = 120.0;
        enemy.y = 80.0;
        enemy.x2 = 360.0;
        enemy.y2 = 80.0;
        enemy.maxHp = enemy.hp = 200; // HPはやや多めに
    }
    else {
        // 【移動パターン】互いに画面中央ですれ違い、端で折り返すサイン波移動
        double t = count * 0.015;
        enemy.x = 120.0 + 120.0 * sin(t);       // 120.0 〜 240.0
        enemy.x2 = 360.0 - 120.0 * sin(t);       // 360.0 〜 240.0 (対称)

        // Y座標は位相をずらして、すれ違い時に高さが変わるようにする
        enemy.y = 200.0 + 60.0 * cos(t * 1.3);
        enemy.y2 = 200.0 + 60.0 * cos(t * 1.3 + DX_PI);
    }

    // ---------------------------------------------------------
    // 1. ボス1（赤）の発射：相手(ボス2)を狙う
    // ---------------------------------------------------------
    if (count % 14 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBetrayal;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->param_i[0] = 1; // ボス1フラグ

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ---------------------------------------------------------
    // 2. ボス2（青）の発射：相手(ボス1)を狙う
    // ---------------------------------------------------------
    if (count % 14 == 8) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBetrayal;
        pEnemyShotSet->x = enemy.x2;
        pEnemyShotSet->y = enemy.y2 + 10.0;
        pEnemyShotSet->param_i[0] = 2; // ボス2フラグ

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ---------------------------------------------------------
    // 3. 【ギミック】干渉（火花）の発生
    // 2体の距離が近づいたとき、中間地点から「紫の棘弾」を爆発させる
    // ---------------------------------------------------------
    double dist = sqrt((enemy.x - enemy.x2) * (enemy.x - enemy.x2) + (enemy.y - enemy.y2) * (enemy.y - enemy.y2));
    if (dist < 80.0 && count % 6 == 0) {
        sEnemyShotSet* pInterfereSet = new sEnemyShotSet;
        pInterfereSet->count = 0;
        // 干渉専用の簡易パターン関数（ここでは無名ラムダの代わりに既存構造を使い回すため、count==0で1回だけ弾を出すロジックを組む）
        // 実際には ShotBetrayal を流用し、param_i[0]=3 として「紫の全方向弾」を出させるのが綺麗

        pInterfereSet->patternFunc = [](sEnemyShotSet* pSet) {
            if (pSet->count == 0) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                double midX = (enemy.x + enemy.x2) / 2.0;
                double midY = (enemy.y + enemy.y2) / 2.0;

                for (int i = 0; i < 12; i++) {
                    sEnemyShot* pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = midX + GetRand(20) - 10;
                    pEnemyShot->y = midY + GetRand(20) - 10;
                    pEnemyShot->muki = (i / 12.0) * 2.0 * DX_PI + (GetRand(20) - 10) / 100.0;
                    pEnemyShot->speed = 1.5 + GetRand(100) / 100.0;
                    pEnemyShot->kind = img_enemyShotDiamond[5]; // 5:マゼンタ(紫)

                    pEnemyShot->prev = pSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
            sEnemyShot* pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                pShot = pShot->next;
            }
        };

        pInterfereSet->x = (enemy.x + enemy.x2) / 2.0;
        pInterfereSet->y = (enemy.y + enemy.y2) / 2.0;
        pInterfereSet->pEnemyShotHead = new sEnemyShot;
        pInterfereSet->pEnemyShotHead->prev = pInterfereSet->pEnemyShotHead;
        pInterfereSet->pEnemyShotHead->next = pInterfereSet->pEnemyShotHead;

        pInterfereSet->prev = enemyShotSetHead.prev;
        pInterfereSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pInterfereSet;
        enemyShotSetHead.prev = pInterfereSet;
    }
}