// enemyPat_Tmp.cpp
// 桜と彗星をモチーフにした斑鳩のような弾幕実装

// ============================================================
//  弾幕パターン関数
// ============================================================

static void ShotDummy(sEnemyShotSet* pEnemyShotSet) {

}

// 桜モチーフ：マゼンタの渦巻き弾幕
static void ShotSakura(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % 15 == 1) {
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 渦巻き状に展開
            double base_angle = pEnemyShotSet->muki + (i * (DX_PI * 2.0 / 5.0));
            double spiral_offset = pEnemyShotSet->count * 0.08; // 徐々に回転

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_angle + spiral_offset;
            pEnemyShot->speed = 2.0; // 遅めで隙間を作りやすい速度

            // 小玉(2.5x2.5) または 鱗弾(4.0x3.0) を使用
            if (pEnemyShotSet->count % 30 == 1) {
                pEnemyShot->kind = img_enemyShotScale[5];
                pEnemyShot->param_d[2] = 2.0; // 鱗弾の長径の半分
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[5];
                pEnemyShot->param_d[2] = 1.25; // 小玉の長径の半分
            }

            pEnemyShot->param_i[1] = 5; // 色: マゼンタ

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

// 彗星モチーフ：シアンの直線・自機狙い弾幕
static void ShotSuisei(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count % 15 == 8) { // サクラとずらして発射し、重なりで密度を上げる
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 自機狙い＋少しのばらつき
            double target_angle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            double spread = (GetRand(20) - 10) / 10.0; // -1.0 ～ 1.0 ラジアンのばらつき

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = target_angle + spread + (i - 1) * 0.15;
            pEnemyShot->speed = 4.5; // 速めで直線的

            // 小玉(2.5x2.5) または 銃弾(5.0x2.0) を使用
            if (GetRand(1) == 0) {
                pEnemyShot->kind = img_enemyShotBullet[3];
                pEnemyShot->param_d[2] = 2.5; // 銃弾の長径の半分
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[3];
                pEnemyShot->param_d[2] = 1.25; // 小玉の長径の半分
            }

            pEnemyShot->param_i[1] = 3; // 色: シアン

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

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_miComet_Qwen()
{
    static int muki, muki2;
    static sEnemyShotSet* pShieldSet = nullptr; // 追従小玉管理用セット

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 120.0;
        enemy.y = 40.0;
        enemy.x2 = 360.0;
        enemy.y2 = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        muki2 = 1;
        pShieldSet = nullptr;

        // 0秒後: 予告音
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    else {
        // 敵の移動
        enemy.x += 0.98 * (double)muki;
        enemy.x2 += 0.98 * (double)muki2;
        if (enemy.x < 60) muki = 1;
        if (enemy.x > 420) muki = -1;
        if (enemy.x2 < 60) muki2 = 1;
        if (enemy.x2 > 420) muki2 = -1;

        // ========================================================
        //  小玉の切り替えと効果音制御 (3秒 = 180フレーム周期)
        // ========================================================
        int cycle = count / 180;
        int phase = count % 180;

        if (phase == 0 && count > 0) {
            // 3秒後, 6秒後, 9秒後... : 予告音
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        else if (phase == 60) {
            // 1秒後, 4秒後, 7秒後... : 小玉生成・変更と極限発射音
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            int targetColor = (cycle % 2 == 0) ? 5 : 3; // 偶数サイクル:マゼンタ(5), 奇数サイクル:シアン(3)

            // 既存の小玉を全て削除
            if (pShieldSet != nullptr) {
                sEnemyShot* pShot = pShieldSet->pEnemyShotHead->next;
                while (pShot != pShieldSet->pEnemyShotHead) {
                    sEnemyShot* next = pShot->next;
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                    pShot = next;
                }
            }
            else {
                // 初回生成時にセット自体を作成
                pShieldSet = new sEnemyShotSet;
                pShieldSet->count = 0;
                pShieldSet->patternFunc = ShotDummy;
                pShieldSet->pEnemyShotHead = new sEnemyShot;
                pShieldSet->pEnemyShotHead->prev = pShieldSet->pEnemyShotHead;
                pShieldSet->pEnemyShotHead->next = pShieldSet->pEnemyShotHead;

                pShieldSet->prev = enemyShotSetHead.prev;
                pShieldSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pShieldSet;
                enemyShotSetHead.prev = pShieldSet;
            }

            // 新しい小玉を自機周囲に8個配置
            for (int i = 0; i < 20; i++) {
                sEnemyShot* pShield = new sEnemyShot;
                pShield->param_i[0] = 99; // シールド小玉識別フラグ
                pShield->param_i[1] = targetColor; // 消去対象の色

                if (targetColor == 5) {
                    pShield->kind = img_enemyShotSmallBall[5];
                }
                else {
                    pShield->kind = img_enemyShotSmallBall[3];
                }

                pShield->margin = 999.0; // 画面外判定で消去されない
                pShield->param_d[0] = i * (DX_PI * 2.0 / 20.0); // 初期配置角度
                pShield->param_d[2] = 1.25; // 小玉の長径の半分

                pShield->prev = pShieldSet->pEnemyShotHead->prev;
                pShield->next = pShieldSet->pEnemyShotHead;
                pShieldSet->pEnemyShotHead->prev->next = pShield;
                pShieldSet->pEnemyShotHead->prev = pShield;
            }
        }

        // ========================================================
        //  シールド小玉の更新と敵弾消去処理
        // ========================================================
        if (pShieldSet != nullptr) {
            sEnemyShot* pShield = pShieldSet->pEnemyShotHead->next;

            // 1. 小玉の座標更新（自機追従＋緩やかな回転）
            while (pShield != pShieldSet->pEnemyShotHead) {
                double angle = pShield->param_d[0] + count * 0.03;
                pShield->x = player.x + cos(angle) * 30.0;
                pShield->y = player.y + sin(angle) * 30.0;
                pShield = pShield->next;
            }

            // 2. 敵弾との当たり判定と消去
            for (sEnemyShotSet* pSet = enemyShotSetHead.next; pSet != &enemyShotSetHead; pSet = pSet->next) {
                if (pSet == pShieldSet) continue; // シールドセット自体は判定除外

                sEnemyShot* pEnemy = pSet->pEnemyShotHead->next;
                while (pEnemy != pSet->pEnemyShotHead) {
                    sEnemyShot* nextEnemy = pEnemy->next;
                    bool hit = false;

                    // 小玉との当たり判定
                    sEnemyShot* pShieldCheck = pShieldSet->pEnemyShotHead->next;
                    while (pShieldCheck != pShieldSet->pEnemyShotHead) {
                        // 色が一致する場合のみ消去対象
                        if (pShieldCheck->param_i[1] == pEnemy->param_i[1]) {
                            double dx = pShieldCheck->x - pEnemy->x;
                            double dy = pShieldCheck->y - pEnemy->y;
                            double dist = sqrt(dx * dx + dy * dy);

                            // 小玉の半径(1.25) + 敵弾の長径の半分
                            if (dist < (5 + pEnemy->param_d[2])) {
                                hit = true;
                                break;
                            }
                        }
                        pShieldCheck = pShieldCheck->next;
                    }

                    if (hit) {
                        // 敵弾をリストから外して消去
                        pEnemy->prev->next = pEnemy->next;
                        pEnemy->next->prev = pEnemy->prev;
                        delete pEnemy;
                    }

                    pEnemy = nextEnemy;
                }
            }
        }

        // ========================================================
        //  弾幕発射制御
        // ========================================================
        // 常に両方の弾幕を発射することで「単独なら避けられるが、同時だと回避不可能」な密度を維持する
        // （小玉が片方を消去してくれるおかげで、プレイヤーは実質的に単色の弾幕として対処できる）
        if (count % 120 == 2) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            // ボス1（桜・マゼンタ）
            sEnemyShotSet* pSetM = new sEnemyShotSet;
            pSetM->count = 0;
            pSetM->patternFunc = ShotSakura;
            pSetM->x = true ? enemy.x : enemy.x2;
            pSetM->y = (true ? enemy.y : enemy.y2) + 10.0;
            pSetM->muki = DX_PI / 2.0; // 下向きを基準に展開

            pSetM->pEnemyShotHead = new sEnemyShot;
            pSetM->pEnemyShotHead->prev = pSetM->pEnemyShotHead;
            pSetM->pEnemyShotHead->next = pSetM->pEnemyShotHead;

            pSetM->prev = enemyShotSetHead.prev;
            pSetM->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSetM;
            enemyShotSetHead.prev = pSetM;

            // ボス2（彗星・シアン）
            sEnemyShotSet* pSetS = new sEnemyShotSet;
            pSetS->count = 0;
            pSetS->patternFunc = ShotSuisei;
            pSetS->x = true ? enemy.x2 : enemy.x; // 位置を交互にして変化をつける
            pSetS->y = (true ? enemy.y2 : enemy.y) + 10.0;
            pSetS->muki = DX_PI / 2.0;

            pSetS->pEnemyShotHead = new sEnemyShot;
            pSetS->pEnemyShotHead->prev = pSetS->pEnemyShotHead;
            pSetS->pEnemyShotHead->next = pSetS->pEnemyShotHead;

            pSetS->prev = enemyShotSetHead.prev;
            pSetS->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSetS;
            enemyShotSetHead.prev = pSetS;
        }
    }
}