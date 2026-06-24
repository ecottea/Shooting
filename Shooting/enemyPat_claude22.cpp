// enemyPat_rhythmGame.cpp
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// 音ゲーをモチーフにした弾幕「BEAT BARRAGE」
//
//  ゲーム画面: 480×480 px  /  60fps
//  BPM: 120  →  1拍 = 30f,  8分音符 = 15f,  16分音符 ≒ 8f
//
//  7レーン（IIDX/BMS風）を持つ敵が BPM120 のリズムで弾幕を展開する。
//  4小節（480f）を1サイクルとしてループする。
//
//  【小節構成】
//    小節1 (c =   0.. 119) : 8分音符でランダムレーンの単ノート
//    小節2 (c = 120.. 239) : 4分音符でコード（2〜4レーン同時発射）
//    小節3 (c = 240.. 359) : スクラッチ（扇状鱗弾を左右交互）
//    小節4 (c = 360.. 479) : 全レーン大爆発 ＋ 16分音符の単ノート乱打
//
//  【弾幕サブルーチン】
//    ShotRhythmNote   … 単ノート   : ダイヤ弾1発，レーン色，やや狙い寄り
//    ShotScratch      … スクラッチ : 扇状鱗弾12発，左右交互
//    ShotMeasureBlast … 小節爆発   : 全レーン中玉 ＋ 放射状小玉16発
//
//  【素材確認メモ】
//    弾種 : img_enemyShotSmallBall[0..7]  小玉
//           img_enemyShotMediumBall[0..7] 中玉
//           img_enemyShotLargeBall[0..7]  大玉
//           img_enemyShotBullet[0..7]     銃弾
//           img_enemyShotScale[0..7]      鱗弾
//           img_enemyShotDiamond[0..7]    菱形弾
//    色   : 0赤, 1黄, 2緑, 3シアン, 4青, 5マゼンタ, 6白, 7黒 (, 8橙)
//    SE   : sound_enemyShot_light / medium / heavy / extreme
//    関数 : GetRand(x) → 0..x の整数（x+1通り）
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ────────────────────────────────────────────────────────────────────────
// レーン定義
//   画面幅 480px に 7レーンを等間隔配置（両端に余白 36px）
//   IIDX 配色: 白鍵→赤・緑・青・白，黒鍵→黄・シアン・マゼンタ
// ────────────────────────────────────────────────────────────────────────
static const double LANE_X[7] = { 36.0, 104.0, 172.0, 240.0, 308.0, 376.0, 444.0 };
static const int    LANE_COLOR[7] = { 0,     2,     4,     6,     1,     3,     5 };
//                                    赤    緑    青    白    黄   シアン  マゼ

// ────────────────────────────────────────────────────────────────────────
// BPM タイミング定数（BPM120, 60fps）
// ────────────────────────────────────────────────────────────────────────
static const int BPM_BEAT = 30;   // 1拍
static const int BPM_EIGHTH = 15;   // 8分音符
static const int BPM_SIXTEENTH = 8;   // 16分音符（7.5f を切り上げ）


// ════════════════════════════════════════════════════════════════════════
// ヘルパー関数
// ════════════════════════════════════════════════════════════════════════

// EnemyShot を生成して ShotSet のリストに末尾挿入する
static void AppendShot(sEnemyShotSet* set,
    double x, double y,
    double muki, double speed, int imgKind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = imgKind;
    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;
}

// ShotSet を生成してグローバルキューに末尾挿入する
static void AppendShotSet(sEnemyShotSet::PatternFunc func,
    double x, double y,
    double muki, int kind)
{
    sEnemyShotSet* s = new sEnemyShotSet;
    s->count = 0;
    s->patternFunc = func;
    s->x = x;
    s->y = y;
    s->muki = muki;
    s->kind = kind;
    s->pEnemyShotHead = new sEnemyShot;
    s->pEnemyShotHead->prev = s->pEnemyShotHead;
    s->pEnemyShotHead->next = s->pEnemyShotHead;
    s->prev = enemyShotSetHead.prev;
    s->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = s;
    enemyShotSetHead.prev = s;
}

// ShotSet 内の全弾を直進移動させる（各フレームの終わりに呼ぶ）
static void MoveAllStraight(sEnemyShotSet* s)
{
    for (sEnemyShot* p = s->pEnemyShotHead->next;
        p != s->pEnemyShotHead; p = p->next)
    {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
    }
}

// プレイヤー狙い角度と「真下(π/2)」を downWeight で混合した角度を返す
//   downWeight=0.0 → 純粋な狙い撃ち
//   downWeight=1.0 → 完全に真下
static double MixedAngle(double fromX, double fromY, double downWeight)
{
    double aim = atan2(player.y - fromY, player.x - fromX);
    return aim * (1.0 - downWeight) + (DX_PI / 2.0) * downWeight;
}


// ════════════════════════════════════════════════════════════════════════
// 弾幕サブルーチン A：ShotRhythmNote（単ノート）
// ────────────────────────────────────────────────────────────────────────
//  音ゲーの通常ノートをモチーフに，指定レーンから菱形弾を1発発射する。
//  向きはプレイヤー方向寄り（狙い40% ＋ 真下60%）なので，
//  レーンの位置からほぼ真下に落ちながら少しプレイヤーを追う。
//
//  pEnemyShotSet->muki : 呼び出し元で MixedAngle() 済みの発射角度
//  pEnemyShotSet->kind : レーン番号 (0..6)
// ════════════════════════════════════════════════════════════════════════
static void ShotRhythmNote(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 1)
            StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        AppendShot(pEnemyShotSet,
            pEnemyShotSet->x,
            pEnemyShotSet->y,
            pEnemyShotSet->muki,
            3.5,
            img_enemyShotLargeBall[LANE_COLOR[pEnemyShotSet->kind]]);
    }
    MoveAllStraight(pEnemyShotSet);
}


// ════════════════════════════════════════════════════════════════════════
// 弾幕サブルーチン B：ShotScratch（スクラッチ）
// ────────────────────────────────────────────────────────────────────────
//  DJのスクラッチ動作をモチーフに，真下を中心とした扇状に鱗弾12発を広げる。
//  kind == 0 → 左扇（左端から右端へ角度が増える順に生成）
//  kind == 1 → 右扇（右端から左端へ角度が増える順に生成）
//  左右交互に呼ぶことでスクラッチの「往復感」を演出する。
//  内側ほど遅く外側ほど速い速度差が，扇の広がりをくっきり見せる。
// ════════════════════════════════════════════════════════════════════════
static void ShotScratch(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int    N = 12;
        const double SPREAD = DX_PI / 2.5;   // 扇の広がり（約 72°）
        const double CENTER = DX_PI / 2.0;   // 中心方向 = 真下

        for (int i = 0; i < N; i++) {
            double t = (double)i / (N - 1);   // 0.0 … 1.0

            double muki = (pEnemyShotSet->kind == 0)
                ? CENTER - SPREAD / 2.0 + SPREAD * t   // 左扇：左端→右端
                : CENTER + SPREAD / 2.0 - SPREAD * t;  // 右扇：右端→左端

            double speed = 1.8 + i * 0.25;   // i が大きいほど速い

            AppendShot(pEnemyShotSet,
                pEnemyShotSet->x,
                pEnemyShotSet->y,
                muki, speed,
                img_enemyShotScale[i % 7]);
        }
    }
    MoveAllStraight(pEnemyShotSet);
}


// ════════════════════════════════════════════════════════════════════════
// 弾幕サブルーチン C：ShotMeasureBlast（小節爆発）
// ────────────────────────────────────────────────────────────────────────
//  強拍（小節の頭）を飾る派手なパターン。
//    ① 全7レーンのX座標から中玉を1発ずつ（レーン色，やや狙い寄り）
//    ② 敵の中心から放射状に小玉16発（全方位，虹色）
//  の2層構造で「ビートの花火」を演出する。
// ════════════════════════════════════════════════════════════════════════
static void ShotMeasureBlast(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // ① 全7レーンから中玉（プレイヤー方向 50% ＋ 真下 50%）
        for (int i = 0; i < 7; i++) {
            double lx = LANE_X[i];
            double ly = pEnemyShotSet->y;
            double muki = MixedAngle(lx, ly, 0.5);
            AppendShot(pEnemyShotSet, lx, ly, muki, 4.0,
                img_enemyShotLargeBall[LANE_COLOR[i]]);
        }

        // ② 放射状に小玉16発（ビートの花火）
        for (int i = 0; i < 80; i++) {
            double muki = (2.0 * DX_PI / 80) * i;
            AppendShot(pEnemyShotSet,
                pEnemyShotSet->x,
                pEnemyShotSet->y,
                muki, 2.5,
                img_enemyShotSmallBall[i % 7]);
        }
    }
    MoveAllStraight(pEnemyShotSet);
}


// ════════════════════════════════════════════════════════════════════════
// 敵本体：EnemyPat_RhythmGame_Claude
// ════════════════════════════════════════════════════════════════════════
//  【移動】x=60〜420 の範囲を speed 1.2 で左右バウンス
//  【弾幕】4小節（480f）ループの各フェーズで AppendShotSet を呼ぶ
// ════════════════════════════════════════════════════════════════════════
void EnemyPat_RhythmGame_Claude()
{
    static int moveDir;

    // ── 初期化（count == 1 のフレームのみ） ─────────────────────────────
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        moveDir = 1;
    }
    else {
        // 左右バウンス（端に当たったら反転）
        enemy.x += 1.2 * moveDir;
        if (enemy.x > 420.0) { enemy.x = 420.0; moveDir = -1; }
        if (enemy.x < 60.0) { enemy.x = 60.0; moveDir = 1; }
    }

    // ── フレーム位相（4小節 = 480f をループ） ───────────────────────────
    //   c = 0 が各サイクルの先頭（count==1 のフレームも c=0）
    const int c = (count - 1) % 240 * 2;

    // ┌────────────────────────────────────────────────────────────────┐
    // │ 小節1 (c=0..119)                                               │
    // │   8分音符（15f）ごとにランダムレーンへ単ノートを1発投下する。   │
    // │   音ゲーの「ノーツが降ってくる」感覚をそのまま弾幕化。          │
    // └────────────────────────────────────────────────────────────────┘
    if (c < 120 && c % BPM_EIGHTH == 0) {
        int lane = GetRand(6);   // 0..6
        AppendShotSet(ShotRhythmNote,
            LANE_X[lane],
            enemy.y + 10.0,
            MixedAngle(LANE_X[lane], enemy.y + 10.0, 0.6),
            lane);
    }

    // ┌────────────────────────────────────────────────────────────────┐
    // │ 小節2 (c=120..239)                                             │
    // │   4分音符（30f）ごとにコード（複数レーン同時）を発射する。      │
    // │   4種のコードパターンからランダムに1つ選択。                    │
    // │   「コード押し」「密集」「散開」の打ち分けで変化をつける。      │
    // └────────────────────────────────────────────────────────────────┘
    if (c >= 120 && c < 240 && (c - 120) % BPM_BEAT == 0) {
        // レーン番号の組み合わせ（-1 は番兵：そこで break）
        static const int CHORDS[4][4] = {
            { 0, 2, 4, 6 },   // 散開コード（偶数レーン，4発）
            { 0, 1, 2, 3 },   // 左寄せコード（4発）
            { 3, 4, 5, 6 },   // 右寄せコード（4発）
            { 1, 3, 5, -1 },  // 奇数レーンコード（3発）
        };
        int pat = GetRand(3);   // 0..3
        for (int i = 0; i < 4; i++) {
            int lane = CHORDS[pat][i];
            if (lane < 0) break;
            AppendShotSet(ShotRhythmNote,
                LANE_X[lane],
                enemy.y + 10.0,
                MixedAngle(LANE_X[lane], enemy.y + 10.0, 0.6),
                lane);
        }
    }

    // ┌────────────────────────────────────────────────────────────────┐
    // │ 小節3 (c=240..359)                                             │
    // │   1拍（30f）ごとにスクラッチを左右交互で発射する。              │
    // │   左扇→右扇→左扇→右扇 の4回で1小節。                         │
    // └────────────────────────────────────────────────────────────────┘
    if (c >= 240 && c < 360 && (c - 240) % BPM_BEAT == 0) {
        int dir = ((c - 240) / BPM_BEAT) % 2;   // 0→左扇, 1→右扇 交互
        AppendShotSet(ShotScratch,
            enemy.x,
            enemy.y + 10.0,
            MixedAngle(enemy.x, enemy.y + 10.0, 0.6),
            dir);
    }

    // ┌────────────────────────────────────────────────────────────────┐
    // │ 小節4 (c=360..479)                                             │
    // │   小節頭に全レーン大爆発（1回のみ）で強拍を演出する。           │
    // │   続けて 16分音符（8f）ごとにランダムレーンの単ノート乱打。     │
    // │   爆発直後から降り注ぐ菱形弾がフィナーレ感を高める。           │
    // └────────────────────────────────────────────────────────────────┘
    if (c >= 360 && c < 480) {
        if (c == 360) {
            AppendShotSet(ShotMeasureBlast,
                enemy.x,
                enemy.y + 10.0,
                MixedAngle(enemy.x, enemy.y + 10.0, 0.6),
                0);
        }
        if ((c - 360) % BPM_SIXTEENTH == 0) {
            int lane = GetRand(6);
            AppendShotSet(ShotRhythmNote,
                LANE_X[lane],
                enemy.y + 10.0,
                MixedAngle(LANE_X[lane], enemy.y + 10.0, 0.6),
                lane);
        }
    }
}