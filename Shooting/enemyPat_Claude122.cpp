// 弾幕：ゴルトンボード（Galton Board / プリンコ）
//
// 画面上部の投下口(ボス)から小玉を連続的に落とし、三角形状に配置した「釘」に
// 当たるたびに左右どちらかへランダムに屈曲しながら落下していく。
// 段を経るごとに左右への確率的分岐（二項分布のランダムウォーク）が積み重なり、
// 最下段を通過する頃には玉の集団が画面下部でベルカーブ状に横へ広がる。
// 盤面通過後はしばらく慣性のまま進んだのち、自機へ緩やかに収束する。

// ---- ボード定義 ----
static const int    GB_ROWS = 9;      // 釘の段数(0〜8)
static const double GB_ROW_SPACING = 26.0;    // 段の縦間隔
static const double GB_COL_SPACING = 40.0;    // 同一段内での釘の横間隔
static const double GB_ROW0_Y = 90.0;    // 1段目(row=0)のY座標
static const double GB_CENTER_X = 240.0;   // 投下口の真下(画面中央)
static const double GB_VY = 2.6;     // 玉の垂直速度成分
static const double GB_VX = 2.0;     // 段を跨ぐ屈曲時の水平速度成分

// row段目の釘のY座標
static double GB_RowY(int row) { return GB_ROW0_Y + row * GB_ROW_SPACING; }
// row段目の先頭(col=0)の釘のX座標。row段にはcol=0〜rowのrow+1個の釘が並ぶ三角配置
static double GB_RowStartX(int row) { return GB_CENTER_X - row * (GB_COL_SPACING * 0.5); }

// 釘のShotSetへのポインタ。玉が釘に当たった際の発光演出のために参照する
static sEnemyShotSet* pGaltonPegSet = nullptr;

// 弾幕：釘(ボード本体)。静止したまま常駐し、玉が当たると一瞬発光する
static void ShotGaltonPeg(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        for (int row = 0; row < GB_ROWS; row++) {
            double rowStartX = GB_RowStartX(row);
            double y = GB_RowY(row);
            for (int col = 0; col <= row; col++) {
                pEnemyShot = new sEnemyShot;

                pEnemyShot->x = rowStartX + col * GB_COL_SPACING;
                pEnemyShot->y = y;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0; // 釘は静止

                // 弾の種類と半径一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
                // 弾の色一覧:   0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
                pEnemyShot->kind = img_enemyShotMediumOval[6]; // 白(通常色)

                pEnemyShot->param_i[0] = row; // 段番号
                pEnemyShot->param_i[1] = col; // 段内インデックス
                pEnemyShot->param_i[2] = 0;   // 発光の残りフレーム数

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[2] > 0) {
            pShot->param_i[2]--;
            pShot->kind = img_enemyShotMediumOval[1]; // 玉が当たった瞬間は黄色に発光
        }
        else {
            pShot->kind = img_enemyShotMediumOval[6]; // 通常は白
        }
        // 釘自体は不動なのでx, yは更新しない

        pShot = pShot->next;
    }
}

// 指定した段・列の釘を発光させる
static void GaltonFlashPeg(int row, int col)
{
    if (pGaltonPegSet == nullptr) return;
    if (col < 0) col = 0;
    if (col > row) col = row;

    sEnemyShot* pShot = pGaltonPegSet->pEnemyShotHead->next;
    while (pShot != pGaltonPegSet->pEnemyShotHead) {
        if (pShot->param_i[0] == row && pShot->param_i[1] == col) {
            pShot->param_i[2] = 10; // 10フレーム発光
            return;
        }
        pShot = pShot->next;
    }
}

// 弾幕：ゴルトンボードを落下していく玉
static void ShotGaltonBall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // バーストごとに色を切り替え：赤→黄→シアン→マゼンタの順で循環
        static const int colorList[4] = { 0, 1, 3, 5 };
        int color = colorList[pEnemyShotSet->kind % 4];

        const int BALL_COUNT = 48; // 1バーストあたりの玉数
        for (int i = 0; i < BALL_COUNT; i++) {
            pEnemyShot = new sEnemyShot;

            // GetRand(x) は 0 以上 x 以下の x+1 種類の整数をランダムに返す
            pEnemyShot->x = pEnemyShotSet->x + GetRand(6) - 3.0;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI / 2.0; // 真下へ投下
            pEnemyShot->speed = GB_VY;

            pEnemyShot->kind = img_enemyShotSmallBall[color];

            pEnemyShot->param_i[0] = 0; // 次に通過する段番号(未到達なら0)
            pEnemyShot->param_i[1] = 0; // 盤面通過完了後の経過フレーム数
            pEnemyShot->param_i[2] = 0; // 未使用

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int row = pShot->param_i[0];

        if (row < GB_ROWS && pShot->y >= GB_RowY(row)) {
            // この段の釘に到達：左右どちらかへランダムに屈曲する
            // (リプレイ再現性のため、必ずGetRandを介した決定論的乱数を使用)
            int dir = (GetRand(1) == 0) ? -1 : 1;
            double vx = dir * GB_VX;
            double vy = GB_VY;
            pShot->speed = sqrt(vx * vx + vy * vy);
            pShot->muki = atan2(vy, vx);

            // 当たった釘を発光させる(屈曲前のx位置から列インデックスを逆算)
            double rowStartX = GB_RowStartX(row);
            int col = (int)((pShot->x - rowStartX) / GB_COL_SPACING + 0.5);
            GaltonFlashPeg(row, col);

            pShot->param_i[0] = row + 1;
        }

        if (pShot->param_i[0] >= GB_ROWS) {
            // 盤面通過後：しばらく慣性のまま進んでから、自機へ緩やかに収束していく
            pShot->param_i[1]++;
            if (pShot->param_i[1] > 20) {
                double targetMuki = atan2(player.y - pShot->y, player.x - pShot->x);
                double diff = targetMuki - pShot->muki;
                while (diff > DX_PI)  diff -= 2.0 * DX_PI;
                while (diff < -DX_PI) diff += 2.0 * DX_PI;
                pShot->muki += diff * 0.02;

                pShot->speed += 0.02;
                if (pShot->speed > 5.0) pShot->speed = 5.0;
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン：ゴルトンボード
void EnemyPat_GaltonBoard_Claude()
{
    static int burst_kind;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = GB_CENTER_X;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        burst_kind = 0;

        // 釘(ボード本体)を1度だけ生成する
        sEnemyShotSet* pPegSet = new sEnemyShotSet;
        pPegSet->count = 0;
        pPegSet->patternFunc = ShotGaltonPeg;
        pPegSet->x = 0.0;
        pPegSet->y = 0.0;
        pPegSet->muki = 0.0;
        pPegSet->kind = 0;

        pPegSet->pEnemyShotHead = new sEnemyShot;
        pPegSet->pEnemyShotHead->prev = pPegSet->pEnemyShotHead;
        pPegSet->pEnemyShotHead->next = pPegSet->pEnemyShotHead;

        pPegSet->prev = enemyShotSetHead.prev;
        pPegSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pPegSet;
        enemyShotSetHead.prev = pPegSet;

        pGaltonPegSet = pPegSet;
    }
    else {
        // 投下口(ボス)がゆっくり左右に揺れる演出
        enemy.x = GB_CENTER_X + 15.0 * sin(count * 0.015);
    }

    // 60フレームごとに玉のバーストを投下(count==1の時点で初回バーストも発生)
    if (count % 60 == 1) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGaltonBall;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = burst_kind++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}