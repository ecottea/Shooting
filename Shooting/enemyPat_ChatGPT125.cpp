// 南京玉すだれ：複数の弾列を「すだれ」に見立て、伸長・屈曲・交差・収束させる。
static void ShotNanjinTamasudare(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    const int ROW_COUNT = 7;
    const int COL_COUNT = 12;
    const int SHOT_COUNT = ROW_COUNT * COL_COUNT;
    const double ROW_GAP = 30.0*1.5;
    const double COL_GAP = 28.0*1.5;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < SHOT_COUNT; i++) {
            const int row = i / COL_COUNT;
            const int col = i % COL_COUNT;

            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;

            // 0:列番号、1:段番号、2:ランダムな色変化用の位相
            pEnemyShot->param_i[0] = row;
            pEnemyShot->param_i[1] = col;
            pEnemyShot->param_i[2] = GetRand(2);

            // すだれの基本色。列ごとに少しだけ色を変えて節を強調する。
            if (pEnemyShot->param_i[2] == 0) {
                pEnemyShot->kind = img_enemyShotMediumOval[1];
            }
            else if (pEnemyShot->param_i[2] == 1) {
                pEnemyShot->kind = img_enemyShotMediumOval[2];
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumOval[8];
            }
            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    const double t = (double)pEnemyShotSet->count;
    const double cycle = 240.0;
    const double phase = fmod(t, cycle);
    const int cycleIndex = (int)(t / cycle);

    // 周期ごとに「すだれ」の左右を入れ替え、同じ動きでも毎回違って見えるようにする。
    const double mirror = (cycleIndex % 2 == 0) ? 1.0 : -1.0;

    // すだれ全体をゆっくり下へ流して、古い弾が画面外へ抜けるようにする。
    double driftY = 0.0;
    if (t >= cycle) {
        driftY = (t - cycle) * 2.4;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int row = pShot->param_i[0];
        const int col = pShot->param_i[1];

        const double rowCenter = (ROW_COUNT - 1) * 0.5;
        const double rowOffset = row - rowCenter;
        const double colCenter = (COL_COUNT - 1) * 0.5;
        const double colOffset = col - colCenter;

        double x = colOffset * COL_GAP;
        double y = rowOffset * ROW_GAP;
        double angle = 0.0;

        // 伸び始め：束になった状態から一気に横へ広がる。
        if (phase < 45.0) {
            const double q = phase / 45.0;
            const double stretch = q * q * (3.0 - 2.0 * q);
            x *= 0.12 + 0.88 * stretch;
            y *= 0.45 + 0.55 * stretch;
        }
        // 変形第一段階：上下の列が逆方向へ傾き、「橋」のような形を作る。
        else if (phase < 105.0) {
            const double q = (phase - 45.0) / 60.0;
            const double s = q * q * (3.0 - 2.0 * q);
            angle = mirror * rowOffset * 0.085 * s;
            x += mirror * rowOffset * 7.0 * s;
            y += mirror * colOffset * 0.18 * rowOffset * s;
        }
        // 変形第二段階：左右を折り返し、縦長の「すだれの梯子」へ変わる。
        else if (phase < 165.0) {
            const double q = (phase - 105.0) / 60.0;
            const double s = q * q * (3.0 - 2.0 * q);
            angle = mirror * ((row % 2 == 0) ? 1.05 : -1.05) * s;
            x += mirror * rowOffset * 7.0;
            y += mirror * colOffset * 0.55 * s;
        }
        // 収束：折れ曲がったすだれが再び一本の束へ戻る。
        else {
            const double q = (phase - 165.0) / 75.0;
            const double s = q * q * (3.0 - 2.0 * q);
            const double inv = 1.0 - s;
            const double stretch = 1.0 - 0.88 * s;
            angle = mirror * ((row % 2 == 0) ? 1.05 : -1.05) * inv;
            x *= stretch;
            y *= 0.45 + 0.55 * inv;
            x += mirror * rowOffset * 7.0 * inv;
            y += mirror * colOffset * 0.55 * inv;
        }
        angle /= 2;

        // 列の回転を適用。
        const double rx = x * cos(angle) - y * sin(angle);
        const double ry = x * sin(angle) + y * cos(angle);

        pShot->x = pEnemyShotSet->x + rx;
        pShot->y = pEnemyShotSet->y + ry + driftY;
        pShot->muki = angle;

        // 列の両端だけ色を変え、竹の節を強調する。
        if (col == 0 || col == COL_COUNT - 1) {
            if (cycleIndex % 2 == 0) {
                pShot->kind = img_enemyShotMediumOval[8];
            }
            else {
                pShot->kind = img_enemyShotMediumOval[1];
            }
        }

        pShot = pShot->next;
    }
}


// 敵本体のパターン
void EnemyPat_NankinTamasudare_ChatGPT()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 75.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // すだれの動きを邪魔しないよう、ボスは上部をゆっくり左右移動する。
        enemy.x += 0.72 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // すだれが伸び始める前に予告音を鳴らす。
    if (count % 180 == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 少し間を置いてすだれを展開する。
    if (count % 190 == 19) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNanjinTamasudare;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 35.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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
