// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：旋回する天球儀（スフィア・パースペクティブ）
static void ShotSpherePerspective(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double R = 120.0; // 天球儀の半径

    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 緯度方向（-90度から90度まで、30度刻みで7つのリングを形成）
        for (int i = -6; i <= 6; i++) {
            double theta = i * (DX_PI / 12.0);

            // 緯度円の大きさに応じて配置する弾数を決定
            int num;
            if (i == -6 || i == 6) {
                num = 1; // 両極は1発
            }
            else {
                num = (int)(36 * cos(theta)); // 赤道に近いほど弾数を増やす
            }

            for (int j = 0; j < num; j++) {
                pEnemyShot = new sEnemyShot;

                double phi = j * (DX_PI * 2.0 / num);

                // 3D極座標の初期パラメータを保存
                pEnemyShot->param_d[0] = theta; // 初期緯度
                pEnemyShot->param_d[1] = phi;   // 初期経度
                pEnemyShot->param_d[2] = R;     // 半径
                pEnemyShot->param_i[0] = 0;     // ステート（0: 回転中, 1: 拡散中）

                // 双方向循環リストへの追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 300フレーム目で弾け飛ぶ効果音
    if (pEnemyShotSet->count == pEnemyShotSet->param_i[0]) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        if (pEnemyShot->param_i[0] == 0) {
            // ==========================================
            // ステート0: 天球儀としての立体回転モード
            // ==========================================
            double theta = pEnemyShot->param_d[0];
            double phi = pEnemyShot->param_d[1];
            double r = pEnemyShot->param_d[2];

            // 1. Y軸周りの回転（天球儀自体の自転スピン）
            double alpha = pEnemyShotSet->count * 0.025;
            double phi_now = phi + alpha;

            double x0 = r * cos(theta) * cos(phi_now);
            double y0 = r * sin(theta);
            double z0 = r * cos(theta) * sin(phi_now);

            // 2. X軸周りの回転（奥へ傾けて、斜め上から見下ろす角度をつける）
            double beta = DX_PI / 7.0; // 約25度
            double y1 = y0 * cos(beta) - z0 * sin(beta);
            double z1 = y0 * sin(beta) + z0 * cos(beta);

            // 3. Z軸周りの回転（画面上で全体がゆっくり円を描くように回す）
            double gamma = pEnemyShotSet->count * 0.005;
            double x2 = x0 * cos(gamma) - y1 * sin(gamma);
            double y2 = x0 * sin(gamma) + y1 * cos(gamma);

            // 計算した2D座標を敵の中心座標に加算して適用
            pEnemyShot->x = pEnemyShotSet->x + x2;
            pEnemyShot->y = pEnemyShotSet->y + y2;

            // Z座標(奥行き)に応じて弾の大きさと色を変え、擬似的に立体感を表現
            // （手前が正、奥が負）
            if (z1 > 80.0) {
                pEnemyShot->kind = img_enemyShotMediumBall[6];  // シアン大玉 (手前)
            }
            else if (z1 > 0.0) {
                pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン中玉
            }
            else if (z1 > -50.0) {
                pEnemyShot->kind = img_enemyShotSmallBall[3];  // シアン小玉
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[4];  // 青小玉 (最奥は暗い色)
            }

            // 300フレーム経過で一斉に発射（拡散モードへ移行）
            if (pEnemyShotSet->count == pEnemyShotSet->param_i[0]) {
                pEnemyShot->param_i[0] = 1;

                // 現在位置から放射状に飛ぶよう向きを設定
                pEnemyShot->muki = atan2(y2, x2);

                // 発射時点のZ座標（奥行き）を元に速度を決定
                // 手前にあった弾は速く、奥にあった弾は遅く飛ぶことで、2D弾幕化しても立体的な時間差を維持する
                double speed_z = z1 / R; // -1.0 ～ 1.0 の割合
                pEnemyShot->speed = 2.5 + speed_z * 1.5; // 速度 1.0 ～ 4.0
            }

        }
        else {
            // ==========================================
            // ステート1: 2D全方位弾幕としての拡散モード
            // ==========================================
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_3D_Gemini()
{
    static int muki;

    if (count == 1) {
        // 初期化処理
        enemy.x = 240.0;
        enemy.y = 160.0; // 中央より少し上
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ボス本体はゆっくりと左右に揺れる
        enemy.x += 0.5 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }

    // 600フレーム周期で天球儀弾幕を発動
    if (count % 150 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpherePerspective;

        // 敵の現在座標を基準点とする
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->param_i[0] = 120 + GetRand(60);

        // 双方向循環リストのダミーヘッド生成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾幕セットリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}