// EnemyPat_miComet_Gemini.cpp

#include <cmath>

// 桜（マゼンタ）をモチーフにした弾幕
static void SakuraPattern(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 花びらが舞い散るように全方位へ鱗弾をばらまく
        for (int i = 0; i < 24; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->kind = img_enemyShotScale[5];
            pShot->param_d[0] = 4.0; // 楕円の長径 rx (鱗弾 4.0x3.0)
            pShot->param_d[1] = 3.0; // 楕円の短径 ry
            pShot->param_i[1] = 5;   // 属性色 (5: マゼンタ)

            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = (GetRand(360) / 180.0) * DX_PI;
            pShot->speed = (100 + GetRand(150)) / 100.0; // 1.0 ～ 2.5
            pShot->param_d[2] = (GetRand(100) - 50) / 4000.0; // わずかなカーブ(角速度)

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->muki += p->param_d[2];
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// 彗星（シアン）をモチーフにした弾幕
static void CometPattern(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double baseAngle = atan2(player.y - pSet->y, player.x - pSet->x);

        // 彗星の尾のような高速直進レーザー
        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->kind = img_enemyShotLaser[3];
            pShot->param_d[0] = 64.0; // 楕円の長径 rx (短レーザー 64.0x4.0)
            pShot->param_d[1] = 4.0;  // 楕円の短径 ry
            pShot->param_i[1] = 3;    // 属性色 (3: シアン)

            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = baseAngle + (i * 12.0) * DX_PI / 180.0;
            pShot->speed = 6.0 + GetRand(15) / 10.0; // 6.0 ～ 7.5

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // 周囲に散らばる星屑（中楕円弾）
        for (int i = 0; i < 12; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->kind = img_enemyShotMediumOval[3];
            pShot->param_d[0] = 10.5; // 楕円の長径 rx (中楕円弾 10.5x7.0)
            pShot->param_d[1] = 7.0;  // 楕円の短径 ry
            pShot->param_i[1] = 3;    // 属性色 (3: シアン)

            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = baseAngle + (GetRand(60) - 30) * DX_PI / 180.0;
            pShot->speed = 3.0 + GetRand(25) / 10.0; // 3.0 ～ 5.5

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// 自機追従バリア（同色の敵弾を消滅させる）
static void BarrierPattern(sEnemyShotSet* pSet)
{
    // 180フレーム(3秒)ごとにバリアの色を反転
    if (pSet->count > 0 && pSet->count % 180 == 0) {
        if (pSet->param_i[0] == 5) {
            pSet->param_i[0] = 3; // シアンに変化
        }
        else {
            pSet->param_i[0] = 5; // マゼンタに変化
        }

        // 弾の画像も更新
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            if (pSet->param_i[0] == 5) {
                p->kind = img_enemyShotSmallBall[5];
            }
            else {
                p->kind = img_enemyShotSmallBall[3];
            }
            p = p->next;
        }
    }

    // 自機への追従と回転処理
    double radius = 35.0; // 自機を取り囲む円の半径
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->param_d[2] += 0.03; // 回転角速度
        p->x = player.x + radius * cos(p->param_d[2]);
        p->y = player.y + radius * sin(p->param_d[2]);
        p = p->next;
    }

    // 他の敵弾との当たり判定と消去処理
    // enemyShotSetHead 全体を走査し、バリア色と同じ色の弾を消滅させる
    sEnemyShotSet* curSet = enemyShotSetHead.next;
    while (curSet != &enemyShotSetHead) {
        if (curSet != pSet) { // バリア自身は判定から除外
            sEnemyShot* eShot = curSet->pEnemyShotHead->next;
            while (eShot != curSet->pEnemyShotHead) {
                // eShot が現在のバリア色と同じ属性かチェック
                if (eShot->param_i[1] == pSet->param_i[0]) {
                    bool isHit = false;
                    sEnemyShot* bShot = pSet->pEnemyShotHead->next;

                    while (bShot != pSet->pEnemyShotHead) {
                        double dx = bShot->x - eShot->x;
                        double dy = bShot->y - eShot->y;

                        // パフォーマンスのため、距離の二乗でざっくりと枝刈り (64 + 2.5 の二乗より少し大きめ)
                        if (dx * dx + dy * dy <= 4500.0) {
                            double muki = eShot->muki;
                            double rx = eShot->param_d[0];
                            double ry = eShot->param_d[1];
                            double br = 2.5 * 2; // バリア小玉の半径

                            // 敵弾の向き(-muki)に合わせて相対座標を回転
                            double rot_x = dx * cos(-muki) - dy * sin(-muki);
                            double rot_y = dx * sin(-muki) + dy * cos(-muki);

                            // 楕円と円の当たり判定（ミンコフスキー和の近似）
                            if ((rot_x * rot_x) / ((rx + br) * (rx + br)) + (rot_y * rot_y) / ((ry + br) * (ry + br)) <= 1.0) {
                                isHit = true;
                                break;
                            }
                        }
                        bShot = bShot->next;
                    }

                    // 衝突していたら敵弾を消滅（リストから外してdelete）
                    if (isHit) {
                        sEnemyShot* delShot = eShot;
                        eShot = eShot->next;
                        delShot->prev->next = delShot->next;
                        delShot->next->prev = delShot->prev;
                        delete delShot;
                        continue;
                    }
                }
                eShot = eShot->next;
            }
        }
        curSet = curSet->next;
    }
}

// 敵本体のパターン
void EnemyPat_miComet_Gemini()
{
    // 0秒後（初期化）
    if (count == 1) {
        enemy.x = 120.0;
        enemy.y = 80.0;
        enemy.x2 = 360.0; // ボス2
        enemy.y2 = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 敵の移動処理（正弦波による滑らかな加減速運動）
    if (count > 0) {
        // ボス1（桜）：(120, 80) を中心にゆったりと8の字を描く
        enemy.x = 120.0 + 50.0 * sin(count * 0.02);
        enemy.y =  80.0 + 20.0 * sin(count * 0.04);

        // ボス2（彗星）：(360, 80) を中心に左右対称の波状軌道を描く
        enemy.x2 = 360.0 - 50.0 * sin(count * 0.02);
        enemy.y2 =  80.0 + 20.0 * cos(count * 0.04);
    }

    // ----------------------------------------------------
    // イベントタイマー (1秒 = 60フレーム換算)
    // ----------------------------------------------------

    // 0秒後, 3秒後, 6秒後... (周期180F) にチャージ音
    if (count % 180 == 1) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 1秒後, 4秒後, 7秒後... (周期180F) に極音
    if (count > 1 && count % 180 == 61) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 1秒後 (count==61) に自機追従バリアを発生
    if (count == 61) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = BarrierPattern;
        pSet->param_i[0] = 5; // 初期カラーフラグ (5:マゼンタ)

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // 自機を取り囲むように16個の小玉を配置
        for (int i = 0; i < 16; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->kind = img_enemyShotSmallBall[5];
            pShot->margin = 999.0; // 画面外に出ても消えない
            pShot->param_d[2] = i * (DX_PI * 2.0 / 16); // 初期角度

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ----------------------------------------------------
    // 弾幕の発生
    // ----------------------------------------------------
    if (count >= 61) {
        // ボス1(マゼンタ/桜) 30フレーム周期で発射
        if ((count - 61) % 30 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = SakuraPattern;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }

        // ボス2(シアン/彗星) 40フレーム周期で発射
        if ((count - 61) % 40 == 0) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = CometPattern;
            pSet->x = enemy.x2;
            pSet->y = enemy.y2;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}