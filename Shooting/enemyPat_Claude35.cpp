// enemyPat_sumiNagashiWinder.cpp
// 墨流し(すみながし)ワインダー
// 画面上部に6本の発射点(=筋)を扇状に配置し、各発射点が連続的に弾を撃ちながら
// 発射角度をサイン波で揺らすことで、鞭のようにうねる6本の"墨の筋"を描く。
// 展開後は6筋の発射点自体が渦の中心へ螺旋状に収束し、最後に放射状の一斉放出で締める。
//
// フェーズ構成 (pEnemyShotSet->count = t として管理)
//   t <  180            : 導入   振幅小さめ、まっすぐ下向き基調
//   180 <= t <  420      : 展開   振幅を拡大、筋同士が交差し始める
//   420 <= t <  600      : 渦     発射点自体が渦の中心へ螺旋状に収束、振幅は再び縮小
//   t == 600             : フィナーレ 放射状に一斉放出(菊の花)
//   t >  600             : 新規発射は行わず、既存弾の移動のみ

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：墨流しワインダー(1本の"筋"の連続発射 + 渦収束 + フィナーレ放射)
static void ShotWinderStream(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    const int    k = pEnemyShotSet->param_i[0];  // 筋番号(0～5)
    const double phaseK = pEnemyShotSet->param_d[0];  // この筋固有の位相ズレ
    const double periodK = pEnemyShotSet->param_d[1];  // この筋固有の周期(フレーム)
    const int    t = pEnemyShotSet->count;       // このセット生成からの経過フレーム

    const double VORTEX_X = 240.0;  // 渦の収束先(画面はおそらく480x480想定)
    const double VORTEX_Y = 320.0 - 30;

    const int P1_END = 180; // 導入終了
    const int P2_END = 420; // 展開終了 / 渦フェーズ開始
    const int P3_END = 600; // 渦フェーズ終了 / フィナーレ

    // ---- フェーズ3(渦)：発射点自体を渦の中心へ螺旋状に引き込む ----
    if (t >= P2_END && t < P3_END) {
        if (pEnemyShotSet->param_i[1] == 0) {
            // 渦フェーズ突入時点の半径・角度を記録しておく
            double dx = pEnemyShotSet->x - VORTEX_X;
            double dy = pEnemyShotSet->y - VORTEX_Y;
            pEnemyShotSet->param_d[2] = sqrt(dx * dx + dy * dy); // r0
            pEnemyShotSet->param_d[3] = atan2(dy, dx);           // theta0
            pEnemyShotSet->param_i[1] = 1;
        }

        double p = (double)(t - P2_END) / (double)(P3_END - P2_END); // 0.0～1.0
        double r0 = pEnemyShotSet->param_d[2];
        double theta0 = pEnemyShotSet->param_d[3];
        double radius = r0 * (1.0 - 0.7 * p);                 // 半径を70%まで縮める
        double theta = theta0 + p * (2.0 * DX_PI * 1.3);     // 約1.3回転しながら収束

        pEnemyShotSet->x = VORTEX_X + radius * cos(theta);
        pEnemyShotSet->y = VORTEX_Y + radius * sin(theta);
    }

    // ---- フィナーレ：渦の収束が終わった瞬間に放射状の一斉放出(菊の花) ----
    if (t == P3_END) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int BURST_NUM = 8;
        for (int j = 0; j < BURST_NUM; j++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 6筋 x 8発 = 48方向で均等に分散させる
            pEnemyShot->muki = j * (2.0 * DX_PI / BURST_NUM) + k * (2.0 * DX_PI / (BURST_NUM * 6));
            pEnemyShot->speed = 3.3;
            pEnemyShot->kind = img_enemyShotSmallBall[(k * BURST_NUM + j) % 6]; // 虹色に

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---- フェーズ1～3：ワインダー本体(連続発射、角度をサイン波で揺らす) ----
    if (t < P3_END && t % 3 == 0) {
        double amp;
        double centerAngle;

        if (t < P1_END) {
            double p = (double)t / (double)P1_END;
            amp = 0.10 + (0.35 - 0.10) * p;   // 振幅: 緩やかに拡大
            centerAngle = DX_PI / 2.0;        // 真下基調
        }
        else if (t < P2_END) {
            double p = (double)(t - P1_END) / (double)(P2_END - P1_END);
            amp = 0.35 + (0.75 - 0.35) * p;   // 振幅: さらに拡大、筋が交差し始める
            centerAngle = DX_PI / 2.0;
        }
        else {
            double p = (double)(t - P2_END) / (double)(P3_END - P2_END);
            amp = 0.75 + (0.15 - 0.75) * p;   // 振幅: 渦の収束に伴い縮小
            centerAngle = atan2(VORTEX_Y - pEnemyShotSet->y, VORTEX_X - pEnemyShotSet->x);
        }

        double muki = centerAngle + amp * sin(2.0 * DX_PI * t / periodK + phaseK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = muki;
        pEnemyShot->speed = 2.3 + 0.05 * k;
        // 筋ごとに藍(青)と臙脂(赤)を交互に割り当て
        pEnemyShot->kind = (k % 2 == 0) ? img_enemyShotScale[4] : img_enemyShotScale[0];

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        if (t % 5 == 0) {
            if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // ---- 既存弾の移動 ----
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Winder_Claude()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480 想定
        enemy.x = 240.0;
        enemy.y = 90.0 - 30;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;        
    }
    else {
        // 本体はゆったり左右に揺れるだけ
        enemy.x += 0.5 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    if (count % 840 == 40) {
        // 画面上部に6本の発射点を扇状配置(中央を密に、両端を疎に)
        static const double offsetX[6] = { -190.0, -100.0, -35.0, 35.0, 100.0, 190.0 };

        for (int k = 0; k < 6; k++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotWinderStream;
            pEnemyShotSet->x = enemy.x + offsetX[k];
            pEnemyShotSet->y = 40.0 - 30;
            pEnemyShotSet->muki = DX_PI / 2.0;
            pEnemyShotSet->kind = k;

            pEnemyShotSet->param_i[0] = k; // 筋番号
            pEnemyShotSet->param_i[1] = 0; // 渦フェーズ初期化済みフラグ
            // 完全な対称にならないよう位相・周期にわずかな乱数ズレを持たせる
            pEnemyShotSet->param_d[0] = k * (2.0 * DX_PI / 6.0) + (GetRand(40) - 20) / 100.0;
            pEnemyShotSet->param_d[1] = 200.0 + k * 6.0 + GetRand(10);

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}