// enemyPat_QRCode.cpp

// 弾幕：QRコード・バレット
// 既存の敵弾だけでQRコード風の正方形パターンを形成し、
// 読み取り中のようにマスが変化した後、コードが崩壊して一斉に飛び出す。

static bool IsQRFinder(int row, int col)
{
    // QRコードの3つの位置検出パターン
    const int finderSize = 7;

    const int finderPos[3][2] = {
        { 0,  0},
        { 0, 14},
        {14,  0}
    };

    for (int i = 0; i < 3; i++) {
        int fr = finderPos[i][0];
        int fc = finderPos[i][1];

        if (row >= fr && row < fr + finderSize &&
            col >= fc && col < fc + finderSize) {
            int r = row - fr;
            int c = col - fc;

            // 外枠・中央を点灯
            return r == 0 || r == 6 || c == 0 || c == 6 ||
                (r >= 2 && r <= 4 && c >= 2 && c <= 4);
        }
    }

    return false;
}

static bool IsQRModule(int row, int col)
{
    // 21x21のQRコード風データ領域。
    // 位置検出パターン以外は複数の規則を混ぜてランダムすぎない
    // 「コードらしい」密度になるようにする。
    if (IsQRFinder(row, col))
        return true;

    // 位置検出パターン付近の白い分離帯
    if ((row <= 7 && (col == 7 || col == 13)) ||
        (col <= 7 && (row == 7 || row == 13)) ||
        (row <= 7 && col >= 14 && col <= 20 && col == 13) ||
        (col <= 7 && row >= 14 && row == 13))
        return false;

    // 擬似データ。複数の周期を重ねてQRコードらしい非対称性を作る。
    int v = row * 13 + col * 7 + row * col * 3;
    bool data = ((v % 5) == 0 ||
        (v % 7) == 1 ||
        ((row + col * 2) % 9) < 3);

    // 端の密度を少し下げ、中央の読み取り領域を複雑にする。
    if ((row == 8 || col == 8) && ((row + col) % 2 == 0))
        data = true;

    return data;
}

static void ShotQRCode(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int row = pShot->param_i[0];
        const int col = pShot->param_i[1];
        const int state = pShot->param_i[2];

        // state 0: QRコードとして固定表示
        // state 1: 読み取り走査
        // state 2: 崩壊してプレイヤー方向へ射出
        if (state == 0) {
            pShot->x = pEnemyShotSet->x + (col - 10) * 10.0;
            pShot->y = pEnemyShotSet->y + (row - 10) * 10.0;
            pShot->speed = 0.0;
        }
        else if (state == 1) {
            // 走査線が進むにつれて、通過したマスを少し脈動させる。
            int scan = (pEnemyShotSet->count - 1) / 4;
            double pulse = 1.0 + 0.10 * sin((double)(pEnemyShotSet->count + row * 11 + col * 7) * 0.20);

            pShot->x = pEnemyShotSet->x + (col - 10) * 10.0;
            pShot->y = pEnemyShotSet->y + (row - 10) * 10.0;

            if (row <= scan) {
                pShot->x += sin((double)(pEnemyShotSet->count + col * 9) * 0.25) * 1.5;
                pShot->y += cos((double)(pEnemyShotSet->count + row * 7) * 0.25) * 1.5;
                pShot->speed = pulse * 0.12;
            }
            else {
                pShot->speed = 0.0;
            }
        }
        else {
            // QRコードを構成していた弾を、外側から順番にプレイヤーへ。
            // 向きは射出開始時に固定し、飛行中にワープしない。
            if (pShot->count == 255) {
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);

                // 外周ほど速く、内側ほど遅い波状の射出。
                double centerDist = sqrt(
                    (double)(row - 10) * (row - 10) +
                    (double)(col - 10) * (col - 10)
                );
                pShot->speed = 2.2 + centerDist * 0.07;
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 飛び出した弾の向きを少しずつ散らして扇状に広げる。
            if (pShot->count > 255 && pShot->count < 255 + 18) {
                double spread = 0.012 * sin((double)(row * 17 + col * 23));
                pShot->muki += spread;
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_QRCode_ChatGPT()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.85 * (double)muki;
        if (count % 120 == 60)
            muki *= -1;
    }

    // 1回だけQRコード弾幕を生成。
    if (count % 75 == 1) {
        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotQRCode;
        pEnemyShotSet->x = 240.0 + GetRand(300) - 150;
        pEnemyShotSet->y = 180.0 + GetRand(200) - 100;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // QRコードの21x21マスを作成。
        for (int row = 0; row < 21; row++) {
            for (int col = 0; col < 21; col++) {
                if (!IsQRModule(row, col))
                    continue;

                sEnemyShot* pEnemyShot = new sEnemyShot;

                pEnemyShot->x = pEnemyShotSet->x + (col - 10) * 10.0;
                pEnemyShot->y = pEnemyShotSet->y + (row - 10) * 10.0;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;

                // 小玉をモジュールとして使用。
                // 白・黒でQRコードらしいコントラストを出す。
                pEnemyShot->kind =
                    ((row * 11 + col * 7 + row * col) % 5 == 0)
                    ? img_enemyShotSmallBall[7]   // 黒
                    : img_enemyShotSmallBall[6];  // 白

                pEnemyShot->margin = 999.0;
                pEnemyShot->param_i[0] = row;
                pEnemyShot->param_i[1] = col;
                pEnemyShot->param_i[2] = 0;       // 状態
                pEnemyShot->param_i[3] = 0;       // 未使用
                pEnemyShot->param_d[0] = 0.0;     // 射出方向保存用
                pEnemyShot->param_d[1] = 0.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // QRコードの読み取り演出。
    // コードが形成された状態をしばらく見せてから走査を開始し、
    // 最後に全弾をプレイヤー方向へ解放する。
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        if (pSet->patternFunc == ShotQRCode) {
            if (pSet->count == 150) {
                if (CheckSoundMem(sound_enemyShot_heavy))
                    StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                sEnemyShot* pShot = pSet->pEnemyShotHead->next;
                while (pShot != pSet->pEnemyShotHead) {
                    pShot->param_i[2] = 1;
                    pShot = pShot->next;
                }
            }
            else if (pSet->count == 255) {
                if (CheckSoundMem(sound_enemyShot_extreme))
                    StopSoundMem(sound_enemyShot_extreme);
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                sEnemyShot* pShot = pSet->pEnemyShotHead->next;
                while (pShot != pSet->pEnemyShotHead) {
                    pShot->param_i[2] = 2;
                    pShot->margin = 80.0;
                    pShot = pShot->next;
                }
            }
        }

        pSet = pSet->next;
    }
}
