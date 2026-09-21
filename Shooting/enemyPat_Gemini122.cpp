#define DX_PI_2 (DX_PI / 2.0)

// 弾幕：ゴルトンボード・ディフュージョン（釘も敵弾で表現）
static void ShotGaltonBoard(sEnemyShotSet* pEnemyShotSet)
{
    // --- 1. 初期化処理（最初のフレームで釘となる弾群を設置） ---
    if (pEnemyShotSet->count == 0) {
        // 効果音再生
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 釘（障害物）弾の生成（ピラミッド配置：5段）
        int rows = 5+4;
        double startX = 240.0;
        double startY = 120.0;
        double spacingX = 36.0;
        double spacingY = 32.0;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c <= r; c++) {
                sEnemyShot* pPeg = new sEnemyShot;

                // ピラミッド状の位置計算
                pPeg->x = startX - (r * spacingX / 2.0) + (c * spacingX);
                pPeg->y = startY + (r * spacingY);
                pPeg->muki = 0.0;
                pPeg->speed = 0.0; // 動かない釘として配置

                // 釘弾の識別フラグと見た目設定
                pPeg->kind = img_enemyShotDiamond[3]; // シアンの菱形弾（障害物）
                pPeg->param_i[0] = 0; // 0: 釘弾（障害物）

                // リストへ追加
                pPeg->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pPeg->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pPeg;
                pEnemyShotSet->pEnemyShotHead->prev = pPeg;
            }
        }
    }

    // --- 2. 落下弾（パチンコ玉）の継続生成 ---
    // 6フレームごとに画面上部中央から落下弾を連射（一定時間継続）
    if (pEnemyShotSet->count % 6 == 0 && pEnemyShotSet->count < 360) {
        sEnemyShot* pBall = new sEnemyShot;

        // 中央からわずかなブレを持って生成
        pBall->x = 240.0 + (GetRand(4) - 2);
        pBall->y = 60.0;
        pBall->muki = DX_PI_2; // 真下（90度）へ向かって落下
        pBall->speed = 2.2;

        // 落下弾の識別フラグと初期見た目
        pBall->kind = img_enemyShotMediumBall[0]; // 赤色の中玉
        pBall->param_i[0] = 1; // 1: 落下弾
        pBall->param_i[1] = 0; // 反射後のクールダウンタイマー

        // リストへ追加
        pBall->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pBall->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pBall;
        pEnemyShotSet->pEnemyShotHead->prev = pBall;

        if (pEnemyShotSet->count % 24 == 0) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // --- 3. 弾の移動更新および当たり判定（弾 vs 釘弾） ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // 落下弾（param_i[0] == 1）のみ処理を行う
        if (pShot->param_i[0] == 1) {
            // 位置の更新
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 進行方向を徐々に真下（DX_PI_2）へ戻す補正（重力効果）
            double angleDiff = DX_PI_2 - pShot->muki;
            pShot->muki += angleDiff * 0.04;

            // 反射直後の連続判定を防ぐタイマー減算
            if (pShot->param_i[1] > 0) {
                pShot->param_i[1]--;
            }
            else {
                // 釘弾との接近・衝突判定
                sEnemyShot* pCheck = pEnemyShotSet->pEnemyShotHead->next;
                while (pCheck != pEnemyShotSet->pEnemyShotHead) {

                    // 相手が「釘弾（param_i[0] == 0）」の場合
                    if (pCheck->param_i[0] == 0) {
                        double dx = pShot->x - pCheck->x;
                        double dy = pShot->y - pCheck->y;
                        double distSq = dx * dx + dy * dy;

                        // 距離が一定以内（約14px以内）に接近したら跳ね返り発生
                        if (distSq < 14.0 * 14.0) {
                            // 50%の確率（GetRand(1) == 0）で左か右（±40度）に屈折
                            int deflectDir = (GetRand(1) == 0) ? -1 : 1;
                            pShot->muki = DX_PI_2 + deflectDir * (40.0 / 180.0 * DX_PI);

                            // 重複判定防止のクールダウン（10フレーム）
                            pShot->param_i[1] = 10;

                            // 接触時に弾の色を変更（ランダム色で正規分布の可視化）
                            int newColor = GetRand(7);
                            pShot->kind = img_enemyShotMediumBall[newColor];
                            break;
                        }
                    }
                    pCheck = pCheck->next;
                }
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン関数
void EnemyPat_GaltonBoard_Gemini()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 初期位置とパラメーター設定
        enemy.x = 240.0;
        enemy.y = 45.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 画面中央付近をゆっくりと左右往復移動
        enemy.x += 0.6 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 周期的にゴルトンボード弾幕を発動
    if (count % 420 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGaltonBoard;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI_2;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}