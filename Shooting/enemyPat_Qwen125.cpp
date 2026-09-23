// 弾幕：南京玉すだれ・変幻の舞
static void ShotNanKinTamaSudare(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    // 初期化処理 (count == 0 のとき)
    if (pEnemyShotSet->count == 0) {
        // 予告音または軽い効果音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 親玉の生成 (1個)
        sEnemyShot* pParent = new sEnemyShot;
        pParent->x = pEnemyShotSet->x;
        pParent->y = pEnemyShotSet->y;
        pParent->muki = pEnemyShotSet->muki;
        pParent->speed = 3.0;
        pParent->kind = img_enemyShotMediumBall[0]; // 中玉・赤
        pParent->param_d[0] = 0.0; // 0.0 = 親玉フラグ
        pParent->margin = 80;

        pParent->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pParent->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pParent;
        pEnemyShotSet->pEnemyShotHead->prev = pParent;

        // 子玉と紐の生成 (5個)
        for (int i = 0; i < 5; i++) {
            // 子玉
            sEnemyShot* pChild = new sEnemyShot;
            pChild->x = pEnemyShotSet->x;
            pChild->y = pEnemyShotSet->y;
            pChild->muki = pEnemyShotSet->muki;
            pChild->speed = 0.0; // 位置は補間で制御するため初期速度は0
            pChild->kind = img_enemyShotSmallBall[6]; // 小玉・白
            pChild->param_d[0] = 1.0; // 1.0 = 子玉フラグ
            pChild->param_d[1] = (double)i; // 子玉のインデックス (0~4)
            pChild->margin = 80;

            pChild->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pChild->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pChild;
            pEnemyShotSet->pEnemyShotHead->prev = pChild;

            // 紐 (短レーザーで表現)
            sEnemyShot* pString = new sEnemyShot;
            pString->x = pEnemyShotSet->x;
            pString->y = pEnemyShotSet->y;
            pString->muki = pEnemyShotSet->muki;
            pString->speed = 0.0;
            pString->kind = img_enemyShotLaser[6]; // 短レーザー・白
            pString->param_d[0] = 2.0; // 2.0 = 紐フラグ
            pString->param_d[1] = (double)i; // 対応する子玉のインデックス
            pString->margin = 80;

            pString->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pString->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pString;
            pEnemyShotSet->pEnemyShotHead->prev = pString;
        }
    }

    // 親玉の参照を取得 (ヘッドの次が必ず親玉になるように生成しているため)
    sEnemyShot* pParentShot = pEnemyShotSet->pEnemyShotHead->next;

    // 親玉の移動処理 (直進)
    if (pParentShot->param_d[0] == 0.0) {
        // フェーズに応じて速度変化
        if (pEnemyShotSet->count < 60) {
            pParentShot->speed = 4.5-1; // フェーズ0: 収縮(高速)
        }
        else if (pEnemyShotSet->count < 120) {
            pParentShot->speed = 1.0/2; // フェーズ1: 展開(低速)
        }
        else if (pEnemyShotSet->count < 180) {
            pParentShot->speed = 2.0/2; // フェーズ2: 折りたたみ(中速)
        }
        else {
            pParentShot->speed = 5.0; // フィナーレ: 高速化
        }
        pParentShot->x += pParentShot->speed * cos(pParentShot->muki);
        pParentShot->y += pParentShot->speed * sin(pParentShot->muki);
    }

    // 全弾の更新処理
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_d[0] == 1.0) { // 子玉の処理
            int idx = (int)pShot->param_d[1];
            double targetX = pParentShot->x;
            double targetY = pParentShot->y;
            double dist = 0.0;
            double angle = pParentShot->muki;

            // フェーズごとの目標位置計算
            if (pEnemyShotSet->count < 60) {
                // フェーズ0:【収縮】親玉の背後に一直線に並ぶ
                dist = 15.0;
                angle = pParentShot->muki + DX_PI;
                targetX += cos(angle) * (dist * (idx + 1));
                targetY += sin(angle) * (dist * (idx + 1));
            }
            else if (pEnemyShotSet->count < 120) {
                // フェーズ1:【展開】扇状に広がる (短レーザーの長さ64に合わせる)
                dist = 64.0;
                angle = pParentShot->muki + (idx - 2) * 0.5;
                targetX += cos(angle) * dist;
                targetY += sin(angle) * dist;
            }
            else if (pEnemyShotSet->count < 180) {
                // フェーズ2:【折りたたみ】V字に配置
                dist = 45.0;
                angle = pParentShot->muki + (idx % 2 == 0 ? 0.8 : -0.8);
                targetX += cos(angle) * dist;
                targetY += sin(angle) * dist;
            }
            else {
                // フィナーレ: ばら撒き準備 (位置はそのまま、速度と向きを後で変更)
                targetX = pShot->x;
                targetY = pShot->y;
            }

            // 現在位置から目標位置へ滑らかに補間 (しなる表現)
            pShot->x += (targetX - pShot->x) * 0.15;
            pShot->y += (targetY - pShot->y) * 0.15;
        }
        else if (pShot->param_d[0] == 2.0) { // 紐の処理
            int idx = (int)pShot->param_d[1];

            // 対応する子玉を探す
            sEnemyShot* pTargetChild = nullptr;
            sEnemyShot* pSearch = pEnemyShotSet->pEnemyShotHead->next;
            while (pSearch != pEnemyShotSet->pEnemyShotHead) {
                if (pSearch->param_d[0] == 1.0 && (int)pSearch->param_d[1] == idx) {
                    pTargetChild = pSearch;
                    break;
                }
                pSearch = pSearch->next;
            }

            if (pTargetChild != nullptr) {
                // 親玉と子玉の中間位置に配置
                pShot->x = (pParentShot->x + pTargetChild->x) / 2.0;
                pShot->y = (pParentShot->y + pTargetChild->y) / 2.0;

                // 向きを親玉と子玉を結ぶ方向にする
                pShot->muki = atan2(pTargetChild->y - pParentShot->y, pTargetChild->x - pParentShot->x);
            }
        }

        pShot = pShot->next;
    }

    // フィナーレ処理 (count == 180 のとき、1度だけ実行)
    if (pEnemyShotSet->count == 180) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 親玉と子玉のみを対象にする (紐はそのままでも良いが、速度を上げて追随させる)
            if (pShot->param_d[0] == 0.0 || pShot->param_d[0] == 1.0) {
                // 自機狙い方向に変更
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                // GetRand(20) は 0~20 なので、-10.0 ~ +10.0 の揺らぎを与える (GetRand(200)/10.0 - 10.0 と同等)
                pShot->muki += ((double)GetRand(200) / 10.0 - 10.0) * (DX_PI / 180.0);
                pShot->speed = 6.0; // 高速でばら撒く
            }
            else if (pShot->param_d[0] == 2.0) {
                pShot->speed = 6.0; // 紐も一緒に飛んでいくように見せる
            }
            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_NankinTamasudare_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵の移動処理 (画面端で折り返し)
        enemy.x += 1.5 * (double)muki;
        if (enemy.x >= 440.0) muki = -1;
        if (enemy.x <= 40.0) muki = 1;
    }

    // 120フレームごとに南京玉すだれ弾幕を発射
    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNanKinTamaSudare;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;

        // 自機狙い方向を計算
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        // 弾リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}