// ----------------------------------------------------
// ゴルトンボード共有データ
// ----------------------------------------------------
static double g_nailX[128];
static double g_nailY[128];
static int g_nailCount = 0;

// 釘は静止弾。見た目だけ
static void ShotGaltonNail(sEnemyShotSet* pEnemyShotSet)
{
    // 釘自体は動かさない。必要なら点滅など
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot = pShot->next;
    }
}

// 落下玉 - ゴルトンボード本体
static void ShotGaltonBall(sEnemyShotSet* pEnemyShotSet)
{
    // 7F毎に1発生成。GetRand(x)は0〜xのx+1通り
    if (pEnemyShotSet->count % 2 == 0) {
        if (pEnemyShotSet->count % 8 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        sEnemyShot* pNew = new sEnemyShot;
        // 敵本体の真下から少しランダムに
        pNew->x = enemy.x + (double)(GetRand(10) - 5);
        pNew->y = enemy.y + 10.0;
        pNew->muki = DX_PI * 0.5 + (GetRand(20) - 10) / 180.0 * DX_PI; // 真下±10度
        pNew->speed = 1.8 + GetRand(40) / 100.0; // 1.8〜2.2
        pNew->kind = img_enemyShotMediumBall[6]; // 白中玉
        pNew->param_i[0] = -1; // 最後に当たった釘index
        pNew->param_i[1] = -100; // 最後に当たった時のcount
        pNew->param_i[2] = 0; // 0:落下中 1:変身後
        pNew->param_d[0] = 0.0; // 重力用

        pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pNew->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
        pEnemyShotSet->pEnemyShotHead->prev = pNew;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[2] == 0) {
            // 落下中
            pShot->speed += 0.015; // 重力

            // 釘との衝突判定
            for (int i = 0; i < g_nailCount; ++i) {
                double dx = pShot->x - g_nailX[i];
                double dy = pShot->y - g_nailY[i];
                double dist2 = dx * dx + dy * dy;
                // 半径12以内でヒット、同一釘連打防止に8Fクールダウン
                if (dist2 < 80.0) {
                    if (pShot->param_i[0] != i && pShot->count > pShot->param_i[1] + 8) {
                        pShot->param_i[0] = i;
                        pShot->param_i[1] = pShot->count;
                        int dir = (GetRand(1) == 0) ? -1 : 1; // 50%で左右
                        double spread = (35 + GetRand(10)) / 180.0 * DX_PI; // 35〜45度
                        pShot->muki = DX_PI * 0.5 + dir * spread;
                        pShot->speed = 2.2 + GetRand(30) / 100.0;
                        break;
                    }
                }
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 回収ライン y=360
            if (pShot->y > 360.0) {
                if (pShot->x >= 140.0 && pShot->x <= 340.0) {
                    // 中央60%に落ちた -> 自機狙い3wayの核に
                    pShot->param_i[2] = 1;
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->speed = 1.8 + GetRand(30) / 100.0;
                    pShot->kind = img_enemyShotMediumBall[4]; // 青中玉
                }
                else {
                    // 端 -> 高速レーザーの壁
                    pShot->param_i[2] = 1;
                    pShot->muki = DX_PI * 0.5; // 真下
                    pShot->speed = 3.0 + GetRand(40) / 100.0;
                    pShot->kind = img_enemyShotLaser[8]; // 橙レーザー
                }
            }
        }
        else {
            // 変身後
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ----------------------------------------------------
// 敵本体
// ----------------------------------------------------
void EnemyPat_GaltonBoard_MetaAI()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        g_nailCount = 0;

        // 1. 釘盤面生成
        sEnemyShotSet* pNailSet = new sEnemyShotSet;
        pNailSet->count = 0;
        pNailSet->patternFunc = ShotGaltonNail;
        pNailSet->x = 240.0;
        pNailSet->y = 90.0;
        pNailSet->muki = 0.0;
        pNailSet->kind = 0;
        pNailSet->pEnemyShotHead = new sEnemyShot;
        pNailSet->pEnemyShotHead->prev = pNailSet->pEnemyShotHead;
        pNailSet->pEnemyShotHead->next = pNailSet->pEnemyShotHead;

        const int ROWS = 11;
        for (int r = 0; r < ROWS; ++r) {
            double y = 90.0 + r * 26.0;
            for (int c = 0; c <= r; ++c) {
                double x = 240.0 + (c - r * 0.5) * 28.0;
                if (g_nailCount < 128) {
                    g_nailX[g_nailCount] = x;
                    g_nailY[g_nailCount] = y;
                    g_nailCount++;
                }
                sEnemyShot* pNail = new sEnemyShot;
                pNail->x = x;
                pNail->y = y;
                pNail->muki = 0.0;
                pNail->speed = 0.0;
                pNail->kind = img_enemyShotSmallBall[6]; // 白小玉を釘として
                pNail->prev = pNailSet->pEnemyShotHead->prev;
                pNail->next = pNailSet->pEnemyShotHead;
                pNailSet->pEnemyShotHead->prev->next = pNail;
                pNailSet->pEnemyShotHead->prev = pNail;
            }
        }
        pNailSet->prev = enemyShotSetHead.prev;
        pNailSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pNailSet;
        enemyShotSetHead.prev = pNailSet;

        // 2. 落下玉スポナー生成
        sEnemyShotSet* pBallSet = new sEnemyShotSet;
        pBallSet->count = 0;
        pBallSet->patternFunc = ShotGaltonBall;
        pBallSet->x = enemy.x;
        pBallSet->y = enemy.y;
        pBallSet->muki = DX_PI * 0.5;
        pBallSet->kind = 0;
        pBallSet->pEnemyShotHead = new sEnemyShot;
        pBallSet->pEnemyShotHead->prev = pBallSet->pEnemyShotHead;
        pBallSet->pEnemyShotHead->next = pBallSet->pEnemyShotHead;

        pBallSet->prev = enemyShotSetHead.prev;
        pBallSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBallSet;
        enemyShotSetHead.prev = pBallSet;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 敵を左右に揺らして落下位置をずらす
        enemy.x = 240.0 + sin(count * 0.015) * 40.0;
    }
}