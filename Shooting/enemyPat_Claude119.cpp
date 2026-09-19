// 弾幕：禁忌「レーヴァテイン」
//
// 赤色の短レーザーを連ねて1本の長いレーザーとして扱い、
//   ・速めの平行移動
//   ・遅めの平行移動と速めの回転移動の合成
// のいずれかの動きを、周期(フェーズ)ごとに切り替えながら振り回す。
//
// レーザーが動いている間、レーザーを等間隔に内分する点から、
// レーザーの向きと垂直かつ「その点の実際の移動方向」との内積が正になる側へ
// 赤菱形弾を発射する。菱形弾は初速0から徐々に加速し、終端速度に達した後は
// 等速直線運動する。
//
// ボスの位置は常にレーザーの根本(一方の端)に一致させる。

// このショットセットが管理する弾の種別タグ(param_i[0])
//   0 = 短レーザー片(レーザー本体を構成する部品)
//   1 = 赤菱形弾

static void ShotLaevateinn(sEnemyShotSet* pEnemyShotSet)
{
    const int    NUM_SEGMENTS = 13;    // レーザーを構成する短レーザー片の数
    const double SEGMENT_SPACING = 58.0;  // 短レーザー片どうしの間隔
    const int    PHASE_DURATION_MOVE = 150;   // 平行移動フェーズの長さ(フレーム)
    const int    PHASE_DURATION_ROTATE = 200;   // 回転フェーズの長さ(フレーム)
    const int    DIAMOND_EMIT_INTERVAL = 2;      // 菱形弾を発射する間隔(フレーム)
    const double DIAMOND_ACCEL = 0.09;   // 菱形弾の加速度
    const double DIAMOND_TERMINAL_SPEED = 4.5;    // 菱形弾の終端速度
    const double BOUND_MARGIN = -60.0;  // レーザー根本が画面外へ漂流してよい範囲

    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // param_i[0] : 周期(フェーズ)カウンタ
    // param_i[1] : 現在のモード(0=平行移動, 1=回転移動)
    // param_i[3] : 現在のフェーズが開始した pEnemyShotSet->count
    // param_d[0] : レーザーの向き(角度)
    // param_d[1] : レーザー根本(ボス位置)の x
    // param_d[2] : レーザー根本(ボス位置)の y
    // param_d[3] : 根本の移動速度 vx
    // param_d[4] : 根本の移動速度 vy
    // param_d[5] : 角速度 ω

    int mode = pEnemyShotSet->param_i[1];
    int elapsed = pEnemyShotSet->count - pEnemyShotSet->param_i[3];
    int phaseDuration = (mode == 0) ? PHASE_DURATION_MOVE : PHASE_DURATION_ROTATE;

    double angle = pEnemyShotSet->param_d[0];
    double pivotX = pEnemyShotSet->param_d[1];
    double pivotY = pEnemyShotSet->param_d[2];

    if (pEnemyShotSet->count == 0) {
        // 初回：周期0番(速めの平行移動)から開始する
        pEnemyShotSet->param_i[0] = 0;
        mode = 0;
        angle = GetRand(359) / 180.0 * DX_PI;
        pivotX = 240.0;
        pivotY = 120.0;

        double moveDir = angle + ((GetRand(1) == 0) ? 1.0 : -1.0) * DX_PI / 2.0;
        double speed = 4.0;
        pEnemyShotSet->param_d[3] = speed * cos(moveDir); // vx
        pEnemyShotSet->param_d[4] = speed * sin(moveDir); // vy
        pEnemyShotSet->param_d[5] = 0.0;                  // ω

        pEnemyShotSet->param_i[1] = mode;
        pEnemyShotSet->param_i[3] = 0;
        elapsed = 0;
    }
    else if (elapsed >= phaseDuration) {
        // 周期の切り替わり：レーザーの動きを変える
        pEnemyShotSet->param_i[0]++;
        mode = pEnemyShotSet->param_i[0] % 2;
        pEnemyShotSet->param_i[1] = mode;
        pEnemyShotSet->param_i[3] = pEnemyShotSet->count;
        elapsed = 0;

        if (mode == 0) {
            // 速めの平行移動：現在のレーザーの向きに対して垂直な方向へ
            double moveDir = angle + ((GetRand(1) == 0) ? 1.0 : -1.0) * DX_PI / 2.0;
            double speed = 4.0;
            pEnemyShotSet->param_d[3] = speed * cos(moveDir);
            pEnemyShotSet->param_d[4] = speed * sin(moveDir);
            pEnemyShotSet->param_d[5] = 0.0;
        }
        else {
            // 遅めの平行移動 + 速めの回転移動
            double moveDir = GetRand(359) / 180.0 * DX_PI;
            double speed = 0.5;
            pEnemyShotSet->param_d[3] = speed * cos(moveDir);
            pEnemyShotSet->param_d[4] = speed * sin(moveDir);
            pEnemyShotSet->param_d[5] = ((GetRand(1) == 0) ? 1.0 : -1.0) * (2.0 * DX_PI / 120.0);
        }
    }

    double vx = pEnemyShotSet->param_d[3];
    double vy = pEnemyShotSet->param_d[4];
    double omega = pEnemyShotSet->param_d[5];

    // ---- レーザー根本(=ボス)の位置と、レーザーの向きを更新 ----
    pivotX += vx;
    pivotY += vy;
    angle += omega;

    // 画面外マージンを越えたら跳ね返す(固定境界で速度を止めると根本が張り付いて
    // 止まって見えてしまうため、位置を戻すだけでなく速度も反転させる)
    if (pivotX < -BOUND_MARGIN) { pivotX = -BOUND_MARGIN;         vx = -vx; }
    if (pivotX > 480.0 + BOUND_MARGIN) { pivotX = 480.0 + BOUND_MARGIN; vx = -vx; }
    if (pivotY < -BOUND_MARGIN) { pivotY = -BOUND_MARGIN;         vy = -vy; }
    if (pivotY > 480.0 + BOUND_MARGIN) { pivotY = 480.0 + BOUND_MARGIN; vy = -vy; }

    pEnemyShotSet->param_d[0] = angle;
    pEnemyShotSet->param_d[1] = pivotX;
    pEnemyShotSet->param_d[2] = pivotY;
    pEnemyShotSet->param_d[3] = vx;
    pEnemyShotSet->param_d[4] = vy;
    pEnemyShotSet->x = pivotX;
    pEnemyShotSet->y = pivotY;
    pEnemyShotSet->muki = angle;

    enemy.x = pivotX;
    enemy.y = pivotY;

    double ux = cos(angle), uy = sin(angle);  // レーザーの向き
    double tx = -sin(angle), ty = cos(angle); // レーザーに垂直な向き(反時計回り側)

    // ---- 前フレームの短レーザー片を削除し、現在の位置・向きに並べ直す ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        if (pShot->param_i[0] == 0) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        pShot = pNext;
    }

    for (int i = 0; i < NUM_SEGMENTS; i++) {
        double d = (i + 0.5) * SEGMENT_SPACING;

        sEnemyShot* pSeg = new sEnemyShot;
        pSeg->x = pivotX + ux * d;
        pSeg->y = pivotY + uy * d;
        pSeg->muki = angle;
        pSeg->kind = img_enemyShotLaser[0]; // 赤色の短レーザー
        pSeg->param_i[0] = 0; // 種別：短レーザー片
        pSeg->margin = 999;

        pSeg->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pSeg->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pSeg;
        pEnemyShotSet->pEnemyShotHead->prev = pSeg;
    }

    // ---- レーザーを等間隔に内分する点から赤菱形弾を発射 ----
    if (elapsed % DIAMOND_EMIT_INTERVAL == 0) {
        for (int j = 1; j < NUM_SEGMENTS; j++) {
            double d = j * SEGMENT_SPACING;

            // この内分点の実際の速度(根本の並進 + 回転による接線速度の合成)
            double pvx = vx + omega * d * tx;
            double pvy = vy + omega * d * ty;

            // レーザーに垂直な2方向のうち、実速度との内積が正になる側を選ぶ
            double dot = tx * pvx + ty * pvy;
            double dirx = (dot >= 0.0) ? tx : -tx;
            double diry = (dot >= 0.0) ? ty : -ty;

            sEnemyShot* pDia = new sEnemyShot;
            pDia->x = pivotX + ux * d;
            pDia->y = pivotY + uy * d;
            pDia->muki = atan2(diry, dirx);
            pDia->speed = 0.0; // 初速0から加速開始
            pDia->kind = img_enemyShotDiamond[0]; // 赤菱形弾
            pDia->param_i[0] = 1; // 種別：菱形弾
            pDia->margin = 480;

            pDia->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pDia->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pDia;
            pEnemyShotSet->pEnemyShotHead->prev = pDia;
        }
    }

    // ---- 既存の赤菱形弾を加速させながら移動 ----
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            double spd = pShot->speed + DIAMOND_ACCEL;
            if (spd > DIAMOND_TERMINAL_SPEED) spd = DIAMOND_TERMINAL_SPEED;
            pShot->speed = spd;

            pShot->x += spd * cos(pShot->muki);
            pShot->y += spd * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン：禁忌「レーヴァテイン」
void EnemyPat_Levatain_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 120.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLaevateinn;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->alive = 9999;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}