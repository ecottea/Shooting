// enemyPat_chladni.cpp
// クラドニ図形（Chladni Figures）をモチーフにした弾幕パターン
// 節線に沿った弾幕壁と、反節線を走る流動弾を組み合わせた3フェーズ構成

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// パターン1: 放射状節線（Radial Nodes）
// ボス中心から n 本の線上に小玉を等間隔に配置し、隣り合う線を逆方向に
// ゆるやかに回転させる。まるで振動平板の節線が生きているような演出。
// ------------------------------------------------------------
static void ShotChladniRadial(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // フェーズ開始の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        // 弾生成音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int n = pSet->param_i[0];      // 放射状の線の本数
        int layers = pSet->param_i[1]; // 各線上の弾の数
        double baseAngle = pSet->param_d[0];

        for (int i = 0; i < n; i++) {
            double angle = baseAngle + (2.0 * DX_PI / n) * i;
            for (int j = 1; j <= layers; j++) {
                sEnemyShot* p = new sEnemyShot;
                double dist = j * 28.0; // ボスからの距離
                p->x = pSet->x + dist * cos(angle);
                p->y = pSet->y + dist * sin(angle);
                p->muki = 0.0;
                p->speed = 0.0;
                // 小玉で細かい節線を表現。色は kind で変化。
                p->kind = img_enemyShotSmallBall[pSet->kind % 9];
                // param_d[0,1]: 回転中心, [2]: 距離, [3]: 角度, [4]: 角速度
                p->param_d[0] = pSet->x;
                p->param_d[1] = pSet->y;
                p->param_d[2] = dist;
                p->param_d[3] = angle;
                // 隣り合う線は逆回転させ、幾何学的な美しさと緊張感を出す
                p->param_d[4] = (i % 2 == 0 ? 0.006 : -0.006);
                p->margin = 240;

                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // 中心周りを回転（敵は固定位置の前提）
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->param_d[3] += p->param_d[4];
        double cx = p->param_d[0];
        double cy = p->param_d[1];
        double r = p->param_d[2];
        double a = p->param_d[3];
        p->x = cx + r * cos(a);
        p->y = cy + r * sin(a);
        p = p->next;
    }
}

// ------------------------------------------------------------
// パターン2: 同心円状節線（Concentric Nodes）
// 同心円状に小玉を配置。隣接する円は半ピッチずらし、
// 全体を脈動（拡縮）させながら回転する。
// ------------------------------------------------------------
static void ShotChladniConcentric(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int m = pSet->param_i[0];          // 同心円の数
        int numPerCircle = pSet->param_i[1]; // 円周上の弾数

        for (int i = 0; i < m; i++) {
            double baseR = 45.0 + i * 38.0; // 基準半径
            for (int j = 0; j < numPerCircle; j++) {
                sEnemyShot* p = new sEnemyShot;
                // 隣接する円は半ピッチずらして幾何学的な美しさを強調
                double angle = (2.0 * DX_PI / numPerCircle) * j + (i % 2) * (DX_PI / numPerCircle);
                p->x = pSet->x + baseR * cos(angle);
                p->y = pSet->y + baseR * sin(angle);
                p->muki = 0.0;
                p->speed = 0.0;
                // 色を放射状パターンとずらして対比を付ける
                p->kind = img_enemyShotSmallBall[(pSet->kind + 3) % 9];
                p->param_d[0] = pSet->x; // 中心X
                p->param_d[1] = pSet->y; // 中心Y
                p->param_d[2] = baseR;   // 基準半径
                p->param_d[3] = angle;   // 角度
                p->param_d[4] = 0.005;   // 全層共通の角速度
                p->margin = 240;

                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // 脈動（拡縮）+ 回転
    double pulse = sin(pSet->count * 0.04) * 10.0; // 脈動幅
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->param_d[3] += p->param_d[4];
        double cx = p->param_d[0];
        double cy = p->param_d[1];
        double r = p->param_d[2] + pulse;
        double a = p->param_d[3];
        p->x = cx + r * cos(a);
        p->y = cy + r * sin(a);
        p = p->next;
    }
}

// ------------------------------------------------------------
// パターン3: 反節線を走る流動弾（Spiral Flow）
// 節線の間（反節線）を縫うように、螺旋状に広がる弾を発射。
// 銃弾を使い、まるで振動エネルギーが伝わっていくような演出。
// ------------------------------------------------------------
static void ShotChladniFlow(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int num = pSet->param_i[0]; // 弾数
        double startAngle = pSet->param_d[0];

        for (int i = 0; i < num; i++) {
            sEnemyShot* p = new sEnemyShot;
            double angle = startAngle + (2.0 * DX_PI / num) * i;
            // ボス中心から少し離れた位置に生成
            p->x = pSet->x + 18.0 * cos(angle);
            p->y = pSet->y + 18.0 * sin(angle);
            p->muki = angle;
            p->speed = 1.2 + (i % 3) * 0.6; // 速度バリエーション
            // 銃弾で流れる感じを演出。弾ごとに色を変える。
            p->kind = img_enemyShotBullet[(pSet->kind + i) % 9];
            // param_d[0]: 螺旋角速度, [1]: 中心X, [2]: 中心Y, [3]: 現在半径
            p->param_d[0] = 0.012;
            p->param_d[1] = pSet->x;
            p->param_d[2] = pSet->y;
            p->param_d[3] = 18.0;
            p->margin = 240;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // 螺旋移動（極座標で管理）
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double dx = p->x - p->param_d[1];
        double dy = p->y - p->param_d[2];
        double currentAngle = atan2(dy, dx);
        double currentR = p->param_d[3];

        // 外側へ螺旋
        currentR += p->speed;
        currentAngle += p->param_d[0];
        p->param_d[3] = currentR;

        p->x = p->param_d[1] + currentR * cos(currentAngle);
        p->y = p->param_d[2] + currentR * sin(currentAngle);
        p->muki = currentAngle;

        p = p->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// 3フェーズを約3.5秒ごとに循環させる。
// 敵は画面中央上部に固定し、クラドニ図形の中心安定性を演出。
// ------------------------------------------------------------
void EnemyPat_Chladni_Kimi()
{
    static int phase;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        shot_count = 0;
    }
    // 敵は固定（クラドニ図形の中心は安定しているイメージ）

    // 約3.5秒（210フレーム）ごとにフェーズ切り替え
    if (count % 210 == 1) {
        phase = (phase + 1) % 3;

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->kind = shot_count++;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        switch (phase) {
        case 0: // 放射状節線: 8本の線、各6個の小玉
            pSet->patternFunc = ShotChladniRadial;
            pSet->param_i[0] = 8;  // n: 放射状の線の本数
            pSet->param_i[1] = 12;  // layers: 各線上の弾数
            // GetRand(360) は 0〜360 の整数を返す
            pSet->param_d[0] = GetRand(360) / 180.0 * DX_PI;
            break;

        case 1: // 同心円状節線: 4つの円、各20個の小玉
            pSet->patternFunc = ShotChladniConcentric;
            pSet->param_i[0] = 6;  // m: 同心円の数
            pSet->param_i[1] = 21; // 円周上の弾数
            break;

        case 2: // 反節線流動弾: 20発の螺旋銃弾
            pSet->patternFunc = ShotChladniFlow;
            pSet->param_i[0] = 60; // 弾数
            pSet->param_d[0] = GetRand(360) / 180.0 * DX_PI;
            break;
        }

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}