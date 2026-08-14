// enemyPat_tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "MidiFile.h"
#include <math.h>
#include <vector>
#include <algorithm>

// ----------------------------------------------------------------
// 17msフレーム駆動および音階設定
// ----------------------------------------------------------------
static const double FRAME_TIME_SEC = 0.017;  // 1フレーム = 17ms
static const double SYNC_OFFSET_SEC = 0.020; // 遅延補正（秒）

static const int minNote = 31; // ピアノ88鍵盤の最低音 (A0) は 21
static const int maxNote = 91; // ピアノ88鍵盤の最高音 (C8) は 108

static int g_bgmHandle = -1;

// ドレミファソラシ (C, D, E, F, G, A, B) に対応する 0〜6 のカラーインデックス変換表
// 半音(#付き)は直前のナチュラル音と同じ色に割り当て
// C(0), C#(0), D(1), D#(1), E(2), F(3), F#(3), G(4), G#(4), A(5), A#(5), B(6)
static const int noteToColor[12] = { 0, 0, 8, 8, 1, 2, 2, 3, 3, 4, 4, 5 };

struct MidiNoteOnEvent {
    double seconds;
    int note;
    int velocity;
};

static std::vector<MidiNoteOnEvent> g_midiNoteEvents;
static size_t g_eventIndex = 0;
static bool g_midiLoaded = false;

// 全弾共通の直線移動処理関数
static void ShotMoveLinear(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// レーザー鍵盤用の移動停止処理関数（画面上部に固定）
static void ShotMoveLaserKey(sEnemyShotSet* pEnemyShotSet)
{
    // 鍵盤レーザーは移動させず初期位置に固定
}

// ----------------------------------------------------------------
// EnemyShotLaser を用いて画面上部にピアノの鍵盤を生成・配置
// ----------------------------------------------------------------
static void SpawnPianoKeysLaser()
{
    const double startX = 20.0;
    const double endX = 460.0;
    const double topY = 10.0; // 画面最上部

    for (int n = minNote; n <= maxNote; ++n) {
        double norm = (double)(n - minNote) / (double)(maxNote - minNote);
        double spawnX = startX + norm * (endX - startX);
        int pitchClass = n % 12;

        // 黒鍵(#付き)判定: C#, D#, F#, G#, A#
        bool isBlackKey = (pitchClass == 1 || pitchClass == 3 || pitchClass == 6 || pitchClass == 8 || pitchClass == 10);

        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotMoveLaserKey; // 位置固定
        pSet->x = spawnX;
        pSet->y = topY;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        sEnemyShot* pLaser = new sEnemyShot;
        pLaser->x = spawnX;
        pLaser->y = topY;
        pLaser->muki = DX_PI / 2.0; // 真下 (90度)
        pLaser->speed = 0.0;        // 鍵盤として配置するため速度0

        // ドレミ7色インデックス変換
        int colorIdx = noteToColor[pitchClass];

        // レーザー弾種を指定（黒鍵は別の指定色、白鍵はドレミの対応色）
        if (isBlackKey) {
            pLaser->kind = img_enemyShotLaser[7]; // 黒鍵用レーザー画像/色
            pLaser->y -= 10;
        }
        else {
            pLaser->kind = img_enemyShotLaser[colorIdx]; // ドレミに対応する7色レーザー
        }

        pLaser->prev = pSet->pEnemyShotHead->prev;
        pLaser->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pLaser;
        pSet->pEnemyShotHead->prev = pLaser;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}

// ----------------------------------------------------------------
// [ピアノロール弾幕] 中楕円弾・7色ドレミマッピング
// ----------------------------------------------------------------
static void SpawnPianoRollShot(int note, int velocity)
{
    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

    // 音階(note)を画面横幅 (X: 40.0 ～ 440.0) にマッピング
    double norm = (double)(note - minNote) / (double)(maxNote - minNote);

    if (norm < 0.0) norm = 0.0;
    if (norm > 1.0) norm = 1.0;

    double spawnX = 20.0 + norm * (460.0 - 20.0);
    double spawnY = 25.0; // 鍵盤レーザーの位置から真下へ発射

    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = ShotMoveLinear;
    pSet->x = spawnX;
    pSet->y = spawnY;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = spawnX;
    pShot->y = spawnY;
    pShot->muki = DX_PI / 2.0; // 真下 (90度)
    pShot->speed = 2.5;        // 一定速度

    // ドレミファソラシの7色インデックス変換 (0〜6)
    int pitchClass = note % 12;
    int colorIdx = noteToColor[pitchClass];

    // 中楕円弾を設定
    pShot->kind = img_enemyShotMediumOval[colorIdx];

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ----------------------------------------------------------------
// MIDI解析処理
// ----------------------------------------------------------------
static void LoadMidiEvents()
{
    g_midiNoteEvents.clear();
    g_eventIndex = 0;

    smf::MidiFile midifile;
    if (!midifile.read("AI_work/oldBGM/Turkish March.mid")) {
        g_midiLoaded = false;
        return;
    }

    midifile.doTimeAnalysis();
    midifile.linkNotePairs();

    for (int track = 0; track < midifile.getTrackCount(); ++track) {
        for (int event = 0; event < midifile.getEventCount(track); ++event) {
            const smf::MidiEvent& mev = midifile[track][event];

            if (mev.isNoteOn() && mev.getVelocity() > 0) {
                MidiNoteOnEvent e;
                e.seconds = mev.seconds;
                e.note = mev.getKeyNumber();
                e.velocity = mev.getVelocity();
                g_midiNoteEvents.push_back(e);
            }
        }
    }

    std::sort(g_midiNoteEvents.begin(), g_midiNoteEvents.end(),
        [](const MidiNoteOnEvent& a, const MidiNoteOnEvent& b) {
        return a.seconds < b.seconds;
    });

    g_midiLoaded = true;
}

// ----------------------------------------------------------------
// 現在時刻取得（17ms精度 / BGMハンドル連動）
// ----------------------------------------------------------------
static double GetCurrentBgmTimeInSeconds()
{
    if (g_bgmHandle >= 0 && CheckSoundMem(g_bgmHandle) == 1) {
        LONGLONG currentMs = GetSoundCurrentTime(g_bgmHandle);
        if (currentMs >= 0) {
            return (double)currentMs / 1000.0;
        }
    }
    return count * FRAME_TIME_SEC;
}

// ----------------------------------------------------------------
// 敵本体のメイン制御関数
// ----------------------------------------------------------------
void EnemyPat_TurkishMarch()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 10 * 60;
        muki = 1;

        // BGMハンドルの初期化
        g_bgmHandle = currentBGMHandle;

        // 鍵盤用レーザー弾の生成・配置
        SpawnPianoKeysLaser();

        LoadMidiEvents();
    }
    else {
        enemy.x += 2.0 * (double)muki;
        if (enemy.x > 380.0) { enemy.x = 380.0; muki = -1; }
        else if (enemy.x < 100.0) { enemy.x = 100.0; muki = 1; }
    }

    // ------------------------------------------------------------
    // 17ms精度同期・ピアノロール弾幕処理
    // ------------------------------------------------------------
    if (g_midiLoaded && !g_midiNoteEvents.empty()) {
        double currentBgmTime = GetCurrentBgmTimeInSeconds() + SYNC_OFFSET_SEC;

        while (g_eventIndex < g_midiNoteEvents.size() &&
            g_midiNoteEvents[g_eventIndex].seconds <= currentBgmTime)
        {
            const auto& noteEv = g_midiNoteEvents[g_eventIndex];

            // ピアノロール中楕円弾を発射
            SpawnPianoRollShot(noteEv.note, noteEv.velocity);

            g_eventIndex++;
        }
    }
}