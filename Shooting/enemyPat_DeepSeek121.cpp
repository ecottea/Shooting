// ============================================================
//  カルマン渦弾幕  『カルマン・ストリート ―交互渦弾―』
//  enemyPat_sampleForAI.cpp と同じ書式で実装した1ファイル版。
//
//  ・画面上部の敵（円柱に見立てたエミッタ）から、
//    左右交互に逆向きの渦（カルマン渦列）を放出する。
//  ・各渦は 3本の腕 × 8発 = 24発で構成。
//  ・渦の中心は 1.2 秒間だけ安全な空洞。
//  ・1.2 秒後に中心から自機狙い弾を1発放ち、渦は崩壊しながら
//    外側へ飛散する。
//  ・渦と渦の間には低速の流線弾が落ちてくる。
//
//  count, pEnemyShotSet->count, pEnemyShot->count のインクリメント
//  および画面外の弾の消去はメインルーチン側で行うため、
//  このファイル内では一切触らない。
// ============================================================

#include <DxLib.h>
#include <math.h>
#include "gv.h"

// ------------------------------------------------------------
// 弾幕：カルマン渦列
// ------------------------------------------------------------
static void ShotKarmanStreet(sEnemyShotSet* pEnemyShotSet)
{
    // パラメータ
    const int    SPAWN_INTERVAL = 42;   // 渦の発生間隔(フレーム) ≒0.7秒
    const int    ARMS = 3;    // 渦の腕の本数
    const int    PER_ARM = 8;    // 1本の腕あたりの弾数
    const int    COLLAPSE_FRAME = 72;   // 崩壊までのフレーム数 ≒1.2秒
    const double DRIFT_Y = 80.0 / 60.0;                    // 下流への速度(px/frame)
    const double ROT_SPEED = (120.0 / 180.0 * DX_PI) / 60.0; // 120度/秒 → rad/frame
    const double WIGGLE_FREQ = 0.10; // 横揺れの周波数
    const double WIGGLE_AMP = 15.0; // 横揺れの振幅(px)

    // --------------------------------------------------------
    // 1) 渦の発生（左右交互）
    // --------------------------------------------------------
    if (pEnemyShotSet->count % SPAWN_INTERVAL == 0) {
        int vortexIndex = pEnemyShotSet->count / SPAWN_INTERVAL;
        int side = (vortexIndex % 2 == 0) ? -1 : 1; // 偶数=左、奇数=右

        // 使える効果音一覧: sound_enemyShot_light / medium / heavy / extreme / sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double baseX = pEnemyShotSet->x + side * 70.0;
        double baseY = pEnemyShotSet->y;
        double rotDir = (double)side;                            // 左右で回転方向を逆に
        double wigglePhase = (double)(vortexIndex % 4) * (DX_PI * 0.5);
        int    color = (side < 0) ? 3 : 5;                      // 左=シアン、右=マゼンタ

        for (int a = 0; a < ARMS; a++) {
            double armBase = (double)a * (2.0 * DX_PI / (double)ARMS);
            for (int j = 0; j < PER_ARM; j++) {
                sEnemyShot* p = new sEnemyShot;

                p->x = baseX;
                p->y = baseY;
                p->muki = 0.0;
                p->speed = 0.0;
                p->count = 0;

                // 弾の種類と半径一覧: 小玉(2.5x2.5)、中玉(7.0x7.0) 等
                // 弾の色一覧: 0:赤,1:黄,2:緑,3:シアン,4:青,5:マゼンタ,6:白,7:黒,8:橙
                p->kind = img_enemyShotSmallBall[color];

                // 自由パラメータ
                p->param_i[0] = 1;                          // 1=渦弾マーカー
                p->param_i[1] = a;                          // 腕番号
                p->param_i[2] = (a == 0 && j == 0) ? 1 : 0; // 1=リーダー
                p->param_i[3] = vortexIndex;

                p->param_d[0] = baseX;                      // 渦中心X(発生時)
                p->param_d[1] = baseY;                      // 渦中心Y(発生時)
                p->param_d[2] = armBase + (double)j * 0.10; // 腕内の位相ずらし
                p->param_d[3] = 40.0;                       // 初期半径
                p->param_d[4] = rotDir;                     // 回転方向
                p->param_d[5] = ROT_SPEED;                  // 回転速度
                p->param_d[6] = wigglePhase;                // 横揺れ位相
                p->param_d[7] = WIGGLE_AMP;                 // 横揺れ振幅
                p->param_d[8] = DRIFT_Y;                    // 下流への速度

                p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                p->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = p;
                pEnemyShotSet->pEnemyShotHead->prev = p;
            }
        }
    }

    // --------------------------------------------------------
    // 2) 流線弾（渦と渦の間を縫うように落ちる細い弾）
    // --------------------------------------------------------
    if (pEnemyShotSet->count % 14 == 7) {
        sEnemyShot* p = new sEnemyShot;
        p->x = pEnemyShotSet->x + (GetRand(40) - 20);
        p->y = pEnemyShotSet->y + 5.0;
        p->muki = DX_PI * 0.5;   // 下向き
        p->speed = 1.2;
        p->count = 0;
        p->kind = img_enemyShotBullet[3]; // 水色の銃弾
        p->param_i[0] = 0;                // 0=通常弾

        p->prev = pEnemyShotSet->pEnemyShotHead->prev;
        p->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = p;
        pEnemyShotSet->pEnemyShotHead->prev = p;
    }

    // --------------------------------------------------------
    // 3) 第1パス：渦崩壊時に中心から自機狙い弾を1発生成
    //    （リスト追加を先に済ませ、次パスで移動させる）
    // --------------------------------------------------------
    {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 1 && p->param_i[2] == 1 && p->count == COLLAPSE_FRAME) {
                // 渦中心を再計算
                double cx = p->param_d[0]
                    + p->param_d[7] * sin(WIGGLE_FREQ * (double)p->count + p->param_d[6]);
                double cy = p->param_d[1] + p->param_d[8] * (double)p->count;

                sEnemyShot* aim = new sEnemyShot;
                aim->x = cx;
                aim->y = cy;
                aim->muki = atan2(player.y - cy, player.x - cx);
                aim->speed = 4.0;
                aim->count = 0;
                aim->kind = img_enemyShotMediumBall[6]; // 白の中玉
                aim->param_i[0] = 0; // 通常弾マーカー

                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                aim->prev = pEnemyShotSet->pEnemyShotHead->prev;
                aim->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = aim;
                pEnemyShotSet->pEnemyShotHead->prev = aim;
            }
            p = p->next;
        }
    }

    // --------------------------------------------------------
    // 4) 第2パス：全弾の位置更新
    // --------------------------------------------------------
    {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 1) {
                // --- 渦弾：極座標で位置を直接計算 ---
                double cx = p->param_d[0]
                    + p->param_d[7] * sin(WIGGLE_FREQ * (double)p->count + p->param_d[6]);
                double cy = p->param_d[1] + p->param_d[8] * (double)p->count;

                double r;
                if (p->count < COLLAPSE_FRAME) {
                    // ゆっくり膨張（寿命2.5秒で半径80まで）
                    r = p->param_d[3] + 40.0 * ((double)p->count / 150.0);
                    if (r > 80.0) r = 80.0;
                }
                else {
                    // 崩壊：外側へ飛散
                    double t = (double)(p->count - COLLAPSE_FRAME);
                    r = 80.0 + t * 3.5;
                }

                double angle = p->param_d[2]
                    + p->param_d[4] * p->param_d[5] * (double)p->count;

                p->x = cx + r * cos(angle);
                p->y = cy + r * sin(angle);

                // 見た目の向き（進行方向の接線）
                p->muki = angle + p->param_d[4] * DX_PI * 0.5;
            }
            else {
                // --- 通常弾：自機狙い弾・流線弾など ---
                p->x += p->speed * cos(p->muki);
                p->y += p->speed * sin(p->muki);
            }
            p = p->next;
        }
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_KarmanVortex_DeepSeek()
{
    static int muki;
    static sEnemyShotSet* karmanSet;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;

        // カルマン渦のエミッタ（円柱に見立てたセット）
        karmanSet = new sEnemyShotSet;
        karmanSet->count = 0;
        karmanSet->patternFunc = ShotKarmanStreet;
        karmanSet->x = enemy.x;
        karmanSet->y = enemy.y + 20.0;
        karmanSet->muki = 0.0;
        karmanSet->kind = 0;

        karmanSet->pEnemyShotHead = new sEnemyShot;
        karmanSet->pEnemyShotHead->prev = karmanSet->pEnemyShotHead;
        karmanSet->pEnemyShotHead->next = karmanSet->pEnemyShotHead;

        karmanSet->prev = enemyShotSetHead.prev;
        karmanSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = karmanSet;
        enemyShotSetHead.prev = karmanSet;
    }
    else {
        // 円柱（エミッタ）に見立ててゆっくり左右に揺れる
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 140.0) muki = 1;
        if (enemy.x > 340.0) muki = -1;

        if (karmanSet) {
            karmanSet->x = enemy.x;
            karmanSet->y = enemy.y + 20.0;
        }
    }
}