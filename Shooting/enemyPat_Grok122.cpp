// 弾幕：ゴルトンボード（正規分布カスケード）
// 障害物（釘）も敵弾として生成し、落下弾がそれらに衝突して左右へ分岐する。
// 最終的に画面下部で中央が濃い正規分布状の弾幕になる。

// 釘の段数
static const int GALTON_ROWS = 6;

// 各段のY座標
static const double galtonPegY[GALTON_ROWS] = {
    90.0, 140.0, 190.0, 240.0, 290.0, 340.0
};

// 各段の釘数
static const int galtonPegCount[GALTON_ROWS] = {
    1, 2, 3, 4, 5, 6
};

// 釘用ショットセットのポインタ（敵パターン開始時に設定）
static sEnemyShotSet* pGaltonPegSet = nullptr;

// 各段の釘X座標を計算（中央基準、間隔50）
static double GetGaltonPegX(int row, int pegIndex)
{
    int n = galtonPegCount[row];
    double spacing = 50.0;
    double center = 240.0;
    double firstX = center - (n - 1) * spacing * 0.5;
    return firstX + pegIndex * spacing;
}

// 釘（障害物）用のショットセット処理
// 初回に全釘を静止弾として生成し、以降は動かさない
static void ShotGaltonPegs(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 釘生成時は音を鳴らさない
        for (int row = 0; row < GALTON_ROWS; row++) {
            int n = galtonPegCount[row];
            for (int i = 0; i < n; i++) {
                sEnemyShot* pPeg = new sEnemyShot;
                pPeg->x = GetGaltonPegX(row, i);
                pPeg->y = galtonPegY[row];
                pPeg->muki = 0.0;
                pPeg->speed = 0.0; // 完全静止

                // 釘らしい見た目：菱形弾を使用、色は黒
                pPeg->kind = img_enemyShotDiamond[7]; // 黒

                // この弾が釘であることと、所属段を記録
                pPeg->param_i[0] = row;   // 段番号
                pPeg->param_i[1] = 1;     // 釘フラグ（1=釘）
                pPeg->param_d[0] = 0.0;

                // リストに追加
                pPeg->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pPeg->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pPeg;
                pEnemyShotSet->pEnemyShotHead->prev = pPeg;
            }
        }
    }
    // 静止なので移動処理は不要
}

// 落下弾が釘弾に衝突したかを判定し、衝突していれば左右に分岐
// param_i[0] : 最後に衝突した段（-1で未衝突）
// param_i[1] : 分岐用の固有値
static void ProcessGaltonCollision(sEnemyShot* pShot)
{
    // すでに最下段まで通過済みなら終了
    if (pShot->param_i[0] >= GALTON_ROWS - 1) return;
    if (pGaltonPegSet == nullptr) return;

    int nextRow = pShot->param_i[0] + 1;
    double targetY = galtonPegY[nextRow];

    // 次の段のY付近に来るまで待つ
    if (pShot->y < targetY - 10.0) return;

    // 釘ショットセット内を走査して最も近い釘を探す
    double closestDist = 9999.0;
    sEnemyShot* pClosestPeg = nullptr;

    sEnemyShot* pPeg = pGaltonPegSet->pEnemyShotHead->next;
    while (pPeg != pGaltonPegSet->pEnemyShotHead) {
        // 釘フラグと段が一致するものだけ対象
        if (pPeg->param_i[1] == 1 && pPeg->param_i[0] >= nextRow) {
            double dx = pShot->x - pPeg->x;
            double dy = pShot->y - pPeg->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist < closestDist) {
                closestDist = dist;
                pClosestPeg = pPeg;
            }
        }
        pPeg = pPeg->next;
    }

    // 衝突判定（釘弾の見た目サイズに合わせた半径）
    const double collisionRadius = 10.0;
    if (pClosestPeg != nullptr && closestDist < collisionRadius) {
        // 衝突した段を記録
        pShot->param_i[0] = nextRow;

        // 左右分岐（GetRandは0以上x以下を返す）
        int dir = (GetRand(1) + pShot->param_i[1] + nextRow) % 2;

        double base = DX_PI * 0.5; // 真下
        double offset = 0.55 + GetRand(20) / 100.0; // 約0.55〜0.75
        if (dir == 0) {
            pShot->muki = base - offset; // 左
        }
        else {
            pShot->muki = base + offset; // 右
        }

        // 衝突感を出すため少し減速
        pShot->speed *= 0.90;
        if (pShot->speed < 1.5) pShot->speed = 1.5;
    }
}

// 落下弾のショットセット処理
static void ShotGalton(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ弾を生成
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int spawnNum = 7*2;
        for (int i = 0; i < spawnNum; i++) {
            pEnemyShot = new sEnemyShot;

            // 上部中央付近から少し横にばらけさせて生成
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(60) - 30);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(16) - 8);

            // ほぼ真下へ
            pEnemyShot->muki = DX_PI * 0.5 + (GetRand(30) - 15) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.2 + GetRand(40) / 100.0;

            // 小玉を使用。色はセットごとに変化
            int colorIndex;
            switch (pEnemyShotSet->kind % 5) {
            case 0: colorIndex = 3; break; // シアン
            case 1: colorIndex = 4; break; // 青
            case 2: colorIndex = 6; break; // 白
            case 3: colorIndex = 2; break; // 緑
            default: colorIndex = 5; break; // マゼンタ
            }
            pEnemyShot->kind = img_enemyShotSmallBall[colorIndex];

            // パラメータ初期化
            pEnemyShot->param_i[0] = -1;            // 最後に衝突した段
            pEnemyShot->param_i[1] = GetRand(1000); // 分岐用固有値
            pEnemyShot->param_i[2] = 0;             // 落下弾フラグ（0）
            pEnemyShot->param_d[0] = 0.0;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム：移動と釘衝突判定
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        ProcessGaltonCollision(pShot);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_GaltonBoard_Grok()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 36.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
        pGaltonPegSet = nullptr;

        // 釘用のショットセットを最初に1つだけ生成
        sEnemyShotSet* pPegSet = new sEnemyShotSet;
        pPegSet->count = 0;
        pPegSet->patternFunc = ShotGaltonPegs;
        pPegSet->x = 240.0;
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

        // ポインタを保持して落下弾から参照できるようにする
        pGaltonPegSet = pPegSet;
    }
    else {
        // 緩やかに左右移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
        if (count % 180 == 90) muki *= -1;
    }

    // 一定間隔で落下弾のショットセットを生成
    if (count % 18 == 1 && count > 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGalton;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = DX_PI * 0.5;
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