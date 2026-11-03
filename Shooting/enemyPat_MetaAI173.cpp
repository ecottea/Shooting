#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 素材選定メモ
//  - 五線譜の線 : 小玉(2.5x2.5) 白(6) -> 最も細く、線に見える。隙間を縫って避けられる
//  - 音符の頭   : 中玉(7.0x7.0) 白(6) -> 線より大きく、音符の玉として視認性が高い。丸なので回転の影響なし
//  - 符幹・連桁 : 短レーザー(64.0x4.0) 白(6) -> 細長い直線。唯一縦横の棒を表現できる素材
//    ※レーザーは muki が見た目の角度になるため、移動ベクトルは param_d[0], param_d[1] に分離して
//      見た目角度(muki)を固定したまま移動できるようにしている
// ============================================================

static const double STAFF_SPEED_Y = 1.2+0.5;   // 五線譜の落下速度
static const double NOTE_SPEED_X = 2.5;   // 音符の横移動速度
static const double LINE_SPACING = 12.0*2;  // 五線の間隔
static const int    DOTS_PER_LINE = 38;    // 1本の線を何個の小玉で作るか (480px / 14px間隔 ≈ 34 + 余裕)
static const double DOT_SPACING = 14.0;

// ------------------------------------------------------------
// 五線譜を流す弾幕セット
// ------------------------------------------------------------
static void ShotStaff(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 控えめな発射音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 5本の線を同時に生成
        for (int line = 0; line < 5; line++) {
            for (int i = 0; i < DOTS_PER_LINE; i++) {
                pEnemyShot = new sEnemyShot;

                // GetRand(x) は 0..x の x+1通りなので注意
                // 横に少しランダムを入れると手書き風になる
                pEnemyShot->x = -30.0 + i * DOT_SPACING + (GetRand(2) - 1); // -30は画面外から出すため
                pEnemyShot->y = pEnemyShotSet->y + line * LINE_SPACING;

                // 見た目は横向きのままでいいので muki は 0。移動は param_d で行う
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                pEnemyShot->param_d[0] = 0.0;              // vx
                pEnemyShot->param_d[1] = STAFF_SPEED_Y;    // vy
                pEnemyShot->margin = 25.0;

                // 小玉 白 -> 五線譜の線
                // img_enemyShotSmallBall[色] 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
                pEnemyShot->kind = img_enemyShotSmallBall[6];

                // 連結リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 更新：五線譜を真下へ流す
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 音符を左から右へ流す弾幕セット
// 四分音符 or 八分音符(2つ連結)をランダムで生成
// ------------------------------------------------------------
static void ShotNotes(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 音符の種類を決定 0:四分音符, 1:八分音符ペア
        // GetRand(1) は 0か1を返す (0以上1以下)
        int noteType = GetRand(1); // 0 or 1
        // 画面左外から出現させる
        double baseX = pEnemyShotSet->x; // EnemyPat_StaffNotation_MetaAI側で -30 に設定
        double baseY = pEnemyShotSet->y;

        // 音の高さは五線の線 or 線の間に量子化して音楽っぽくする
        // LINE_SPACING=12 なので、半分の6px刻みにすると線と隙間に乗る
        int pitchStep1 = GetRand(8) - 4; // -4 .. 4 の 9段階 (中央から上下)
        double y1 = baseY + pitchStep1 * (LINE_SPACING * 0.5);

        double vx = NOTE_SPEED_X + GetRand(60) / 100.0; // 2.5 ~ 3.1 で少しランダム
        double vy = STAFF_SPEED_Y; // 五線譜と同じ速度で落ちることで、五線上に留まって見える

        if (noteType == 0) {
            // ===== 四分音符 x1 =====
            // 1. 玉
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = baseX;
            pEnemyShot->y = y1;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->param_d[0] = vx;
            pEnemyShot->param_d[1] = vy;
            pEnemyShot->margin = 120.0;
            pEnemyShot->kind = img_enemyShotMediumBall[6]; // 中玉 白 : 音符の頭
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 2. 符幹 (縦棒)
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = baseX + 6.0; // 玉の右側に
            pEnemyShot->y = y1 - 18.0;   // 玉から上へ伸びる
            pEnemyShot->muki = -DX_PI * 0.5; // -90度で縦向きに見せる
            pEnemyShot->speed = 0.0;
            pEnemyShot->param_d[0] = vx;
            pEnemyShot->param_d[1] = vy;
            pEnemyShot->margin = 120.0;
            pEnemyShot->kind = img_enemyShotLaser[6]; // 短レーザー 白 : 符幹
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        else {
            // ===== 八分音符 2つ連結 =====
            int pitchStep2 = pitchStep1 + (GetRand(2) - 1); // 2つ目は少しだけ音程を変える
            double y2 = baseY + pitchStep2 * (LINE_SPACING * 0.5);
            double x2 = baseX + 34.0; // 2つ目の音符は34px右

            // 1. 玉1
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = baseX;
            pEnemyShot->y = y1;
            pEnemyShot->muki = 0.0; pEnemyShot->speed = 0.0;
            pEnemyShot->param_d[0] = vx; pEnemyShot->param_d[1] = vy;
            pEnemyShot->margin = 120.0;
            pEnemyShot->kind = img_enemyShotMediumBall[6];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 2. 玉2
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = x2;
            pEnemyShot->y = y2;
            pEnemyShot->muki = 0.0; pEnemyShot->speed = 0.0;
            pEnemyShot->param_d[0] = vx; pEnemyShot->param_d[1] = vy;
            pEnemyShot->margin = 120.0;
            pEnemyShot->kind = img_enemyShotMediumBall[6];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 3. 符幹1
            double stem1x = baseX + 6.0;
            double stem1y = y1 - 18.0;
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = stem1x; pEnemyShot->y = stem1y;
            pEnemyShot->muki = -DX_PI * 0.5;
            pEnemyShot->speed = 0.0;
            pEnemyShot->param_d[0] = vx; pEnemyShot->param_d[1] = vy;
            pEnemyShot->margin = 120.0;
            pEnemyShot->kind = img_enemyShotLaser[6];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 4. 符幹2
            double stem2x = x2 + 6.0;
            double stem2y = y2 - 18.0;
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = stem2x; pEnemyShot->y = stem2y;
            pEnemyShot->muki = -DX_PI * 0.5;
            pEnemyShot->speed = 0.0;
            pEnemyShot->param_d[0] = vx; pEnemyShot->param_d[1] = vy;
            pEnemyShot->margin = 120.0;
            pEnemyShot->kind = img_enemyShotLaser[6];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 5. 連桁 (2つの符幹の上を繋ぐ横棒)
            // 2つの符幹の頂点同士を結ぶ。頂点は符幹の中心からさらに -24px上
            double top1x = stem1x;
            double top1y = stem1y - 24.0;
            double top2x = stem2x;
            double top2y = stem2y - 24.0;
            double beamCx = (top1x + top2x) * 0.5;
            double beamCy = (top1y + top2y) * 0.5;
            double beamMuki = atan2(top2y - top1y, top2x - top1x); // 少し斜めに傾くのが音楽っぽい

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = beamCx;
            pEnemyShot->y = beamCy;
            pEnemyShot->muki = beamMuki; // 横方向(斜め)に見せる
            pEnemyShot->speed = 0.0;
            pEnemyShot->param_d[0] = vx;
            pEnemyShot->param_d[1] = vy;
            pEnemyShot->margin = 120.0;
            pEnemyShot->kind = img_enemyShotLaser[6]; // 短レーザーで連桁を表現 (64pxで34px間をカバーできる)
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 更新：全てのパーツを同じ速度で流す
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_StaffNotation_MetaAI()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵は左右にゆっくり往復 (原作のサンプルを踏襲)
        enemy.x += 0.7 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // ---- 五線譜を生成 ----
    // 18フレームごとに1セット。線が途切れないように少し早めに
    if (count % 180 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotStaff;
        pSet->x = 240.0; // 使わないが念のため中央
        pSet->y = enemy.y + 10.0; // 敵の下から五線譜が出てくる
        pSet->muki = DX_PI * 0.5;
        pSet->kind = shot_count++;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ---- 音符を生成 ----
    // 45フレームごとに左から流す。五線譜より少し遅れて出す
    if (count % 45 == 15) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotNotes;
        pSet->x = -30.0; // 左画面外から
        // yは五線上に乗るように、五線譜の生成位置 + ランダムな音程分
        // 敵のy + 80 を基準にすると、ちょうど生成されたばかりの五線譜の中に出現する
        pSet->y = enemy.y + 80.0-40 + (GetRand(4) - 2) * LINE_SPACING + count % 180 * STAFF_SPEED_Y;
        pSet->muki = 0.0;
        pSet->kind = shot_count++;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}
