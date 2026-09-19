// 弾幕：禁忌「レーヴァテイン」
static void ShotLevatein(sEnemyShotSet* pEnemyShotSet)
{
    // 周期開始時の初期化
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧から重厚な発射音を選択
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // パターン選択 (0: 速めの平行移動, 1: 遅めの平行移動＋速めの回転移動)
        pEnemyShotSet->param_i[0] = pEnemyShotSet->kind % 2;

        if (pEnemyShotSet->param_i[0] == 0) {
            // パターンA: 速めの平行移動
            pEnemyShotSet->x = -60.0;
            pEnemyShotSet->y = 240.0 + GetRand(100) - 50;
            pEnemyShotSet->param_d[2] = 3.5; // Vx
            pEnemyShotSet->param_d[3] = 0.0; // Vy
            // レーザーの向きと移動方向が完全に平行だと垂直方向との内積が0になるため、少し角度をつける
            pEnemyShotSet->muki = DX_PI / 6.0;
        }
        else {
            // パターンB: 遅めの平行移動と速めの回転移動の合成
            pEnemyShotSet->param_d[4] = 240.0; // 中心X (cx)
            pEnemyShotSet->param_d[5] = 240.0; // 中心Y (cy)
            pEnemyShotSet->param_d[6] = 0.0;   // 初期角度 (theta)
            pEnemyShotSet->param_d[7] = 0.08;  // 角速度 (omega)
            pEnemyShotSet->param_d[8] = 0.5;   // 中心移動速度X (Vcx)
            pEnemyShotSet->param_d[9] = 0.5;   // 中心移動速度Y (Vcy)
            pEnemyShotSet->param_d[10] = 120.0; // 回転半径 (R)

            double theta = pEnemyShotSet->param_d[6];
            double cx = pEnemyShotSet->param_d[4];
            double cy = pEnemyShotSet->param_d[5];
            double R = pEnemyShotSet->param_d[10];

            pEnemyShotSet->x = cx + R * cos(theta);
            pEnemyShotSet->y = cy + R * sin(theta);
            pEnemyShotSet->muki = theta + DX_PI / 2.0;

            pEnemyShotSet->param_d[2] = pEnemyShotSet->param_d[8] - R * pEnemyShotSet->param_d[7] * sin(theta); // Vx
            pEnemyShotSet->param_d[3] = pEnemyShotSet->param_d[9] + R * pEnemyShotSet->param_d[7] * cos(theta); // Vy
        }

        // 短レーザーを8個生成して1本の長いレーザーとして扱う (64.0 x 8 = 512.0)
        for (int i = 0; i < 8; i++) {
            sEnemyShot* pLaser = new sEnemyShot;
            pLaser->kind = img_enemyShotLaser[0]; // 0:赤, 短レーザー(64.0x4.0)
            pLaser->param_i[0] = i; // レーザー上のインデックスを保存
            pLaser->margin = 999;

            pLaser->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pLaser->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pLaser;
            pEnemyShotSet->pEnemyShotHead->prev = pLaser;
        }
    }

    // --- 毎フレームの更新処理 ---
    int pattern = pEnemyShotSet->param_i[0];
    double x = pEnemyShotSet->x;
    double y = pEnemyShotSet->y;
    double muki = pEnemyShotSet->muki;
    double Vx = pEnemyShotSet->param_d[2];
    double Vy = pEnemyShotSet->param_d[3];

    if (pattern == 0) {
        // パターンA: 平行移動
        x += Vx;
        y += Vy;
        // muki は固定
    }
    else {
        // パターンB: 回転移動＋平行移動
        double theta = pEnemyShotSet->param_d[6];
        double omega = pEnemyShotSet->param_d[7];
        double cx = pEnemyShotSet->param_d[4];
        double cy = pEnemyShotSet->param_d[5];
        double Vcx = pEnemyShotSet->param_d[8];
        double Vcy = pEnemyShotSet->param_d[9];
        double R = pEnemyShotSet->param_d[10];

        theta += omega;
        cx += Vcx;
        cy += Vcy;

        pEnemyShotSet->param_d[4] = cx;
        pEnemyShotSet->param_d[5] = cy;
        pEnemyShotSet->param_d[6] = theta;

        x = cx + R * cos(theta);
        y = cy + R * sin(theta);
        muki = theta + DX_PI / 2.0;

        Vx = Vcx - R * omega * sin(theta);
        Vy = Vcy + R * omega * cos(theta);
        pEnemyShotSet->param_d[2] = Vx;
        pEnemyShotSet->param_d[3] = Vy;
    }

    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;

    // ボスの位置をレーザーの一方の端 (i=0) の位置に一致させる
    enemy.x = x;
    enemy.y = y;

    // 短レーザーの位置・向き更新、および菱形弾の加速・移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->kind == img_enemyShotLaser[0]) {
            // 短レーザーはボスに追従させる
            int i = pShot->param_i[0];
            pShot->x = x + i * 64.0 * cos(muki);
            pShot->y = y + i * 64.0 * sin(muki);
            pShot->muki = muki;
        }
        else if (pShot->kind == img_enemyShotDiamond[0]) {
            // 赤菱形弾の加速処理: 初速度0から徐々に加速して終端速度に達したら等速直線運動
            if (pShot->speed < pShot->param_d[0]) {
                pShot->speed += pShot->param_d[1];
                if (pShot->speed > pShot->param_d[0]) {
                    pShot->speed = pShot->param_d[0];
                }
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // 菱形弾の発射処理
    if (pEnemyShotSet->count % 12 == 0) {
        // レーザーの向きに対する垂直ベクトル (左90度)
        double Nx = -sin(muki);
        double Ny = cos(muki);

        // レーザーの移動方向との内積が負であれば、逆向き(右90度)にして内積を正にする
        if (Vx * Nx + Vy * Ny < 0.0) {
            Nx = -Nx;
            Ny = -Ny;
        }

        double shootMuki = atan2(Ny, Nx);

        // レーザーを等間隔に内分する点 (i=1 から 6) から射出
        for (int i = 1; i <= 6; i++) {
            sEnemyShot* pDiamond = new sEnemyShot;
            pDiamond->kind = img_enemyShotDiamond[0]; // 0:赤, 菱形弾(4.5x2.5)
            pDiamond->x = x + i * 64.0 * cos(muki);
            pDiamond->y = y + i * 64.0 * sin(muki);
            pDiamond->muki = shootMuki;
            pDiamond->speed = 0.0;
            pDiamond->param_d[0] = 3.5;  // 終端速度
            pDiamond->param_d[1] = 0.15; // 加速度

            pDiamond->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pDiamond->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pDiamond;
            pEnemyShotSet->pEnemyShotHead->prev = pDiamond;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Levatain_Qwen()
{
    static sEnemyShotSet* pCurrentSet = nullptr;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 100; // 200で固定
        pCurrentSet = nullptr;
    }

    // 300フレーム(約5秒)を1周期とし、周期ごとにパターンを切り替えて繰り返す
    if (count % 300 == 31) {
        if (pCurrentSet != nullptr) {
            // 既存のセットを使い回すため、属する弾をすべて削除
            sEnemyShot* pShot = pCurrentSet->pEnemyShotHead->next;
            while (pShot != pCurrentSet->pEnemyShotHead) {
                sEnemyShot* next = pShot->next;
                delete pShot;
                pShot = next;
            }
            pCurrentSet->pEnemyShotHead->prev = pCurrentSet->pEnemyShotHead;
            pCurrentSet->pEnemyShotHead->next = pCurrentSet->pEnemyShotHead;
            pCurrentSet->kind++;
            pCurrentSet->count = -1; // カウントをリセットして新規周期として扱う
        }
        else {
            // 初回のみ新規作成してリストに連結
            pCurrentSet = new sEnemyShotSet;
            pCurrentSet->patternFunc = ShotLevatein;
            pCurrentSet->kind = 0;

            pCurrentSet->pEnemyShotHead = new sEnemyShot;
            pCurrentSet->pEnemyShotHead->prev = pCurrentSet->pEnemyShotHead;
            pCurrentSet->pEnemyShotHead->next = pCurrentSet->pEnemyShotHead;

            pCurrentSet->prev = enemyShotSetHead.prev;
            pCurrentSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pCurrentSet;
            enemyShotSetHead.prev = pCurrentSet;
        }
    }
}