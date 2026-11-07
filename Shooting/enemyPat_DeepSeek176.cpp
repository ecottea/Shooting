// ============================================================
//  弾幕：線香花火「垂れ火花」
//
//  既存の敵弾素材のみで以下の要素を表現する：
//    ・線香本体     … 短レーザー(白)を斜め上に伸ばす
//    ・火球         … 大玉(赤) → 中玉(橙) → 小玉(黄) と縮小
//    ・垂れる火花   … 小玉(橙)を下向きに放出＋落下加速
//    ・縮小の火花   … 各段階で下向き半円に小玉(黄)を放射
//    ・最後の爆散   … 小玉(赤)を下向き中心に放射し、線香と火球を消去
//
//  弾種(param_i[0])の内訳:
//    0 = 線香本体（静止）
//    1 = 火球（静止）
//    2 = 火花（落下加速）
// ============================================================

// 弾をセットに追加する補助関数
//   type: 0=線香本体, 1=火球, 2=火花
static void AddIncenseShot(sEnemyShotSet* set, double x, double y,
    double muki, double speed, int kind, int type)
{
    sEnemyShot* s = new sEnemyShot;
    s->x = x;
    s->y = y;
    s->muki = muki;
    s->speed = speed;
    s->kind = kind;
    s->param_i[0] = type;
    s->prev = set->pEnemyShotHead->prev;
    s->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = s;
    set->pEnemyShotHead->prev = s;
}

// 線香花火のメイン更新
// セットの param 使用:
//   param_i[0] : 初期化フラグ (0=未, 1=済)
//   param_i[1] : 火球ステージ   (0=大, 1=中, 2=小)
//   param_i[2] : 最終爆散フラグ
//   param_d[0] : 火球のX座標
//   param_d[1] : 火球のY座標
static void ShotIncenseFirework(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;

    // ==================== 初期化 ====================
    if (pEnemyShotSet->param_i[0] == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double fb_x = pEnemyShotSet->x;
        double fb_y = pEnemyShotSet->y + 80.0;
        pEnemyShotSet->param_d[0] = fb_x;
        pEnemyShotSet->param_d[1] = fb_y;

        // --- 線香本体（短レーザー・白）: 火球から斜め上へ伸ばす ---
        double ang = -DX_PI / 2.0 + 0.3;   // 真上から約17度傾ける
        double ux = cos(ang);
        double uy = sin(ang);
        double stick_len = 70.0;
        double stick_cx = fb_x + ux * (stick_len * 0.5);
        double stick_cy = fb_y + uy * (stick_len * 0.5);
        AddIncenseShot(pEnemyShotSet, stick_cx, stick_cy, ang, 0.0,
            img_enemyShotLaser[6], 0);   // 短レーザー・白

        // --- 火球（大玉・赤） ---
        AddIncenseShot(pEnemyShotSet, fb_x, fb_y, 0.0, 0.0,
            img_enemyShotLargeBall[0], 1); // 大玉・赤

        pEnemyShotSet->param_i[0] = 1;
        pEnemyShotSet->param_i[1] = 0;
        pEnemyShotSet->param_i[2] = 0;
        return;
    }

    // ==================== 通常更新 ====================
    double fb_x = pEnemyShotSet->param_d[0];
    double fb_y = pEnemyShotSet->param_d[1];
    int    stage = pEnemyShotSet->param_i[1];
    int    ended = pEnemyShotSet->param_i[2];

    // 火球を探す
    sEnemyShot* fb = nullptr;
    for (sEnemyShot* s = pEnemyShotSet->pEnemyShotHead->next;
        s != pEnemyShotSet->pEnemyShotHead; s = s->next) {
        if (s->param_i[0] == 1) { fb = s; break; }
    }

    if (fb && !ended) {
        // ---- 火球ステージ遷移 ----
        if (stage == 0 && t >= 90) {
            // 大玉 → 中玉
            fb->kind = img_enemyShotMediumBall[8]; // 中玉・橙
            pEnemyShotSet->param_i[1] = 1;
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            for (int i = 0; i < 12; i++) {
                double a = DX_PI * (0.25 + 0.5 * i / 11.0); // 下向き半円
                AddIncenseShot(pEnemyShotSet, fb_x, fb_y, a,
                    1.5 + GetRand(50) / 100.0,
                    img_enemyShotSmallBall[1], 2); // 小玉・黄
            }
        }
        else if (stage == 1 && t >= 180) {
            // 中玉 → 小玉
            fb->kind = img_enemyShotSmallBall[1];  // 小玉・黄
            pEnemyShotSet->param_i[1] = 2;
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            for (int i = 0; i < 12; i++) {
                double a = DX_PI * (0.25 + 0.5 * i / 11.0);
                AddIncenseShot(pEnemyShotSet, fb_x, fb_y, a,
                    1.5 + GetRand(50) / 100.0,
                    img_enemyShotSmallBall[1], 2);
            }
        }
        else if (stage == 2 && t >= 240) {
            // ---- 最終爆散 ----
            pEnemyShotSet->param_i[2] = 1;
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            for (int i = 0; i < 24; i++) {
                double a = DX_PI * (0.15 + 0.7 * i / 23.0); // 下向き中心の放射
                AddIncenseShot(pEnemyShotSet, fb_x, fb_y, a,
                    2.0 + GetRand(200) / 100.0,
                    img_enemyShotSmallBall[0], 2); // 小玉・赤
            }
            // 線香本体と火球をリストから削除
            sEnemyShot* s = pEnemyShotSet->pEnemyShotHead->next;
            while (s != pEnemyShotSet->pEnemyShotHead) {
                sEnemyShot* next = s->next;
                if (s->param_i[0] == 0 || s->param_i[0] == 1) {
                    s->prev->next = s->next;
                    s->next->prev = s->prev;
                    delete s;
                }
                s = next;
            }
        }

        // ---- 垂れ火花（爆散直前まで） ----
        bool burst_time = (stage == 2 && t >= 240);
        if (!burst_time && t % 6 == 0) {
            int n = 2 + GetRand(1); // 2 or 3
            for (int i = 0; i < n; i++) {
                double a = DX_PI / 2.0 + (GetRand(30) - 15) / 180.0 * DX_PI; // 真下±15度
                AddIncenseShot(pEnemyShotSet,
                    fb_x + (GetRand(20) - 10),
                    fb_y + 4.0,
                    a,
                    0.5 + GetRand(30) / 100.0,
                    img_enemyShotSmallBall[8], 2); // 小玉・橙
            }
        }
    }

    // ---- 全弾更新（火花の落下加速） ----
    sEnemyShot* s = pEnemyShotSet->pEnemyShotHead->next;
    while (s != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* next = s->next;
        if (s->param_i[0] == 2) {
            s->speed += 0.04;
            s->x += s->speed * cos(s->muki);
            s->y += s->speed * sin(s->muki);
        }
        s = next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Sparkler_DeepSeek()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    else {
        // 420フレーム周期で同じ位置に戻るようゆっくり左右に移動
        enemy.x = 240.0 + 60.0 * sin(count * (2.0 * DX_PI / 420.0));
    }

    // 420フレーム周期（約7秒）で線香花火を1回発射
    if (count % 320 == 30) for (int i= -1; i <= 1; i++){
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotIncenseFirework;
        pEnemyShotSet->x = enemy.x + 240 * i;
        pEnemyShotSet->y = enemy.y - 50 * abs(i);
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->alive = 600; // 火花の落下を最後まで見せる

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}