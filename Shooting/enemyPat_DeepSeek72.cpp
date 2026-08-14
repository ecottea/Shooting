// enemyPat_tmp.cpp
// 「絶対零度の墓碑銘（フローズン・エピタフ）」 
// エターナルフォースブリザードをモチーフにした三連弾幕パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------- 定数 ----------
static const int PHASE_DURATION = 200;          // 1フェーズの長さ（フレーム）
static const double ENEMY_BASE_X = 240.0;
static const double ENEMY_BASE_Y = 40.0;

// ---------- フェーズ1用：氷晶散布 ----------
static void ShotIceSeeding(sEnemyShotSet* pEnemyShotSet)
{
    const int STOP_TIME = 100;   // 停止するまでの時間
    const int SPAWN_INTERVAL = 20; // 追加射出の間隔

    // 初期化（セット開始時のみ）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 定期的に新しい針氷弾を画面端から追加
    if (pEnemyShotSet->count % SPAWN_INTERVAL == 0 && pEnemyShotSet->count < PHASE_DURATION) {
        int spawnNum = 3 + GetRand(2) + 25;  // 3～5個
        for (int i = 0; i < spawnNum; ++i) {
            sEnemyShot* p = new sEnemyShot;

            // 画面端のランダムな位置に出現
            int edge = GetRand(3); // 0:上 1:下 2:左 3:右
            switch (edge) {
            case 0: p->x = GetRand(480);          p->y = -10.0; break;
            case 1: p->x = GetRand(480);          p->y = 490.0; break;
            case 2: p->x = -10.0;                 p->y = GetRand(480); break;
            case 3: p->x = 490.0;                 p->y = GetRand(480); break;
            }

            // 画面中心へ向かう角度にランダムな拡散を加える
            double toCenter = atan2(240.0 - p->y, 240.0 - p->x);
            p->muki = toCenter + (GetRand(60) - 30) / 180.0 * DX_PI;
            p->speed = (40 + GetRand(60)) / 100.0; // 0.4～1.0 の低速

            // 色は白(6)または青(4)の小玉
            p->kind = (GetRand(1) == 0) ? img_enemyShotSmallBall[6] : img_enemyShotSmallBall[4];
            p->param_i[0] = 0; // 0:移動中

            // リストに追加
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 全弾の移動と停止処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) { // まだ移動中
            // 一定時間経過で停止
            if (pShot->count == STOP_TIME) {
                pShot->speed = 0.0;
                pShot->param_i[0] = 1; // 停止フラグ
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        // 停止中は位置更新しない（speed=0 なのでそのまま）
        pShot = pShot->next;
    }
}

// ---------- フェーズ2用：絶対零度の波 ----------
static void ShotFreezingWave(sEnemyShotSet* pEnemyShotSet)
{
    const int WALL_LIFE = 120;    // 壁が存在する時間
    const double LASER_DIST = 200.0; // 壁の出現距離

    // 初期化
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 自機狙いの基本角度
        double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 3本のレーザー壁を生成（角度を少しずらす）
        for (int i = -7; i <= 7; ++i) {
            sEnemyShot* p = new sEnemyShot;
            double angle = baseAngle + i * 0.25; // ±0.25ラジアン
            p->x = pEnemyShotSet->x + LASER_DIST * cos(angle);
            p->y = pEnemyShotSet->y + LASER_DIST * sin(angle);
            p->muki = angle + DX_PI / 2.0;
            p->speed = 0.0;               // 固定
            p->kind = img_enemyShotLaser[3]; // シアンの短レーザー
            p->param_i[0] = 0;            // 0:壁として存在中

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 壁が消滅するタイミングで砕け散る + 凍結していた弾を再起動
    //if (pEnemyShotSet->count == WALL_LIFE) {
    //    // 全ショットセットを走査して停止中の氷弾を再起動
    //    for (sEnemyShotSet* pSet = enemyShotSetHead.next; pSet != &enemyShotSetHead; pSet = pSet->next) {
    //        sEnemyShot* pS = pSet->pEnemyShotHead->next;
    //        while (pS != pSet->pEnemyShotHead) {
    //            // 停止フラグ付きの弾（param_i[0]==1 で speed==0）を動かす
    //            if (pS->param_i[0] == 1 && pS->speed == 0.0) {
    //                pS->param_i[0] = 0;                      // フラグ解除
    //                pS->speed = (50 + GetRand(100)) / 100.0; // 0.5～1.5
    //                pS->muki = GetRand(360) / 180.0 * DX_PI; // ランダム方向
    //                // 見た目を菱形弾（シアン）に変える
    //                pS->kind = img_enemyShotDiamond[3];
    //            }
    //            pS = pS->next;
    //        }
    //    }
    //}

    // 壁の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) { // 壁として存在
            if (pShot->count >= WALL_LIFE) {
                // 砕ける：自弾を拡散弾に変化させ、追加の破片を8方向に出す
                pShot->param_i[0] = 1; // 状態変更（このフレームで処理済み）
                // 自分自身を破片に
                pShot->kind = img_enemyShotDiamond[3]; // シアン菱形
                pShot->speed = 2.0;
                pShot->muki = GetRand(360) / 180.0 * DX_PI;

                // 追加の7個の破片
                for (int i = 0; i < 7*2; ++i) {
                    sEnemyShot* pFrag = new sEnemyShot;
                    pFrag->x = pShot->x;
                    pFrag->y = pShot->y;
                    pFrag->muki = GetRand(360) / 180.0 * DX_PI;
                    pFrag->speed = (150 + GetRand(100)) / 100.0; // 1.5～2.5
                    pFrag->kind = img_enemyShotDiamond[3];
                    pFrag->param_i[0] = 2; // 破片マーク（特に意味はない）

                    pFrag->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pFrag->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pFrag;
                    pEnemyShotSet->pEnemyShotHead->prev = pFrag;
                }
            }
        }
        // 壁は移動しないので位置更新は不要（speed=0）
        // 破片になったものは通常移動
        if (pShot->speed > 0.0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ---------- フェーズ3用：永劫の氷棺 ----------
static void ShotEternalFreeze(sEnemyShotSet* pEnemyShotSet)
{
    const int VORTEX_LIFE = 200;   // 渦が縮む時間
    const double INIT_RADIUS = 200.0;
    const double END_RADIUS = 30.0;

    // 初期化
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // まず全ステージの停止弾を一斉に再起動（最終崩壊）
        for (sEnemyShotSet* pSet = enemyShotSetHead.next; pSet != &enemyShotSetHead; pSet = pSet->next) {
            sEnemyShot* pS = pSet->pEnemyShotHead->next;
            while (pS != pSet->pEnemyShotHead) {
                if (pS->param_i[0] == 1 && pS->speed == 0.0) {
                    pS->param_i[0] = 0;
                    pS->speed = (80 + GetRand(120)) / 100.0; // 0.8～2.0
                    pS->muki = GetRand(360) / 180.0 * DX_PI;
                    pS->kind = img_enemyShotDiamond[3];      // シアン菱形に
                }
                pS = pS->next;
            }
        }

        // 渦を構成する微粒弾を生成（60個程度）
        int numBullets = 60;
        for (int i = 0; i < numBullets; ++i) {
            sEnemyShot* p = new sEnemyShot;
            double startAngle = (360.0 / numBullets) * i / 180.0 * DX_PI + (GetRand(20) - 10) / 180.0 * DX_PI;
            p->param_d[0] = startAngle;      // 現在角度
            p->param_d[1] = INIT_RADIUS;     // 現在半径
            p->speed = 0.0;                   // 手動更新
            p->kind = img_enemyShotSmallBall[(i % 2 == 0) ? 6 : 4]; // 白と青が混ざる
            p->param_i[0] = 0;                // 0:渦巻き中
            p->margin = 480;

            // 位置は後で更新するので仮
            p->x = player.x + INIT_RADIUS * cos(startAngle);
            p->y = player.y + INIT_RADIUS * sin(startAngle);

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 渦の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) { // 渦中
            if (pEnemyShotSet->count >= VORTEX_LIFE) {
                // 渦終了：外向きに飛び去る
                pShot->param_i[0] = 1;
                double escapeAngle = atan2(pShot->y - player.y, pShot->x - player.x);
                pShot->muki = escapeAngle + (GetRand(40) - 20) / 180.0 * DX_PI;
                pShot->speed = (200 + GetRand(100)) / 100.0; // 2.0～3.0
                pShot->kind = img_enemyShotDiamond[3];       // 見た目を菱形に
            }
            else {
                // 半径を徐々に縮小、角度を進める
                pShot->param_d[1] -= 0.8; // 毎フレーム縮小
                if (pShot->param_d[1] < END_RADIUS) pShot->param_d[1] = END_RADIUS;
                pShot->param_d[0] += 0.04; // 角速度
                // 自機の現在位置を中心に
                pShot->x = player.x + pShot->param_d[1] * cos(pShot->param_d[0]);
                pShot->y = player.y + pShot->param_d[1] * sin(pShot->param_d[0]);
            }
        }
        // 渦が終わった弾（speed>0）は通常移動
        if (pShot->speed > 0.0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ---------- 敵本体 ----------
void EnemyPat_EternalForceBlizzard_DeepSeek()
{
    static int moveDir = 1; // 横移動方向

    // 初回初期化
    if (count == 1) {
        enemy.x = ENEMY_BASE_X;
        enemy.y = ENEMY_BASE_Y;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;
    }
    else {
        // ゆっくり左右移動
        enemy.x += 0.5 * moveDir;
        if (enemy.x < 60.0 || enemy.x > 420.0) moveDir *= -1;
    }

    // 現在のフェーズを決定（300フレームごとに 0→1→2→0…）
    int phase = ((count - 1) / PHASE_DURATION) % 3;
    // 各フェーズの先頭フレームかどうか
    bool phaseStart = ((count - 1) % PHASE_DURATION) == 0;

    if (phaseStart) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0; // 使用しないが一応
        pSet->kind = phase; // フェーズ情報を格納（デバッグ用）

        // フェーズごとのパターン関数を割り当て
        switch (phase) {
        case 0: pSet->patternFunc = ShotIceSeeding; break;
        case 1: pSet->patternFunc = ShotFreezingWave; break;
        case 2: pSet->patternFunc = ShotEternalFreeze; break;
        }

        // ショットセットをグローバルリストに追加
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}