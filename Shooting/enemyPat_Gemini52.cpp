// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static void ShotSub(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 1) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 200; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(240) - 120) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.1 + GetRand(400) / 100.0;
            pEnemyShot->kind = img_enemyShotScale[1];            

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 弾幕：ハニカム・プリズン ＆ デス・ニードル
static void ShotHoneycombPrison(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ----------------------------------------------------
    // Phase 1: 初期化 ＆ ハニカム（六角形）を形成する弾を一斉に配置
    // ----------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        // 予告チャージ音を再生
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double cx = pEnemyShotSet->x;
        double cy = pEnemyShotSet->y;
        double R = 150.0; // ハニカムの半径

        // 六角形の6つの頂点座標を事前に計算
        double vx[6], vy[6];
        for (int i = 0; i < 6; i++) {
            double theta = (i * 60.0) / 180.0 * DX_PI; // 60度ずつ展開
            vx[i] = cx + R * cos(theta);
            vy[i] = cy + R * sin(theta);
        }

        // 1. 頂点弾の生成 (6個の中玉・黄)
        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = enemy.x;
            pEnemyShot->y = enemy.y + 10.0;

            double tx = vx[i];
            double ty = vy[i];
            double dist = hypot(tx - pEnemyShot->x, ty - pEnemyShot->y);

            // ちょうど60フレーム後に目標座標(tx, ty)へピタッと静止するように速度を逆算
            pEnemyShot->speed = dist / 60.0;
            pEnemyShot->muki = atan2(ty - pEnemyShot->y, tx - pEnemyShot->x);
            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 中玉・黄

            pEnemyShot->param_i[0] = 0;   // 弾タイプ [0: ハニカム壁弾]
            pEnemyShot->param_i[1] = 1;   // 壁弾サブ [1: 頂点弾]
            pEnemyShot->param_d[0] = tx;  // 目標X
            pEnemyShot->param_d[1] = ty;  // 目標Y

            // 双方向リストに挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 2. 辺弾の生成 (6辺 × 各辺3個 = 18個の小玉・黄)
        for (int i = 0; i < 6; i++) {
            int next_i = (i + 1) % 6;
            for (int j = 1; j <= 3; j++) {
                double rate = j / 4.0; // 4分割した間の3点に弾を配置
                double px = (1.0 - rate) * vx[i] + rate * vx[next_i];
                double py = (1.0 - rate) * vy[i] + rate * vy[next_i];

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = enemy.x;
                pEnemyShot->y = enemy.y + 10.0;

                double dist = hypot(px - pEnemyShot->x, py - pEnemyShot->y);

                pEnemyShot->speed = dist / 60.0; // 同様に60フレームで配置につく
                pEnemyShot->muki = atan2(py - pEnemyShot->y, px - pEnemyShot->x);
                pEnemyShot->kind = img_enemyShotSmallBall[1]; // 小玉・黄

                pEnemyShot->param_i[0] = 0;   // 弾タイプ [0: ハニカム壁弾]
                pEnemyShot->param_i[1] = 0;   // 壁弾サブ [0: 辺弾]
                pEnemyShot->param_d[0] = px;  // 目標X
                pEnemyShot->param_d[1] = py;  // 目標Y

                // 双方向リストに挿入
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // Phase 2で蜜を垂らすための、現在の回転中頂点座標の保存バッファ
    double vx_temp[6];
    double vy_temp[6];
    int vertex_count = 0;

    // ----------------------------------------------------
    // 弾の更新（毎フレームの移動・挙動制御ループ）
    // ----------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        if (pShot->param_i[0] == 0) {
            // ==========================================
            // ハニカム壁弾の制御
            // ==========================================
            if (pEnemyShotSet->count < 60) {
                // 1. 展開フェーズ: 目標に向かって直進
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (pEnemyShotSet->count == 60) {
                // 2. 展開完了: 座標のズレを綺麗に補正し、静止させる
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];
                pShot->speed = 0.0;

                if (pShot->kind == img_enemyShotMediumBall[1]) {
                    sEnemyShotSet* pSet = new sEnemyShotSet;
                    pSet->count = 0;
                    pSet->patternFunc = ShotSub;
                    pSet->x = pShot->x;
                    pSet->y = pShot->y;
                    pSet->muki = atan2(pShot->y - pEnemyShotSet->y, pShot->x - pEnemyShotSet->x);

                    pSet->pEnemyShotHead = new sEnemyShot;
                    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

                    pSet->prev = enemyShotSetHead.prev;
                    pSet->next = &enemyShotSetHead;
                    enemyShotSetHead.prev->next = pSet;
                    enemyShotSetHead.prev = pSet;
                }
            }
            else if (pEnemyShotSet->count > 60 && pEnemyShotSet->count < 280) {
                // 3. 静止 ＆ 回転フェーズ: ハニカムの中心を軸に、全体をゆっくり回転させる
                double cx = pEnemyShotSet->x;
                double cy = pEnemyShotSet->y;
                double omega = 0.0015; // 回転角速度（毎フレーム約0.08度）

                // 2次元の回転行列計算
                double dx = pShot->param_d[0] - cx;
                double dy = pShot->param_d[1] - cy;
                pShot->param_d[0] = cx + dx * cos(omega) - dy * sin(omega);
                pShot->param_d[1] = cy + dx * sin(omega) + dy * cos(omega);

                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];

                // Phase 2で使うため、現在回転している最新の頂点座標を退避
                if (pShot->param_i[1] == 1 && vertex_count < 6) {
                    vx_temp[vertex_count] = pShot->x;
                    vy_temp[vertex_count] = pShot->y;
                    vertex_count++;
                }
            }
            else if (pEnemyShotSet->count >= 280) {
                // 4. ハニカム崩壊: 檻の中心から外側に向けて加速しながら霧散する
                double cx = pEnemyShotSet->x;
                double cy = pEnemyShotSet->y;
                pShot->muki = atan2(pShot->y - cy, pShot->x - cx);
                pShot->speed += 0.12; // 毎フレーム加速

                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        else if (pShot->param_i[0] == 1) {
            // ==========================================
            // Phase 2: ハニードロップ弾（蜜：ゆっくり微追尾）
            // ==========================================
            double target_muki = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = target_muki - pShot->muki;

            // 角度差を -PI〜PI に補正
            while (diff < -DX_PI) diff += DX_PI * 2;
            while (diff > DX_PI)  diff -= DX_PI * 2;

            // 毎フレーム1.2%だけプレイヤーの方向に軌道を修正（緩やかな追尾）
            pShot->muki += diff * 0.012;

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] == 2) {
            // ==========================================
            // Phase 3: デス・ニードル弾（針：超高速・直線）
            // ==========================================
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }

    // ----------------------------------------------------
    // Phase 2: ハチミツの滴り（120フレーム目、150フレーム目の2回発生）
    // ----------------------------------------------------
    if (pEnemyShotSet->count == 120 || pEnemyShotSet->count == 150) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 回転しているハニカムの6つの頂点から、それぞれプレイヤーへ滴らせる
        for (int i = 0; i < vertex_count; i++) {
            sEnemyShot* pNewShot = new sEnemyShot;
            pNewShot->x = vx_temp[i];
            pNewShot->y = vy_temp[i];

            // 粘り気（ポタポタ感）を出すため、速度を1.0〜1.4の間で若干の不均等（不規則）にする
            pNewShot->speed = 1.0 + (double)GetRand(40) / 100.0;
            pNewShot->muki = atan2(player.y - vy_temp[i], player.x - vx_temp[i]);
            pNewShot->kind = img_enemyShotMediumOval[8]; // 中楕円弾・橙 (蜜の滴りにベストマッチ)

            pNewShot->param_i[0] = 1; // 弾タイプ [1: ハニードロップ弾]

            // 双方向リストに挿入
            pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNewShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
            pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
        }
    }

    // ----------------------------------------------------
    // Phase 3: 必殺の一刺し（180, 195, 210フレーム目に自機狙い 3way を連射）
    // ----------------------------------------------------
    if (pEnemyShotSet->count == 180 || pEnemyShotSet->count == 195 || pEnemyShotSet->count == 210) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double base_muki = atan2(player.y - enemy.y, player.x - enemy.x);

        // 自機狙いを中心とした、間隔約9度（0.15ラジアン）の3way弾
        double angles[3] = {
            base_muki - 9.0 / 180.0 * DX_PI,
            base_muki,
            base_muki + 9.0 / 180.0 * DX_PI
        };

        for (int i = 0; i < 3; i++) {
            sEnemyShot* pNewShot = new sEnemyShot;
            pNewShot->x = enemy.x;
            pNewShot->y = enemy.y + 10.0;

            // 驚異的な一刺しを表現するため超高速（5.5〜6.0に揺らしを入れる）
            pNewShot->speed = 5.5 + (double)GetRand(50) / 100.0;
            pNewShot->muki = angles[i];
            pNewShot->kind = img_enemyShotLaser[0]; // 短レーザー・赤 (まさに狙撃される死の針)

            pNewShot->param_i[0] = 2; // 弾タイプ [2: デス・ニードル弾]

            // 双方向リストに挿入
            pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNewShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
            pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
        }
    }
}

// 敵本体の処理ルーチン
void EnemyPat_Bee_Gemini()
{
    static double base_x;
    static int move_dir;

    // 1サイクル 360フレーム(約6秒) で弾幕を展開
    int t = count % 360;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200; // HPは200固定
        base_x = 240.0;
        move_dir = 1;
    }

    // ----------------------------------------------------
    // ボス本体の移動制御
    // ----------------------------------------------------
    if (t < 280) {
        // 檻の展開・静止中は、その場付近を羽ばたくように、サイン・コサインでゆったり揺れる
        enemy.x = base_x + 20.0 * sin(count / 25.0);
        enemy.y = 100.0 + 8.0 * cos(count / 15.0);
    }
    else {
        // 弾幕インターバル（ハニカム崩壊後、t >= 280）の期間に、ボスが大きくスライド移動
        // これにより次のサイクルは新鮮な位置からハニカムを展開できる
        enemy.x += 1.5 * move_dir;

        // 画面外に出ていかないように移動範囲を制限
        if (enemy.x < 120.0) {
            enemy.x = 120.0;
            move_dir = 1;
        }
        if (enemy.x > 360.0) {
            enemy.x = 360.0;
            move_dir = -1;
        }
        base_x = enemy.x; // 揺れのベース位置を更新
    }

    // ----------------------------------------------------
    // 弾幕セットの生成（毎サイクル最初のフレーム t == 1 に実行）
    // ----------------------------------------------------
    if (t == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHoneycombPrison;

        // ハニカムの中心位置を定義
        // ボスから120px下がった位置（概ね画面中央の y=220 あたり）に展開し、プレイヤーを包み込む
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 120.0;

        // 画面下部を圧迫しすぎないようにY最大値を240(中央)にクリップ
        if (pEnemyShotSet->y > 240.0) pEnemyShotSet->y = 240.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルリストに新セットを挿入
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}