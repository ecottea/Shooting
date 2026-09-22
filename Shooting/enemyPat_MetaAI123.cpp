// enemyPat_Tmp.cpp - エアホッケーモチーフ弾幕
// 使用素材一覧（enemyPat_sampleForAI.cppで確認できるものから抜粋）
// 弾形状: 小玉(2.5x2.5), 中玉(7.0x7.0), 大玉(20.0x20.0), 銃弾(5.0x2.0), 鱗弾, 菱形弾, 中楕円弾, 短レーザー(64.0x4.0)
// 弾色: 0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒, 8:橙
// 効果音: sound_enemyShot_light, medium, heavy, extreme, sound_enemyCharge
// 選定理由:
//   大玉(白) -> パック本体。大きくて見やすい
//   大玉(赤/シアン) -> 敵マレット2つ。上部で動く
//   小玉(白/橙) -> ヒット時の火花、パックの軌跡
//   短レーザー(白) -> エアホッケー台のセンターラインとゴールライン表現

// ------------------------------------------------------------
// 弾幕：エアホッケー・ラリー
// ------------------------------------------------------------
static void ShotAirHockey(sEnemyShotSet* pEnemyShotSet)
{
    // param_d 割り当て
    // [0] puckX, [1] puckY, [2] puckVX, [3] puckVY
    // [4] malletL BaseX, [5] malletR BaseX
    // param_i
    // [0] phase 0:サーブ予告 1:ラリー中
    // [1] rallyCount
    // [2] serveWait

    if (pEnemyShotSet->count == 0) {
        // 初期化
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x; // puckX 中央
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y + 20.0; // puckY
        pEnemyShotSet->param_d[2] = 0.0;
        pEnemyShotSet->param_d[3] = 0.0;
        pEnemyShotSet->param_d[4] = 120.0; // malletL base
        pEnemyShotSet->param_d[5] = 360.0; // malletR base
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_i[1] = 0;
        pEnemyShotSet->param_i[2] = 0;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // --- テーブル線生成（短レーザー白）センターラインを点線風に ---
        for (int i = 0; i < 5; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = 48.0 + i * 96.0; // 64幅なので少し隙間を空ける
            p->y = 240.0;
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = img_enemyShotLaser[6]; // 白レーザー
            p->param_i[0] = 10; // role: table line
            p->margin = 100.0; // 消えないように余裕持たせる
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
        // ゴールライン（上） - 敵側
        for (int i = 0; i < 2; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = 160.0 + i * 160.0;
            p->y = 60.0;
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = img_enemyShotLaser[6];
            p->param_i[0] = 10;
            p->margin = 100.0;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }

        // --- マレット2つ（大玉 赤とシアン） ---
        {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->param_d[4];
            p->y = 80.0;
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = img_enemyShotLargeBall[0]; // 赤マレット
            p->param_i[0] = 1; // role mallet L
            p->margin = 100.0;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
        {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->param_d[5];
            p->y = 80.0;
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = img_enemyShotLargeBall[3]; // シアンマレット
            p->param_i[0] = 2; // role mallet R
            p->margin = 100.0;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }

        // --- パック本体（大玉 白） ---
        {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->param_d[0];
            p->y = pEnemyShotSet->param_d[1];
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = img_enemyShotLargeBall[6]; // 白パック
            p->param_i[0] = 3; // role puck
            p->margin = 100.0;
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
        return;
    }

    // 毎フレーム更新
    // マレットの動きを計算
    double malletL_x = pEnemyShotSet->param_d[4] + sin(pEnemyShotSet->count * 0.045) * 70.0;
    double malletR_x = pEnemyShotSet->param_d[5] + sin(pEnemyShotSet->count * 0.045 + DX_PI) * 70.0;
    double mallet_y = 80.0 + sin(pEnemyShotSet->count * 0.02) * 8.0;

    // パック参照を探す
    sEnemyShot* pPuck = nullptr;
    sEnemyShot* pMalletL = nullptr;
    sEnemyShot* pMalletR = nullptr;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) pMalletL = pShot;
        else if (pShot->param_i[0] == 2) pMalletR = pShot;
        else if (pShot->param_i[0] == 3) pPuck = pShot;
        pShot = pShot->next;
    }

    if (pMalletL) { pMalletL->x = malletL_x; pMalletL->y = mallet_y; }
    if (pMalletR) { pMalletR->x = malletR_x; pMalletR->y = mallet_y; }

    // フェーズ処理
    if (pEnemyShotSet->param_i[0] == 0) {
        // サーブ予告中 30フレーム待つ
        pEnemyShotSet->param_i[2]++;
        if (pEnemyShotSet->param_i[2] > 30) {
            pEnemyShotSet->param_i[0] = 1;
            // 自機狙いで初速を与える
            // GetRand(x)は0..x なので -30..30 を作るには GetRand(60)-30
            double baseMuki = atan2(player.y - pEnemyShotSet->param_d[1], player.x - pEnemyShotSet->param_d[0]);
            double rnd = (GetRand(150) - 75) / 180.0 * DX_PI;
            double speed = 3.2 + pEnemyShotSet->param_i[1] * 0.5; // ラリー回数で加速
            pEnemyShotSet->param_d[2] = speed * cos(baseMuki + rnd);
            pEnemyShotSet->param_d[3] = speed * sin(baseMuki + rnd);

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }
    else {
        // ラリー中 パック移動
        pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[2];
        pEnemyShotSet->param_d[1] += pEnemyShotSet->param_d[3];

        double px = pEnemyShotSet->param_d[0];
        double py = pEnemyShotSet->param_d[1];

        // 左右壁反射
        if (px < 24.0 || px > 456.0) {
            pEnemyShotSet->param_d[2] *= -1.0;
            px = (px < 24.0) ? 24.0 : 456.0;
            pEnemyShotSet->param_d[0] = px;
            // 火花
            for (int i = 0; i < 4*5; i++) {
                sEnemyShot* p = new sEnemyShot;
                p->x = px; p->y = py;
                p->muki = (DX_PI * 0.5) + (GetRand(120) - 60) / 180.0 * DX_PI;
                if (pEnemyShotSet->param_d[2] > 0) p->muki += DX_PI;
                p->speed = 1.5 + GetRand(100) / 100.0;
                p->kind = img_enemyShotSmallBall[8]; // 橙の火花
                p->param_i[0] = 20; // effect
                p->margin = 100;
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 敵マレットとの衝突（上部 y<130 の時のみ）
        if (py < 130.0) {
            bool hit = false;
            if (pMalletL) {
                double dx = px - pMalletL->x;
                double dy = py - pMalletL->y;
                if (dx * dx + dy * dy < 38.0 * 38.0) hit = true;
            }
            if (!hit && pMalletR) {
                double dx = px - pMalletR->x;
                double dy = py - pMalletR->y;
                if (dx * dx + dy * dy < 38.0 * 38.0) hit = true;
            }
            if (hit) {
                // 打ち返す：自機狙いに再設定、加速
                pEnemyShotSet->param_i[1]++; // rallyCount++
                double baseMuki = atan2(player.y - py, player.x - px);
                double rnd = (GetRand(40) - 20) / 180.0 * DX_PI;
                double speed = 3.5 + pEnemyShotSet->param_i[1] * 0.6;
                if (speed > 7.0) speed = 7.0;
                pEnemyShotSet->param_d[2] = speed * cos(baseMuki + rnd);
                pEnemyShotSet->param_d[3] = speed * sin(baseMuki + rnd);

                // ヒットエフェクト 小玉放射
                for (int i = 0; i < 8; i++) {
                    sEnemyShot* p = new sEnemyShot;
                    p->x = px; p->y = py;
                    p->muki = (2.0 * DX_PI / 8.0) * i + (GetRand(20) - 10) / 180.0 * DX_PI;
                    p->speed = 2.0 + GetRand(80) / 100.0;
                    p->kind = img_enemyShotSmallBall[(i % 2 == 0) ? 0 : 6]; // 赤と白交互
                    p->param_i[0] = 20;
                    p->margin = 100;
                    p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    p->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = p;
                    pEnemyShotSet->pEnemyShotHead->prev = p;
                }
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                // 3往復ごとに分裂して難易度上昇
                if (pEnemyShotSet->param_i[1] % 3 == 0 && pEnemyShotSet->param_i[1] > 0) {
                    for (int i = -1; i <= 1; i += 2) {
                        sEnemyShot* p = new sEnemyShot;
                        p->x = px; p->y = py;
                        p->muki = baseMuki + i * (20.0 / 180.0 * DX_PI);
                        p->speed = 2.2;
                        p->kind = img_enemyShotSmallBall[4]; // 青の子パック
                        p->param_i[0] = 21; // 子パック（直進のみ）
                        p->margin = 100;
                        p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        p->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = p;
                        pEnemyShotSet->pEnemyShotHead->prev = p;
                    }
                }
            }
        }

        // パックが下まで行ったらゴール扱いでサーブやり直し
        if (py < -40 || py > 520.0) {
            pEnemyShotSet->param_d[0] = pEnemyShotSet->x + GetRand(120) - 60;
            pEnemyShotSet->param_d[1] = pEnemyShotSet->y + 20.0;
            pEnemyShotSet->param_d[2] = 0.0;
            pEnemyShotSet->param_d[3] = 0.0;
            pEnemyShotSet->param_i[0] = 0;
            pEnemyShotSet->param_i[2] = 0;
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        // 軌跡をたまに出す
        if (pEnemyShotSet->count % 3 == 0) {
            sEnemyShot* p = new sEnemyShot;
            p->x = px; p->y = py;
            p->muki = 0.0; p->speed = 0.0;
            p->kind = img_enemyShotSmallBall[6]; // 白軌跡、すぐ消えるようにmargin小
            p->param_i[0] = 20;
            p->margin = 5.0;
            // countでフェード代わりに自動消去に任せるためparamは使わず、speed 0で置くだけ
            // 少しだけ残るので残像になる
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 全弾の座標反映
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int role = pShot->param_i[0];
        if (role == 3) {
            // パック本体は param_d に同期
            pShot->x = pEnemyShotSet->param_d[0];
            pShot->y = pEnemyShotSet->param_d[1];
        }
        else if (role == 10) {
            // テーブル線は静止
        }
        else if (role == 1 || role == 2) {
            // マレットは上で更新済み
        }
        else {
            // エフェクト・子パックは通常移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            // 少し減速して消えやすく
            if (role == 20) pShot->speed *= 0.996;
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_AirHockey_MetaAI()
{
    static int muki;
    static int shot_cycle;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_cycle = 0;
    }
    else {
        // エアホッケー台の審判のように左右にゆっくり移動
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 160.0 || enemy.x > 320.0) muki *= -1;
    }

    // 240フレームごとに新しいラリーを開始（前のラリーは残り続けるので重なる）
    // 最初は少し早めに出す
    if (count == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotAirHockey;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = shot_cycle++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}