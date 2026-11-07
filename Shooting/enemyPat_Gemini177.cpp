// ---------------------------------------------------------
// 弾幕コールバック：幾何符「ピタゴラスの風車」
// ---------------------------------------------------------
static void ShotPythagoras(sEnemyShotSet* pSet)
{
    int c = pSet->count;

    // 【フェーズ1】図形の初期配置 (0フレーム目)
    if (c == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double baseX = pSet->x;
        double baseY = pSet->y;
        double angle = atan2(player.y - pSet->y, player.x - pSet->x) + (GetRand(60) + 0) * DX_PI / 180.0; // 毎回ランダムな傾きで展開
        double D = 18.0; // 弾の間隔（スケール）

        // --- 弾生成用のローカルラムダ ---
        auto createShot = [&](double lx, double ly, int kind, int type, int idx = -1, double tlx = 0, double tly = 0) {
            sEnemyShot* p = new sEnemyShot;
            // ローカル座標を回転させて配置
            p->x = baseX + lx * cos(angle) - ly * sin(angle);
            p->y = baseY + lx * sin(angle) + ly * cos(angle);
            p->muki = 0;
            p->speed = 0; // 最初は静止
            p->kind = kind;
            p->margin = 120;
            p->param_i[0] = type; // 0: 枠などの静止弾, 1: 移動して合体する弾
            p->param_i[1] = idx;  // ターゲット配列のインデックス

            // ターゲット座標も回転して保持
            p->param_d[0] = baseX + tlx * cos(angle) - tly * sin(angle);
            p->param_d[1] = baseY + tlx * sin(angle) + tly * cos(angle);
            // 現在地を移動開始地点として記憶
            p->param_d[3] = p->x;
            p->param_d[4] = p->y;

            // リストへ追加
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        };

        // 1. 斜辺(c^2)となる 5x5 正方形のターゲット座標を事前計算 (ローカル座標)
        // 頂点 C(0, -3D) から B(4D, 0) へ向かうベクトルと、その外側への法線ベクトルでグリッドを作る
        double target_lx[25], target_ly[25];
        int idx = 0;
        for (int i = 1; i <= 5; ++i) {      // 法線（外側）方向
            for (int j = 0; j <= 4; ++j) {  // 斜辺に沿った方向
                target_lx[idx] = 0 + (4.0 / 5.0) * (j * D) + (3.0 / 5.0) * (i * D);
                target_ly[idx] = -3.0 * D + (3.0 / 5.0) * (j * D) - (4.0 / 5.0) * (i * D);
                idx++;
            }
        }

        // 2. 正方形 a^2 (3x3: 赤の小丸弾) の配置
        idx = 0;
        for (int i = 1; i <= 3; ++i) {
            for (int j = 0; j <= 2; ++j) {
                createShot(-i * D, -j * D, img_enemyShotMediumBall[0], 1, idx, target_lx[idx], target_ly[idx]);
                idx++;
            }
        }

        // 3. 正方形 b^2 (4x4: 青の小丸弾) の配置
        for (int i = 1; i <= 4; ++i) {
            for (int j = 0; j <= 3; ++j) {
                createShot(j * D, i * D, img_enemyShotMediumBall[4], 1, idx, target_lx[idx], target_ly[idx]);
                idx++;
            }
        }

        // 4. 直角三角形の枠線 (白い銃弾) の配置
        for (int k = 0; k < 8; ++k) { // A(0,0) -> B(4D,0)
            createShot(k * (D / 2.0), 0, img_enemyShotSmallBall[6], 0);
        }
        for (int k = 0; k < 10; ++k) { // B(4D,0) -> C(0,-3D)
            createShot(4 * D - k * (4.0 / 5.0 * D / 2.0), 0 - k * (3.0 / 5.0 * D / 2.0), img_enemyShotSmallBall[6], 0);
        }
        for (int k = 0; k < 6; ++k) { // C(0,-3D) -> A(0,0)
            createShot(0, -3 * D + k * (D / 2.0), img_enemyShotSmallBall[6], 0);
        }

        // 5. 直角三角形の頂点を強調 (白い大玉)
        createShot(0, 0, img_enemyShotMediumBall[6], 0);
        createShot(4 * D, 0, img_enemyShotMediumBall[6], 0);
        createShot(0, -3 * D, img_enemyShotMediumBall[6], 0);
    }

    // 【フェーズ2】チャージ音＆合体移動開始
    if (c == 60) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    if (c > 60 && c <= 120) {
        double t = (c - 60) / 60.0;
        t = t * (2.0 - t); // Ease-out quad（最初は速く、徐々に減速して止まる）

        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 1) { // 合体する弾のみ移動
                p->x = p->param_d[3] + (p->param_d[0] - p->param_d[3]) * t;
                p->y = p->param_d[4] + (p->param_d[1] - p->param_d[4]) * t;
            }
            p = p->next;
        }
    }

    // 【フェーズ3】合体完了（紫の中丸弾に変化）
    if (c == 120) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 1) {
                p->kind = img_enemyShotMediumBall[5]; // 紫色の中丸弾（c^2）へ合体変化
            }
            p = p->next;
        }
    }

    // 【フェーズ4】完成した正方形のみ、自機に向かって射出（Q.E.D.）
    if (c == 180) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double aim = atan2(player.y - pSet->y, player.x - pSet->x);
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (p->param_i[0] == 1) {
                p->muki = aim;
                p->speed = 3.2; // 正方形の形を保ったまま直進
            }
            p = p->next;
        }
    }

    // 【フェーズ5】全方位への拡散崩壊（枠を含めた全弾）
    if (c == 240) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            // ランダムにバラけさせて全方位へ散らす
            p->muki = GetRand(360) / 180.0 * DX_PI;
            p->speed = 1.0 + GetRand(250) / 100.0;
            p = p->next;
        }
    }

    // --- メインルーチンの代わりとしての移動更新（speedがある場合のみ） ---
    // ※指定された通り、通常はメインルーチンで行われますが
    // 発射後の自律移動のために適用します。
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->speed > 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}


// ---------------------------------------------------------
// 敵本体のパターン関数
// ---------------------------------------------------------
void EnemyPat_PythagoreanTheorem_Gemini()
{
    // 初期化
    if (count == 1) {
        // 画面中央上部より少し下に配置して、図形が画面に収まりやすくする
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 320フレーム周期で弾幕を展開
    if (count % 320 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPythagoras;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        // リストのダミーヘッドを作成
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 全体リストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}