// 弾幕：渦列「カルマンストリート」
//
// ボスの両側から交互に「渦そのもの」を切り離し、
// 回転する渦の塊が下流へ流れていくカルマン渦列を表現する。
// 各渦は複数の同心円状の弾で構成し、半径ごとに回転速度を変えることで
// ただの輪ではなく、内部まで回転している立体的な渦に見せる。
//
// count / pEnemyShotSet->count / pEnemyShot->count のインクリメントや、
// 画面外の弾の消去はメインルーチン側で行う仕様。

static void ShotKarmanVortex(sEnemyShotSet* pEnemyShotSet)
{
    // 渦が発生した瞬間に、内側・中間・外側の3層をまとめて生成する。
    // 左右交互に発生させることで、カルマン渦列の「互い違いの渦」を作る。
    if (pEnemyShotSet->count == 0) {
        const int side = pEnemyShotSet->param_i[0];

        // 内側の渦
        for (int i = 0; i < 6; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            const double angle = DX_PI * 2.0 * i / 6.0 + 0.18 * side;

            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * 12.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * 12.0;
            pEnemyShot->kind = (side < 0) ? img_enemyShotMediumBall[0] : img_enemyShotMediumBall[3];

            pEnemyShot->param_i[0] = 0; // 層
            pEnemyShot->param_i[1] = i; // 層内番号
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = 12.0*2;   // 半径
            pEnemyShot->param_d[2] = 0.095/5;  // 回転速度
            pEnemyShot->param_d[3] = 0.0;    // 半径変化用位相
            pEnemyShot->param_d[4] = (double)side;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 中間の渦
        for (int i = 0; i < 8; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            const double angle = DX_PI * 2.0 * i / 8.0 - 0.10 * side;

            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * 28.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * 28.0;
            pEnemyShot->kind = (side < 0) ? img_enemyShotMediumBall[0] : img_enemyShotMediumBall[3];

            pEnemyShot->param_i[0] = 1;
            pEnemyShot->param_i[1] = i;
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = 28.0*2;
            pEnemyShot->param_d[2] = 0.072/5;
            pEnemyShot->param_d[3] = 0.6;
            pEnemyShot->param_d[4] = (double)side;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 外側の渦
        for (int i = 0; i < 10; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            const double angle = DX_PI * 2.0 * i / 10.0 + 0.16 * side;

            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * 44.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * 44.0;
            pEnemyShot->kind = (side < 0) ? img_enemyShotMediumBall[0] : img_enemyShotMediumBall[3];

            pEnemyShot->param_i[0] = 2;
            pEnemyShot->param_i[1] = i;
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = 44.0*2;
            pEnemyShot->param_d[2] = 0.052/5;
            pEnemyShot->param_d[3] = 1.2;
            pEnemyShot->param_d[4] = (double)side;
            pEnemyShot->margin = 120;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        if (CheckSoundMem(sound_enemyShot_medium))
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double t = (double)pEnemyShotSet->count;
        const double side = pShot->param_d[4];

        // 渦の中心は発生位置から下流へ流れる。
        // 発生直後は外側へ少し張り出し、その後ゆっくり中心側へ戻ることで
        // 後続の渦と交互に並ぶ「渦列」を強調する。
        const double centerX = pEnemyShotSet->x
            + side * (72.0 + 10.0 * sin(t * 0.018));
        const double centerY = pEnemyShotSet->y + t * 2.05;

        // 同心円ごとに回転速度を変え、渦全体が本当に回転しているようにする。
        const double angle = pShot->param_d[0]
            + pShot->param_d[2] * t * side;

        // 円周を完全な固定輪にせず、内外へわずかに膨縮させる。
        // これにより、渦の流れが継続して見える。
        const double radius = pShot->param_d[1]
            * (1.0 + 0.10 * sin(pShot->param_d[3] + t * 0.030));

        pShot->x = centerX + cos(angle) * radius;
        pShot->y = centerY + sin(angle) * radius;

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_KarmanVortex_ChatGPT()
{
    static int vortexSide;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        vortexSide = -1;

        if (CheckSoundMem(sound_enemyCharge))
            StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // ボス自身は発生源として左右にゆっくり揺れる。
        enemy.x = 240.0 + 105.0 * sin(count * 0.006);
    }

    // 渦を一定間隔で発生させ、左右交互に配置する。
    if (count == 30 || (count > 30 && count % 44 == 0)) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKarmanVortex;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->param_i[0] = vortexSide;
        pEnemyShotSet->param_i[1] = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        vortexSide *= -1;
    }
}
