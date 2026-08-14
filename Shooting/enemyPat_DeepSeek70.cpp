// enemyPat_chladni.cpp
// クラドニ図形をモチーフにした弾幕パターン
// ボスが画面中央に透明の響板を出現させ、砂粒弾が振動モードによって
// 節線（安全地帯）とそれ以外（危険地帯）を刻々と変化させる。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 振動モードの振幅を返す（節線上で 0 になるように設計）
static double chladniAmplitude(double rx, double ry, int phase, double half, int m, int n)
{
    switch (phase) {
    case 0: // 砂粒集め（節線なし）
        return 1.0;
    case 1: // 十字節（+） … rx*ry = 0 が節線
        return rx * ry;
    case 2: { // 対角線節（X） … (x+y)(x-y)=0 が節線
        double u = (rx + ry) / sqrt(2.0);
        double v = (ry - rx) / sqrt(2.0);
        return u * v;
    }
    case 3: // 花びら模様（円板：放射節＋同心円節）
    {
        double r = sqrt(rx * rx + ry * ry);
        if (r < 0.001) return 0.0;
        double theta = atan2(ry, rx);
        return sin(m * theta) * sin(n * DX_PI * r / half);
    }
    case 4: // 高次花びら（さらに複雑）
    {
        double r = sqrt(rx * rx + ry * ry);
        if (r < 0.001) return 0.0;
        double theta = atan2(ry, rx);
        return sin(m * theta) * sin(n * DX_PI * r / half);
    }
    case 5: // 崩壊フェーズ（節線が揺らぐ＋縮小）
    {
        double r = sqrt(rx * rx + ry * ry);
        if (r < 0.001) return 0.0;
        double theta = atan2(ry, rx);
        // 時間変化と縮小を追加
        double decay = half * 0.7; // 仮のスケール
        return sin(m * theta + sin(r * 0.05) * 0.8) * sin(n * DX_PI * r / decay);
    }
    default:
        return 1.0;
    }
}

// 数値微分で節線の法線方向を求める
static void chladniGradient(double rx, double ry, int phase, double half, int m, int n,
    double* gx, double* gy)
{
    const double eps = 0.5;
    double a0 = chladniAmplitude(rx, ry, phase, half, m, n);
    double axp = chladniAmplitude(rx + eps, ry, phase, half, m, n);
    double axm = chladniAmplitude(rx - eps, ry, phase, half, m, n);
    double ayp = chladniAmplitude(rx, ry + eps, phase, half, m, n);
    double aym = chladniAmplitude(rx, ry - eps, phase, half, m, n);
    *gx = (axp - axm) / (2.0 * eps);
    *gy = (ayp - aym) / (2.0 * eps);
}

// 実際の弾幕処理
static void ShotChladni(sEnemyShotSet* pEnemyShotSet)
{
    double cx = pEnemyShotSet->x;
    double cy = pEnemyShotSet->y;
    double half = pEnemyShotSet->param_d[0];
    int& phase = pEnemyShotSet->param_i[0];
    int& phaseStart = pEnemyShotSet->param_i[1];
    int& m = pEnemyShotSet->param_i[2];   // モードパラメータ
    int& n = pEnemyShotSet->param_i[3];

    // 初期化（count == 0 のときだけ実行）
    if (pEnemyShotSet->count == 0) {
        half = 240.0;
        pEnemyShotSet->param_d[0] = half;
        phase = 0;
        phaseStart = 0;
        m = 3;
        n = 2;

        // 砂粒弾を 200 個生成（オレンジ色の小玉）
        for (int i = 0; i < 200*5; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            // 響板（正方形）内にランダム配置
            while (true) {
                double rx = (GetRand(480) - 240);
                double ry = (GetRand(480) - 240);
                pShot->x = cx + rx;
                pShot->y = cy + ry;
                if (hypot(pShot->x - player.x, pShot->y - player.y) > 30) break;
            }

            // 初速はランダム（小刻みに震える）
            pShot->muki = GetRand(360) / 180.0 * DX_PI;
            pShot->speed = 0.5 + GetRand(10) / 10.0; // 0.5 ～ 1.5

            // 砂粒の見た目：小玉・橙
            pShot->kind = img_enemyShotSmallBall[8]; // 8:橙
            pShot->margin = 240;

            // リストに追加
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // 発生時の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        return;
    }

    // 毎フレームの経過時間（count 基準）
    int elapsed = pEnemyShotSet->count - phaseStart;

    // フェーズ遷移
    switch (phase) {
    case 0: // 砂粒が集まって震えている状態
        if (elapsed >= 90) { // 約1.5秒後
            phase = 1;
            phaseStart = pEnemyShotSet->count;
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        break;
    case 1: // 十字節（+）
        if (elapsed >= 180) { // 3秒
            phase = 2;
            phaseStart = pEnemyShotSet->count;
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        break;
    case 2: // 対角線節（X）
        if (elapsed >= 180) {
            phase = 3;
            phaseStart = pEnemyShotSet->count;
            m = 3;
            n = 2;
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        break;
    case 3: // 花びら模様１
        if (elapsed >= 180) {
            phase = 4;
            phaseStart = pEnemyShotSet->count;
            m = 5;
            n = 3;
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        break;
    case 4: // 花びら模様２
        if (elapsed >= 180) {
            phase = 5;
            phaseStart = pEnemyShotSet->count;
            // 最終崩壊音
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        break;
    case 5: // 崩壊フェーズ
        if (elapsed >= 150) {
            // 終了：すべての砂粒弾を削除
            sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pShot != pEnemyShotSet->pEnemyShotHead) {
                sEnemyShot* next = pShot->next;
                delete pShot;
                pShot = next;
            }
            // 自身も削除予約（実際は外部から削除されるが安全のためリストを空に）
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
            // これ以上処理しない
            return;
        }
        break;
    }

    // 各弾の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // フェーズ0の間はランダムな速度でその場震えさせる
        if (phase == 0) {
            pShot->muki = GetRand(360) / 180.0 * DX_PI;
            pShot->speed = 0.5 + GetRand(10) / 50.0;
        }

        // 移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        double rx = pShot->x - cx;
        double ry = pShot->y - cy;

        // 板の境界で反射（正方形の間は正方形、円板モードでは円形）
        if (phase <= 2) {
            // 正方形板
            if (fabs(rx) > half) {
                pShot->x = cx + (rx > 0 ? half : -half);
                double vx = pShot->speed * cos(pShot->muki);
                double vy = pShot->speed * sin(pShot->muki);
                vx = -vx;
                pShot->muki = atan2(vy, vx);
                rx = pShot->x - cx;
            }
            if (fabs(ry) > half) {
                pShot->y = cy + (ry > 0 ? half : -half);
                double vx = pShot->speed * cos(pShot->muki);
                double vy = pShot->speed * sin(pShot->muki);
                vy = -vy;
                pShot->muki = atan2(vx, vy);
                ry = pShot->y - cy;
            }
        }
        else {
            // 円形板（半径 half）
            double r = sqrt(rx * rx + ry * ry);
            if (r > half) {
                // 円周上に戻す
                double nx = rx / r;
                double ny = ry / r;
                pShot->x = cx + nx * half;
                pShot->y = cy + ny * half;
                // 速度の半径方向成分を反転
                double vx = pShot->speed * cos(pShot->muki);
                double vy = pShot->speed * sin(pShot->muki);
                double vn = vx * nx + vy * ny;
                vx -= 2.0 * vn * nx;
                vy -= 2.0 * vn * ny;
                pShot->muki = atan2(vy, vx);
                rx = pShot->x - cx;
                ry = pShot->y - cy;
            }
        }

        // 節線（安全地帯）への侵入を反射で防ぐ（フェーズ1以降）
        if (phase >= 1) {
            double amp = chladniAmplitude(rx, ry, phase, half, m, n);
            double threshold = (phase <= 2) ? 400.0 : 100.0; // 節線の“太さ”
            
            if (fabs(amp) < threshold) {
                // 勾配（法線）を計算
                double gx, gy;
                chladniGradient(rx, ry, phase, half, m, n, &gx, &gy);
                gx /= 30;
                gy /= 30;
                double len = sqrt(gx * gx + gy * gy);
                if (len > 0.0005) {
                    double nx = gx / len;
                    double ny = gy / len;

                    // 速度を節線に対して反射
                    double vx = pShot->speed * cos(pShot->muki);
                    double vy = pShot->speed * sin(pShot->muki);
                    double dot = vx * nx + vy * ny;
                    vx -= 2.0 * dot * nx;
                    vy -= 2.0 * dot * ny;
                    pShot->muki = atan2(vy, vx);

                    // 節線の内側から少し押し出す
                    pShot->x += nx * 3.0;
                    pShot->y += ny * 3.0;
                }
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン（呼び出し名は EnemyPat_Chladni_DeepSeek に固定）
void EnemyPat_Chladni_DeepSeek()
{
    // 1フレーム目のみ初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 60フレーム目（約1秒後）にクラドニ弾幕セットを一つだけ生成
    if (count % 1000 == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotChladni;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // グローバルな弾幕リストに追加
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 敵本体はこれ以上動作しない（必要なし）
}