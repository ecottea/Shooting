// enemyPat_Tmp.cpp
// 弾幕：花冠落果「ケントの林檎」
//   ケントの花が咲き → リンゴが実り → 重力で落ち → 割れて種と花粉を散らす
//
// 敵本体のパターン関数：void EnemyPat_FlowerOfKent_DeepSeek()
//
// 使用素材：
//   画像：
//     img_enemyShotLargeBall[6]   （白）        花芯
//     img_enemyShotMediumBall[4]  （青）        外花弁
//     img_enemyShotMediumBall[5]  （マゼンタ）  内花弁
//     img_enemyShotSmallBall[1]   （黄）        花粉・種
//     img_enemyShotSmallBall[2]   （緑）        葉
//     img_enemyShotSmallBall[0]   （赤）        リンゴ破片
//     img_enemyShotSmallBall[8]   （橙）        種
//     img_enemyShotLargeBall[0]   （赤）        リンゴ本体
//   サウンド：
//     sound_enemyShot_light  … 開花・散花
//     sound_enemyShot_medium … 落下開始
//     sound_enemyShot_heavy  … 破裂
//     sound_enemyCharge      … リンゴ出現の予告音
//
// 仕様メモ：
//   count, pEnemyShotSet->count, pEnemyShot->count のインクリメントと
//   画面外の弾の消去はメインルーチンが行うため、ここでは何もしない。
//   GetRand(x) は 0〜x の (x+1) 種類の整数を返す点に注意。

// ============================================================
//  弾の役割（param_i[0] に格納）
// ============================================================
enum {
    ROLE_CORE = 0,   // 花芯
    ROLE_PETAL,      // 花弁
    ROLE_POLLEN,     // 花粉
    ROLE_APPLE,      // リンゴ本体
    ROLE_LEAF,       // 葉
    ROLE_SEED,       // 種・破片
};

// 前方宣言
static void ShotKentApple(sEnemyShotSet* pEnemyShotSet);

// ============================================================
//  弾をセットに追加するヘルパー
// ============================================================
static sEnemyShot* KentApple_AddShot(
    sEnemyShotSet* set, double x, double y,
    double muki, double speed, int kind, int role)
{
    sEnemyShot* s = new sEnemyShot;
    s->x = x;
    s->y = y;
    s->muki = muki;
    s->speed = speed;
    s->kind = kind;
    s->param_i[0] = role;
    s->prev = set->pEnemyShotHead->prev;
    s->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = s;
    set->pEnemyShotHead->prev = s;
    return s;
}

// ============================================================
//  弾をセットから削除するヘルパー
// ============================================================
static void KentApple_RemoveShot(sEnemyShot* s)
{
    s->prev->next = s->next;
    s->next->prev = s->prev;
    delete s;
}

// ============================================================
//  花を1輪咲かせる（新しい弾幕セットを生成）
// ============================================================
static void KentApple_SpawnFlower(double x, double y, int id)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotKentApple;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = id;

    // ダミーヘッド生成
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    // 弾幕セットリストへ登録
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ============================================================
//  弾幕本体：花冠落果「ケントの林檎」
// ============================================================
static void ShotKentApple(sEnemyShotSet* pEnemyShotSet)
{
    const int    t = pEnemyShotSet->count;
    const double cx = pEnemyShotSet->x;
    const double cy = pEnemyShotSet->y;

    // --------------------------------------------------------
    //  フェーズ1：開花
    // --------------------------------------------------------
    if (t == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int N = 12; // 花弁数

        // 花芯（白の大玉）
        KentApple_AddShot(pEnemyShotSet, cx, cy, 0.0, 0.0,
            img_enemyShotLargeBall[6], ROLE_CORE);

        // 外花弁（青の中玉）
        for (int i = 0; i < N; i++) {
            double a = 2.0 * DX_PI * i / N;
            sEnemyShot* p = KentApple_AddShot(pEnemyShotSet, cx, cy, a, 2.2,
                img_enemyShotMediumBall[4], ROLE_PETAL);
            p->param_d[0] = a;     // 基準角度
            p->param_d[1] = 32.0;  // 停止半径
        }
        // 内花弁（マゼンタの中玉）
        for (int i = 0; i < N; i++) {
            double a = 2.0 * DX_PI * (i + 0.5) / N;
            sEnemyShot* p = KentApple_AddShot(pEnemyShotSet, cx, cy, a, 1.6,
                img_enemyShotMediumBall[5], ROLE_PETAL);
            p->param_d[0] = a;
            p->param_d[1] = 18.0;
        }
        // 花粉（黄の小玉）
        for (int i = 0; i < 8; i++) {
            double a = 2.0 * DX_PI * i / 8 + 0.2;
            KentApple_AddShot(pEnemyShotSet, cx, cy, a, 0.9,
                img_enemyShotSmallBall[1], ROLE_POLLEN);
        }
    }

    // --------------------------------------------------------
    //  フェーズ2：結実（リンゴ出現）
    // --------------------------------------------------------
    if (t == 60) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 葉（緑の小玉）… リスト順安定のため先に生成
        KentApple_AddShot(pEnemyShotSet, cx + 10.0, cy - 50.0, 0.0, 0.0,
            img_enemyShotSmallBall[2], ROLE_LEAF);

        // リンゴ本体（赤の大玉）
        sEnemyShot* apple = KentApple_AddShot(pEnemyShotSet, cx, cy - 40.0, 0.0, 0.0,
            img_enemyShotLargeBall[0], ROLE_APPLE);
        apple->param_d[0] = 0.0;  // 落下速度 vy
        apple->param_d[1] = 0.0;  // 横ドリフト vx
        apple->param_i[1] = 0;    // 落下開始フラグ
    }

    // --------------------------------------------------------
    //  フェーズ3：散花（花の終わり）
    // --------------------------------------------------------
    if (t == 180) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 16; i++) {
            double a = 2.0 * DX_PI * i / 16;
            KentApple_AddShot(pEnemyShotSet, cx, cy, a, 2.2,
                img_enemyShotSmallBall[1], ROLE_SEED);
        }
    }
    if (t > 180) {
        // 花芯・花弁・花粉を削除
        sEnemyShot* s = pEnemyShotSet->pEnemyShotHead->next;
        while (s != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* next = s->next;
            const int role = s->param_i[0];
            if (role == ROLE_CORE || role == ROLE_PETAL) {
                KentApple_RemoveShot(s);
            }
            s = next;
        }
    }

    // --------------------------------------------------------
    //  各弾の更新
    // --------------------------------------------------------
    sEnemyShot* s = pEnemyShotSet->pEnemyShotHead->next;
    while (s != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = s->next;
        const int role = s->param_i[0];

        s->param_i[2]++;                 // 自前タイマー
        const int myT = s->param_i[2];

        switch (role) {
        case ROLE_CORE:
            // 花芯は静止
            break;

        case ROLE_PETAL: {
            const double baseA = s->param_d[0];
            const double stopR = s->param_d[1];
            if (myT < 15) {
                // 外向きに移動
                s->x += s->speed * cos(s->muki);
                s->y += s->speed * sin(s->muki);
            }
            else {
                // 停止半径を保ったままゆっくり回転
                const double rot = 0.4 * (double)(myT - 15) * DX_PI / 180.0;
                const double a = baseA + rot;
                s->x = cx + stopR * cos(a);
                s->y = cy + stopR * sin(a);
            }
            break;
        }

        case ROLE_POLLEN:
            s->x += s->speed * cos(s->muki);
            s->y += s->speed * sin(s->muki);
            break;

        case ROLE_APPLE: {
            if (t < 90) break; // 落下開始まで静止

            // 落下開始時の初期化
            if (s->param_i[1] == 0) {
                s->param_i[1] = 1;
                s->param_d[1] = (player.x - s->x) * 0.003; // 自機方向への微弱ドリフト
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }

            // 重力加速（終端速度あり）
            s->param_d[0] += 0.18;
            if (s->param_d[0] > 9.0) s->param_d[0] = 9.0;
            s->y += s->param_d[0];
            s->x += s->param_d[1];

            // 落下中に花粉をこぼす
            if (myT % 4 == 0) {
                const double a = DX_PI * 0.5 + (GetRand(40) - 20) / 180.0 * DX_PI;
                KentApple_AddShot(pEnemyShotSet, s->x, s->y + 6.0, a, 1.4,
                    img_enemyShotSmallBall[1], ROLE_POLLEN);
            }

            // 葉をリンゴに追従させる
            {
                sEnemyShot* lf = pEnemyShotSet->pEnemyShotHead->next;
                while (lf != pEnemyShotSet->pEnemyShotHead) {
                    if (lf->param_i[0] == ROLE_LEAF) {
                        lf->x = s->x + 10.0;
                        lf->y = s->y - 10.0;
                        break;
                    }
                    lf = lf->next;
                }
            }

            // 破裂判定：画面下から約 1/4 の高さ
            if (s->y > 360.0) {
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                // 種（黄・橙）
                const int M = 12;
                for (int i = 0; i < M; i++) {
                    const double a = 2.0 * DX_PI * i / M
                        + (GetRand(10) - 5) / 180.0 * DX_PI;
                    const int color = (i % 2 == 0) ? 1 : 8;
                    KentApple_AddShot(pEnemyShotSet, s->x, s->y, a, 2.6,
                        img_enemyShotSmallBall[color], ROLE_SEED);
                }
                // 赤い破片
                for (int i = 0; i < 6; i++) {
                    const double a = 2.0 * DX_PI * i / 6 + 0.3;
                    KentApple_AddShot(pEnemyShotSet, s->x, s->y, a, 3.4,
                        img_enemyShotSmallBall[0], ROLE_SEED);
                }

                // 葉を削除（リンゴより前のリスト位置にあるので next は安全）
                {
                    sEnemyShot* lf = pEnemyShotSet->pEnemyShotHead->next;
                    while (lf != pEnemyShotSet->pEnemyShotHead) {
                        sEnemyShot* lnext = lf->next;
                        if (lf->param_i[0] == ROLE_LEAF) {
                            KentApple_RemoveShot(lf);
                        }
                        lf = lnext;
                    }
                }

                // リンゴ本体を削除
                KentApple_RemoveShot(s);
            }
            break;
        }

        case ROLE_LEAF:
            // 位置はリンゴ側で同期するため何もしない
            break;

        case ROLE_SEED:
            s->x += s->speed * cos(s->muki);
            s->y += s->speed * sin(s->muki);
            break;
        }

        s = next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_FlowerOfKent_DeepSeek()
{
    static int muki;
    static int flower_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200 で固定
        muki = 1;
        flower_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (enemy.x < 80.0) muki = 1;
        if (enemy.x > 400.0) muki = -1;
    }

    const int T = 600;
    int countT = count % T;

    // 第1波：3輪の花を時差で咲かせる
    if (countT == 60 || countT == 100 || countT == 140) {
        static const double xs[3] = { 90.0, 240.0, 390.0 };
        static const double ys[3] = { 90.0,  70.0,  90.0 };
        if (flower_count < 3) {
            KentApple_SpawnFlower(xs[flower_count], ys[flower_count], flower_count);
            flower_count++;
        }
    }

    // 第2波：3輪同時
    if (countT == 340) {
        KentApple_SpawnFlower(90.0, 90.0, 3);
        KentApple_SpawnFlower(240.0, 70.0, 4);
        KentApple_SpawnFlower(390.0, 90.0, 5);
        flower_count = 0;
    }
}