// enemyPat_sampleForAI.cpp (の一部として追加/置換してください)

#include <cmath> // fmod 使用のため

// 弾幕：コッホ雪片 (Koch Snowflake)
static void ShotKochSnowflake(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 予告音再生
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double base_r = 80.0; // 輪郭の基本半径
        double amp_r = 50.0;  // 輪郭の変形振幅（膨らみの大きさ）

        // ---------------------------------------------------------
        // 1. 骨格の生成 (6方向の短レーザー)
        // ---------------------------------------------------------
        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = i * 60.0 * DX_PI / 180.0;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.3; // ゆっくり回転・拡大させるための微速
            pEnemyShot->kind = img_enemyShotLaser[6]; // 短レーザー, 白

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // ---------------------------------------------------------
        // 2. 輪郭の生成 (72方向の小玉) + 3. 細部の仕込み
        // ---------------------------------------------------------
        for (int i = 0; i < 72*2; i++) {
            double angle_deg = i * 5.0/2;
            double angle_rad = angle_deg * DX_PI / 180.0;

            // コッホ曲線風の変形計算: 
            // 60度の倍数(頂点)で最大、30度の倍数(辺の中央)で最小になるようにする
            double rem = fmod(angle_deg, 60.0);
            double wave = cos(rem * DX_PI / 30.0); // rem=0で1.0, rem=30で-1.0
            double r = base_r + amp_r * wave;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + r * cos(angle_rad);
            pEnemyShot->y = pEnemyShotSet->y + r * sin(angle_rad);
            pEnemyShot->muki = angle_rad; // 外向きに発射
            pEnemyShot->speed = 1.2;
            pEnemyShot->kind = img_enemyShotSmallBall[3]; // 小玉, シアン
            pEnemyShot->param_i[0] = 0; // 初期化

            // 頂点付近（remが0~15または45~60）ならフラグを立てる
            if (rem <= 15.0 || rem >= 45.0) {
                pEnemyShot->param_i[0] = 1; // 頂点フラグ
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---------------------------------------------------------
    // 毎フレームの移動・分岐処理
    // ---------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // 骨格（短レーザー）の処理: ゆっくり回転させて雪片全体を回す
        if (pShot->kind == img_enemyShotLaser[6]) {
            pShot->muki += 0.015;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pEnemyShotSet->count >= 120) pShot->margin = -9999;
        }
        // 輪郭（小玉）の処理
        else if (pShot->kind == img_enemyShotSmallBall[3]) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 頂点フラグがある場合、ある程度進んだら分裂して自己相似を表現
            if (pShot->param_i[0] == 1 && pEnemyShotSet->count == 25) {
                // 分裂処理: 3方向に菱形弾を発射
                for (int j = -1; j <= 1; j++) {
                    sEnemyShot* pSubShot = new sEnemyShot;
                    pSubShot->x = pShot->x;
                    pSubShot->y = pShot->y;
                    pSubShot->muki = pShot->muki + j * 30.0 * DX_PI / 180.0;
                    pSubShot->speed = 2.5;
                    pSubShot->kind = img_enemyShotDiamond[6]; // 菱形弾, 白

                    pSubShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pSubShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pSubShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pSubShot;
                }
                pShot->param_i[0] = 0; // 分裂済みフラグ
            }
        }
        // 細部（菱形弾）およびその他の処理
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン (テスト用)
void EnemyPat_KochSnowflake_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0; // 画面中央より少し上
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 2秒(120フレーム)ごとに弾幕セットを生成
    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKochSnowflake;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 雪片は内部で回転するため初期向きは0
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