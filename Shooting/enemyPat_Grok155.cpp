#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  平安京の悪夢 - 基本パターン（縦方向グリッド・下向き赤レーザー）
// ============================================================
// param_i[0] : N
// param_i[1] : phase (0:出現待ち, 1:移動中, 2:移動後待ち, 3:消去待ち, 4:終了)
// param_i[2] : 移動開始フレーム (count基準)
// param_i[3] : T (加速・減速時間)
// param_i[4] : transpose (0:通常, 1:x/y転置)
// param_d[0] : W (x方向間隔, 固定広め)
// param_d[1] : H (y方向間隔, 狭め)
// param_d[2] : margin
//
// 各弾の param_i[0]=i (列), param_i[1]=j (行=qN+r)
// param_i[2]=i2, param_i[3]=j2
// param_d[0]=startX, param_d[1]=startY
// param_d[2]=targetX, param_d[3]=targetY
// param_d[4]=startTime (移動開始時のpEnemyShotSet->count)

static void ShotHeiankyo(sEnemyShotSet* pEnemyShotSet)
{
    const int N = pEnemyShotSet->param_i[0];
    const int transpose = pEnemyShotSet->param_i[4];
    const double W = pEnemyShotSet->param_d[0];
    const double H = pEnemyShotSet->param_d[1];
    const double margin = pEnemyShotSet->param_d[2];
    const int T = pEnemyShotSet->param_i[3];

    // 出現グリッド原点 (margin, margin) ～ (480-margin, 480-margin) をカバー
    // x: 左から i=0.. 間隔W
    // y: 上から j=0.. 間隔H (下向き)
    auto gridX = [&](int i) -> double {
        return i * W;
    };
    auto gridY = [&](int j) -> double {
        return j * H;
    };

    if (pEnemyShotSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // P: 0..N-1 のランダム順列
        int P[32];
        for (int k = 0; k < N; k++) P[k] = k;
        for (int k = N - 1; k > 0; k--) {
            int r = GetRand(k);
            int tmp = P[k];
            P[k] = P[r];
            P[r] = tmp;
        }

        // R, D: 長さNのランダムビット列
        int R[32], D[32];
        for (int k = 0; k < N; k++) {
            R[k] = GetRand(1);
            D[k] = GetRand(1);
        }

        // Q[k] = (P[k] - k) mod N
        int Q[32];
        for (int k = 0; k < N; k++) {
            Q[k] = (P[k] - k) % N;
            if (Q[k] < 0) Q[k] += N;
        }

        // グリッドサイズ: (-60,-60)～(540,540) をカバーするよう
        // i,j を負の値から正の十分大きな値まで取る
        // margin=60 なので gridX(0)=60, 負のiで左外もカバー
        int iMin = -2;
        int iMax = (int)((540.0 - margin) / W) + 2;
        int jMin = -2;
        int jMax = (int)((540.0 - margin) / H) + 2;
        // Nの倍数分余分に確保して移動後も隙間なく見えるようにする
        if (iMax - iMin < N * 3) iMax = iMin + N * 3;
        if (jMax - jMin < N * 3) jMax = jMin + N * 3;

        // 赤レーザーを配置 (下向き / 転置時は右向き相当)
        for (int i = iMin; i <= iMax; i++) {
            for (int j = jMin; j <= jMax; j++) {
                sEnemyShot* p = new sEnemyShot;
                double gx = gridX(i);
                double gy = gridY(j);

                if (transpose) {
                    // x/y 転置: 出現位置を入れ替え、横並びのレーザー群になる
                    p->x = gy;
                    p->y = gx;
                    p->muki = 0.0; // 右向き（見た目用）
                }
                else {
                    p->x = gx;
                    p->y = gy;
                    p->muki = DX_PI / 2.0; // 下向き
                }
                p->speed = 0.0;
                p->kind = img_enemyShotLaser[0]; // 赤レーザー
                p->count = 0;

                // 移動先計算
                // 左から i 番目、上から j = qN + r 番目
                int r = ((j % N) + N) % N; // 負のjにも対応
                // R[r]=1 → i2=i+1, R[r]=0 → i2=i-1
                int i2 = (R[r] == 1) ? (i + 1) : (i - 1);
                // D[r]=1 → j2=j+Q[r], D[r]=0 → j2=j+Q[r]-N
                int j2 = (D[r] == 1) ? (j + Q[r]) : (j + Q[r] - N);

                p->param_i[0] = i;
                p->param_i[1] = j;
                p->param_i[2] = i2;
                p->param_i[3] = j2;

                if (transpose) {
                    // 転置時: 出現も移動先も x↔y 入れ替え
                    p->param_d[0] = p->x;           // startX
                    p->param_d[1] = p->y;           // startY
                    p->param_d[2] = gridY(j2);      // targetX
                    p->param_d[3] = gridX(i2);      // targetY
                }
                else {
                    p->param_d[0] = gx;
                    p->param_d[1] = gy;
                    p->param_d[2] = gridX(i2);
                    p->param_d[3] = gridY(j2);
                }
                p->param_d[4] = 0.0; // 移動開始時刻 (後で設定)
                p->margin = 160;

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }

        pEnemyShotSet->param_i[1] = 0; // phase = 出現待ち
        pEnemyShotSet->param_i[2] = 0; // 移動開始フレーム
    }

    int phase = pEnemyShotSet->param_i[1];
    int waitFrames = 60; // 1秒 (60fps想定)

    if (phase == 0) {
        // 出現後1秒待つ (速度0)
        if (pEnemyShotSet->count >= waitFrames) {
            pEnemyShotSet->param_i[1] = 1; // 移動開始
            pEnemyShotSet->param_i[2] = pEnemyShotSet->count;

            // 全弾を中玉に変更し、移動開始時刻を記録
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                p->kind = img_enemyShotMediumBall[0]; // 赤中玉
                p->param_d[4] = (double)pEnemyShotSet->count;
                p = p->next;
            }
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }
    else if (phase == 1) {
        // 移動中: T加速 + T減速 = 2T
        int elapsed = pEnemyShotSet->count - pEnemyShotSet->param_i[2];
        int totalMove = 2 * T;

        if (elapsed >= totalMove) {
            // 移動完了 → レーザーに戻す
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                p->x = p->param_d[2];
                p->y = p->param_d[3];
                p->speed = 0.0;
                p->kind = img_enemyShotLaser[0];
                p = p->next;
            }
            pEnemyShotSet->param_i[1] = 2; // 移動後待ち
            pEnemyShotSet->param_i[2] = pEnemyShotSet->count;
        }
        else {
            // イージング: 加速T + 減速T
            // 距離に対する進捗率 t (0～1)
            double progress;
            if (elapsed < T) {
                // 加速: 0.5 * (elapsed/T)^2
                double u = (double)elapsed / (double)T;
                progress = 0.5 * u * u;
            }
            else {
                // 減速: 0.5 + 0.5*(1 - ((2T-elapsed)/T)^2) 相当
                double u = (double)(elapsed - T) / (double)T;
                progress = 0.5 + u - 0.5 * u * u;
            }
            if (progress > 1.0) progress = 1.0;
            if (progress < 0.0) progress = 0.0;

            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                double sx = p->param_d[0];
                double sy = p->param_d[1];
                double tx = p->param_d[2];
                double ty = p->param_d[3];
                p->x = sx + (tx - sx) * progress;
                p->y = sy + (ty - sy) * progress;
                p->speed = 0.0; // 位置直接制御
                p = p->next;
            }
        }
    }
    else if (phase == 2) {
        // 移動後1秒待ち
        if (pEnemyShotSet->count - pEnemyShotSet->param_i[2] >= waitFrames) {
            pEnemyShotSet->param_i[1] = 3; // 消去
            pEnemyShotSet->param_i[2] = pEnemyShotSet->count;
        }
    }
    else if (phase == 3) {
        // 全レーザー消去して1秒待ち
        // 弾はメインルーチンで画面外消去される想定だが、
        // 明示的にリストから外す必要がある場合はここで処理。
        // 仕様「全レーザーを消去して1秒待ち」なので、弾を画面外に飛ばす
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            p->x = -9999.0;
            p->y = -9999.0;
            p = p->next;
        }
        if (pEnemyShotSet->count - pEnemyShotSet->param_i[2] >= waitFrames) {
            pEnemyShotSet->param_i[1] = 4; // 終了
        }
    }
    // phase==4 は何もしない（基本パターン終了）
}

// ============================================================
//  自機狙い大玉列（上下 or 左右から）
// ============================================================
// param_i[0] : direction (0:上下, 1:左右)
// param_i[1] : fromTopOrLeft (1:上/左, 0:下/右)
// param_d[0] : 目標座標 (自機のx or y)
// param_d[1] : 速度

static void ShotAimingLargeBalls(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int dir = pEnemyShotSet->param_i[0];
        const int fromPositive = pEnemyShotSet->param_i[1];
        const double target = pEnemyShotSet->param_d[0];
        const double spd = pEnemyShotSet->param_d[1];
        const int num = 7; // 列の弾数
        const double spacing = 28.0;

        for (int k = 0; k < num; k++) {
            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotLargeBall[0]; // 赤大玉
            p->speed = spd;
            p->count = 0;

            if (dir == 0) {
                // 上下方向に動く → 画面上下端から出現、xは自機位置付近に並べる
                p->x = target + (k - (num - 1) / 2.0) * spacing;
                if (fromPositive) {
                    p->y = -30.0;
                    p->muki = DX_PI / 2.0; // 下へ
                }
                else {
                    p->y = 510.0;
                    p->muki = -DX_PI / 2.0; // 上へ
                }
            }
            else {
                // 左右方向に動く → 画面左右端から出現、yは自機位置付近に並べる
                p->y = target + (k - (num - 1) / 2.0) * spacing;
                if (fromPositive) {
                    p->x = -30.0;
                    p->muki = 0.0; // 右へ
                }
                else {
                    p->x = 510.0;
                    p->muki = DX_PI; // 左へ
                }
            }
            p->margin = 80;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    // 移動
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_Heiankyo_Grok()
{
    // 固定パラメータ
    static const double W_FIXED = 80.0; // x方向間隔（広め・固定）
    static const double margin = 60.0;

    // 可変パラメータ（毎回変化）
    static int N;
    static int T;
    static double H;
    static int patternCycle; // 基本パターンの周期管理
    static int lastPhase;    // 前回のHPフェーズ
    static int shotTimer;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 変更禁止
        patternCycle = 0;
        lastPhase = -1;
        shotTimer = 0;
        N = 4 + GetRand(3); // 4～7
        T = 40 + GetRand(40); // 40～80
        H = 22.0 + GetRand(10); // 22～32 (狭め)
    }

    // ボスはほぼ固定位置（わずかに揺らぐ程度）
    enemy.x = 240.0 + 8.0 * sin(count * 0.03);
    enemy.y = 80.0 + 4.0 * cos(count * 0.025);

    // HPフェーズ判定
    // 2/3以上: phase0, 2/3～1/3: phase1, 1/3以下: phase2
    double hpRatio = (double)enemy.hp / (double)enemy.maxHp;
    int phase;
    if (hpRatio >= 2.0 / 3.0) phase = 0;
    else if (hpRatio >= 1.0 / 3.0) phase = 1;
    else phase = 2;
    if (hpRatio == 1.0)phase = -1;

    // フェーズが変わったらパラメータを再抽選し、タイマーリセット
    if (phase != lastPhase) {
        lastPhase = phase;
        patternCycle = 0;
        shotTimer = 0;
        N = 4 + GetRand(3);
        T = 40 + GetRand(40)+40;
        H = 22.0 + GetRand(10)+10;
    }

    // 基本パターンの周期
    // 出現1秒 + 移動2T + 移動後1秒 + 消去1秒 ≈ 180 + 2T フレーム
    int cycleLen = 180 + 2 * T + 30; // 余裕を持たせる
    if (shotTimer >= cycleLen) {
        shotTimer = 0;
        patternCycle++;
        // 毎回N,P,R,D,H,Tを変化
        N = 4 + GetRand(3);
        T = 40 + GetRand(40)+40;
        H = 22.0 + GetRand(10)+10;
    }

    // --- 基本パターン発射 ---
    if (shotTimer == 0) {
        auto spawnHeiankyo = [&](int transpose) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotHeiankyo;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->param_i[0] = N;
            pSet->param_i[1] = 0; // phase
            pSet->param_i[2] = 0;
            pSet->param_i[3] = T;
            pSet->param_i[4] = transpose;
            pSet->param_d[0] = W_FIXED;
            pSet->param_d[1] = H;
            pSet->param_d[2] = margin;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        };

        if (phase == 0) {
            // 基本パターンのみ
            spawnHeiankyo(0);
        }
        else if (phase == 1) {
            // x/y転置した基本パターン
            spawnHeiankyo(1);
        }
        else if (phase == 2) {
            // 基本 + 転置を同時
            spawnHeiankyo(0);
            spawnHeiankyo(1);
        }
    }

    // --- 大玉列（phase 0,1 のみ） ---
    // 基本パターンの移動中あたりに撃つ
    if (phase < 2 && shotTimer == 70) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotAimingLargeBalls;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        if (phase == 0) {
            // 自機と同じ左右位置の画面上下端の一方から、上下方向に動く大玉列
            pSet->param_i[0] = 0; // 上下
            pSet->param_i[1] = (player.y < 240.0) ? 0 : 1; // 自機が上なら下から、下なら上から
            pSet->param_d[0] = player.x;
            pSet->param_d[1] = 3.5 + GetRand(15) / 10.0; // 速度
        }
        else {
            // 自機と同じ上下位置の画面左右端の一方から、左右方向に動く大玉列
            pSet->param_i[0] = 1; // 左右
            pSet->param_i[1] = (player.x < 240.0) ? 0 : 1; // 自機が左なら右から、右なら左から
            pSet->param_d[0] = player.y;
            pSet->param_d[1] = 3.5 + GetRand(15) / 10.0;
        }

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    shotTimer++;
}