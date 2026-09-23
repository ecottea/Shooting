// ============================================================
// 玉すだれ弾幕「伸縮自在の曲芸」
// 使う素材の選定理由
// ------------------------------------------------------------
// ・小玉(2.5x2.5) 白 : すだれの骨の関節。最小で当たり判定が小さいため
//   18個を等間隔の鎖として並べても理不尽にならない。鎖の芯になる。
// ・鱗弾(4.0x3.0) 黄 : 竹ヒゴ。米粒に近い小さい楕円で、向きを90度ずらすと
//   すだれの横糸に見える。色は竹らしい黄(1)を使用。
// ・中玉(7.0x7.0) 橙 : 房の飾り。しなった内側だけに零して危険地帯を演出。
//   大玉は大きすぎ、短レーザー(64x4)は曲げ表現と相性が悪いため不採用。
// ============================================================

static void ShotTamasudare(sEnemyShotSet* pEnemyShotSet)
{
    const int JOINT_NUM = 18;
    const double CHAIN_SP = 13.0+5;

    // 生成時
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // ベース角度と基本速度をセット側に保存
        pEnemyShotSet->param_d[0] = pEnemyShotSet->muki; // base angle
        pEnemyShotSet->param_d[1] = 1.9;                  // base speed
        pEnemyShotSet->param_i[0] = JOINT_NUM;

        // 関節 + 竹ヒゴ
        for (int j = 0; j < JOINT_NUM; j++) {
            double dist = j * CHAIN_SP;
            double bx = pEnemyShotSet->x + cos(pEnemyShotSet->muki) * dist;
            double by = pEnemyShotSet->y + sin(pEnemyShotSet->muki) * dist;

            // ---- 0: 関節 = 小玉 白 ----
            {
                sEnemyShot* p = new sEnemyShot;
                p->x = bx;
                p->y = by;
                p->muki = pEnemyShotSet->muki;
                p->speed = 1.9;
                p->kind = img_enemyShotSmallBall[6]; // 白
                p->param_i[0] = j;  // joint index
                p->param_i[1] = 0;  // type 0=joint
                p->param_d[0] = dist;
                p->param_d[2] = pEnemyShotSet->muki; // move angle
                p->margin = 240;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
            // ---- 1: 竹ヒゴ = 鱗弾 黄 ----
            {
                sEnemyShot* p = new sEnemyShot;
                p->x = bx;
                p->y = by;
                p->muki = pEnemyShotSet->muki + DX_PI / 2.0; // 見た目を90度ずらして横糸に
                p->speed = 1.9;
                p->kind = img_enemyShotScale[1]; // 黄 = 竹色
                p->param_i[0] = j;
                p->param_i[1] = 1; // type 1=slat
                p->param_d[0] = dist;
                p->param_d[2] = pEnemyShotSet->muki;
                p->margin = 240;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
        // ---- 2: 房 = 中玉 橙 (先端3つだけ) ----
        for (int k = 0; k < 3; k++) {
            int j = JOINT_NUM - 1 + k;
            double dist = j * CHAIN_SP;
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x + cos(pEnemyShotSet->muki) * dist;
            p->y = pEnemyShotSet->y + sin(pEnemyShotSet->muki) * dist;
            p->muki = pEnemyShotSet->muki;
            p->speed = 2.0;
            p->kind = img_enemyShotMediumBall[8]; // 橙
            p->param_i[0] = j;
            p->param_i[1] = 2; // type 2=deco
            p->param_d[0] = dist;
            p->param_d[2] = pEnemyShotSet->muki;
            p->margin = 240;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
        return;
    }

    // 毎フレーム更新
    int t = pEnemyShotSet->count;
    double baseAngle = pEnemyShotSet->param_d[0];

    // 速度フェーズ：伸び -> 溜め(しなり) -> 巻き戻し
    double curSpeed;
    if (t < 70) {
        curSpeed = 1.9 - t * 0.012; // だんだん減速
        if (curSpeed < 0.3) curSpeed = 0.3;
    }
    else if (t < 160) {
        curSpeed = 0.35; // しなって見せるホールド
    }
    else {
        curSpeed = -2.6; // 巻き取り：ボス方向へ戻る
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int j = pShot->param_i[0];
        int type = pShot->param_i[1];

        // 波を根本から先端へ伝播させる
        // GetRandは使わず、tとjで決定的に計算（リプレイ対応）
        double phase = t * 0.06 - j * 0.32;
        double bendFactor = (double)j / (double)JOINT_NUM; // 先端ほど曲がる
        double bend = sin(phase) * 0.70 * (0.35 + bendFactor);

        // ホールド中はしなりを強調
        if (t >= 70 && t < 160) bend *= 1.35;

        double moveAng = baseAngle + bend;
        pShot->param_d[2] = moveAng;
        pShot->speed = curSpeed;
        if (type == 2) pShot->speed = curSpeed * 1.15; // 房は少し速く

        // 見た目の向き
        if (type == 0) {
            pShot->muki = moveAng; // 小丸は向き関係なし
        }
        else if (type == 1) {
            pShot->muki = moveAng + DX_PI / 2.0; // 竹ヒゴは進行方向に垂直
        }
        else {
            pShot->muki = moveAng;
        }

        // 移動はmoveAngで（mukiが90度ずれていても進行方向は維持）
        pShot->x += pShot->speed * cos(moveAng);
        pShot->y += pShot->speed * sin(moveAng);

        pShot = pShot->next;
    }
}

// 敵本体
void EnemyPat_NankinTamasudare_MetaAI()
{
    static int muki;
    static int waveCount;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        waveCount = 0;
    }
    else {
        enemy.x += 0.85 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 180Fごとに玉すだれを展開。3パターンをローテーション
    if (count % 180 == 1) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double baseMuki = atan2(player.y - (enemy.y + 10.0), player.x - enemy.x);

        int pat = waveCount % 3;
        int rodNum;
        double pitchDeg;
        if (pat == 0) { rodNum = 3; pitchDeg = 16.0; }      // 釣り竿：3本細く
        else if (pat == 1) { rodNum = 5; pitchDeg = 12.0; } // 橋：5本で扇
        else { rodNum = 4; pitchDeg = 20.0; }               // 大技：4本広く

        for (int r = 0; r < rodNum; r++) {
            // 中央からのオフセットを計算
            // GetRand(x)は0..xなので、-5..5の揺らぎは GetRand(10)-5
            double offsetIdx = r - (rodNum - 1) / 2.0;
            double rnd = (double)(GetRand(10) - 5) * DX_PI / 180.0; // ±5度

            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotTamasudare;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = baseMuki + offsetIdx * (pitchDeg * DX_PI / 180.0) + rnd;
            pSet->kind = waveCount % 9;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            // param初期化（PoolAllocator継承でも念のため0クリアはコンストラクタでされる）
            // リスト連結
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        waveCount++;
    }
}