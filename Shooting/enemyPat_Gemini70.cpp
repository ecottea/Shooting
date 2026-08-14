// enemyPat_Chladni.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// クラドニ図形のポテンシャル関数（値が0に近いほど「節」になる）
// fabsの代わりに2乗にして、勾配計算時の微分連続性を高めています
static double GetChladniPotentialSq(double x, double y, int n, int m) {
    double L = 480.0;
    double px = DX_PI * x / L;
    double py = DX_PI * y / L;
    double v1 = cos(n * px) * cos(m * py);
    double v2 = cos(m * px) * cos(n * py);
    return (v1 - v2) * (v1 - v2);
}

// 弾幕：共鳴幾何：音叉の檻（Resonant Chladni Cell）
static void ShotChladniCell(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;

    // ----------------------------------------------------
    // フェーズ進行と弾の生成・パラメータ更新
    // ----------------------------------------------------

    // [Phase 0] 0～119フレーム : 砂弾の散布
    if (t < 120) {
        // 音が重なりすぎないよう間引いて再生
        if (t % 4 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 毎フレーム15発ずつ、計1800発の弾を生成
        for (int i = 0; i < 15; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 全方位にランダム発射
            double angle = GetRand(3600) / 1800.0 * DX_PI;
            double speed = (200 + GetRand(1500)) / 100.0; // 初速 2.0 ～ 6.0

            // X,Y軸の速度成分としてparam_d[0], param_d[1]を利用
            pEnemyShot->param_d[0] = speed * cos(angle);
            pEnemyShot->param_d[1] = speed * sin(angle);

            // 白とシアンを混ぜてクラドニの砂を表現
            pEnemyShot->kind = (GetRand(3) == 0) ? img_enemyShotSmallBall[3] : img_enemyShotSmallBall[6];
            pEnemyShot->margin = 240;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // フェーズ・モード切り替えタイミング
    if (t == 120 || t == 300 || t == 480) {
        if (t == 120) {
            pEnemyShotSet->param_i[0] = 1; // Phase 1
            pEnemyShotSet->param_i[1] = 1 + GetRand(3); // n = 1
            pEnemyShotSet->param_i[2] = 1 + GetRand(5); // m = 2
        }
        else if (t == 300) {
            pEnemyShotSet->param_i[0] = 2; // Phase 2
            pEnemyShotSet->param_i[1] = 1 + GetRand(3); // n = 2
            pEnemyShotSet->param_i[2] = 1 + GetRand(5); // m = 3
        }
        else if (t == 480) {
            pEnemyShotSet->param_i[0] = 3; // Phase 3
            pEnemyShotSet->param_i[1] = 1 + GetRand(3); // n = 3
            pEnemyShotSet->param_i[2] = 1 + GetRand(5); // m = 5
        }

        // 周波数変化（モード遷移）のSE
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 弾を「跳ねさせる」ことで再配列前の砂の浮遊感を出す
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            double angle = GetRand(3600) / 1800.0 * DX_PI;
            double burstSpeed = (100 + GetRand(200)) / 100.0;
            pShot->param_d[0] += burstSpeed * cos(angle);
            pShot->param_d[1] += burstSpeed * sin(angle);
            pShot = pShot->next;
        }
    }

    // [Phase 4] パターン解散
    if (t == 660) {
        pEnemyShotSet->param_i[0] = 4;
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }


    // ----------------------------------------------------
    // 各弾の物理挙動の更新
    // ----------------------------------------------------

    int phase = pEnemyShotSet->param_i[0];
    int n = pEnemyShotSet->param_i[1];
    int m = pEnemyShotSet->param_i[2];

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        if (phase == 0) {
            // 散布フェーズ：慣性で動きつつ徐々に減速
            pShot->param_d[0] *= 0.96;
            pShot->param_d[1] *= 0.96;
        }
        else if (phase >= 1 && phase <= 3) {
            // 共鳴フェーズ：クラドニ図形の「節（V=0）」へ向かって力を加える
            double eps = 1.0;
            // 周辺4方向のポテンシャルをサンプリング
            double v_right = GetChladniPotentialSq(pShot->x + eps, pShot->y, n, m);
            double v_left = GetChladniPotentialSq(pShot->x - eps, pShot->y, n, m);
            double v_down = GetChladniPotentialSq(pShot->x, pShot->y + eps, n, m);
            double v_up = GetChladniPotentialSq(pShot->x, pShot->y - eps, n, m);

            // 勾配（傾き）を計算
            double dx = (v_right - v_left) / (2.0 * eps);
            double dy = (v_down - v_up) / (2.0 * eps);
            double gradLen = sqrt(dx * dx + dy * dy);

            if (gradLen > 0.0001) {
                double force = 1.5; // 節へ引き寄せる強さ
                pShot->param_d[0] -= (dx / gradLen) * force;
                pShot->param_d[1] -= (dy / gradLen) * force;
            }

            // 微振動（砂のランダムウォーク）
            pShot->param_d[0] += (GetRand(100) - 50) / 100.0;
            pShot->param_d[1] += (GetRand(100) - 50) / 100.0;

            // 画面境界への反発（模様形成中に画面外に消えないようにするバリア）
            if (pShot->x < 10.0)  pShot->param_d[0] += 0.8;
            if (pShot->x > 470.0) pShot->param_d[0] -= 0.8;
            if (pShot->y < 10.0)  pShot->param_d[1] += 0.8;
            if (pShot->y > 470.0) pShot->param_d[1] -= 0.8;

            // 摩擦（速度減衰：これが無いと節を通り過ぎて永遠に振動してしまう）
            pShot->param_d[0] *= 0.82;
            pShot->param_d[1] *= 0.82;
        }
        else if (phase == 4) {
            // 解散フェーズ：画面中央から外側に向かって弾き飛ばす
            double dist_x = pShot->x - 240.0;
            double dist_y = pShot->y - 240.0;
            double angle = atan2(dist_y, dist_x);
            pShot->param_d[0] += cos(angle) * 0.05;
            pShot->param_d[1] += sin(angle) * 0.05;

            // 徐々に加速
            //pShot->param_d[0] *= 1.02;
            //pShot->param_d[1] *= 1.02;
        }

        // 実際の座標更新
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];

        // 当たり判定や描画システムのために、成分からspeedとmukiを逆算してセット
        pShot->speed = sqrt(pShot->param_d[0] * pShot->param_d[0] + pShot->param_d[1] * pShot->param_d[1]);
        if (pShot->speed > 0.001) {
            pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Chladni_Gemini()
{
    if (count == 1) {
        // ゲーム画面の中央やや上に配置
        enemy.x = 240.0;
        enemy.y = 140.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 敵をゆっくりと8の字に揺らし、振動源っぽい不気味さを出す
    enemy.x = 240.0 + sin(count * DX_PI / 180.0) * 15.0;
    enemy.y = 140.0 + sin(count * DX_PI / 90.0) * 5.0;

    // 800フレーム周期でクラドニ弾幕を発動
    if (count % 800 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotChladniCell;

        // 弾のばら撒き中心はボスの位置とする
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->param_i[0] = 0; // 初期フェーズ

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}