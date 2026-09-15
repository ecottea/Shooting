// enemyPat_QR.cpp
// QRコードをモチーフにした弾幕パターン

#include "gv.h" // プロジェクトの構成に合わせてパスを調整してください

// 弾幕：QRスキャン・バレット
static void ShotQRCode(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 効果音：重厚な発射音でQRコードの展開を演出
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int N = 12;          // グリッドサイズ (12x12)
        const double gap = 24.0;   // セル間隔
        const double startX = 240.0 - (N * gap) / 2.0 + gap / 2.0;
        const double startY = 80.0;

        // 安全な列（正解ルート）をランダムに決定 (0 から N-1)
        int safe_col = GetRand(N - 1);
        pEnemyShotSet->param_i[0] = safe_col; // 安全列を保存
        pEnemyShotSet->param_i[1] = -1;       // 現在スキャン済みの最大行
        pEnemyShotSet->param_i[2] = -2;       // 前回スキャンした行（色変更トリガー用）

        // グリッドの弾を生成
        for (int r = 0; r < N; r++) {
            for (int c = 0; c < N; c++) {
                bool is_black = false;

                // コーナーマーカー（定位パターン）の判定
                bool is_top_left = (r < 4 && c < 4);
                bool is_top_right = (r < 4 && c >= N - 4);
                bool is_bottom_left = (r >= N - 4 && c < 4);

                if (is_top_left) {
                    if ((r == 1 || r == 2) && (c == 1 || c == 2)) is_black = true;
                }
                else if (is_top_right) {
                    if ((r == 1 || r == 2) && (c == N - 3 || c == N - 2)) is_black = true;
                }
                else if (is_bottom_left) {
                    if ((r == N - 3 || r == N - 2) && (c == 1 || c == 2)) is_black = true;
                }
                else {
                    // データ領域：約45%の確率で黒モジュールを配置
                    if (GetRand(10) < 5) {
                        is_black = true;
                    }
                }

                if (is_black) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = startX + c * gap;
                    pEnemyShot->y = startY + r * gap;
                    pEnemyShot->kind = img_enemyShotMediumBall[7]; // 中玉・黒
                    pEnemyShot->speed = 0.0;
                    pEnemyShot->muki = 0.0;
                    pEnemyShot->param_i[0] = r; // 行番号
                    pEnemyShot->param_i[1] = c; // 列番号

                    // 双方向リストに追加
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }

        // スキャンライン用の弾を1つ生成
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = 240.0;
        pEnemyShot->y = startY - 30.0;
        pEnemyShot->kind = img_enemyShotLaser[3]; // 短レーザー・シアン
        pEnemyShot->speed = 3.5;
        pEnemyShot->muki = DX_PI / 2.0; // 真下
        pEnemyShot->param_i[0] = 99;    // スキャンライン識別用フラグ

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }
    else {
        // スキャンラインの位置に基づいて、行ごとのスキャン完了を判定
        const double startY = 80.0;
        const double gap = 24.0;

        sEnemyShot* pScanLine = nullptr;

        // リストを走査して処理
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 99) {
                // スキャンラインの移動
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
                pScanLine = pShot;
            }
            pShot = pShot->next;
        }

        // スキャンラインが存在し、画面内にある場合
        if (pScanLine != nullptr) {
            int current_scan_line = (int)((pScanLine->y - startY) / gap);

            // 新しい行をスキャンした場合
            if (current_scan_line > pEnemyShotSet->param_i[1]) {
                pEnemyShotSet->param_i[1] = current_scan_line;

                // スキャンされた行の弾を赤くハイライトする演出
                int target_r = pEnemyShotSet->param_i[1];
                pShot = pEnemyShotSet->pEnemyShotHead->next;
                while (pShot != pEnemyShotSet->pEnemyShotHead) {
                    if (pShot->param_i[0] == target_r && pShot->param_i[0] != 99) {
                        pShot->kind = img_enemyShotMediumBall[0]; // 中玉・赤に変更
                    }
                    pShot = pShot->next;
                }
            }
        }

        // 発射フェーズ（スキャン開始から120フレーム後）
        if (pEnemyShotSet->count == 120) {
            // 効果音：発射音
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            int safe_col = pEnemyShotSet->param_i[0];

            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[0] != 99) {
                    int c = pShot->param_i[1];
                    // 安全列でない場合のみ発射
                    if (c != safe_col) {
                        pShot->kind = img_enemyShotMediumBall[7]; // 黒に戻す
                        pShot->muki = DX_PI / 2.0;                // 真下へ発射
                        pShot->speed = 3.0 + GetRand(100) / 100.0; // 速度に少しばらつきを持たせる
                    }
                }
                pShot = pShot->next;
            }
        }

        // 発射後の弾の移動処理
        if (pEnemyShotSet->count > 120) {
            pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[0] != 99) {
                    // 速度が0より大きい（＝発射された）弾のみ移動
                    if (pShot->speed > 0.0) {
                        pShot->x += pShot->speed * cos(pShot->muki);
                        pShot->y += pShot->speed * sin(pShot->muki);
                    }
                }
                pShot = pShot->next;
            }
        }
    }
}

// 敵本体のパターン
void EnemyPat_QRCode_Qwen()
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
        // 敵の移動：左右にゆっくり往復
        enemy.x += 0.8 * (double)muki;
        if (enemy.x > 400.0 || enemy.x < 80.0) {
            muki *= -1;
        }
    }

    // 300フレーム（約5秒）ごとにQRコード弾幕を展開
    if (count % 300 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotQRCode;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->kind = shot_count++;

        // 弾リストのヘッダ初期化
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