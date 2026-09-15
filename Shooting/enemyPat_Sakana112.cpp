// enemyPat_QRCode.cpp
//
// 弾幕：QRコード「デコード・マトリクス」
//   静止した敵弾でQRコードを"印刷"し、スキャンライン通過後に一斉解放する。
//
// ------------------------------------------------------------------
// 【選んだ素材とその理由】
//   ・中玉(7.0x7.0) / 色6:白  … QRの各モジュール(マス)。7pxの玉を20px間隔で
//                                並べることで、白背景想定でも視認でき、かつ
//                                隙間(=白マス)を安地として抜けられる。
//   ・短レーザー(64.0x4.0)/色0:赤 … スキャナの読み取り光。横64pxを画面幅
//                                480pxに8本並べて横一線のスキャンラインを構成。
//   ・効果音 sound_enemyCharge     … コード生成中の"印刷/スキャン"予告音。
//   ・効果音 sound_enemyShot_heavy … デコード成功＝一斉解放の号砲。
//   ※ 小玉/大玉/銃弾/鱗弾/菱形弾/中楕円弾は今回の表現には不要なので不採用。
// ------------------------------------------------------------------
//
// 仕様メモ：
//   ・count, pEnemyShotSet->count, pEnemyShot->count のインクリメント、
//     および画面外弾の消去はメインルーチンが行うため、ここでは行わない。
//   ・GetRand(x) は 0 以上 x 以下 (x+1 種類) の整数を返す。
//   ・ゲーム画面は 480x480。

// ---- 弾に持たせる役割フラグ（param_i[0]）----
//   0 : QRモジュール（解放前は静止）
//   2 : スキャンライン（常時下方向へ移動）
static const int SHOT_MODULE = 0;
static const int SHOT_SCAN = 2;

// ---- ショットセットの進行フェーズ（param_i[0]）----
static const int PH_GENERATE = 0; // コード生成中（行ごとに出現）
static const int PH_HOLD = 1; // 完成後の静止（読み取り猶予）
static const int PH_SCAN = 2; // スキャンライン通過中
static const int PH_RELEASE = 3; // デコード＝一斉解放

// ---- レイアウト定数 ----
static const int    QR_N = 21;     // 21x21 モジュール（QR version1 相当）
static const double QR_STEP = 20.0;   // モジュール間隔[px]
static const double QR_ORG = 40.0;   // 左上モジュール中心座標(x,y共通)
//   中心座標 = QR_ORG + index * QR_STEP  →  40 ～ 440（480画面に収まる）

// ---- 進行タイミング（ショットセットの count 基準）----
static const int GEN_INTERVAL = 3;                              // 1行あたりのフレーム数
static const int GEN_END = (QR_N - 1) * GEN_INTERVAL;      // 60：全行出現完了
static const int HOLD_END = GEN_END + 90;                   // 150：読み取り猶予終了
static const int SCAN_SPEED = 5;                              // スキャンライン速度
static const int RELEASE_AT = HOLD_END + 100;                 // 250：解放発動

// QRコードのビットマップ（1=モジュールを置く / 0=空白）
// 3隅の位置検出パターン・タイミングパターンを含む本物のQR風レイアウト。
static const char* QR_MAP[QR_N] = {
    "111111100101001111111",
    "100000101111101000001",
    "101110100101001011101",
    "101110101111101011101",
    "101110100101001011101",
    "100000101111101000001",
    "111111101010101111111",
    "000000001111100000000",
    "010101110101010101010",
    "111111011111111111111",
    "010101110101010101010",
    "111111011111111111111",
    "010101110101010101010",
    "000000001111111111111",
    "111111100101010101010",
    "100000101111111111111",
    "101110100101010101010",
    "101110101111111111111",
    "101110100101010101010",
    "100000101111111111111",
    "111111100101010101010"
};

// リストへ弾を1個追加するヘルパー
static sEnemyShot* AddShot(sEnemyShotSet* pSet)
{
    sEnemyShot* p = new sEnemyShot;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
    return p;
}

// 弾幕：QRコード デコード・マトリクス
static void ShotQRMatrix(sEnemyShotSet* pEnemyShotSet)
{
    const int  cnt = pEnemyShotSet->count;
    int& phase = pEnemyShotSet->param_i[0];

    // ============ フェーズ遷移＆弾の生成 ============
    if (phase == PH_GENERATE) {
        // 上の行から順に、QR_MAP の "1" の位置へ白の中玉を静止配置していく
        if (cnt % GEN_INTERVAL == 0) {
            int row = cnt / GEN_INTERVAL;
            if (row < QR_N) {
                if (row == 0) {
                    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
                }
                for (int col = 0; col < QR_N; col++) {
                    if (QR_MAP[row][col] != '1') continue;
                    sEnemyShot* p = AddShot(pEnemyShotSet);
                    p->x = QR_ORG + col * QR_STEP;
                    p->y = QR_ORG + row * QR_STEP;
                    p->muki = 0.0;
                    p->speed = 0.0;                     // 解放まで静止
                    p->kind = img_enemyShotMediumBall[6]; // 白の中玉＝モジュール
                    p->param_i[0] = SHOT_MODULE;
                }
            }
        }
        if (cnt >= GEN_END) phase = PH_HOLD;
    }
    else if (phase == PH_HOLD) {
        // コード完成。プレイヤーは白マス（弾の隙間）へ移動して安地を探す
        if (cnt >= HOLD_END) {
            // スキャンライン出現：画面上端(y=-10)に横一線の短レーザーを並べる
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            for (int i = 0; i < 8; i++) { // 64px * 8 = 512px で画面幅480pxを覆う
                sEnemyShot* p = AddShot(pEnemyShotSet);
                p->x = 32.0 + i * 64.0;
                p->y = -10.0;
                p->muki = DX_PI / 2.0;                 // 真下
                p->speed = (double)SCAN_SPEED;
                p->kind = img_enemyShotLaser[0];       // 赤の短レーザー＝読み取り光
                p->param_i[0] = SHOT_SCAN;
            }
            phase = PH_SCAN;
        }
    }
    else if (phase == PH_SCAN) {
        // スキャンライン通過中。下端到達でデコード成功＝解放へ
        if (cnt >= RELEASE_AT) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 全モジュールを画面中心(240,240)から外向きに解き放つ（QRが砕け散る）
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                if (pShot->param_i[0] == SHOT_MODULE) {
                    double dx = pShot->x - 240.0;
                    double dy = pShot->y - 240.0;
                    pShot->muki = atan2(dy, dx) + (GetRand(20) - 10) / 180.0 * DX_PI; // ±10度の揺らぎ
                    pShot->speed = 2.2 + GetRand(120) / 100.0;                          // 2.2～3.4
                }
                pShot = pShot->next;
            }
            phase = PH_RELEASE;
        }
    }

    // ============ 弾の移動 ============
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // スキャンラインは常時移動。モジュールは解放後のみ移動。
        if (pShot->param_i[0] == SHOT_SCAN || phase == PH_RELEASE) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_QRCode_Sakana()
{
    static const int PERIOD = 360; // QR弾幕1サイクルの長さ

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }
    else {
        // 上部で左右に小さく揺れる（演出用）
        enemy.x = 240.0 + 30.0 * sin(count / 40.0);
    }

    // 一定周期ごとにQRコード弾幕を1セット発射
    if (count % PERIOD == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotQRMatrix;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_i[0] = PH_GENERATE;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
