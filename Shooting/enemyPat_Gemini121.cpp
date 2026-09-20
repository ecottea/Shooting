// ======================================================================
// 弾幕：主軸（流体ガイド）
// 中央から絶え間なく真下に降り注ぐ高速のレーザー列
// ======================================================================
static void ShotMainStream(sEnemyShotSet* pEnemyShotSet)
{
    // 発生源は常にボスの位置に追従
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // 4フレームごとに高速レーザーを真下に発射
    if (pEnemyShotSet->count % 8 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y + 10.0;
        pEnemyShot->muki = DX_PI / 2.0; // 真下
        pEnemyShot->speed = 7.0;

        // シアンの短レーザー
        pEnemyShot->kind = img_enemyShotLaser[3];

        // リストに繋ぐ
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 弾の移動（等速直線運動）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// ======================================================================
// 弾幕：カルマン渦
// 1つの「渦」を管理し、回転・拡散しながら落下させる
// ======================================================================
static void ShotKarmanVortex(sEnemyShotSet* pEnemyShotSet)
{
    bool is_left = (pEnemyShotSet->param_i[0] == 0); // 0: 左渦, 1: 右渦

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int num = 8; // 1つの渦を構成する弾数
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 弾の極座標を保存 (param_d[0]: 角度, param_d[1]: 半径)
            pEnemyShot->param_d[0] = (DX_PI * 2.0 / num) * i;
            pEnemyShot->param_d[1] = 5.0; // 初期半径

            pEnemyShot->x = pEnemyShotSet->x + pEnemyShot->param_d[1] * cos(pEnemyShot->param_d[0]);
            pEnemyShot->y = pEnemyShotSet->y + pEnemyShot->param_d[1] * sin(pEnemyShot->param_d[0]);

            // 左渦は青(4)、右渦はマゼンタ(5)の鱗弾を使用
            pEnemyShot->kind = is_left ? img_enemyShotScale[4] : img_enemyShotScale[5];

            // リストに繋ぐ
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 渦全体の中心座標の移動 ---
    double center_x = pEnemyShotSet->param_d[0]; // 発生時の敵X座標（主軸）
    double target_offset = 70.0;                 // 主軸から横へ広がる最大距離
    double current_offset = std::abs(pEnemyShotSet->x - center_x);

    // 一定距離までは横に押し出される（流体の剥離を表現）
    if (current_offset < target_offset) {
        pEnemyShotSet->x += is_left ? -1.0 : 1.0;
    }

    // 下降（時間経過で落下速度が少し上がる）
    pEnemyShotSet->y += 1.5 + pEnemyShotSet->count * 0.01;


    // --- 渦を構成する弾の更新 ---
    double d_radius = 0.4;    // 毎フレームの半径増加量
    double max_radius = 45.0; // 渦の最大半径（広がりすぎ防止）
    double d_angle = is_left ? -0.025 : 0.025; // 回転速度（左は反時計、右は時計回り）

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 回転
        pShot->param_d[0] += d_angle;

        // 拡散（最大半径に達するまで広がる）
        if (pShot->param_d[1] < max_radius) {
            pShot->param_d[1] += d_radius;
        }

        // 渦の中心座標からの相対位置を計算して更新
        pShot->x = pEnemyShotSet->x + pShot->param_d[1] * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y + pShot->param_d[1] * sin(pShot->param_d[0]);

        // 鱗弾の向きを「円の接線方向」に向けて、流れている感を出す
        pShot->muki = pShot->param_d[0] + (is_left ? -DX_PI / 2.0 : DX_PI / 2.0);

        pShot = pShot->next;
    }
}


// ======================================================================
// 敵本体のパターン
// ======================================================================
void EnemyPat_KarmanVortex_Gemini()
{
    // 初期化とボス本体の動き
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // 流体の発生源らしく、ゆっくりと左右に揺らぐ
        enemy.x = 240.0 + 15.0 * sin(count * DX_PI / 120.0);
    }

    // [1] カウント開始時に主軸（流体ガイド）のセットを1つだけ永続で登録
    if (count == 1) {
        sEnemyShotSet* pMainSet = new sEnemyShotSet;
        pMainSet->count = 0;
        pMainSet->patternFunc = ShotMainStream;
        pMainSet->x = enemy.x;
        pMainSet->y = enemy.y;

        pMainSet->pEnemyShotHead = new sEnemyShot;
        pMainSet->pEnemyShotHead->prev = pMainSet->pEnemyShotHead;
        pMainSet->pEnemyShotHead->next = pMainSet->pEnemyShotHead;

        pMainSet->prev = enemyShotSetHead.prev;
        pMainSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMainSet;
        enemyShotSetHead.prev = pMainSet;
    }

    // [2] 0.4秒(24フレーム)周期で交互に渦を生成
    // 0, 24, 48... で左渦 ／ 12, 36, 60... で右渦
    if (count > 30 && count % 14 == 0) {
        bool is_left = (count % 28 == 0);

        sEnemyShotSet* pVortexSet = new sEnemyShotSet;
        pVortexSet->count = 0;
        pVortexSet->patternFunc = ShotKarmanVortex;

        pVortexSet->param_i[0] = is_left ? 0 : 1; // 左右判定フラグを渡す
        pVortexSet->param_d[0] = enemy.x;         // 主軸のX座標を記憶させておく

        // 発生位置は主軸の少し横
        pVortexSet->x = enemy.x + (is_left ? -15.0 : 15.0);
        pVortexSet->y = enemy.y + 10.0;

        pVortexSet->pEnemyShotHead = new sEnemyShot;
        pVortexSet->pEnemyShotHead->prev = pVortexSet->pEnemyShotHead;
        pVortexSet->pEnemyShotHead->next = pVortexSet->pEnemyShotHead;

        pVortexSet->prev = enemyShotSetHead.prev;
        pVortexSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pVortexSet;
        enemyShotSetHead.prev = pVortexSet;
    }
}