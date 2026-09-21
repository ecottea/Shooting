//==============================================================
// 弾幕：ゴルトンボード「確率の pin」
//  上部のペグ(障害物)の間を縫って落ちる小弾が、ペグに当たる
//  たび左右へ反射し、正規分布状に降り積もる。
//==============================================================

// ---- ペグ配置(ファイル内共有: 落下弾側の当たり判定でも使う) ----
static const int    PEG_MAX = 15;    // 1+2+3+4+5
static const double PEG_HIT_R = 12.0;  // 弾←→ペグの当たり判定半径
static double pegX[PEG_MAX];
static double pegY[PEG_MAX];
static int    pegNum = 0;

// ペグを三角形状に配置(1段目1個→5段目5個 / 間隔: 横48 縦50)
static void SetupPegs()
{
    pegNum = 0;
    for (int r = 0; r < 5; r++) {
        int n = r + 1;
        for (int i = 0; i < n; i++) {
            pegX[pegNum] = 240.0 + (i - (n - 1) / 2.0) * 48.0;
            pegY[pegNum] = 100.0 + r * 50.0;
            pegNum++;
        }
    }
}

// ショットセット生成ヘルパー
static sEnemyShotSet* CreateShotSet(void (*func)(sEnemyShotSet*), double x, double y)
{
    sEnemyShotSet* p = new sEnemyShotSet;
    p->count = 0;
    p->patternFunc = func;
    p->x = x;
    p->y = y;
    p->muki = 0.0;
    p->kind = 0;
    p->pEnemyShotHead = new sEnemyShot;
    p->pEnemyShotHead->prev = p->pEnemyShotHead;
    p->pEnemyShotHead->next = p->pEnemyShotHead;
    p->prev = enemyShotSetHead.prev;
    p->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = p;
    enemyShotSetHead.prev = p;
    return p;
}

//--------------------------------------------------------------
// ペグ(障害物弾): 速度0でほぼ固定、わずかに振動する
//--------------------------------------------------------------
static void ShotGaltonPeg(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        SetupPegs();
        for (int i = 0; i < pegNum; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pegX[i];
            pEnemyShot->y = pegY[i];
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白の中玉
            pEnemyShot->param_i[0] = i;        // ペグ番号
            pEnemyShot->param_d[0] = pegX[i];  // 振動の中心x
            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ペグ全体がゆっくり振動(終盤は振幅アップ)
    double amp = (enemy.hp <= 60) ? 8.0 : 3.0;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0]
            + sin(pEnemyShotSet->count * 0.03 + pShot->param_i[0] * 1.3) * amp;
        pegX[pShot->param_i[0]] = pShot->x; // 弾側の判定用配列へ反映
        pShot = pShot->next;
    }
}

//--------------------------------------------------------------
// 落下弾: ペグに当たるたび左右へ反射する(ゴルトンボードのビー玉)
//--------------------------------------------------------------
static void ShotGaltonBall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 敵HPに応じて落下間隔が短くなる(=正規分布が速く完成する)
    int interval;
    if (enemy.hp > 150) interval = 14;  // 序盤: ぽつぽつ
    else if (enemy.hp > 100)  interval = 6;   // 中盤: 雨
    else                     interval = 4;   // 終盤: 激流

    // 弾の発射(セットが空になると消されるため、最初の1発は即座に撃つ)
    if (pEnemyShotSet->count % interval == 0) {
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = 240.0 + GetRand(4) - 2;  // 最上段ペグの真上から
        pEnemyShot->y = 40.0;
        pEnemyShot->muki = DX_PI / 2.0;          // 真下へ
        pEnemyShot->speed = 2.0;
        pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙の小玉
        pEnemyShot->param_i[0] = 0;              // 反射直後の再ヒット防止カウンタ
        pEnemyShot->param_d[0] = 0.0;
        // リストへ追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 反射直後は数フレーム判定をスキップ
        if (pShot->param_i[0] > 0) pShot->param_i[0]--;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot->y += pShot->param_d[0];
        pShot->param_d[0] += 0.08;

        // 画面下に近づくと減速して「堆積」する(やがて画面外へ流れて消える)
        if (pShot->y > 430.0) {
            pShot->speed = 0.2;
            pShot->param_d[0] = 0.0;
        }

        // ペグとの当たり判定(最下段より下では不要)
        if (pShot->param_i[0] == 0 && pShot->y < 360.0) {
            for (int i = 0; i < pegNum; i++) {
                double dx = pShot->x - pegX[i];
                double dy = pShot->y - pegY[i];
                if (dx * dx + dy * dy < PEG_HIT_R * PEG_HIT_R) {
                    // ペグのどちら側へ流れるか + ランダムな反射角(22〜34度)
                    // GetRand(12) は 0〜12 を返すので角度は 22〜34 になる
                    double a = (22 + GetRand(12)) / 180.0 * DX_PI;
                    if (dx < 0) pShot->muki = DX_PI / 2.0 + a; // 左下へ
                    else        pShot->muki = DX_PI / 2.0 - a; // 右下へ
                    // ペグの横へ押し出して再ヒットを防ぐ
                   // pShot->x = pegX[i] + ((dx < 0) ? -PEG_HIT_R : PEG_HIT_R);
                   // pShot->y = pegY[i];
                    pShot->param_i[0] = 8;
                    pShot->param_d[0] = 0.0;
                    break;
                }
            }
        }
        pShot = pShot->next;
    }
}

//--------------------------------------------------------------
// 終盤攻撃: ペグ列の隙間を横走りする弾(両裾への避難を封じる)
//--------------------------------------------------------------
static void ShotGaltonSweep(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 左右同時に発射(弾の生存時間 > 発射間隔 なのでセットは空にならない)
    if (pEnemyShotSet->count % 100 == 0) {
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
        for (int dir = 0; dir < 2; dir++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = (dir == 0) ? 10.0 : 470.0;
            pEnemyShot->y = 275.0;                     // 4段目と5段目の間
            pEnemyShot->muki = (dir == 0) ? 0.0 : DX_PI;
            pEnemyShot->speed = 1.4;
            pEnemyShot->kind = img_enemyShotDiamond[1]; // 黄の菱形弾
            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

//--------------------------------------------------------------
// 敵本体のパターン
//--------------------------------------------------------------
void EnemyPat_GaltonBoard_Zai()
{
    static int sweepCreated;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 30.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        sweepCreated = 0;
    }
    else {
        // 本体はほぼ固定でわずかに浮遊
        enemy.y = 30.0 + sin(count * 0.05) * 4.0;
    }

    // 開幕: 予告音とともにペグ設置
    if (count == 30) {
        CreateShotSet(ShotGaltonPeg, 240.0, 0.0);
    }
    // 落下弾開始
    if (count == 90) {
        CreateShotSet(ShotGaltonBall, 240.0, 40.0);
    }
    // 終盤: 横薙ぎ弾を解禁(1回だけ)
    if (enemy.hp <= 100 && sweepCreated == 0) {
        sweepCreated = 1;
        CreateShotSet(ShotGaltonSweep, 0.0, 275.0);
    }
}