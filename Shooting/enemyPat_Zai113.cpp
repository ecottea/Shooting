// enemyPat_Tmp.cpp
// 桜（マゼンタ）× 彗星（シアン）をモチーフにした斑鳩風ボス弾幕
//
// 【動作概要】※60FPS想定（1秒=60フレーム）
// ・0秒後  : sound_enemyCharge が鳴る
// ・1秒後  : sound_enemyShot_extreme が鳴り、自機を取り囲むマゼンタ小玉img_enemyShotSmallBall[5]の輪が出現し自機に追従
//            輪の小玉に触れたマゼンタの敵弾は消滅する
// ・3秒後  : sound_enemyCharge が鳴る
// ・4秒後  : sound_enemyShot_extreme が鳴り、輪がシアン小玉img_enemyShotSmallBall[3]に変化
//            シアン小玉に触れたシアンの敵弾は消滅する
// ・以降、3秒おきに「予告音→変化」を繰り返す（マゼンタ⇔シアン反転）
// ・左のボス=桜（マゼンタ弾）、右のボス=彗星（シアン弾）
// ・単色なら回避可能だが両色同時は回避困難 → 輪の色替えに合わせた立ち回りがカギ
//
// img_enemyShotSmallBall[5] / img_enemyShotSmallBall[3] / sound_enemyCharge /
// sound_enemyShot_extreme はこの守護リング専用とする

// ---------------------------------------------------------------------------
// 定数（調整用）
// ---------------------------------------------------------------------------
static const int    RING_NUM = 18*2;    // 守護リングの弾数
static const double RING_RADIUS = 88.0;  // 自機からの半径
static const double RING_ROT_SPD = 0.012; // リングの回転速度(rad/フレーム)
static const double GUARD_R = 2.5*2;   // 小玉(2.5x2.5)の当たり半径
static const int    COLOR_MAGENTA = 5;     // 弾の色: マゼンタ
static const int    COLOR_CYAN = 3;     // 弾の色: シアン

// ---------------------------------------------------------------------------
// ヘルパー
// ---------------------------------------------------------------------------

// 弾を生成してセットの弾リスト末尾に追加する
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// リストから弾を取り除いて解放する
static void RemoveShot(sEnemyShot* p)
{
    p->prev->next = p->next;
    p->next->prev = p->prev;
    delete p;
}

// kindが何型かを調べる（0:小玉 1:中玉 2:大玉 3:銃弾 4:鱗弾 5:菱形弾 6:中楕円弾 7:短レーザー）
static int ShotTypeOf(int kind)
{
    for (int i = 0; i < 9; i++) {
        if (kind == img_enemyShotSmallBall[i])  return 0;
        if (kind == img_enemyShotMediumBall[i]) return 1;
        if (kind == img_enemyShotLargeBall[i])  return 2;
        if (kind == img_enemyShotBullet[i])     return 3;
        if (kind == img_enemyShotScale[i])      return 4;
        if (kind == img_enemyShotDiamond[i])    return 5;
        if (kind == img_enemyShotMediumOval[i]) return 6;
        if (kind == img_enemyShotLaser[i])      return 7;
    }
    return -1;
}

// 弾種ごとの当たり判定楕円（長半径a・短半径b、長径は弾の進行方向mukiとする）
static void GetShotEllipse(int type, double* pa, double* pb)
{
    switch (type) {
    case 0: *pa = 2.5;  *pb = 2.5;  break; // 小玉
    case 1: *pa = 7.0;  *pb = 7.0;  break; // 中玉
    case 2: *pa = 20.0; *pb = 20.0; break; // 大玉
    case 3: *pa = 5.0;  *pb = 2.0;  break; // 銃弾
    case 4: *pa = 4.0;  *pb = 3.0;  break; // 鱗弾
    case 5: *pa = 4.5;  *pb = 2.5;  break; // 菱形弾
    case 6: *pa = 10.5; *pb = 7.0;  break; // 中楕円弾
    case 7: *pa = 64.0; *pb = 4.0;  break; // 短レーザー
    default: *pa = 4.0; *pb = 4.0;  break;
    }
}

// 指定色の弾かどうか（守護リング専用のimg_enemyShotSmallBall[5]/[3]は対象外）
static bool IsTargetColor(int kind, int color)
{
    if (kind == img_enemyShotSmallBall[COLOR_MAGENTA] ||
        kind == img_enemyShotSmallBall[COLOR_CYAN]) return false;

    return kind == img_enemyShotSmallBall[color] ||
        kind == img_enemyShotMediumBall[color] ||
        kind == img_enemyShotLargeBall[color] ||
        kind == img_enemyShotBullet[color] ||
        kind == img_enemyShotScale[color] ||
        kind == img_enemyShotDiamond[color] ||
        kind == img_enemyShotMediumOval[color] ||
        kind == img_enemyShotLaser[color];
}

// 楕円弾（中心ex,ey 向きemuki 長半径ea 短半径eb）と円（中心cx,cy 半径cr）の当たり判定
static bool EllipseHitCircle(double ex, double ey, double emuki, double ea, double eb,
    double cx, double cy, double cr)
{
    double dx = cx - ex;
    double dy = cy - ey;
    double dist2 = dx * dx + dy * dy;
    double sum = ea + cr;
    if (dist2 > sum * sum) return false; // 早期リジェクト
    double dist = sqrt(dist2);
    if (dist < 0.0001) return true;
    // 楕円中心→円中心方向の楕円の極半径を求めて比較
    double d = atan2(dy, dx) - emuki;
    double c = cos(d);
    double s = sin(d);
    double re = ea * eb / sqrt(eb * eb * c * c + ea * ea * s * s);
    return dist <= re + cr;
}

// ---------------------------------------------------------------------------
// 守護リング: 自機を囲み同色の敵弾を消す小玉の輪（マゼンタ⇔シアン）
// ---------------------------------------------------------------------------
static void ShotGuardRing(sEnemyShotSet* pSet)
{
    // ---- 予告音（0秒後、3秒後、6秒後…）----
    if (pSet->count % 180 == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ---- 変化（1秒後、4秒後、7秒後…）----
    if (pSet->count % 180 == 60) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        if (pSet->param_i[0] == 0) {
            // 初回: 自機を取り囲むようにマゼンタ小玉を配置
            pSet->param_i[0] = COLOR_MAGENTA;
            for (int i = 0; i < RING_NUM; i++) {
                sEnemyShot* p = AddShot(pSet, player.x, player.y, 0.0, 0.0,
                    img_enemyShotSmallBall[COLOR_MAGENTA]);
                p->margin = 999.0;      // 画面外判定で消去されないようにする
                p->param_i[0] = i;      // 何番目の小玉か
            }
        }
        else {
            // 2回目以降: マゼンタ⇔シアンで反転
            pSet->param_i[0] = (pSet->param_i[0] == COLOR_MAGENTA) ? COLOR_CYAN : COLOR_MAGENTA;
            sEnemyShot* p = pSet->pEnemyShotHead->next;
            while (p != pSet->pEnemyShotHead) {
                p->kind = img_enemyShotSmallBall[pSet->param_i[0]];
                p = p->next;
            }
        }
    }

    if (pSet->param_i[0] == 0) return; // リング未生成

    // ---- 自機に追従（ゆっくり回転しながら）----
    double rot = pSet->count * RING_ROT_SPD;
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double ang = p->param_i[0] * (2.0 * DX_PI / RING_NUM) + rot;
        p->x = player.x + RING_RADIUS * cos(ang);
        p->y = player.y + RING_RADIUS * sin(ang);
        p->muki = ang + DX_PI * 0.5;
        p = p->next;
    }

    // ---- 消弾: リングと同色の敵弾はリング小玉に触れると消滅 ----
    int color = pSet->param_i[0];
    sEnemyShotSet* pOther = enemyShotSetHead.next;
    while (pOther != &enemyShotSetHead) {
        if (pOther != pSet && pOther->pEnemyShotHead != nullptr) {
            sEnemyShot* pShot = pOther->pEnemyShotHead->next;
            while (pShot != pOther->pEnemyShotHead) {
                sEnemyShot* pNext = pShot->next;
                if (IsTargetColor(pShot->kind, color)) {
                    double a, b;
                    GetShotEllipse(ShotTypeOf(pShot->kind), &a, &b);
                    sEnemyShot* pGuard = pSet->pEnemyShotHead->next;
                    while (pGuard != pSet->pEnemyShotHead) {
                        if (EllipseHitCircle(pShot->x, pShot->y, pShot->muki, a, b,
                            pGuard->x, pGuard->y, GUARD_R)) {
                            RemoveShot(pShot); // 触れたので消滅
                            break;
                        }
                        pGuard = pGuard->next;
                    }
                }
                pShot = pNext;
            }
        }
        pOther = pOther->next;
    }
}

// ---------------------------------------------------------------------------
// 桜（マゼンタ）: 自機へ飛び近づくと開花する花芯と、舞い散る花びら
// ---------------------------------------------------------------------------
static void ShotSakura(sEnemyShotSet* pSet)
{
    // ---- 花芯の発射（自機狙い、0.5秒おき）----
    if (pSet->count % 15 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double base = atan2(player.y - enemy.y, player.x - enemy.x);
        double muki = base + (GetRand(25) - 12) / 180.0 * DX_PI;
        AddShot(pSet, enemy.x, enemy.y + 10.0, muki, 2.4 + GetRand(7) * 0.1,
            img_enemyShotMediumBall[COLOR_MAGENTA]);
    }

    // ---- 舞い散る花びら（画面上から降る）----
    if (pSet->count % 14 == 0) {
        sEnemyShot* p = AddShot(pSet, GetRand(480), -10.0, DX_PI / 2.0, 1.0 + GetRand(8) * 0.1,
            img_enemyShotScale[COLOR_MAGENTA]);
        p->param_i[0] = 1;            // 落下花弁フラグ
        p->param_i[1] = GetRand(628); // 揺れの位相
    }

    // ---- 弾の移動と開花 ----
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->kind == img_enemyShotMediumBall[COLOR_MAGENTA]) {
            // 花芯: 直進し、自機に近づくか一定時間で開花して消える
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            double dx = player.x - pShot->x;
            double dy = player.y - pShot->y;
            if ((pShot->count >= 30 && dx * dx + dy * dy < 150.0 * 150.0) || pShot->count >= 150) {
                double base = GetRand(360) * (2.0 * DX_PI) / 360.0;
                for (int k = 0; k < 8; k++) { // 外側の花びら
                    AddShot(pSet, pShot->x, pShot->y, base + k * (2.0 * DX_PI / 8), 3.2,
                        img_enemyShotScale[COLOR_MAGENTA]);
                }
                for (int k = 0; k < 8; k++) { // 内側の花びら
                    AddShot(pSet, pShot->x, pShot->y, base + (k + 0.5) * (2.0 * DX_PI / 8), 2.2,
                        img_enemyShotScale[COLOR_MAGENTA]);
                }
                RemoveShot(pShot); // 開花したので花芯は消滅
            }
        }
        else if (pShot->param_i[0] == 1) {
            // 落下花弁: 左右に揺れながら落ちる
            pShot->muki = DX_PI / 2.0 + 0.6 * sin((pShot->count * 3 + pShot->param_i[1]) * 0.01);
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else {
            // 開花した花びら: 放射状に直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// ---------------------------------------------------------------------------
// 彗星（シアン）: 弧を描いて飛ぶ彗星の頭と、その後ろに残る尾
// ---------------------------------------------------------------------------
static void ShotSuisei(sEnemyShotSet* pSet)
{
    // ---- 彗星の頭の発射（2秒おきに2基、自機の左右へ放つ）----
    if (pSet->count % 60 == 30) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double base = atan2(player.y - enemy.y2, player.x - enemy.x2);
        for (int i = 0; i < 2; i++) {
            double muki = base + (i == 0 ? -0.45 : 0.45) + (GetRand(21) - 10) * 0.01;
            sEnemyShot* p = AddShot(pSet, enemy.x2, enemy.y2 + 10.0, muki, 3.6 + GetRand(9) * 0.1,
                img_enemyShotMediumOval[COLOR_CYAN]);
            p->param_i[0] = (i == 0 ? -1 : 1) * (3 + GetRand(4)); // 曲がる向きと強さ(×0.001rad/フレーム)
            p->param_i[1] = 0; // 尾の間隔カウンタ
        }
    }

    // ---- 弾の移動と尾の生成 ----
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->kind == img_enemyShotMediumOval[COLOR_CYAN]) {
            // 頭: ゆるやかに弧を描いて進み、尾を引く
            pShot->muki += pShot->param_i[0] * 0.001;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            pShot->param_i[1]++;
            if (pShot->param_i[1] >= 4) {
                pShot->param_i[1] = 0;
                // 尾: 頭の後方にゆっくり残る鱗弾
                sEnemyShot* p = AddShot(pSet, pShot->x, pShot->y,
                    pShot->muki + DX_PI + (GetRand(41) - 20) * 0.01,
                    0.3 + GetRand(5) * 0.05,
                    img_enemyShotScale[COLOR_CYAN]);
                p->param_i[0] = 2; // 尾フラグ
                p->param_i[2] = 0; // 寿命カウンタ
            }
        }
        else if (pShot->param_i[0] == 2) {
            // 尾: ほぼその場に留まり、寿命が来ると消滅
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (++pShot->param_i[2] >= 150) {
                RemoveShot(pShot);
            }
        }
        else {
            // その他: 直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// ---------------------------------------------------------------------------
// 弾幕セット生成
// ---------------------------------------------------------------------------
static void CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = func;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->alive = 99999;

    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;

    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
}

// ---------------------------------------------------------------------------
// 敵本体のパターン
// ---------------------------------------------------------------------------
void EnemyPat_miComet_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 120.0;
        enemy.y = 40.0;
        enemy.x2 = 360.0;
        enemy.y2 = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        // 守護リング／桜（左）／彗星（右）の3セットを生成
        CreateShotSet(ShotGuardRing, enemy.x, enemy.y, 0.0);
        CreateShotSet(ShotSakura, enemy.x, enemy.y, 0.0);
        CreateShotSet(ShotSuisei, enemy.x2, enemy.y2, 0.0);
    }
    else {
        // 2体のボスが左右へ往復しながら移動する
        double t = count * 0.011;
        enemy.x = 120.0 + 80.0 * sin(t);
        enemy.y = 40.0 + 12.0 * sin(count * 0.017);
        enemy.x2 = 360.0 - 80.0 * sin(t);
        enemy.y2 = 40.0 + 12.0 * cos(count * 0.013);
    }
}