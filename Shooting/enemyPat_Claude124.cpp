// enemyPat_kochSnowflake.cpp
//
// 弾幕：コッホ雪片(自己相似分裂弾)
//
// コッホ曲線の生成規則 F -> F+F--F+F (分岐角60°) を弾の分裂挙動に変換したパターン。
// 「辺」を表す1発の弾が一定フレーム経過後に4発の子弾へ分裂し、
// 子弾の進行方向は親弾の進行方向を基準に {0°, +60°, -60°, 0°} のオフセットを取る。
// これがコッホ曲線特有の「直進→山型の出っ張り→直進」という凹凸そのものになる。
//
// 第0世代(正三角形の3辺)から第5世代まで、計6世代にわたって分裂を繰り返す。
// 世代が進むごとに 大玉 -> 中楕円弾 -> 中玉 -> 鱗弾 -> 菱形弾 -> 小玉 と弾種を小さくし、
// 色もシアン->青->白と変化させることで、雪の結晶が枝分かれしながら
// 段階的に細かくなっていく質感を表現する。
// 親弾は分裂後も消滅せず飛び続けるため(画面外消去はメインルーチン任せ)、
// 世代を重ねるごとに画面上の弾数が雪だるま式に増えていく設計。

static const int    KOCH_MAX_GEN = 5;                // 最大世代数(0〜5の6世代)
static const double KOCH_ANGLE = DX_PI / 3.0;         // コッホ曲線の分岐角(60°)

// 世代 0->1->2->3->4->5 それぞれの、分裂までの待機フレーム数
static const int KOCH_SPLIT_DELAY[KOCH_MAX_GEN] = { 50, 45, 40, 35, 30 };

// 世代0〜5それぞれの弾速(世代が進むほど速く・鋭くなる)
static const double KOCH_SPEED[KOCH_MAX_GEN + 1] = { 1.6, 1.9, 2.2, 2.5, 2.8, 3.2 };

// pEnemyShot->param_i の用途
enum {
    KOCH_PARAM_GEN = 0,   // 世代番号(0〜KOCH_MAX_GEN)
    KOCH_PARAM_SPLIT = 1, // 分裂済みフラグ(0:未分裂 1:分裂済み)
};

// 世代に応じた弾の種類(形状+色)を選ぶ
// 弾の種類と半径一覧: 小玉(2.5x2.5)、中玉(7.0x7.0)、大玉(20.0x20.0)、銃弾(5.0x2.0)、
//                     鱗弾(4.0x3.0)、菱形弾(4.5x2.5)、中楕円弾(10.5x7.0)、短レーザー(64.0x4.0)
// 弾の色一覧:         0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
static int KochShotKind(int gen)
{
    switch (gen) {
    case 0:  return img_enemyShotLargeBall[3];   // 第0世代: 大玉・シアン(雪片の骨格)
    case 1:  return img_enemyShotMediumOval[3];  // 第1世代: 中楕円弾・シアン
    case 2:  return img_enemyShotMediumBall[4];  // 第2世代: 中玉・青
    case 3:  return img_enemyShotScale[4];       // 第3世代: 鱗弾・青
    case 4:  return img_enemyShotDiamond[6];     // 第4世代: 菱形弾・白
    default: return img_enemyShotSmallBall[6];   // 第5世代(末端): 小玉・白(結晶の先端)
    }
}

// 弾を1発生成し、shot set の末尾(head の手前)へ追加する
static sEnemyShot* KochAddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, int gen)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;               // 進行方向＝見た目の向きとして設定
    pEnemyShot->speed = KOCH_SPEED[gen];
    pEnemyShot->kind = KochShotKind(gen);
    pEnemyShot->param_i[KOCH_PARAM_GEN] = gen;
    pEnemyShot->param_i[KOCH_PARAM_SPLIT] = 0;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

    return pEnemyShot;
}

// 弾幕：コッホ雪片(自己相似分裂弾)
static void ShotKochSnowflake(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 波ごとに初期回転角をランダムに変え、単調な繰り返しにならないようにする
        // GetRand(359) は 0〜359 の360通りを返すので、度数として使える
        double baseMuki = GetRand(359) / 180.0 * DX_PI;

        // 第0世代：正三角形を構成する3辺(120°間隔)
        for (int i = 0; i < 3; i++) {
            double muki = baseMuki + i * (2.0 * DX_PI / 3.0);
            KochAddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, muki, 0);
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int gen = pShot->param_i[KOCH_PARAM_GEN];

        // 分裂判定(最終世代は分裂しない)
        if (gen < KOCH_MAX_GEN &&
            pShot->param_i[KOCH_PARAM_SPLIT] == 0 &&
            pShot->count >= KOCH_SPLIT_DELAY[gen]) {

            pShot->param_i[KOCH_PARAM_SPLIT] = 1;

            // コッホ曲線の生成規則 F+F--F+F による4方向オフセット
            // (直進 / 左60° / 右60° / 直進 の順で山型の出っ張りを作る)
            static const double offset[4] = { 0.0, KOCH_ANGLE, -KOCH_ANGLE, 0.0 };
            for (int i = 0; i < 4; i++) {
                KochAddShot(pEnemyShotSet, pShot->x, pShot->y, pShot->muki + offset[i], gen + 1);
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_KochSnowflake_Claude()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 150フレームごとに新しい雪片を1つ展開する(波状に連続発生)
    if (count % 150 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKochSnowflake;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 未使用
        pEnemyShotSet->kind = 0;   // 未使用

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}