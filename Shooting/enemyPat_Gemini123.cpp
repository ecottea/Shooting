// enemyPat_Tmp.cpp
#include <cmath>

// ---------------------------------------------------------
// 弾の役割識別用定数
// ---------------------------------------------------------
enum eShotRole {
    ROLE_WALL = 0,     // サイドガード（ホッケー台の壁）
    ROLE_PADDLE,       // パドル（打球具）
    ROLE_PUCK,         // パック（メイン大玉）
    ROLE_TRAIL,        // ホバー軌跡（空気の層・遅れて広がる小玉）
    ROLE_BURST         // パック破裂時の全方位弾
};

// =========================================================
// 弾幕パターン：「リバウンド・スマッシュ」
// =========================================================
static void ShotReboundSmash(sEnemyShotSet* pEnemyShotSet)
{
    // 弾幕の起点を常に敵の現在位置に設置
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // -----------------------------------------------------
    // 1. フィールド生成（初回フレームのみ壁とパドルを配置）
    // -----------------------------------------------------
    if (pEnemyShotSet->count == 1) {
        // 左右の壁（シアンの銃弾を画面左右端に縦列配置）
        for (int i = 0; i < 16; i++) {
            double wallY = 30.0 + i * 28.0;

            // 左壁 (x = 20.0)
            sEnemyShot* pLeft = new sEnemyShot;
            pLeft->x = 20.0;
            pLeft->y = wallY;
            pLeft->muki = DX_PI / 2.0;
            pLeft->speed = 0.0;
            pLeft->kind = img_enemyShotBullet[3]; // シアンの銃弾
            pLeft->param_i[0] = ROLE_WALL;

            pLeft->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pLeft->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pLeft;
            pEnemyShotSet->pEnemyShotHead->prev = pLeft;

            // 右壁 (x = 460.0)
            sEnemyShot* pRight = new sEnemyShot;
            pRight->x = 460.0;
            pRight->y = wallY;
            pRight->muki = DX_PI / 2.0;
            pRight->speed = 0.0;
            pRight->kind = img_enemyShotBullet[3];
            pRight->param_i[0] = ROLE_WALL;

            pRight->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRight->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRight;
            pEnemyShotSet->pEnemyShotHead->prev = pRight;
        }

        // ボス両脇のパドル（赤い大玉 2個）
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pPaddle = new sEnemyShot;
            pPaddle->x = enemy.x + (i == 0 ? -45.0 : 45.0);
            pPaddle->y = enemy.y + 10.0;
            pPaddle->muki = 0.0;
            pPaddle->speed = 0.0;
            pPaddle->kind = img_enemyShotLargeBall[0]; // 赤の大玉
            pPaddle->param_i[0] = ROLE_PADDLE;
            pPaddle->param_i[1] = i; // 0:左パドル, 1:右パドル

            pPaddle->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pPaddle->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pPaddle;
            pEnemyShotSet->pEnemyShotHead->prev = pPaddle;
        }
    }

    // -----------------------------------------------------
    // 2. スマッシュ射出（約4秒周期＝240フレーム）
    // -----------------------------------------------------
    // 発射直前の予告音
    if (pEnemyShotSet->count % 240 == 180-150) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // パック（白い大玉）のスマッシュ射出
    if (pEnemyShotSet->count % 240 == 220-150) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pPuck = new sEnemyShot;
        pPuck->x = enemy.x;
        pPuck->y = enemy.y + 15.0;

        // 自機方向への角度 ＋ 揺らぎ (-10°～ +10°)
        double angleToPlayer = atan2(player.y - pPuck->y, player.x - pPuck->x);
        pPuck->muki = angleToPlayer + (GetRand(20) - 10) / 180.0 * DX_PI;
        pPuck->speed = 4.5;
        pPuck->kind = img_enemyShotLargeBall[6]; // 白の大玉（パック）
        pPuck->param_i[0] = ROLE_PUCK;
        pPuck->param_i[1] = 0; // ラリー回数カウンタ

        pPuck->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pPuck->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pPuck;
        pEnemyShotSet->pEnemyShotHead->prev = pPuck;
    }

    // -----------------------------------------------------
    // 3. 各弾の固有処理（移動・反射・軌跡生成・時間差拡散）
    // -----------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        switch (pShot->param_i[0]) {
        case ROLE_PADDLE:
            // パドルはボスの移動に合わせて両脇に追従
            pShot->x = enemy.x + (pShot->param_i[1] == 0 ? -45.0 : 45.0);
            pShot->y = enemy.y + 10.0;
            break;

        case ROLE_PUCK:
            // パックの移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 【壁（左右）での反射処理】
            if (pShot->x <= 30.0 && cos(pShot->muki) < 0.0) {
                pShot->muki = DX_PI - pShot->muki;
                pShot->x = 30.0; // 食い込み補正
                pShot->speed += 0.2; // 壁反射でわずかに加速
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
            else if (pShot->x >= 450.0 && cos(pShot->muki) > 0.0) {
                pShot->muki = DX_PI - pShot->muki;
                pShot->x = 450.0; // 食い込み補正
                pShot->speed += 0.2;
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }

            // 【画面下端での跳ね返り】
            if (pShot->y >= 450.0 && sin(pShot->muki) > 0.0) {
                pShot->muki = -pShot->muki;
                pShot->y = 450.0;
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }

            // 【画面上部（ボスライン）でのラリー・再スマッシュ】
            if (pShot->y <= 70.0 && sin(pShot->muki) < 0.0) {
                int rallyCount = pShot->param_i[1];
                if (rallyCount < 3) {
                    // 3回まではボスが打ち返して加速！
                    double aimAngle = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->muki = aimAngle + (GetRand(30) - 15) / 180.0 * DX_PI;
                    pShot->speed += 1.2;
                    pShot->param_i[1]++; // ラリー数加算

                    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                }
                else {
                    // 4回目の打ち返しで限界を迎え、パックが全方位破裂！
                    if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
                    PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                    for (int i = 0; i < 16; i++) {
                        sEnemyShot* pBurst = new sEnemyShot;
                        pBurst->x = pShot->x;
                        pBurst->y = pShot->y;
                        pBurst->muki = i * (2.0 * DX_PI / 16.0);
                        pBurst->speed = 2.0 + (GetRand(100) / 100.0);
                        pBurst->kind = img_enemyShotMediumBall[1]; // 黄色の全方位中玉
                        pBurst->param_i[0] = ROLE_BURST;

                        pBurst->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pBurst->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pBurst;
                        pEnemyShotSet->pEnemyShotHead->prev = pBurst;
                    }

                    // メインループに消去させるため、パック本体を画面外へ退避
                    pShot->x = -999.0;
                    pShot->y = -999.0;
                }
            }

            // 【ホバー軌跡（トレイル弾）の生成】
            // 移動中に3フレームごとに青い小玉を配置（初期速度0）
            if (pShot->x > 0.0 && pShot->count % 3 == 0) {
                sEnemyShot* pTrail = new sEnemyShot;
                pTrail->x = pShot->x;
                pTrail->y = pShot->y;
                pTrail->muki = 0.0;
                pTrail->speed = 0.0;
                pTrail->kind = img_enemyShotSmallBall[4]; // 青小玉
                pTrail->param_i[0] = ROLE_TRAIL;
                pTrail->param_i[1] = pShot->count; // 生成時点のカウント
                pTrail->param_i[2] = 0;             // 0:静止中, 1:拡散中

                pTrail->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pTrail->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pTrail;
                pEnemyShotSet->pEnemyShotHead->prev = pTrail;
            }
            break;

        case ROLE_TRAIL:
            // 生成から50フレーム静止した後、ふわっと全方位ランダムに低速拡散
            if (pShot->param_i[2] == 0) {
                if (pShot->count - pShot->param_i[1] > 50) {
                    pShot->param_i[2] = 1; // 拡散フラグON
                    pShot->muki = (GetRand(360) / 180.0) * DX_PI;
                    pShot->speed = 0.6 + (GetRand(80) / 100.0);
                }
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            break;

        case ROLE_BURST:
            // 破裂弾の単純移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            break;

        case ROLE_WALL:
        default:
            // 壁は移動処理なし（画面端に常駐）
            break;
        }

        pShot = pShot->next;
    }
}

// =========================================================
// 敵本体のパターン
// =========================================================
void EnemyPat_AirHockey_Gemini()
{
    // 出現時の初期化処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;

        // リバウンド・スマッシュ専用の弾幕セットを1つ生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotReboundSmash;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // ダミーヘッドノードの生成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // enemyShotSet リストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵の左右揺れる移動（横幅を限定して壁に寄りすぎないように調整）
        enemy.x = 240.0 + 100.0 * sin(count * DX_PI / 120.0);
        enemy.y = 60.0 + 15.0 * sin(count * DX_PI / 60.0);
    }
}