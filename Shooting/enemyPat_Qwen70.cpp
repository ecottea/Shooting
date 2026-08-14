#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <vector>
#include <algorithm>

// ------------------------------------------------------------
// クラドニ図形弾幕用データ
// ------------------------------------------------------------
struct ChladniNode {
    double x, y;       // 目標座標
    double nx, ny;     // 法線ベクトル（崩壊時の飛散方向用）
};

static std::vector<ChladniNode> g_nodes;
static std::vector<int>         g_nodeIndices;

// 節線ターゲットの初期化
// 画面サイズ 480x480 を [-1, 1] の正規化座標に見立てて節線(F=0)を探索する
static void InitChladniNodes() {
    g_nodes.clear();
    g_nodeIndices.clear();

    const int M = 5;          // クラドニ図形モード m
    const int N = 3;          // クラドニ図形モード n
    const double EPSILON = 0.06;
    const int RES = 80;       // 走査解像度

    for (int i = 0; i <= RES; ++i) {
        for (int j = 0; j <= RES; ++j) {
            double u = -1.0 + 2.0 * i / RES;
            double v = -1.0 + 2.0 * j / RES;

            // F(u, v) = cos(m*pi*u)*cos(n*pi*v) - cos(n*pi*u)*cos(m*pi*v)
            double val = cos(M * DX_PI * u) * cos(N * DX_PI * v)
                - cos(N * DX_PI * u) * cos(M * DX_PI * v);

            if (fabs(val) < EPSILON) {
                // 画面座標へ変換
                double x = 240.0 + u * 240.0;
                double y = 240.0 + v * 240.0;

                // 勾配(法線)を数値微分で計算
                double h = 0.01;
                double vx1 = cos(M * DX_PI * (u + h)) * cos(N * DX_PI * v) - cos(N * DX_PI * (u + h)) * cos(M * DX_PI * v);
                double vx2 = cos(M * DX_PI * (u - h)) * cos(N * DX_PI * v) - cos(N * DX_PI * (u - h)) * cos(M * DX_PI * v);
                double vy1 = cos(M * DX_PI * u) * cos(N * DX_PI * (v + h)) - cos(N * DX_PI * u) * cos(M * DX_PI * (v + h));
                double vy2 = cos(M * DX_PI * u) * cos(N * DX_PI * (v - h)) - cos(N * DX_PI * u) * cos(M * DX_PI * (v - h));

                double dx = (vx1 - vx2) / (2 * h);
                double dy = (vy1 - vy2) / (2 * h);
                double len = sqrt(dx * dx + dy * dy);
                if (len > 0) { dx /= len; dy /= len; }

                g_nodes.push_back({ x, y, dx, dy });
            }
        }
    }

    // ターゲットインデックスリスト作成とシャッフル
    for (size_t i = 0; i < g_nodes.size(); ++i) g_nodeIndices.push_back((int)i);

    // 再現性のあるシャッフル (Fisher-Yates)
    for (int i = (int)g_nodeIndices.size() - 1; i > 0; --i) {
        int j = GetRand(i);
        std::swap(g_nodeIndices[i], g_nodeIndices[j]);
    }
}

// ------------------------------------------------------------
// 弾幕パターン関数
// ------------------------------------------------------------
const int T = 500;
static void ShotChladni(sEnemyShotSet* pSet) {
    int t = pSet->count;

    // Phase 1: 生成・収束 (t = 0〜179)
    if (t >= 0 && t < 180) {
        // 4フレームに1回生成
        if (t % 4 == 0) for (int j = 0; j < 6; j++) {
            // ターゲットが残っていれば消費、なければ補充
            if (g_nodeIndices.empty()) {
                for (size_t i = 0; i < g_nodes.size(); ++i) g_nodeIndices.push_back((int)i);
                for (int i = (int)g_nodeIndices.size() - 1; i > 0; --i) {
                    int j = GetRand(i);
                    std::swap(g_nodeIndices[i], g_nodeIndices[j]);
                }
            }

            int idx = g_nodeIndices.back();
            g_nodeIndices.pop_back();
            ChladniNode target = g_nodes[idx];

            sEnemyShot* pShot = new sEnemyShot;

            // 初期位置: 画面外ランダム
            int side = GetRand(3);
            if (side == 0) { pShot->x = GetRand(480); pShot->y = -20.0; }
            else if (side == 1) { pShot->x = GetRand(480); pShot->y = 500.0; }
            else if (side == 2) { pShot->x = -20.0; pShot->y = GetRand(480); }
            else { pShot->x = 500.0; pShot->y = GetRand(480); }

            pShot->muki = atan2(target.y - pShot->y, target.x - pShot->x);
            pShot->speed = 1.5; // 収束速度

            // ターゲット情報をパラメータに保存
            pShot->param_d[0] = target.x;
            pShot->param_d[1] = target.y;
            pShot->param_d[2] = target.nx;
            pShot->param_d[3] = target.ny;
            pShot->param_d[4] = GetRand(360) / 180.0 * DX_PI; // 振動位相オフセット

            pShot->param_i[0] = 0; // State: 0=Converging, 1=Vibrating, 2=Scattering

            // [素材] 砂粒：小玉(白)
            pShot->kind = img_enemyShotSmallBall[6];

            // リスト接続
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;

            // 予兆音 (最初のみ)
            if (t == 0) {
                if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            }
        }
    }
    // Phase 3: 崩壊トリガー (t = 240)
    else if (t == T) {
        // [素材] 崩壊音：重めのショット音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 弾の更新
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int state = pShot->param_i[0];

        if (state == 0) {
            // 収束フェーズ
            double tx = pShot->param_d[0];
            double ty = pShot->param_d[1];
            double dx = tx - pShot->x;
            double dy = ty - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist < 3.0) {
                // 到達 -> 振動モードへ
                pShot->param_i[0] = 1;
                pShot->x = tx;
                pShot->y = ty;
                // [素材] 節線：菱形(シアン)
                pShot->kind = img_enemyShotDiamond[3];
            }
            else {
                // ターゲットへ直進
                pShot->muki = atan2(dy, dx);
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else if (state == 1) {
            // 共振フェーズ
            if (t >= T) {
                // 崩壊フェーズへ移行
                pShot->param_i[0] = 2;

                // 飛散方向計算 (法線 + 外向きバイアス)
                double nx = pShot->param_d[2];
                double ny = pShot->param_d[3];
                double outX = pShot->x - 240.0;
                double outY = pShot->y - 240.0;
                double lenOut = sqrt(outX * outX + outY * outY);
                if (lenOut > 0) { outX /= lenOut; outY /= lenOut; }

                double dirX = nx * 0.4 + outX * 0.6;
                double dirY = ny * 0.4 + outY * 0.6;
                double lenDir = sqrt(dirX * dirX + dirY * dirY);
                if (lenDir > 0) { dirX /= lenDir; dirY /= lenDir; }

                // ランダム散布
                double angle = atan2(dirY, dirX) + (GetRand(100) - 50) / 100.0 * 0.3;

                pShot->muki = angle;
                pShot->speed = 2.5 + GetRand(30) / 10.0;

                // [素材] 破片：中玉(橙)
                pShot->kind = img_enemyShotMediumBall[8];
            }
            else {
                // 振動
                double baseX = pShot->param_d[0];
                double baseY = pShot->param_d[1];
                double phase = pShot->param_d[4] + t * 0.2;
                double amp = 3.0;

                // 接線方向に振動させることで「図形が揺れている」ように見せる
                double tx = -pShot->param_d[3];
                double ty = pShot->param_d[2];

                pShot->x = baseX + tx * sin(phase) * amp;
                pShot->y = baseY + ty * sin(phase) * amp;
            }
        }
        else if (state == 2) {
            // 飛散フェーズ
            // 直進運動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Chladni_Qwen()
{
    if (count == 1) {
        // 敵初期化
        enemy.x = 240.0;
        enemy.y = 240.0; // 画面中央
        enemy.maxHp = enemy.hp = 200;

        // 節線データ初期化
        InitChladniNodes();
    }

    // 敵の微動
    enemy.x = 240.0 + sin(count * 0.015) * 10.0;
    enemy.y = 240.0 + cos(count * 0.015) * 10.0;

    // 弾幕セット生成 (1回のみ)
    if (count % (T + 60) == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotChladni;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}