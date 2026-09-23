// 弾幕：ア、チョイと伸ばせば（変形ジャバラ弾幕）
static void ShotSudare(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;
    const int N = 35;

    // 初期化：すべての弾（竹パーツ・結び目パーツ）を敵座標に生成
    if (t == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // リストに追加するラムダ式
        auto addShot = [&](int kind, int type, int idx) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = DX_PI / 2.0;
            pShot->speed = 0.0;
            pShot->kind = kind;
            pShot->param_i[0] = type; // 役割 (0:竹A, 1:竹B, 2:結び目A, 3:結び目B)
            pShot->param_i[1] = idx;  // 横に並べるときのインデックス (0～20)
            pShot->margin = 240;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        };

        // 南京玉すだれの構成要素を 21セット（交差するように）生成
        for (int i = 0; i < N; i++) {
            addShot(img_enemyShotLaser[2], 0, i); // 竹A (緑レーザー)
            addShot(img_enemyShotLaser[1], 1, i); // 竹B (黄レーザー)
            addShot(img_enemyShotSmallBall[0], 2, i); // 結び目A (赤小玉)
            addShot(img_enemyShotDiamond[3], 3, i); // 結び目B (シアン菱形弾)
        }
    }

    // 各弾の制御
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int type = pShot->param_i[0];
        int idx = pShot->param_i[1];

        if (t <= 60) {
            // フェーズ①：アコーディオンのように横へ広がる（すだれの展開）
            double targetX = pEnemyShotSet->x + (idx - N/2) * 16.0;
            double targetY = pEnemyShotSet->y + 40.0;

            // 目標座標へLerp（直線補間）でじわっと移動させる
            pShot->x += (targetX - pShot->x) * 0.1;
            pShot->y += (targetY - pShot->y) * 0.1;

            // 竹（レーザー）が少し斜めに交差するように向きをつける
            if (type == 0) pShot->muki = DX_PI / 2.0 + 0.4;
            if (type == 1) pShot->muki = DX_PI / 2.0 - 0.4;
            if (type == 2 || type == 3) pShot->muki = DX_PI / 2.0;
        }
        else if (t <= 120) {
            // フェーズ②：「ア、さて！」東京タワー（または富士山）のような三角形に変形
            double targetX = pEnemyShotSet->x + (idx - N/2) * 18.0;
            double targetY = pEnemyShotSet->y + 20.0 + abs(idx - N/2) * 16.0; // 中央が一番高い

            pShot->x += (targetX - pShot->x) * 0.1;
            pShot->y += (targetY - pShot->y) * 0.1;

            // 三角形の傾斜に沿うようにレーザーの向きを調整して輪郭を作る
            if (idx < N/2) {
                if (type == 0) pShot->muki = atan2(16.0, -18.0);
                if (type == 1) pShot->muki = atan2(16.0, 18.0);
            }
            else if (idx > N/2) {
                if (type == 0) pShot->muki = atan2(16.0, 18.0);
                if (type == 1) pShot->muki = atan2(16.0, -18.0);
            }
            else {
                if (type == 0 || type == 1) pShot->muki = DX_PI / 2.0; // 中央は真っ直ぐ
            }
        }
        else if (t <= 180) {
            // フェーズ③：技の完成・タメ
            if (t == 121 && pShot == pEnemyShotSet->pEnemyShotHead->next) {
                // ピタッと止まり、チャージ音で予告
                if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            }
        }
        else {
            // フェーズ④：崩し・しだれ
            if (t == 181) {
                if (pShot == pEnemyShotSet->pEnemyShotHead->next) {
                    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                }

                if (type == 0 || type == 1) {
                    // 竹パーツ：自機狙いをベースに、X字に交差しながら一気に射出
                    double baseAng = atan2(player.y - pShot->y, player.x - pShot->x);
                    if (type == 0) pShot->muki = baseAng + 0.3;
                    if (type == 1) pShot->muki = baseAng - 0.3;
                    pShot->speed = 0.5 + GetRand(50) / 100.0;
                    pShot->param_d[0] = 0.03 + GetRand(20) / 1000.0; // 徐々に加速
                }
                else {
                    // 結び目パーツ：しだれ柳のように重力の影響を受けて放物線落下
                    pShot->speed = 1.5 + GetRand(30) / 10.0;
                    pShot->muki = DX_PI / 2.0 + (GetRand(120) - 60) * DX_PI / 180.0;
                    pShot->param_d[0] = pShot->speed * cos(pShot->muki); // vx
                    pShot->param_d[1] = -(1.0 + GetRand(20) / 10.0);    // vy (初速は上に跳ねる)
                }
            }

            // 発射後の移動処理
            if (type == 0 || type == 1) {
                pShot->speed += pShot->param_d[0]; // 加速
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                pShot->param_d[1] += 0.06; // 重力加速度を足す
                pShot->x += pShot->param_d[0];
                pShot->y += pShot->param_d[1];
                pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]); // 落下方向に合わせる
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_NankinTamasudare_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 演舞を行うため、ゆっくりと揺れる程度にする
        enemy.x += 0.5 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 1サイクルが約200フレームかかるため、間隔を空けて玉すだれを展開する
    if (count % 240 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSudare;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
