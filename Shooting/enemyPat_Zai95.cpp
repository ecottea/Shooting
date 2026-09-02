// ============================================================================
//  enemyPat_quickLoad.cpp
//  反則技「クイックロード」～負けた時だけセーブに戻る敵～
// ----------------------------------------------------------------------------
//  流れ：
//   1. 開幕、敵がセーブ。セーブ地点(自機の位置＝画面下半分)には弾のマーカーが
//      常時表示され、プレイヤーは「戻される場所」を選べる。残量メーターも表示。
//   2. 敵HPが30%以下になると反則「ロード」詠唱。LOADの文字弾が画面中央に
//      組み上がる間は被ダメージ2倍(削り切れば反則勝ち＝割り込みルート)。
//   3. ロード成立で HP復元(毎回劣化 100%→70%→40%) ＋ 全弾が軌道を巻き戻り ＋
//      自機はセーブ地点へ強制吸寄せ。弾幕は決定的に再生成され「リプレイ」される
//      (サイクル毎に+10%高速、上限+30%)。
//   4. ロードは3回まで。4回目はセーブデータが破損し「ERR0R」→全弾消滅、硬直、
//      最後の自機狙いラッシュ。以後は徐々に自壊して撃破される。
//   5. サブ反則「ショット強奪」：周期的に展開する吸収リングの間は
//      自機ショットの座標を直接曲げて吸い込み、撃ち返してくる。
//
//  仕様メモ：
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//     画面外の弾の消去はメインルーチン側で行われる前提。
//     弾の削除は「座標を画面外へ飛ばす」ことで代替している。
//   ・GetRand(x) は 0～x の (x+1) 種類を返す。本パターンはリプレイの決定性を
//     優先するため、乱数は最終ラッシュの撃角ブレのみに使用。
// ============================================================================

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <string.h>

// ============================================================
//  定数
// ============================================================
static const int    LOAD_HP_MAX = 3;                             // ロード可能回数
static const int    INTRO_DUR = 280;                           // 開幕演出の長さ
static const int    INTRO_ENTRY = 60;                            // 敵の入場フレーム数
static const int    SAVE_LOCK_PT = 200;                           // セーブ地点確定フレーム
static const int    CAST_BASE = 180;                           // 初回詠唱時間(3秒)
static const int    CAST_ADD = 60;                            // 2回目以降の延長(+1秒/回)
static const int    ERR_CAST = 240;                           // ERR0R表示時間(4秒)
static const int    FREEZE_DUR = 180;                           // クラッシュ硬直(3秒)
static const int    REWIND_DUR = 100;                           // ロード演出の長さ
static const int    REWIND_BULLET = 40;                            // うち弾を巻き戻す期間
static const double RESTORE_RATE[LOAD_HP_MAX] = { 1.0, 0.7, 0.4 }; // HP復元率(劣化)

enum {
    PH_INTRO,   // 開幕：SAVE演出
    PH_BATTLE,  // 通常弾幕(リプレイ対象)
    PH_CAST,    // 反則詠唱：LOAD(被ダメージ2倍)
    PH_REWIND,  // ロード成立：巻き戻し＋自機吸寄せ
    PH_ERRTEXT, // 4回目：ERR0R
    PH_FREEZE,  // セーブデータ破損：硬直
    PH_FINAL,   // 最後の足掻き
};

// 弾の役割フラグ(param_i[15]に格納)
static const int BFLAG_NORMAL = 0;  // 通常弾(巻き戻し・全消去の対象)
static const int BFLAG_KEEP = 1;  // 常設表示弾(全消去の対象外)
static const int BFLAG_TEXT = 2;  // 文字弾(巻き戻し時は敵の中へ帰る)

// ============================================================
//  パターン関数(前方宣言)
// ============================================================
static void ShotStraight(sEnemyShotSet* s);
static void ShotText(sEnemyShotSet* s);
static void ShotAbsorbRing(sEnemyShotSet* s);
static void ShotSaveMarker(sEnemyShotSet* s);
static void ShotSaveMeter(sEnemyShotSet* s);

// ============================================================
//  内部状態(count==1で全て初期化)
// ============================================================
static int    g_phase;          // 現在のフェーズ
static int    g_pt;             // フェーズ内経過フレーム
static int    g_cheatCount;     // 反則(ロード)を使用した回数
static int    g_castDur;        // 現在の詠唱時間
static double g_rate;           // リプレイ速度倍率 1.0→1.3
static double g_clock;          // 弾幕クロック(g_rate倍で進む)
static double g_nextRing;       // 次のリング弾まで
static double g_nextAimed;      // 次の自機狙いまで
static double g_nextAbsorb;     // 次のショット強奪まで
static int    g_ringVolley;     // リング弾の発射回数
static double g_ringAngle;      // リング弾の基準角
static int    g_ringDir;        // 回転方向(5回毎に反転)
static double g_saveX, g_saveY; // セーブ地点(自機が戻される場所)
static int    g_saveLocked;     // セーブ地点確定フラグ
static double g_swayT;          // 敵の揺れ用時刻(巻き戻し中也に減る)
static int    g_lastHp;         // 被ダメージ倍率処理用

// ============================================================
//  共通ヘルパー
// ============================================================
static double ClampD(double v, double lo, double hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

// 弾種(0:小玉 1:中玉 2:大玉 3:銃弾 4:鱗 5:菱形 6:中楕円 7:短レーザー)×色
static int ShotKind(int type, int color)
{
    switch (type) {
    case 0:  return img_enemyShotSmallBall[color];
    case 1:  return img_enemyShotMediumBall[color];
    case 2:  return img_enemyShotLargeBall[color];
    case 3:  return img_enemyShotBullet[color];
    case 4:  return img_enemyShotScale[color];
    case 5:  return img_enemyShotDiamond[color];
    case 6:  return img_enemyShotMediumOval[color];
    default: return img_enemyShotLaser[color];
    }
}

// 効果音(鳴り途中ならやり直す)
static void PlaySe(int id)
{
    int h = -1;
    if (id == 1) h = sound_enemyShot_light;
    else if (id == 2) h = sound_enemyShot_medium;
    else if (id == 3) h = sound_enemyShot_heavy;
    else if (id == 4) h = sound_enemyShot_extreme;
    else if (id == 5) h = sound_enemyCharge;
    if (h == -1) return;
    if (CheckSoundMem(h)) StopSoundMem(h);
    PlaySoundMem(h, DX_PLAYTYPE_BACK);
}

// 弾セットを新規作成してリストへ繋ぐ
static sEnemyShotSet* CreateSet(void(*func)(sEnemyShotSet*), double x, double y, double muki)
{
    sEnemyShotSet* s = new sEnemyShotSet;
    s->count = 0;
    s->patternFunc = func;
    s->x = x;
    s->y = y;
    s->muki = muki;

    s->pEnemyShotHead = new sEnemyShot;
    s->pEnemyShotHead->prev = s->pEnemyShotHead;
    s->pEnemyShotHead->next = s->pEnemyShotHead;

    s->prev = enemyShotSetHead.prev;
    s->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = s;
    enemyShotSetHead.prev = s;
    return s;
}

// 弾を1発リストへ繋ぐ
static sEnemyShot* AddShot(sEnemyShotSet* s, double x, double y, double muki,
    double speed, int kind, int flag)
{
    sEnemyShot* b = new sEnemyShot;
    b->x = x;
    b->y = y;
    b->muki = muki;
    b->speed = speed;
    b->kind = kind;
    b->param_i[15] = flag;
    b->margin = 480;

    b->prev = s->pEnemyShotHead->prev;
    b->next = s->pEnemyShotHead;
    s->pEnemyShotHead->prev->next = b;
    s->pEnemyShotHead->prev = b;
    return b;
}

// 全ての弾を画面外へ飛ばす(実際の消去はメインルーチンが行う)
static void VanishAllBullets(bool killText)
{
    for (sEnemyShotSet* s = enemyShotSetHead.next; s != &enemyShotSetHead; s = s->next) {
        for (sEnemyShot* b = s->pEnemyShotHead->next; b != s->pEnemyShotHead; b = b->next) {
            int f = b->param_i[15];
            if (f == BFLAG_KEEP) continue;
            if (f == BFLAG_TEXT && !killText) continue;
            b->x = -9999.0;
            b->y = -9999.0;
        }
    }
}

// 通常弾の速度を一括変更(詠唱中の弾丸時間など)
static void ScaleAllShotSpeed(double mul)
{
    for (sEnemyShotSet* s = enemyShotSetHead.next; s != &enemyShotSetHead; s = s->next) {
        for (sEnemyShot* b = s->pEnemyShotHead->next; b != s->pEnemyShotHead; b = b->next) {
            if (b->param_i[15] == BFLAG_NORMAL) b->speed *= mul;
        }
    }
}

// セーブマーカーの弾を画面外へ飛ばして消す
static void DeleteMarkerBullets()
{
    for (sEnemyShotSet* s = enemyShotSetHead.next; s != &enemyShotSetHead; s = s->next) {
        if (s->patternFunc != ShotSaveMarker) continue;
        for (sEnemyShot* b = s->pEnemyShotHead->next; b != s->pEnemyShotHead; b = b->next) {
            b->x = -9999.0;
            b->y = -9999.0;
        }
    }
}

// HPを書き換える(lastHpも更新し、倍率処理と干渉させない)
static void SetEnemyHp(int v)
{
    enemy.hp = v;
    g_lastHp = v;
}

// ============================================================
//  弾幕パターン
// ============================================================

// 汎用直進弾：生成時に set->muki を基準に一斉射出
//  set->param_i[0]=弾種 [1]=色 [2]=弾数 [3]=効果音(0=無し)
//  set->param_d[0]=速さ [1]=広がり(2π以上で全周リング)
static void ShotStraight(sEnemyShotSet* s)
{
    if (g_phase == PH_REWIND) return; // 巻き戻し中は移動を本体に任せる

    if (s->count == 0) {
        int    n = s->param_i[2];
        double spread = s->param_d[1];
        for (int i = 0; i < n; i++) {
            double a;
            if (n <= 1)             a = s->muki;
            else if (spread >= 6.2) a = s->muki + spread * i / (double)n;               // 全周
            else                    a = s->muki + spread * ((double)i / (n - 1) - 0.5); // 扇状
            AddShot(s, s->x, s->y, a, s->param_d[0],
                ShotKind(s->param_i[0], s->param_i[1]), BFLAG_NORMAL);
        }
        if (s->param_i[3] > 0) PlaySe(s->param_i[3]);
    }

    for (sEnemyShot* b = s->pEnemyShotHead->next; b != s->pEnemyShotHead; b = b->next) {
        b->x += b->speed * cos(b->muki);
        b->y += b->speed * sin(b->muki);
    }
}

// 5x7 ドット文字の字形
static const char* GetGlyph(int c)
{
    switch (c) {
    case 'S': return ".XXXX" "X...." "X...." ".XXX." "....X" "....X" "XXXX.";
    case 'A': return "..X.." ".X.X." "X...X" "X...X" "XXXXX" "X...X" "X...X";
    case 'V': return "X...X" "X...X" "X...X" "X...X" "X...X" ".X.X." "..X..";
    case 'E': return "XXXXX" "X...." "X...." "XXXX." "X...." "X...." "XXXXX";
    case 'L': return "X...." "X...." "X...." "X...." "X...." "X...." "XXXXX";
    case 'O': return ".XXX." "X...X" "X...X" "X...X" "X...X" "X...X" ".XXX.";
    case 'D': return "XXXX." "X...X" "X...X" "X...X" "X...X" "X...X" "XXXX.";
    case 'R': return "XXXX." "X...X" "X...X" "XXXX." "X.X.." "X..X." "X...X";
    case '0': return ".XXX." "X...X" "X..XX" "X.X.X" "XX..X" "X...X" ".XXX.";
    }
    return "....." "....." "....." "....." "....." "....." ".....";
}

// ドット文字弾：敵の周りに集まったドットが飛び出し、画面中央に文字を組み上げる
//  set->param_i[0]=モード(0:SAVE/1:LOAD/2:ERR0R) [1]=弾けるフレーム
//  弾 param_i[0]=ドット番号 [1]=状態(0:飛行中 1:保持 2:弾けた後)
//      param_d[0..1]=目標座標 [2]=遅延 [3]=飛行時間 [4..5]=出発座標
static void ShotText(sEnemyShotSet* s)
{
    if (g_phase == PH_REWIND) return; // 巻き戻し中は本体側で敵へ帰す

    const double DOT = 12.0; // ドット間隔

    if (s->count == 0) {
        const char* str = (s->param_i[0] == 0) ? "SAVE" : (s->param_i[0] == 1) ? "LOAD" : "ERR0R";
        int         col = (s->param_i[0] == 0) ? 2 : (s->param_i[0] == 1) ? 6 : 0;
        int         kind = img_enemyShotMediumBall[col];
        int         n = (int)strlen(str);
        double      x0 = 240.0 - ((n * 5 + (n - 1)) * DOT) * 0.5 + DOT * 0.5;
        double      y0 = 240.0 - 3.0 * DOT;
        int         idx = 0;

        for (int li = 0; li < n; li++) {
            const char* g = GetGlyph(str[li]);
            for (int r = 0; r < 7; r++) {
                for (int c = 0; c < 5; c++) {
                    if (g[r * 5 + c] != 'X') continue;
                    // 出発点は敵の周りに黄金角スパイラルで集める(チャージ演出)
                    double sa = idx * 2.39996;
                    double sx = enemy.x + cos(sa) * (8.0 + (idx % 7) * 2.5);
                    double sy = enemy.y + sin(sa) * (8.0 + (idx % 7) * 2.5);
                    sEnemyShot* b = AddShot(s, sx, sy, 0.0, 0.0, kind, BFLAG_TEXT);
                    b->param_d[0] = x0 + (li * 6 + c) * DOT;
                    b->param_d[1] = y0 + r * DOT;
                    b->param_d[2] = (r * 5 + c) + li * 8; // 飛び出し遅延
                    b->param_d[3] = 40.0;
                    b->param_d[4] = sx;
                    b->param_d[5] = sy;
                    b->param_i[0] = idx++;
                }
            }
        }
    }

    // SAVE/ERR0R は指定フレームで弾ける(LOADは巻き戻し処理に委ねる)
    if (s->count == s->param_i[1]) {
        for (sEnemyShot* b = s->pEnemyShotHead->next; b != s->pEnemyShotHead; b = b->next) {
            if (b->param_i[1] != 1) continue;
            b->param_i[1] = 2;
            b->muki = atan2(b->y - 240.0, b->x - 240.0) + ((b->param_i[0] * 37) % 13 - 6) * 0.03;
            b->speed = 2.1 + ((b->param_i[0] * 29) % 10) * 0.14;
        }
        PlaySe(s->param_i[0] == 2 ? 4 : 2);
    }

    for (sEnemyShot* b = s->pEnemyShotHead->next; b != s->pEnemyShotHead; b = b->next) {
        if (b->param_i[1] == 0) {
            // 敵から文字位置へ滑らかに飛ぶ
            double t = (s->count - b->param_d[2]) / b->param_d[3];
            if (t < 0.0) {
                b->x = b->param_d[4];
                b->y = b->param_d[5];
            }
            else {
                if (t > 1.0) t = 1.0;
                double e = t * t * (3.0 - 2.0 * t);
                b->x = b->param_d[4] + (b->param_d[0] - b->param_d[4]) * e;
                b->y = b->param_d[5] + (b->param_d[1] - b->param_d[5]) * e;
                if (t >= 1.0) b->param_i[1] = 1;
            }
        }
        else if (b->param_i[1] == 1) {
            if (s->param_i[0] == 2) {
                // ERR0R：文字化けジッター＋色ノイズ
                b->x = b->param_d[0] + sin(s->count * 0.7 + b->param_i[0] * 1.7) * 2.5;
                b->y = b->param_d[1] + cos(s->count * 0.9 + b->param_i[0] * 2.3) * 2.5;
                if ((s->count + b->param_i[0] * 13) % 60 < 8) {
                    b->x += sin(b->param_i[0] * 12.9 + s->count) * 10.0;
                    b->y += cos(b->param_i[0] * 7.7 + s->count * 1.3) * 8.0;
                }
                static const int colTbl[3] = { 0, 6, 8 }; // 赤白橙
                b->kind = img_enemyShotMediumBall[colTbl[((s->count >> 2) + b->param_i[0]) % 3]];
            }
            else {
                b->x = b->param_d[0];
                b->y = b->param_d[1];
            }
        }
        else {
            // 弾けた後は通常弾として直進
            b->x += b->speed * cos(b->muki);
            b->y += b->speed * sin(b->muki);
        }
    }
}

// サブ反則「ショット強奪」：1.2秒間、自機ショットの座標を曲げて吸い込む
//  リング弾 param_i[0]=リング番号 [1]=0(リング)/1(直進)
//  set->param_i[3]=吸収数カウンタ(演出用)
static void ShotAbsorbRing(sEnemyShotSet* s)
{
    if (g_phase == PH_REWIND) return;

    if (s->count == 0) {
        for (int i = 0; i < 12; i++) {
            sEnemyShot* b = AddShot(s, enemy.x, enemy.y, 0.0, 0.0,
                img_enemyShotMediumOval[3], BFLAG_NORMAL);
            b->param_i[0] = i;
        }
        PlaySe(5); // 予告音：この間は撃つな、の合図
    }

    // ウィンドウ終了：リングが外向きの弾に変わる
    if (s->count == 72) {
        for (sEnemyShot* b = s->pEnemyShotHead->next; b != s->pEnemyShotHead; b = b->next) {
            if (b->param_i[1] != 0) continue;
            b->param_i[1] = 1;
            b->muki = atan2(b->y - enemy.y, b->x - enemy.x);
            b->speed = 2.6;
        }
    }

    for (sEnemyShot* b = s->pEnemyShotHead->next; b != s->pEnemyShotHead; b = b->next) {
        if (b->param_i[1] == 0) {
            if (s->count < 72) {
                // リングは敵の周りを回転(半径が脈動する)
                double rr = 46.0 + sin(s->count * 0.25) * 5.0;
                double a = s->count * 0.13 + b->param_i[0] * (DX_PI / 6.0);
                b->x = enemy.x + cos(a) * rr;
                b->y = enemy.y + sin(a) * rr;
            }
        }
        else {
            b->x += b->speed * cos(b->muki);
            b->y += b->speed * sin(b->muki);
        }
    }

    // 自機ショットを吸い込む(座標を直接改竄する反則)
    if (s->count < 72) {
        for (sPlayerShot* ps = playerShotHead.next; ps != &playerShotHead; ps = ps->next) {
            double dx = enemy.x - ps->x;
            double dy = enemy.y - ps->y;
            double d = sqrt(dx * dx + dy * dy);
            if (d < 1.0) d = 1.0;
            ps->x += dx / d * 8.0; // 弾道が敵へ曲がっていく
            ps->y += dy / d * 8.0;
            if (d < 42.0) {
                ps->y = -80.0; // 画面外へ＝消滅
                // 吸ったぶんだけ自機狙いで撃ち返す(白い銃弾＝自機の弾に偽装)
                sEnemyShot* cb = AddShot(s, enemy.x, enemy.y,
                    atan2(player.y - enemy.y, player.x - enemy.x),
                    5.0, img_enemyShotBullet[6], BFLAG_NORMAL);
                cb->param_i[1] = 1;
                s->param_i[3]++;
                PlaySe(1);
            }
        }
    }
}

// セーブ地点マーカー：自機が戻される場所を示す常設表示
//  ロック前(白)は自機に追従、ロック後(シアン)で固定
static void ShotSaveMarker(sEnemyShotSet* s)
{
    if (g_phase == PH_REWIND) return; // ロード中は消去済み

    if (s->count == 0) {
        for (int i = 0; i < 4; i++) {
            double a = i * (DX_PI / 2.0);
            sEnemyShot* b = AddShot(s, g_saveX + cos(a) * 26.0, g_saveY + sin(a) * 26.0,
                0.0, 0.0, img_enemyShotDiamond[6], BFLAG_KEEP);
            b->param_i[0] = i;
        }
    }
    for (sEnemyShot* b = s->pEnemyShotHead->next; b != s->pEnemyShotHead; b = b->next) {
        double a = s->count * 0.04 + b->param_i[0] * (DX_PI / 2.0);
        b->x = g_saveX + cos(a) * 26.0;
        b->y = g_saveY + sin(a) * 26.0;
        b->kind = img_enemyShotDiamond[g_saveLocked ? 3 : 6];
    }
}

// セーブ残量メーター(画面上部 [■■■])：ロードを使う毎にセルが消えていく
//  弾 param_i[0]=0/1(括弧) 2..4(セル) [1]=括弧の発射フラグ
static void ShotSaveMeter(sEnemyShotSet* s)
{
    if (s->count == 0) {
        sEnemyShot* b;
        b = AddShot(s, 200.0, 24.0, 0.0, 0.0, img_enemyShotSmallBall[6], BFLAG_KEEP);
        b->param_i[0] = 0;
        b = AddShot(s, 280.0, 24.0, 0.0, 0.0, img_enemyShotSmallBall[6], BFLAG_KEEP);
        b->param_i[0] = 1;
        for (int k = 0; k < LOAD_HP_MAX; k++) {
            b = AddShot(s, 216.0 + k * 24.0, 24.0, 0.0, 0.0,
                img_enemyShotMediumBall[2], BFLAG_KEEP);
            b->param_i[0] = 2 + k;
        }
    }
    for (sEnemyShot* b = s->pEnemyShotHead->next; b != s->pEnemyShotHead; b = b->next) {
        if (b->param_i[0] >= 2) {
            // 消費されたセルは画面外へ(消去はメインルーチン)
            if (b->param_i[0] - 2 >= LOAD_HP_MAX - g_cheatCount) {
                b->x = -9999.0;
                b->y = -9999.0;
            }
        }
        else {
            // 括弧は ERR0R で画面外へ飛び去る
            if (g_phase >= PH_ERRTEXT && b->param_i[1] == 0) {
                b->param_i[1] = 1;
                b->muki = (b->param_i[0] == 0) ? DX_PI : 0.0;
                b->speed = 3.0;
            }
            if (b->param_i[1] == 1) {
                b->x += b->speed * cos(b->muki);
                b->y += b->speed * sin(b->muki);
            }
        }
    }
}

// 文字弾セットを作る(0:SAVE / 1:LOAD / 2:ERR0R)
static void CreateTextSet(int mode)
{
    sEnemyShotSet* s = CreateSet(ShotText, enemy.x, enemy.y, 0.0);
    s->param_i[0] = mode;
    s->param_i[1] = (mode == 0) ? 165 : (mode == 1) ? 99999 : ERR_CAST;
}

// ============================================================
//  敵本体
// ============================================================
void EnemyPat_Violate_Zai()
{
    if (count == 1) {
        // ---------- 初期化 ----------
        enemy.x = 240.0;
        enemy.y = -60.0;
        enemy.maxHp = enemy.hp = 200;

        g_phase = PH_INTRO;
        g_pt = 0;
        g_cheatCount = 0;
        g_castDur = 0;
        g_rate = 1.0;
        g_clock = 0.0;
        g_nextRing = 40.0;
        g_nextAimed = 150.0;
        g_nextAbsorb = 260.0;
        g_ringVolley = 0;
        g_ringAngle = 0.0;
        g_ringDir = 1;
        g_saveX = ClampD(player.x, 48.0, 432.0);
        g_saveY = ClampD(player.y, 200.0, 440.0);
        g_saveLocked = 0;
        g_swayT = 0.0;
        g_lastHp = enemy.hp;

        CreateSet(ShotSaveMeter, 0.0, 0.0, 0.0); // セーブ残量メーター
        CreateSet(ShotSaveMarker, 0.0, 0.0, 0.0); // セーブ地点マーカー
    }
    else {
        g_pt++;
    }

    // ---------- 反則その1：詠唱中の被ダメージを2倍に改竄 ----------
    // メインルーチンが減らしたHPをさらに減らすことで実現。
    // この隙に削り切れば「割り込みルート」で反則勝ちできる。
    {
        int delta = g_lastHp - enemy.hp;
        if (delta > 0 && g_phase == PH_CAST) enemy.hp -= delta;
        g_lastHp = enemy.hp;
    }

    switch (g_phase) {

        // ---------- 開幕：セーブ ----------
    case PH_INTRO: {
        if (g_pt < INTRO_ENTRY) {
            double t = (double)g_pt / INTRO_ENTRY;
            double e = t * t * (3.0 - 2.0 * t);
            enemy.x = 240.0;
            enemy.y = -60.0 + 140.0 * e;
        }
        else {
            g_swayT += 1.0;
            enemy.x = 240.0 + sin(g_swayT * 0.02) * 24.0;
            enemy.y = 80.0;
        }
        // セーブ地点は自機の位置に追従(画面下半分に制限)
        // → プレイヤーは「戻される場所」を自分で選べる
        if (!g_saveLocked) {
            g_saveX = ClampD(player.x, 48.0, 432.0);
            g_saveY = ClampD(player.y, 200.0, 440.0);
            if (g_pt == SAVE_LOCK_PT) {
                g_saveLocked = 1; // 確定(マーカーが白→シアンに変化)
                PlaySe(3);
            }
        }
        if (g_pt == 70) CreateTextSet(0); // SAVE の文字
        if (g_pt >= INTRO_DUR) {
            g_phase = PH_BATTLE;
            g_pt = 0;
        }
        break;
    }

                 // ---------- 通常弾幕(リプレイ対象：ここは乱数不使用の決定的構成) ----------
    case PH_BATTLE: {
        g_swayT += 1.0;
        enemy.x = 240.0 + sin(g_swayT * 0.02) * 24.0;
        enemy.y = 80.0;
        g_clock += g_rate; // リプレイ速度で加速するクロック

        // 16方向リング弾(1.2秒毎 / 基準角を毎回15°回転 / 5回毎に逆転)
        if (g_clock >= g_nextRing) {
            g_nextRing += 72.0;
            g_ringAngle += g_ringDir * (DX_PI / 12.0);
            g_ringVolley++;
            if (g_ringVolley % 5 == 0) g_ringDir = -g_ringDir;

            static const int ringCol[4] = { 0, 8, 1, 6 }; // 赤橙黄白
            sEnemyShotSet* s = CreateSet(ShotStraight, enemy.x, enemy.y, g_ringAngle);
            s->param_i[0] = 0;                          // 小玉
            s->param_i[1] = ringCol[g_ringVolley % 4];
            s->param_i[2] = 16;
            s->param_i[3] = 2;
            s->param_d[0] = 2.1 * g_rate;
            s->param_d[1] = 2.0 * DX_PI;
        }

        // 自機狙い3-way(4秒毎)
        if (g_clock >= g_nextAimed) {
            g_nextAimed += 240.0;
            sEnemyShotSet* s = CreateSet(ShotStraight, enemy.x, enemy.y + 10.0,
                atan2(player.y - enemy.y, player.x - enemy.x));
            s->param_i[0] = 1;  // 中玉
            s->param_i[1] = 8;  // 橙
            s->param_i[2] = 3;
            s->param_i[3] = 1;
            s->param_d[0] = 3.0 * g_rate;
            s->param_d[1] = 0.44;
        }

        // サブ反則「ショット強奪」(6秒毎 / 1.2秒間)
        if (g_clock >= g_nextAbsorb) {
            g_nextAbsorb += 360.0;
            CreateSet(ShotAbsorbRing, enemy.x, enemy.y, 0.0);
        }

        // ---------- 反則トリガー：残りHP30%以下でロード ----------
        if (enemy.hp * 10 <= enemy.maxHp * 3) {
            if (g_cheatCount < LOAD_HP_MAX) {
                g_phase = PH_CAST;
                g_pt = 0;
                g_castDur = CAST_BASE + CAST_ADD * g_cheatCount; // 詠唱は毎回長くなる
            }
            else {
                g_phase = PH_ERRTEXT; // 4回目：セーブデータ破損
                g_pt = 0;
            }
        }
        break;
    }

                  // ---------- 反則詠唱：LOAD ----------
    case PH_CAST: {
        // 詠唱が進むほど震えが激しくなる
        double amp = 3.0 + 5.0 * g_pt / (double)g_castDur;
        enemy.x = 240.0 + sin(g_pt * 0.9) * amp;
        enemy.y = 80.0 + sin(g_pt * 0.5) * 2.0;

        if (g_pt == 0) {
            g_cheatCount++;         // セーブ残量を消費(メーターのセルが消える)
            ScaleAllShotSpeed(0.5); // 既存弾は弾丸時間(半減速)
            CreateTextSet(1);       // 画面中央に LOAD の文字が組み上がる
            PlaySe(5);
        }
        if (g_pt >= g_castDur) {
            g_phase = PH_REWIND;    // 詠唱完了＝ロード成立
            g_pt = 0;
        }
        break;
    }

                // ---------- ロード成立：巻き戻し＋自機吸寄せ ----------
    case PH_REWIND: {
        if (g_pt == 0) {
            // 反則その2：HPをセーブ時の値へ復元(ただし毎回劣化する)
            SetEnemyHp((int)(enemy.maxHp * RESTORE_RATE[g_cheatCount - 1] + 0.5));
            DeleteMarkerBullets(); // マーカーも一度消える
            PlaySe(3);
        }
        if (g_pt < REWIND_BULLET) {
            // 反則その3：全ての弾の軌道を逆再生する
            double decay = 1.0 - (double)g_pt / REWIND_BULLET;
            for (sEnemyShotSet* st = enemyShotSetHead.next; st != &enemyShotSetHead; st = st->next) {
                for (sEnemyShot* b = st->pEnemyShotHead->next; b != st->pEnemyShotHead; b = b->next) {
                    int f = b->param_i[15];
                    if (f == BFLAG_NORMAL) {
                        b->x -= b->speed * cos(b->muki) * 1.1 * decay;
                        b->y -= b->speed * sin(b->muki) * 1.1 * decay;
                    }
                    else if (f == BFLAG_TEXT) {
                        // 文字弾は敵の中へ吸い込まれていく
                        double dx = enemy.x - b->x;
                        double dy = enemy.y - b->y;
                        double d = sqrt(dx * dx + dy * dy);
                        if (d < 24.0) {
                            b->x = -9999.0;
                            b->y = -9999.0;
                        }
                        else {
                            b->x += dx / d * 10.0;
                            b->y += dy / d * 10.0;
                        }
                    }
                }
            }
            // 敵の揺れも一緒に巻き戻る
            g_swayT -= 2.0;
            enemy.x = 240.0 + sin(g_swayT * 0.02) * 24.0 + sin(g_pt * 1.7) * 6.0;
            enemy.y = 80.0;
        }
        else if (g_pt == REWIND_BULLET) {
            VanishAllBullets(true); // 巻き戻し完了：全弾消滅
        }
        else {
            // 反則その4：自機をセーブ地点へ強制吸寄せ(1秒)
            // この間は弾が無いので安全
            player.x += (g_saveX - player.x) * 0.09;
            player.y += (g_saveY - player.y) * 0.09;
            enemy.x = 240.0 + sin(g_pt * 1.1) * 3.0;
            enemy.y = 80.0;
        }
        if (g_pt >= REWIND_DUR) {
            // リプレイ開始：サイクル毎に10%高速(3回で+30%)
            g_rate = 1.0 + 0.1 * g_cheatCount;
            g_clock = 0.0;
            g_nextRing = 40.0;
            g_nextAimed = 150.0;
            g_nextAbsorb = 260.0;
            CreateSet(ShotSaveMarker, 0.0, 0.0, 0.0); // マーカー再出現
            g_phase = PH_BATTLE;
            g_pt = 0;
        }
        break;
    }

                  // ---------- 4回目：セーブデータ破損 ----------
    case PH_ERRTEXT: {
        enemy.x = 240.0 + sin(g_pt * 1.3) * 8.0;
        enemy.y = 80.0 + sin(g_pt * 2.1) * 5.0;
        if (g_pt == 0) {
            ScaleAllShotSpeed(0.5);
            CreateTextSet(2); // 文字化けしながら ERR0R が組み上がる
            PlaySe(5);
        }
        if (g_pt >= ERR_CAST) {
            // 全弾消滅(文字弾は次の瞬間に弾け飛ぶ)。残HPを僅かにして止めを許す
            VanishAllBullets(false);
            if (enemy.hp > 25) SetEnemyHp(25);
            g_phase = PH_FREEZE;
            g_pt = 0;
            PlaySe(4);
        }
        break;
    }

                   // ---------- クラッシュ：3秒硬直 ----------
    case PH_FREEZE: {
        enemy.x = 240.0;
        enemy.y = 80.0 + g_pt * 0.05; // ゆっくり沈む
        if (g_pt >= FREEZE_DUR) {
            g_phase = PH_FINAL;
            g_pt = 0;
        }
        break;
    }

                  // ---------- 最後の足掻き：自機狙いラッシュ ----------
    case PH_FINAL: {
        g_swayT += 1.0;
        enemy.x = 240.0 + sin(g_swayT * 0.05) * 30.0;
        enemy.y = 80.0;

        if (g_pt % 24 == 0) {
            // 高速自機狙い(撃角がわずかにブレる)
            // ※GetRand(20) は 0～20 の21種類を返すので -10 で中心化
            double aim = atan2(player.y - enemy.y, player.x - enemy.x)
                + (GetRand(20) - 10) * 0.012;
            sEnemyShotSet* s = CreateSet(ShotStraight, enemy.x, enemy.y, aim);
            s->param_i[0] = 1;  // 中玉
            s->param_i[1] = 0;  // 赤
            s->param_i[2] = 1;
            s->param_i[3] = 1;
            s->param_d[0] = 5.2;
            s->param_d[1] = 0.0;
        }
        if (g_pt % 90 == 40) {
            sEnemyShotSet* s = CreateSet(ShotStraight, enemy.x, enemy.y,
                atan2(player.y - enemy.y, player.x - enemy.x));
            s->param_i[0] = 0;  // 小玉
            s->param_i[1] = 6;  // 白
            s->param_i[2] = 5;
            s->param_i[3] = 2;
            s->param_d[0] = 3.4;
            s->param_d[1] = 0.9;
        }
        // データ崩壊：放置しても徐々に自壊する(必ず決着が付く)
        if (g_pt % 20 == 0) SetEnemyHp(enemy.hp - 1);
        break;
    }
    }
}