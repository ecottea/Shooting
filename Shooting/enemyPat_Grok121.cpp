// カルマン渦列をモチーフにした弾幕パターン
// 敵本体関数: void EnemyPat_KarmanVortex_Grok()
// 使用素材: 小玉(img_enemyShotSmallBall) + 中玉(img_enemyShotMediumBall)
//           色: シアン(3) / 青(4) を左右で使い分け
//           効果音: sound_enemyShot_medium
// count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
// 画面外弾の消去はメインルーチン側で行う前提

// 1つの渦を管理するショットセット用パターン関数
// param_d[0] : 回転方向 (+1.0 = 時計回り, -1.0 = 反時計回り)
// param_d[1] : 渦中心の下方向移動速度
// param_d[2] : 現在の放出角度（連続放出用）
// param_i[0] : 使用する弾色インデックス
static void ShotKarmanVortex(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初期化（最初の1フレームのみ）
    if (pEnemyShotSet->count == 0) {
        // 効果音（中程度）
        if (CheckSoundMem(sound_enemyShot_medium)) {
            StopSoundMem(sound_enemyShot_medium);
        }
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // パラメータ初期化
        // param_d[0] は呼び出し側で既に設定済み（回転方向）
        pEnemyShotSet->param_d[1] = 1.15;          // 渦中心の下方向速度
        pEnemyShotSet->param_d[2] = 0.0;           // 放出開始角度
        // param_i[0] は呼び出し側で設定済み（色）

        // 最初のリングを一気に生成（密度を出す）
        const int num = 14;
        const double initRadius = 18.0;
        const double angSpeed = 0.075;             // 角速度（rad/frame）

        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = (2.0 * DX_PI * i) / num;

            pEnemyShot->x = pEnemyShotSet->x + initRadius * cos(ang);
            pEnemyShot->y = pEnemyShotSet->y + initRadius * sin(ang);
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 0.0;               // 軌道は毎フレーム再計算するため未使用

            // 弾固有パラメータ
            pEnemyShot->param_d[0] = ang;                    // 現在角度
            pEnemyShot->param_d[1] = initRadius;             // 現在半径
            pEnemyShot->param_d[2] = angSpeed * pEnemyShotSet->param_d[0]; // 符号付き角速度
            pEnemyShot->param_d[3] = 0.28;                   // 半径膨張速度
            pEnemyShot->margin = 120;

            // 弾種・色（小玉を基本に、一部中玉を混ぜて変化を付ける）
            if (i % 3 == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[pEnemyShotSet->param_i[0]];
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[pEnemyShotSet->param_i[0]];
            }

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ----- 毎フレーム処理 -----

    // 渦中心を下方向へ移動（カルマン渦列が流れるイメージ）
    pEnemyShotSet->y += pEnemyShotSet->param_d[1];

    // 一定間隔で新しい弾を追加し、渦を継続的に形成する
    // （最初のリングに加えて、時間経過で外側に弾が増えていく）
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count < 90 && (pEnemyShotSet->count % 4) == 0) {
        const double emitRadius = 22.0 + pEnemyShotSet->count * 0.15;
        const double angSpeed = 0.075;

        // 1フレームに1発追加
        pEnemyShot = new sEnemyShot;
        double ang = pEnemyShotSet->param_d[2];

        pEnemyShot->x = pEnemyShotSet->x + emitRadius * cos(ang);
        pEnemyShot->y = pEnemyShotSet->y + emitRadius * sin(ang);
        pEnemyShot->muki = ang;
        pEnemyShot->speed = 0.0;

        pEnemyShot->param_d[0] = ang;
        pEnemyShot->param_d[1] = emitRadius;
        pEnemyShot->param_d[2] = angSpeed * pEnemyShotSet->param_d[0];
        pEnemyShot->param_d[3] = 0.22;               // 後から出た弾は少しゆっくり膨張
        pEnemyShot->margin = 120;

        pEnemyShot->kind = img_enemyShotSmallBall[pEnemyShotSet->param_i[0]];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 次の放出角度を進める（回転方向に合わせる）
        pEnemyShotSet->param_d[2] += 0.55 * pEnemyShotSet->param_d[0];
    }

    // 既存弾の軌道更新（中心周りを回転しながら半径をゆっくり広げる）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 角度更新
        pShot->param_d[0] += pShot->param_d[2];

        // 半径を徐々に拡大（上限あり）
        if (pShot->param_d[1] < 95.0) {
            pShot->param_d[1] += pShot->param_d[3];
        }

        // 位置を中心 + 極座標で再計算
        pShot->x = pEnemyShotSet->x + pShot->param_d[1] * cos(pShot->param_d[0]);
        pShot->y = pEnemyShotSet->y + pShot->param_d[1] * sin(pShot->param_d[0]);

        // 向きを接線方向に合わせておく（将来的に向き付き弾に差し替えやすい）
        pShot->muki = pShot->param_d[0] + (DX_PI * 0.5) * (pShot->param_d[2] > 0.0 ? 1.0 : -1.0);

        pShot = pShot->next;
    }
}

// 敵本体パターン
// 左右交互に回転方向の異なる渦を生成し、カルマン渦列を形成する
void EnemyPat_KarmanVortex_Grok()
{
    static int moveDir;          // 敵の左右移動方向
    static int vortexSide;       // 次に出す渦の左右（0:左, 1:右）
    static int vortexCount;      // 生成した渦の通し番号（色や微調整用）

    // 最初のフレームで初期化
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 48.0;
        enemy.maxHp = enemy.hp = 200;   // 200で固定

        moveDir = 1;
        vortexSide = 0;
        vortexCount = 0;
    }
    else {
        // 敵をゆっくり左右に往復させる（渦の発生源が流れるイメージ）
        enemy.x += 0.65 * (double)moveDir;
        if (enemy.x < 140.0 || enemy.x > 340.0) {
            moveDir *= -1;
        }
        // 上下にもごくわずかに揺らす
        enemy.y = 48.0 + 6.0 * sin(count * 0.025);
    }

    // 一定間隔で左右交互に渦を生成
    // タイミングをずらすことで、互い違いのカルマン渦列になる
    if (count >= 30 && (count % 48) == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKarmanVortex;

        // 左右のオフセットと回転方向を交互に設定
        if (vortexSide == 0) {
            // 左側の渦（時計回り）
            pEnemyShotSet->x = enemy.x - 70.0;
            pEnemyShotSet->y = enemy.y - 12.0;
            pEnemyShotSet->param_d[0] = 1.0;            // 時計回り
            pEnemyShotSet->param_i[0] = 3;              // シアン
        }
        else {
            // 右側の渦（反時計回り）
            pEnemyShotSet->x = enemy.x + 70.0;
            pEnemyShotSet->y = enemy.y - 12.0;
            pEnemyShotSet->param_d[0] = -1.0;           // 反時計回り
            pEnemyShotSet->param_i[0] = 4;              // 青
        }

        // 向きはプレイヤー方向を一応入れておく（未使用でも可）
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = vortexCount++;

        // 弾リストのダミーヘッドを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // ショットセットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 次は反対側
        vortexSide ^= 1;
    }
}