// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 基準となる格子の初期座標（画面480x480を5等分する4本のライン。中央240からのオフセット用）
static const double CAGE_BASE[6] = { 0.0, 96.0, 192.0, 288.0, 384.0, 480.0 };

// 弾幕関数：『カッティング・ケージ（裁断される檻）』のタイムライン制御
static void ShotCuttingCage(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ---- フェーズ1：予兆線（サーチライト）の配置（初回フレームのみ） ----
    if (pEnemyShotSet->count == 0) {
        // 展開の始まりを告げる軽い効果音
        if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 【縦方向】の予兆レーザー 4本 × 8パーツ配置 (画面全体を縦に分断)
        for (int line = 0; line < 6; line++) {
            for (int part = 0; part < 8; part++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->param_i[0] = 0; // 0: 縦線識別
                pEnemyShot->param_i[1] = line; // 何本目の線か (0〜3)
                pEnemyShot->param_i[2] = 1;    // 1: レーザー本体パーツ識別フラグ
                pEnemyShot->param_d[0] = part * 64.0 + 32.0; // 軸に沿った初期位置(y座標の配置中心)

                pEnemyShot->x = CAGE_BASE[line];
                pEnemyShot->y = pEnemyShot->param_d[0];
                pEnemyShot->muki = DX_PI / 2.0; // 垂直方向
                pEnemyShot->speed = 0.0;        // スライド移動は独自の計算で行うため速さは0
                pEnemyShot->kind = img_enemyShotLaser[1]; // 黄色のレーザー（予兆）

                // 巡回双方向リストの末尾へ追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 【横方向】の予兆レーザー 4本 × 8パーツ配置 (画面全体を横に分断)
        for (int line = 0; line < 6; line++) {
            for (int part = 0; part < 8; part++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->param_i[0] = 1; // 1: 横線識別
                pEnemyShot->param_i[1] = line; // 何本目の線か (0〜3)
                pEnemyShot->param_i[2] = 1;    // 1: レーザー本体パーツ識別フラグ
                pEnemyShot->param_d[0] = part * 64.0 + 32.0; // 軸に沿った初期位置(x座標の配置中心)

                pEnemyShot->x = pEnemyShot->param_d[0];
                pEnemyShot->y = CAGE_BASE[line];
                pEnemyShot->muki = 0.0;  // 水平方向
                pEnemyShot->speed = 0.0; // スライド移動は独自の計算で行うため速さは0
                pEnemyShot->kind = img_enemyShotLaser[1]; // 黄色のレーザー（予兆）

                // 巡回双方向リストの末尾へ追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ---- フェーズ2：カウントダウン中の自機誘導・牽制弾 ----
    // 予兆線が敷かれてから本射に切り替わるまでの間、ボスからプレイヤーを部屋へ固定するための低速弾を放つ
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count < 90) {
        if (pEnemyShotSet->count % 20 == 0) {
            if (!CheckSoundMem(sound_enemyShot_light)) PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            // 自機への正確な角度を計算
            double angle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

            // 3方向の少しブレを持たせた自機狙い弾
            for (int i = -1; i <= 1; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                // GetRand(20)-10 によって 0〜20 の整数から10を引き、-10度〜+10度の綺麗なランダムの揺らぎを付与
                pEnemyShot->muki = angle + (i * 15.0 + (GetRand(20) - 10)) / 180.0 * DX_PI;
                pEnemyShot->speed = 1.8;
                pEnemyShot->kind = img_enemyShotBullet[8]; // 橙色の銃弾
                pEnemyShot->param_i[2] = 0; // 0: 通常の弾（レーザーではない）

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ---- フェーズ3：本射（空間の裁断）への切り替え ----
    if (pEnemyShotSet->count == 90) {
        // 危険な本射への変貌を告げる重低音の効果音
        if (!CheckSoundMem(sound_enemyShot_heavy)) PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // セット内にあるすべてのレーザーパーツの画像素材を本射用の「赤色」に一斉差し替え
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[2] == 1) {
                pEnemyShot->kind = img_enemyShotLaser[0]; // 赤色のレーザー（本射）
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // ---- フェーズ3〜4：格子が中央(240, 240)へ向かって徐々に狭まる縮小移動の計算 ----
    double scale = 1.0;
    if (pEnemyShotSet->count >= 90 && pEnemyShotSet->count < 170) {
        // 90Fから170F（80フレーム）かけて、縮小率を 1.0 から 0.45 まで滑らかに減少させる
        scale = 1.0 - (pEnemyShotSet->count - 90) * (0.55 / 80.0);
    }
    else if (pEnemyShotSet->count >= 170) {
        scale = 0.45; // 限界まで狭まったサイズを維持
    }

    // すべての所属弾の座標更新・移動ループ
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[2] == 1) {
            // レーザーパーツ：中央座標 (240.0) を基準とした相対距離に対して縮小率を掛け算し、位置をスライド
            int type = pEnemyShot->param_i[0];
            int line = pEnemyShot->param_i[1];
            double base_pos = CAGE_BASE[line];
            double scaled_pos = 240.0 + (base_pos - 240.0) * scale;

            if (type == 0) { // 縦線
                pEnemyShot->x = scaled_pos;
                pEnemyShot->y = pEnemyShot->param_d[0]; // 縦軸（y）上の配置位置は固定
            }
            else { // 横線
                pEnemyShot->x = pEnemyShot->param_d[0]; // 横軸（x）上の配置位置は固定
                pEnemyShot->y = scaled_pos;
            }
        }
        else {
            // 通常の弾（誘導弾や拡散弾）：設定された向きと速度に従って自然に直進移動
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        pEnemyShot = pEnemyShot->next;
    }

    // ---- フェーズ4：レーザーの消滅と、現在の交差点からの全方位拡散弾 ----
    if (pEnemyShotSet->count == 170) {
        // 檻がパッと激しく弾け飛ぶ中量感のある効果音
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 1. リストを走査し、役目を終えたレーザー本体パーツのみを安全にリストから除外して解放
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pEnemyShot->next;
            if (pEnemyShot->param_i[2] == 1) { // レーザーパーツをリストから切り離す
                pEnemyShot->prev->next = pEnemyShot->next;
                pEnemyShot->next->prev = pEnemyShot->prev;
                delete pEnemyShot; // メモリプールへ返却
            }
            pEnemyShot = pNext;
        }

        // 2. 縮小が極限に達した 4x4 = 16箇所の交差点座標を割り出し、そこから波紋のように小玉を全方位拡散
        for (int i = 0; i < 6; i++) {
            double cx = 240.0 + (CAGE_BASE[i] - 240.0) * scale;
            for (int j = 0; j < 6; j++) {
                double cy = 240.0 + (CAGE_BASE[j] - 240.0) * scale;

                // 各交差点から45度刻みで8方向に小玉を射出
                for (int d = 0; d < 8; d++) {
                    sEnemyShot* pNewShot = new sEnemyShot;
                    pNewShot->x = cx;
                    pNewShot->y = cy;
                    pNewShot->muki = (d * 45.0) / 180.0 * DX_PI; // 全方位
                    pNewShot->speed = 1.5;                         // 比較的避けやすい低速
                    pNewShot->kind = img_enemyShotSmallBall[4];   // 青色の小玉
                    pNewShot->param_i[2] = 0;                       // 0: 通常の弾（削除対象外）

                    // リスト末尾へ追加
                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }
            }
        }
    }
}

// 敵本体の行動パターン
void EnemyPat_Laser_Gemini()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面（480x480）の中央上部にボスを初期配置
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // HPは200固定仕様
        muki = 1;
        shot_count = 0;
    }
    else {
        // 檻が完全に画面中央基準(240, 240)で収縮するため、ボス本体も中央上部で小さく往復させて美しく見せる
        enemy.x += 0.4 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 『カッティング・ケージ』の一連のアニメーションが約170F〜終了するのを見計らい、
    // インターバルを含めて 260フレーム周期（約4.3秒ごと）で次の檻を生成・展開する
    if (count % 260 == 20) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCuttingCage;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 今回の固定格子アルゴリズムでは未使用だが初期化
        pEnemyShotSet->kind = shot_count++;

        // 弾セットに内包される所属弾用リスト（ダミーヘッド）の初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // メインルーチンが管理する全体の敵弾セットリスト（enemyShotSetHead）の末尾にこのセットを連結
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}