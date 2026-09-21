// 弾幕：ゴルトン弾盤
//
// 上から落ちてくる弾が、盤面に並んだ「釘」に見立てた弾の位置を
// 仮想的に参照しながら左右へ分岐し、下部へ多数の列として流れ込む。
// 弾同士の実際の当たり判定には依存せず、このパターン内でゴルトンボードの
// 反射を再現する。

static void ShotGalton(sEnemyShotSet* pEnemyShotSet)
{
    // 初回だけ盤面の「釘」を生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // ゴルトンボードの釘。
        // 奇数段・偶数段で半ピッチずらし、三角形状の盤面を作る。
        const int rowCount = 8;
        const int pegCount = 9;
        const double firstRowY = 92.0;
        const double rowGap = 31.0;
        const double pegGap = 36.0;

        for (int row = 0; row < rowCount; row++) {
            const double offset = (row & 1) ? pegGap * 0.5 : 0.0;
            const double startX = 240.0 - (pegCount - 1) * pegGap * 0.5 + offset;

            for (int i = 0; i < pegCount; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                pEnemyShot->x = startX + pegGap * i;
                pEnemyShot->y = firstRowY + rowGap * row;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白い小玉

                // 釘であることを識別
                pEnemyShot->param_i[0] = 1;
                pEnemyShot->param_i[1] = row;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 定期的に新しい球を投入
    if (pEnemyShotSet->count % 18 == 1) {
        const int ballCount = 5;

        if (pEnemyShotSet->count == 1) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        else if (pEnemyShotSet->count % 72 == 1) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        for (int i = 0; i < ballCount; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // ボス位置付近から複数の球を投入
            double spawnX = pEnemyShotSet->x + GetRand(72) - 36;
            if (spawnX < 125.0) spawnX = 125.0;
            if (spawnX > 355.0) spawnX = 355.0;

            pEnemyShot->x = spawnX;
            pEnemyShot->y = pEnemyShotSet->y + 10.0;

            // 釘に当たるまではほぼ真下へ落下
            const int dir = (GetRand(1) == 0) ? -1 : 1;
            const double vx = 0.65 * dir;
            const double vy = 3.35;

            pEnemyShot->muki = atan2(vy, vx);
            pEnemyShot->speed = sqrt(vx * vx + vy * vy);
            pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙の中玉

            // 落下球
            pEnemyShot->param_i[0] = 0;
            pEnemyShot->param_i[1] = 0; // 次に当たる釘の段
            pEnemyShot->param_i[2] = dir;
            pEnemyShot->param_d[0] = vx;
            pEnemyShot->param_d[1] = vy;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 盤面の釘位置を直接計算して、落下球を左右へ分岐させる
    const int rowCount = 8;
    const int pegCount = 9;
    const double firstRowY = 92.0;
    const double rowGap = 31.0;
    const double pegGap = 36.0;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        // param_i[0] == 1 は盤面の釘なので静止
        if (pShot->param_i[0] == 0) {
            const double vx = pShot->param_d[0];
            const double vy = pShot->param_d[1];

            double oldY = pShot->y;
            pShot->x += vx;
            pShot->y += vy;

            int row = pShot->param_i[1];

            // 次の釘の段を通過した瞬間に左右へ跳ねる
            if (row < rowCount && oldY < firstRowY + rowGap * row &&
                pShot->y >= firstRowY + rowGap * row) {

                const double offset = (row & 1) ? pegGap * 0.5 : 0.0;
                const double startX = 240.0 - (pegCount - 1) * pegGap * 0.5 + offset;

                // 現在位置に最も近い釘を求める
                double t = (pShot->x - startX) / pegGap;
                int pegIndex = (int)floor(t + 0.5);

                if (pegIndex < 0) pegIndex = 0;
                if (pegIndex >= pegCount) pegIndex = pegCount - 1;

                const double pegX = startX + pegGap * pegIndex;
                pShot->x = pegX;

                int dir = (GetRand(1) == 0) ? -1 : 1;

                // 盤面の端では外側へ飛び出しすぎないよう内側へ補正
                if (pegIndex == 0 && dir < 0) dir = 1;
                if (pegIndex == pegCount - 1 && dir > 0) dir = -1;

                const double nextVx = 1.85 * dir;
                const double nextVy = 2.95;

                pShot->param_i[2] = dir;
                pShot->param_i[1] = row + 1;
                pShot->param_d[0] = nextVx;
                pShot->param_d[1] = nextVy;

                pShot->muki = atan2(nextVy, nextVx);
                pShot->speed = sqrt(nextVx * nextVx + nextVy * nextVy);
            }

            // 最終段を抜けた球は、その時点の左右位置に応じて
            // 下部へ流れ込むように横移動を維持する
            if (pShot->param_i[1] >= rowCount) {
                double dx = (pShot->x - 240.0) * 0.018;
                if (dx > 2.6) dx = 2.6;
                if (dx < -2.6) dx = -2.6;

                pShot->param_d[0] = dx;
                pShot->param_d[1] = 4.2;

                pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
                pShot->speed = sqrt(
                    pShot->param_d[0] * pShot->param_d[0] +
                    pShot->param_d[1] * pShot->param_d[1]);
            }
        }

        pShot = pNext;
    }
}


// 敵本体のパターン
void EnemyPat_GaltonBoard_ChatGPT()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 42.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    else {
        // 盤面の入口を横断するようにゆっくり移動
        enemy.x = 240.0 + 145.0 * sin(count * 2.0 * DX_PI / 300.0);
        enemy.y = 42.0;
    }

    // ゴルトンボード本体を1つのShotSetとして管理
    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGalton;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 既存のShotSetの入口位置だけボスに追従させる
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == ShotGalton) {
                pSet->x = enemy.x;
                pSet->y = enemy.y + 8.0;
                break;
            }
            pSet = pSet->next;
        }
    }
}
