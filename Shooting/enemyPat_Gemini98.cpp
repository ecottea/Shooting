// enemyPat_barrier.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 鱗弾の制御用関数
static void ShotBarrier_Scale(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    int T = pEnemyShotSet->param_i[0]; // 鱗弾の射出終了（一斉始動）までの時間
    double v = pEnemyShotSet->param_d[0];

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // param_i[0] : 0=外側向け(止まらない), 1=内側向け(指定距離で停止中), 2=内側向け(再始動後)
        // param_d[0] : 停止するまでの距離
        // param_d[1] : これまでに移動した距離

        if (pShot->param_i[0] == 1) {
            // Tフレーム経過したら一斉に動き出す
            if (pEnemyShotSet->count >= T) {
                pShot->param_i[0] = 2; // 再始動
                pShot->speed = v;
            }
            else {
                // 指定距離まで進んだら停止
                if (pShot->param_d[1] >= pShot->param_d[0]) {
                    pShot->speed = 0.0;
                }
            }
        }

        // 座標更新
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 移動距離の加算
        pShot->param_d[1] += pShot->speed;

        pShot = pShot->next;
    }
}

// 大玉の制御用関数
static void ShotBarrier_LargeBall(sEnemyShotSet* pEnemyShotSet)
{
    // パラメータの取得
    int k = pEnemyShotSet->param_i[0];
    int t1 = pEnemyShotSet->param_i[1];
    int t2 = pEnemyShotSet->param_i[2];
    int T = pEnemyShotSet->param_i[3];

    double a = pEnemyShotSet->param_d[0];
    double v = pEnemyShotSet->param_d[1];
    double th0 = pEnemyShotSet->param_d[2];
    double l0 = pEnemyShotSet->param_d[3];
    double l = pEnemyShotSet->param_d[4];

    // 展開にかかるフレーム数（半径240まで毎フレーム2.0ずつ広がる想定）
    int expandTime = 120;

    if (pEnemyShotSet->count == 0) {
        // 大玉の生成 (青k個、マゼンタk個)
        for (int i = 0; i < k * 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 240.0;
            pEnemyShot->y = 240.0;
            // 角度を均等に配置
            pEnemyShot->muki = (DX_PI * 2.0 / (k * 2)) * i;
            pEnemyShot->speed = 2.0; // 広がる速度

            // 種類と色 (青:4, マゼンタ:5)
            if (i % 2 == 0) {
                pEnemyShot->kind = img_enemyShotLargeBall[4]; // 青の大玉
                pEnemyShot->param_i[1] = 4; // 射出する鱗弾の色
            }
            else {
                pEnemyShot->kind = img_enemyShotLargeBall[5]; // マゼンタの大玉
                pEnemyShot->param_i[1] = 5; // 射出する鱗弾の色
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 鱗弾用のShotSetを生成
        sEnemyShotSet* pScaleSet = new sEnemyShotSet;
        pScaleSet->count = 0;
        pScaleSet->patternFunc = ShotBarrier_Scale;
        pScaleSet->param_i[0] = expandTime + T; // 一斉始動のタイミング
        pScaleSet->param_d[0] = v; // 再始動時の速度
        pScaleSet->alive = 999;

        pScaleSet->pEnemyShotHead = new sEnemyShot;
        pScaleSet->pEnemyShotHead->prev = pScaleSet->pEnemyShotHead;
        pScaleSet->pEnemyShotHead->next = pScaleSet->pEnemyShotHead;

        pScaleSet->prev = enemyShotSetHead.prev;
        pScaleSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pScaleSet;
        enemyShotSetHead.prev = pScaleSet;

        // 64ビットのポインタ値を符号なし64ビット整数として取得
        unsigned long long ptr = (unsigned long long)pScaleSet;

        // 下位32ビットと上位32ビットをそれぞれ別のparam_iに保存
        pEnemyShotSet->param_i[14] = (int)(ptr & 0xFFFFFFFF);
        pEnemyShotSet->param_i[15] = (int)((ptr >> 32) & 0xFFFFFFFF);
    }

    // unsigned intにキャストして負の数の符号拡張を防ぎつつ、64ビットに結合
    unsigned long long ptr = ((unsigned long long)(unsigned int)pEnemyShotSet->param_i[14]) |
        (((unsigned long long)(unsigned int)pEnemyShotSet->param_i[15]) << 32);

    // ポインタ型に戻す
    sEnemyShotSet* pScaleSet = (sEnemyShotSet*)ptr;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 中心座標 (240, 240)
        double cx = 240.0;
        double cy = 240.0;

        // 半径の計算と更新
        double r = sqrt((pShot->x - cx) * (pShot->x - cx) + (pShot->y - cy) * (pShot->y - cy));
        if (pEnemyShotSet->count < expandTime) {
            r += pShot->speed;
        }

        // 角度の更新
        pShot->muki += a;

        // 新しい座標
        pShot->x = cx + r * cos(pShot->muki);
        pShot->y = cy + r * sin(pShot->muki);

        // 鱗弾の射出フェーズ
        if (pEnemyShotSet->count >= expandTime && pEnemyShotSet->count < expandTime + T && pEnemyShotSet->count % 2 == 0) {
            int fireCount = pEnemyShotSet->count - expandTime;
            int cycle = fireCount % (t1 + t2);

            if (cycle < t1) {
                // 撃つタイミング
                if (cycle == 0) {
                    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                }

                // ズレの計算（最初は -th0、最後は th0）
                double shift = -th0 + (th0 * 2.0 * cycle) / (t1 > 1 ? (t1 - 1) : 1);

                // 内側への発射角度 (中心へ向かう角度 + shift)
                double angleIn = atan2(cy - pShot->y, cx - pShot->x) + shift;
                // 外側への発射角度
                double angleOut = atan2(pShot->y - cy, pShot->x - cx) + shift;

                // 停止距離の計算
                double stopDist = l0 + l * fireCount;

                // --- 内側への鱗弾を1個生成 ---
                sEnemyShot* scaleIn = new sEnemyShot;
                scaleIn->x = pShot->x;
                scaleIn->y = pShot->y;
                scaleIn->muki = angleIn;
                scaleIn->speed = v;
                scaleIn->kind = img_enemyShotScale[pShot->param_i[1]]; // 色
                scaleIn->param_i[0] = 1; // 内側向け(停止する)
                scaleIn->param_d[0] = stopDist; // 停止距離
                scaleIn->param_d[1] = 0.0; // 移動距離

                scaleIn->prev = pScaleSet->pEnemyShotHead->prev;
                scaleIn->next = pScaleSet->pEnemyShotHead;
                pScaleSet->pEnemyShotHead->prev->next = scaleIn;
                pScaleSet->pEnemyShotHead->prev = scaleIn;

                // --- 外側への鱗弾を3個生成 ---
                for (int j = 0; j < 3; j++) {
                    sEnemyShot* scaleOut = new sEnemyShot;
                    scaleOut->x = pShot->x;
                    scaleOut->y = pShot->y;
                    // 外側3WAY (少し角度をずらす)
                    scaleOut->muki = angleOut + (j - 1) * 0.2;
                    scaleOut->speed = v * 0.3; // 低速
                    scaleOut->kind = img_enemyShotScale[pShot->param_i[1]];
                    scaleOut->param_i[0] = 0; // 外側向け(止まらない)

                    scaleOut->prev = pScaleSet->pEnemyShotHead->prev;
                    scaleOut->next = pScaleSet->pEnemyShotHead;
                    pScaleSet->pEnemyShotHead->prev->next = scaleOut;
                    pScaleSet->pEnemyShotHead->prev = scaleOut;
                }
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_DanmakuKekkai_Gemini()
{
    static int phase;
    static int phase_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 240.0; // 画面中央に配置
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        phase_count = 0;
    }

    // 1周期を 120(展開) + T(射出) + 120(余韻) フレームとする
    // ここでは初期設定として T = 180 と仮定し、全体を 420 フレームで回す
    int T_base = 180;
    int cycle_length = 120 + T_base + 120;

    if (phase_count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBarrier_LargeBall;

        // 周期ごとにパラメータを変化
        pEnemyShotSet->param_i[0] = 3 + phase;         // k: 大玉の数(片色)
        pEnemyShotSet->param_i[1] = 6 + phase * 2;     // t1: 連続射出フレーム
        pEnemyShotSet->param_i[2] = 10;                // t2: 待機フレーム
        pEnemyShotSet->param_i[3] = T_base;            // T: 総射出時間

        pEnemyShotSet->param_d[0] = 0.01 + phase * 0.002; // a: 角速度
        pEnemyShotSet->param_d[1] = 2.0 + phase * 0.2;    // v: 内側鱗弾の速さ
        pEnemyShotSet->param_d[2] = 0.3 + phase * 0.1;    // th0: ズレ角度
        pEnemyShotSet->param_d[3] = 40.0;                 // l0: 最初の停止距離
        pEnemyShotSet->param_d[4] = 0.5;                  // l: 停止距離の増加量

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    phase_count++;
    if (phase_count >= cycle_length) {
        phase_count = 0;
        phase++;
        // 難易度上限（パラメータが上がりすぎないように）
        if (phase > 4) phase = 4;
    }
}