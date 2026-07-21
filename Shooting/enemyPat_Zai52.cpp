// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ヘルパー関数：2点間を結ぶ直線弾（蜜の糸）を生成する
static void AddLineShot(sEnemyShotSet* pSet, double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    double dist = sqrt(dx * dx + dy * dy);
    if (dist < 1.0) return;

    // 小玉のサイズ(2.5)間隔で弾を配置する
    int numShots = (int)(dist / 2.5);
    if (numShots == 0) numShots = 1;
    double angle = atan2(dy, dx);

    for (int i = 0; i <= numShots; i++) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = x1 + (dx * i / numShots);
        pEnemyShot->y = y1 + (dy * i / numShots);
        pEnemyShot->muki = angle;
        pEnemyShot->speed = 2.0; // ゆっくり外側へ流れる
        pEnemyShot->kind = img_enemyShotSmallBall[1]; // 黄色小玉(蜜の糸)
        pEnemyShot->param_i[0] = 3; // 3:線弾
        pEnemyShot->margin = 240;

        // 双方向リストの最後に追加
        pEnemyShot->prev = pSet->pEnemyShotHead->prev;
        pEnemyShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pEnemyShot;
        pSet->pEnemyShotHead->prev = pEnemyShot;
    }
}

// 弾幕：六角の檻と狂気の羽音
static void ShotHexSwarm(sEnemyShotSet* pEnemyShotSet)
{
    // --- 蜂の巣の設定値 ---
    const double SPEED_FAST = 2.5; // 外側の結晶の速さ
    const double SPEED_SLOW = 1.2; // 内側の結晶の速さ
    const double DIST_FAST = 180; // 外側の結晶が停止する距離
    const double DIST_SLOW = 120;  // 内側の結晶が停止する距離
    const int NUM_VERTICES = 6;     // 六角形の頂点数
    const int DISAPPEAR_FRAME = 120; // 線生成から何フレーム後に結晶を飛ばすか

    // --- 初期化処理（Setのcountが0の時のみ通過） ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 0; // 0:未完了、1:線生成済み
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x; // 発射元Xを記録
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y; // 発射元Yを記録

        // 外側6個、内側6個の結晶を生成
        for (int i = 0; i < NUM_VERTICES * 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 60度間隔で角度を決定
            double angle = pEnemyShotSet->muki + (i % NUM_VERTICES) * (DX_PI / 3.0);
            pEnemyShot->muki = angle;
            // 前半6個が外側、後半6個が内側
            pEnemyShot->speed = (i < NUM_VERTICES) ? SPEED_FAST : SPEED_SLOW;

            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄色中玉(蜂蜜の結晶)
            pEnemyShot->param_i[0] = 0; // 0:移動中
            pEnemyShot->param_i[1] = i; // 頂点のインデックス
            pEnemyShot->param_d[2] = (i < NUM_VERTICES) ? DIST_FAST : DIST_SLOW; // 停止距離
            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 毎フレーム処理 ---

    // 1. 羽音弾（毒針）の連射（線生成が終わるまで）
    // GetRand(60)は0～60を返すので、-30して-30～+30のばらつきにする
    if (pEnemyShotSet->param_i[0] == 0 && pEnemyShotSet->count % 4 == 2) {
        sEnemyShot* pEnemyShot = new sEnemyShot;

        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        double targetX = player.x + (GetRand(60) - 30);
        double targetY = player.y + (GetRand(60) - 30);
        pEnemyShot->muki = atan2(targetY - pEnemyShot->y, targetX - pEnemyShot->x);
        pEnemyShot->speed = 6.0; // 超高速

        pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒色小玉(毒針)
        pEnemyShot->param_i[0] = 2; // 2:羽音弾
        pEnemyShot->margin = 240;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 2. 全ての弾の移動・状態更新
    bool allStopped = true;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int type = pShot->param_i[0];

        // 羽音弾、線弾、収束弾は常に直進する
        if (type == 2 || type == 3 || type == 4) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        // 移動中の結晶
        else if (type == 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 発射元からの距離を計算
            double dx = pShot->x - pEnemyShotSet->param_d[0];
            double dy = pShot->y - pEnemyShotSet->param_d[1];
            double dist = sqrt(dx * dx + dy * dy);

            // 規定距離に達したら停止して座標を記録
            if (dist >= pShot->param_d[2]) {
                pShot->speed = 0.0;
                pShot->param_i[0] = 1; // 1:停止
                pShot->param_d[0] = pShot->x; // 停止位置X
                pShot->param_d[1] = pShot->y; // 停止位置Y
            }
            else {
                allStopped = false; // まだ停止していない結晶がある
            }
        }
        // 停止中の結晶
        else if (type == 1) {
            // 線生成から一定時間経過したら、結晶を外側に逃がす（画面外消去へ任せる）
            if (pEnemyShotSet->param_i[0] == 1 && pEnemyShotSet->count > pEnemyShotSet->param_i[1] + DISAPPEAR_FRAME) {
                pShot->speed = 1.5;
                pShot->param_i[0] = 5; // 5:逃走中
            }
        }
        // 逃走中の結晶
        else if (type == 5) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }

    // 3. 全結晶が停止した瞬間の処理（線と収束弾の生成）
    if (allStopped && pEnemyShotSet->param_i[0] == 0) {
        pEnemyShotSet->param_i[0] = 1; // 線生成済みフラグを立てる
        pEnemyShotSet->param_i[1] = pEnemyShotSet->count; // 完了時のフレームを記録

        // 停止した結晶の座標を配列にまとめる
        double stopX[12], stopY[12];
        sEnemyShot* pS = pEnemyShotSet->pEnemyShotHead->next;
        while (pS != pEnemyShotSet->pEnemyShotHead) {
            if (pS->param_i[0] == 1) {
                int idx = pS->param_i[1];
                stopX[idx] = pS->param_d[0];
                stopY[idx] = pS->param_d[1];
            }
            pS = pS->next;
        }

        // 外側の六角形を結ぶ線を生成
        for (int i = 0; i < NUM_VERTICES; i++) {
            int next = (i + 1) % NUM_VERTICES;
            AddLineShot(pEnemyShotSet, stopX[i], stopY[i], stopX[next], stopY[next]);
        }
        // 内側の六角形を結ぶ線を生成
        for (int i = 0; i < NUM_VERTICES; i++) {
            int next = (i + 1) % NUM_VERTICES;
            AddLineShot(pEnemyShotSet, stopX[i + NUM_VERTICES], stopY[i + NUM_VERTICES], stopX[next + NUM_VERTICES], stopY[next + NUM_VERTICES]);
        }

        // 収束弾の発射（各頂点から中心に向かって3Way）
        double centerX = pEnemyShotSet->param_d[0];
        double centerY = pEnemyShotSet->param_d[1];

        for (int i = 0; i < 12; i++) {
            double baseAngle = atan2(centerY - stopY[i], centerX - stopX[i]);
            // ±15度（PI/12）ずらして扇状にする
            for (int j = -1; j <= 1; j++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = stopX[i];
                pEnemyShot->y = stopY[i];
                pEnemyShot->muki = baseAngle + j * (DX_PI / 12.0);
                pEnemyShot->speed = 4.0;
                pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒色小玉(毒針)
                pEnemyShot->param_i[0] = 4; // 4:収束弾
                pEnemyShot->margin = 240;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
}


// 敵本体のパターン
void EnemyPat_Bee_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 180フレームごとに蜂の巣弾幕を発射
    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHexSwarm;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        // プレイヤーの方向を基準にする（六角形の回転がプレイヤー位置に依存する）
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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