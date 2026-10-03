#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：屈折偏光「スペクトル・レフレクション」
static void ShotPattern_SpectrumReflection(sEnemyShotSet* pEnemyShotSet)
{
    // プリズムの固定座標とサイズ
    const double PRISM_X = 240.0;
    const double PRISM_Y = 160.0;
    const double PRISM_R = 60.0;
    const int CYCLE = 100;

    int t = pEnemyShotSet->count % CYCLE;

    // 初期化：プリズム枠の生成 (count == 0 の時のみ)
    if (pEnemyShotSet->count == 0) {
        for (int edge = 0; edge < 3; edge++) {
            // 上向きの正三角形になるよう頂点を計算
            double a1 = edge * 2.0 * DX_PI / 3.0 - DX_PI / 2.0;
            double a2 = (edge + 1) * 2.0 * DX_PI / 3.0 - DX_PI / 2.0;
            double px1 = PRISM_R * cos(a1), py1 = PRISM_R * sin(a1);
            double px2 = PRISM_R * cos(a2), py2 = PRISM_R * sin(a2);

            int N = 8; // 1辺あたりの弾数
            for (int i = 0; i < N; i++) {
                double r_t = (double)i / N;
                double lx = px1 * (1.0 - r_t) + px2 * r_t;
                double ly = py1 * (1.0 - r_t) + py2 * r_t;

                sEnemyShot* pEnemyShot = new sEnemyShot;
                // 白とシアンを交互に配置
                pEnemyShot->kind = (i % 2 == 0) ? img_enemyShotSmallBall[6] : img_enemyShotSmallBall[3];
                pEnemyShot->param_i[0] = 0; // 役割0: プリズム枠
                pEnemyShot->param_d[0] = sqrt(lx * lx + ly * ly); // 中心からの距離
                pEnemyShot->param_d[1] = atan2(ly, lx);           // 中心からの初期角度
                pEnemyShot->speed = 0.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // プリズムの自転角度を更新
    pEnemyShotSet->param_d[2] += 0.01;

    // 予告音
    if (t == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 入射光の発射 (敵本体からプリズム中心へ)
    if (t >= 60-10 && t <= 76+10 && t % 4 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = enemy.x;
        pEnemyShot->y = enemy.y;
        pEnemyShot->muki = atan2(PRISM_Y - enemy.y, PRISM_X - enemy.x);
        pEnemyShot->speed = 12.0;
        pEnemyShot->kind = img_enemyShotLaser[6]; // 白の短レーザー
        pEnemyShot->param_i[0] = 1; // 役割1: 入射光

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 屈折・分光の発射 (プリズム中心から扇状に)
    // 入射光が約10フレームで到達するため、タイミングをずらして発射
    if (t >= 70-10 && t <= 86+10 && t % 4 == 2) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 7色: 赤, 橙, 黄, 緑, シアン, 青, マゼンタ
        int colors[] = { 0, 8, 1, 2, 3, 4, 5 };
        // 弾速: 波長が長い(赤)ほど速く、短い(紫)ほど遅い
        double speeds[] = { 4.5, 4.0, 3.5, 3.0, 2.5, 2.0, 1.5 };
        // 屈折角: 波長が長いほど直進に近く、短いほど大きく曲がる
        double angle_offsets[] = { 0.1, 0.25, 0.4, 0.6, 0.85, 1.1, 1.4 };

        // 基準角度: 真下(DX_PI/2)を中心に、プリズムの自転に影響されて揺れる
        double base_angle = DX_PI / 2.0 + sin(pEnemyShotSet->param_d[2]) * 0.8;

        for (int i = 0; i < 7; i++) {
            for (int dir = -1; dir <= 1; dir += 2) { // 左右対称
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = PRISM_X;
                pEnemyShot->y = PRISM_Y;

                // 角度と速度に微小なブレ(GetRand)を加えて光の散乱を表現
                double rand_angle = (GetRand(20) - 10) / 1000.0;
                pEnemyShot->muki = base_angle + angle_offsets[i] * dir + rand_angle;
                pEnemyShot->speed = speeds[i] + (GetRand(10) - 5) / 100.0;

                pEnemyShot->kind = img_enemyShotMediumBall[colors[i]]; // 中丸弾
                pEnemyShot->param_i[0] = 2; // 役割2: 分光弾

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 生成済みの弾の挙動更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // プリズム枠: 中心座標と自転角度から位置を計算して配置し続ける
            double r = pShot->param_d[0];
            double th = pShot->param_d[1] + pEnemyShotSet->param_d[2];
            pShot->x = PRISM_X + r * cos(th);
            pShot->y = PRISM_Y + r * sin(th);
        }
        else if (pShot->param_i[0] == 1) {
            // 入射光: 直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // プリズム中心(PRISM_X, PRISM_Y)に到達したら消去（画面外へ飛ばす）
            double dx = pShot->x - PRISM_X;
            double dy = pShot->y - PRISM_Y;
            if (dx * dx + dy * dy < 200.0) { // 距離が約14以下になったら
                pShot->x = -9999.0;
            }
        }
        else if (pShot->param_i[0] == 2) {
            // 分光弾: 直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Prism_Gemini()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 変更不可
        muki = 1;

        // スペクトル・レフレクションの弾幕セットを登録
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPattern_SpectrumReflection;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵は画面上部をゆっくり左右に往復する
        enemy.x += 0.4 * (double)muki;
        if (count % 240 == 120) muki *= -1;
    }
}
