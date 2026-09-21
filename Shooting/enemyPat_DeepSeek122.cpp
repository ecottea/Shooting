//============================================================
// enemyPat_Tmp.cpp
// 弾幕パターン：ゴルトンボード・カスケード ～二項分布の雨～
//
// ゴルトンボード（パチンコの釘盤）をモチーフにした弾幕。
// 上から落ちた白い玉弾が、見えない釘（ペグ）に当たるたびに
// 左右へ反射し、最下段を抜けると赤い攻撃弾に変化して加速。
// 最終的な弾の分布は中央が濃く、端が薄いベル型（二項分布）
// となる。
//
// 必要なインクルード：gv.h, DxLib.h, <cmath>
//============================================================

//------------------------------------------------------------
// 弾幕：ゴルトンボードのペグ（釘）
//
// ゴルトンボード弾幕（ShotGoldtonBoard）と対になる装飾用の弾。
// 敵の前方に三角形のペグ列を静止配置する。攻撃判定は持たない
// 想定で、玉弾の反射位置を視覚的に示すための目印として使う。
//
// ・玉弾側の ROW_Y0 / ROW_DY と揃える必要がある
// ・配置は count == 0 の 1 フレームだけ行う
// ・speed = 0 の静止弾として扱う
//------------------------------------------------------------
static void ShotPegBoard(sEnemyShotSet* pEnemyShotSet)
{
    const int    ROWS = 7;      // ペグの段数（玉弾側と一致させる）
    const double ROW_Y0 = 120.0;  // 1段目のY座標（玉弾側と一致させる）
    const double ROW_DY = 28.0;   // 段の間隔（玉弾側と一致させる）
    const double COL_DX = 25.0;   // ペグ同士の横間隔

    if (pEnemyShotSet->count == 0) {
        // 効果音：軽い「カチッ」とした配置音（無くてもよい）
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int r = 0; r < ROWS; r++) {
            const int n = r + 1; // r段目は (r+1) 個のペグ
            for (int i = 0; i < n; i++) {
                sEnemyShot* p = new sEnemyShot;

                // 中心 x に対して左右対称に並ぶよう (i - r/2) でオフセット
                p->x = pEnemyShotSet->x + (i - r * 0.5) * COL_DX;
                p->y = pEnemyShotSet->y + r * ROW_DY;
                p->muki = -DX_PI / 2.0;
                p->speed = 0.0; // 静止

                // 弾の種類と色：シアンの鱗弾をペグに見立てる
                p->kind = img_enemyShotScale[3];

                // param_i[0] = 1 をペグ識別マーカーとして付与
                // （将来メイン側で判定をスキップしたい場合に使える）
                p->param_i[0] = 1;

                // リストに追加
                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // 静止弾なので毎フレームの移動処理は行わない
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->count >= 300) {
            pShot->margin = -9999;
        }
        pShot = pShot->next;
    }
}

//------------------------------------------------------------
// 弾幕：ゴルトンボード・カスケード
//
// ・玉弾を一定間隔で生成
// ・各弾は生成時に「各段で左右どちらに反射するか」の経路を
//   先に決めておく（GetRand(1) で 0/1）
// ・段を通過するたびに param_d[1]（横速度）を書き換えて
//   ジグザグに落下させる
// ・最下段を抜けたら赤い攻撃弾に変化して加速落下
//------------------------------------------------------------
static void ShotGoldtonBoard(sEnemyShotSet* pEnemyShotSet)
{
    // --- パラメータ ---
    const int    ROWS = 7;      // ペグの段数
    const double ROW_Y0 = 120.0;  // 1段目のY座標
    const double ROW_DY = 28.0;   // 段の間隔
    const double SEED_VY = 2.5;    // 玉弾の落下速度
    const double DEFLECT = 1.2;    // 反射時の横速度
    const double ACTIVE_VY = 3.5;    // 攻撃弾化後の落下速度
    const int    SPAWN_INTERVAL = 2;      // 玉弾の生成間隔（フレーム）
    const int    SPAWN_DURATION = 200;    // 玉弾の生成期間（フレーム）

    // --- 初回のみ効果音（予告的に軽い音） ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // --- 玉弾の生成 ---
    if (pEnemyShotSet->count < SPAWN_DURATION &&
        pEnemyShotSet->count % SPAWN_INTERVAL == 0) {

        sEnemyShot* p = new sEnemyShot;

        p->x = pEnemyShotSet->x;
        p->y = pEnemyShotSet->y;
        p->muki = DX_PI / 2.0;
        p->speed = 0.0;
        p->kind = img_enemyShotMediumBall[6]; // 白：落ちてくる玉弾

        // param_d[0] = 縦速度, param_d[1] = 横速度
        p->param_d[0] = SEED_VY;
        p->param_d[1] = 0.0;

        // param_i[0..ROWS-1] = 各段で左右どちらに反射するか（0:左, 1:右）
        // 独立な 50% 抽選を段数ぶん繰り返すことで、
        // 最終位置は自然と二項分布（ベル型）になる。
        for (int r = 0; r < ROWS; r++) {
            p->param_i[r] = GetRand(1); // 0 or 1
        }
        p->param_i[10] = 0; // 現在の段
        p->param_i[11] = 0; // 0: ボード内, 1: 攻撃弾化済み

        // リストに追加
        p->prev = pEnemyShotSet->pEnemyShotHead->prev;
        p->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = p;
        pEnemyShotSet->pEnemyShotHead->prev = p;
    }

    // --- 玉弾の更新 ---
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {

        // 位置更新（横→縦の順で加算）
        p->x += p->param_d[1];
        p->y += p->param_d[0];

        if (p->param_i[11] == 0) {
            // ボード内：段を超えるたびに反射
            int r = p->param_i[10];
            if (r < ROWS && p->y >= ROW_Y0 + r * ROW_DY) {
                int dir = p->param_i[r];
                p->param_d[1] = (dir == 1) ? DEFLECT : -DEFLECT;
                p->param_i[10] = r + 1;
                // 反射の瞬間だけマゼンタに光らせて「カチッ」を表現
                p->kind = img_enemyShotMediumBall[5];
            }
            if (p->param_i[10] >= ROWS) {
                // 最下段を抜けた：攻撃弾化
                p->x += (GetRand(100) - 50) / 20.0;
                p->param_i[11] = 1;
                p->param_d[0] = ACTIVE_VY;
                p->param_d[1] = 0.0; // 真下に落下
                p->kind = img_enemyShotSmallBall[0]; // 赤：攻撃弾
            }
        }

        p = p->next;
    }
}

//------------------------------------------------------------
// 敵本体のパターン
//------------------------------------------------------------
void EnemyPat_GaltonBoard_DeepSeek()
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
        // ゆっくり左右に往復（ゴルトンボードの中心がずれて見えるよう緩やかに）
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 140.0) muki = 1;
        if (enemy.x > 340.0) muki = -1;
    }

    // 4秒ごとにゴルトンボード弾幕を発射
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet;

         pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGoldtonBoard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
 
        pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPegBoard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 65.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

    }
}