// enemyPat_qrcodeScan.cpp
//
// 弾幕：QRコード・スキャン
//  - 21x21 の QR コード風モジュールを白い中玉で形成する
//  - 三隅のファインダパターン、タイミングパターン、擬似乱数データ領域で
//    QR コードらしさを出す
//  - スキャンラインが上から下へ通過した行の弾を順次発射する
//
// 敵本体の関数名は仕様どおり EnemyPat_QRCode_DeepSeek() にしています。
// count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
// 画面外の弾の自動消去はメインルーチン側で行われる前提です。
//
// このファイル単体で完結させるため、enemyPat_sampleForAI.cpp で
// 使われていた素材（画像ハンドル・サウンド・GetRand・DX_PI 等）は
// すべて extern 宣言しています。プロジェクト側の共通ヘッダで既に
// 宣言済みの場合は重複 extern になるだけで害はありません。

// ============================================================
//  定数
// ============================================================
static const int    QR_GRID = 21;      // 21x21 モジュール
static const double QR_CELL = 22.0;    // 1 セルの大きさ（px）
// 中玉(7x7)を使うので 7px の隙間が空き、プレイヤーが縫える

// ============================================================
//  QR コードのモジュール配置
//  戻り値: true = 黒モジュール（弾を置く） / false = 白モジュール（空き）
// ============================================================
static int A, B;
static bool QR_IsBlack(int row, int col)
{
    const int G = QR_GRID;

    // --- ファインダパターン（三隅 7x7） ---
    auto inFinder = [](int r, int c) -> bool {
        if (r < 0 || r > 6 || c < 0 || c > 6) return false;
        if (r == 0 || r == 6 || c == 0 || c == 6) return true;  // 外周リング
        if (r == 1 || r == 5 || c == 1 || c == 5) return false; // 白リング
        return true;                                            // 中心 3x3
    };
    if (inFinder(row, col))                    return true;  // 左上
    if (inFinder(row, col - (G - 7)))          return true;  // 右上
    if (inFinder(row - (G - 7), col))          return true;  // 左下

    // --- セパレータ（ファインダ周囲の白） ---
    if ((row == 7 && col <= 7) || (col == 7 && row <= 7))         return false;
    if ((row == 7 && col >= G - 8) || (col == G - 8 && row <= 7))         return false;
    if ((row == G - 8 && col <= 7) || (col == 7 && row >= G - 8))     return false;

    // --- タイミングパターン（6 行目・6 列目） ---
    if (row == 6 && col >= 8 && col <= G - 9) return (col % 2 == 0);
    if (col == 6 && row >= 8 && row <= G - 9) return (row % 2 == 0);

    // --- データ領域（決定的なハッシュで埋める） ---
    unsigned h = (unsigned)(row * A + col * B) + 0x9E3779B9u;
    h ^= h >> 16; h *= 0x85EBCA6Bu; h ^= h >> 13;
    h *= 0xC2B2AE35u; h ^= h >> 16;
    return (h & 1u) != 0;
}

// ============================================================
//  弾幕：QRコード・スキャン
// ============================================================
static void ShotQRCodeScan(sEnemyShotSet* pEnemyShotSet)
{
    const double ORIGIN_X = pEnemyShotSet->x - (QR_GRID * QR_CELL) * 0.5;
    const double ORIGIN_Y = pEnemyShotSet->y;

    const int SPAWN_END = QR_GRID;               // 1 フレーム 1 行ずつ生成
    const int HOLD_END = SPAWN_END + 150;        // 完成後しばらくホールド
    const int SCAN_STEP = 4;                     // 4 フレームごとに 1 行スキャン
    const int SCAN_END = HOLD_END + QR_GRID * SCAN_STEP;

    // ---- 予告音 ----
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    // ---- スキャン開始音 ----
    if (pEnemyShotSet->count == HOLD_END) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // --------------------------------------------------------
    //  生成フェーズ：1 フレーム 1 行ずつ QR コードを組み上げる
    // --------------------------------------------------------
    if (pEnemyShotSet->count < SPAWN_END) {
        int row = pEnemyShotSet->count;
        for (int col = 0; col < QR_GRID; col++) {
            if (!QR_IsBlack(row, col)) continue;

            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->param_d[0];   // 敵の位置から発射
            p->y = pEnemyShotSet->param_d[1];
            p->muki = 0.0;
            p->speed = 0.0;
            p->kind = img_enemyShotMediumBall[6];  // 色 6 = 白

            p->param_d[0] = ORIGIN_X + col * QR_CELL + QR_CELL * 0.5; // 目標X
            p->param_d[1] = ORIGIN_Y + row * QR_CELL + QR_CELL * 0.5; // 目標Y
            p->param_i[0] = 0;  // 0:移動中 / 1:待機 / 2:発射済み

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // --------------------------------------------------------
    //  スキャンフェーズ：通過した行の弾を下方向へ発射
    // --------------------------------------------------------
    if (pEnemyShotSet->count >= HOLD_END && pEnemyShotSet->count < SCAN_END) {
        int elapsed = pEnemyShotSet->count - HOLD_END;
        if (elapsed % SCAN_STEP == 0) {
            int scanRow = elapsed / SCAN_STEP;

            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            sEnemyShot* s = pEnemyShotSet->pEnemyShotHead->next;
            while (s != pEnemyShotSet->pEnemyShotHead) {
                if (s->param_i[0] == 1) {
                    double rel = s->param_d[1] - ORIGIN_Y;
                    int r = (int)(rel / QR_CELL);
                    if (r == scanRow) {
                        s->param_i[0] = 2;                     // 発射状態へ
                        s->speed = 3.0 + GetRand(150) / 100.0; // 3.0～4.5
                        // 下方向 ±30 度の範囲でばら撒く
                        s->muki = DX_PI * 0.5 + (GetRand(30) - 15) / 180.0 * DX_PI;
                    }
                }
                s = s->next;
            }
        }
    }

    // --------------------------------------------------------
    //  各弾の更新
    // --------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 目標グリッド位置へ移動
            double dx = pShot->param_d[0] - pShot->x;
            double dy = pShot->param_d[1] - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            const double spd = 8.0;
            if (dist <= spd) {
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];
                pShot->param_i[0] = 1;  // 到着 → 待機
            }
            else {
                pShot->x += spd * dx / dist;
                pShot->y += spd * dy / dist;
            }
        }
        else if (pShot->param_i[0] == 2) {
            // 発射済み：直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_QRCode_DeepSeek()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200 で固定
        muki = 1;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 最初は count==30、以降 600 フレームごとに QR コード弾幕を生成
    if (count == 30 || (count > 30 && (count - 30) % 300 == 0)) {
        A = GetRand(10000);
        B = GetRand(10000);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotQRCodeScan;
        pEnemyShotSet->x = 240.0;  // QR グリッド中心 X（画面中央）
        pEnemyShotSet->y = 30.0;   // QR グリッド上端 Y
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // 弾の発射起点（敵の現在位置）
        pEnemyShotSet->param_d[0] = enemy.x;
        pEnemyShotSet->param_d[1] = enemy.y + 10.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}