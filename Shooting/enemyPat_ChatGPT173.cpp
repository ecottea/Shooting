#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 狂奏「弾幕五線譜」
// 小玉で五線譜、中楕円弾で音符、小玉の列で音符の符幹を表現する。
// 譜面が下へ流れ、音符が順番に飛び出した後、五線譜も一斉に弾ける。
// ============================================================

enum ScoreShotRole {
    SCORE_STAFF = 0,
    SCORE_NOTE_HEAD = 1,
    SCORE_NOTE_STEM = 2
};

// 弾をセットのリスト末尾へ追加する。
// count の更新と画面外弾の削除はメインルーチンに任せる。
static void ScoreAddShot(
    sEnemyShotSet* pSet,
    int image,
    double x,
    double y,
    int role,
    int launchFrame,
    int index,
    double phase,
    double groupX,
    double groupY)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = 0.0;
    pShot->speed = 0.0;
    pShot->kind = image;

    pShot->param_i[0] = role;
    pShot->param_i[1] = launchFrame;
    pShot->param_i[2] = 0; // 発射済みフラグ
    pShot->param_i[3] = index;

    pShot->param_d[0] = x;       // 譜面上の基準X座標
    pShot->param_d[1] = y;       // 譜面上の基準Y座標
    pShot->param_d[2] = phase;   // 譜面の揺れの位相
    pShot->param_d[4] = groupX;  // 音符グループの中心座標
    pShot->param_d[5] = groupY;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 五線譜一段分と、その上を流れる音符を管理する。
static void ScorePattern(sEnemyShotSet* pEnemyShotSet)
{
    const int STAFF_ROWS = 5;
    const int STAFF_POINTS = 19;
    const int NOTE_COUNT = 6;
    const double STAFF_X_START = 14.0;
    const double STAFF_X_INTERVAL = 25.0;
    const double STAFF_Y_INTERVAL = 22.0;
    const double SCROLL_SPEED = 0.38*3;

    if (pEnemyShotSet->count == 0) {
        // 五本の線。白い小玉を等間隔に並べて譜面を描く。
        for (int row = 0; row < STAFF_ROWS; row++) {
            for (int col = 0; col < STAFF_POINTS; col++) {
                const double x = STAFF_X_START + col * STAFF_X_INTERVAL;
                const double y = pEnemyShotSet->param_d[0] + row * STAFF_Y_INTERVAL;
                const double phase = col * 0.17 + row * 0.43;
                ScoreAddShot(
                    pEnemyShotSet,
                    img_enemyShotSmallBall[6],
                    x, y,
                    SCORE_STAFF,
                    260,
                    row,
                    phase,
                    x, y);
            }
        }

        // 音符の高さ。五線上と線の間を使って旋律らしい並びにする。
        const int pitches[NOTE_COUNT] = { 1, 4, 2, 6, 3, 5 };
        const int noteColors[NOTE_COUNT] = { 1, 3, 5, 1, 3, 5 };

        for (int i = 0; i < NOTE_COUNT; i++) {
            const double x = 42.0 + i * 78.0;
            const double y = pEnemyShotSet->param_d[0] + pitches[i] * 11.0;
            const double phase = i * 0.73;
            const int launchFrame = 92 + i * 22;
            const int color = noteColors[i];

            // 音符の頭。
            ScoreAddShot(
                pEnemyShotSet,
                img_enemyShotMediumOval[color],
                x, y,
                SCORE_NOTE_HEAD,
                launchFrame,
                i,
                phase,
                x, y);

            // 音符の符幹。小玉を縦に並べて表現する。
            for (int stem = 1; stem <= 5; stem++) {
                ScoreAddShot(
                    pEnemyShotSet,
                    img_enemyShotSmallBall[color],
                    x + 5.0, y - stem * 4.5,
                    SCORE_NOTE_STEM,
                    launchFrame,
                    i,
                    phase,
                    x, y);
            }
        }

        // 譜面が現れるタイミング。
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 譜面の崩壊を予告。
    if (pEnemyShotSet->count == 235) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 五線譜の小玉が一斉に飛び出す。
    if (pEnemyShotSet->count == 260) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    const double t = (double)pEnemyShotSet->count;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const int role = pShot->param_i[0];
        const int launchFrame = pShot->param_i[1];
        const double baseX = pShot->param_d[0];
        const double baseY = pShot->param_d[1];
        const double phase = pShot->param_d[2];

        if (pShot->param_i[2] == 0) {
            // 発射前は譜面に沿って緩やかに波打ちながら下へ流れる。
            pShot->x = baseX + 3.5 * sin(t * 0.022 + phase);
            pShot->y = baseY + t * SCROLL_SPEED
                + 2.5 * sin(baseX * 0.025 - t * 0.018 + phase);

            if (pEnemyShotSet->count >= launchFrame) {
                double aimX = pShot->x;
                double aimY = pShot->y;

                // 音符の頭と符幹は同じ方向へ飛び、形を保つ。
                if (role == SCORE_NOTE_HEAD || role == SCORE_NOTE_STEM) {
                    const double groupX = pShot->param_d[4]
                        + 3.5 * sin(t * 0.022 + phase);
                    const double groupY = pShot->param_d[5]
                        + t * SCROLL_SPEED
                        + 2.5 * sin(pShot->param_d[4] * 0.025 - t * 0.018 + phase);
                    aimX = groupX;
                    aimY = groupY;
                }

                pShot->muki = atan2(player.y - aimY, player.x - aimX);
                if (role == SCORE_STAFF) {
                    pShot->speed = 2.15 + (pShot->param_i[3] % 3) * 0.18;
                }
                else {
                    pShot->speed = 2.75;
                }
                pShot->param_i[2] = 1;

                // 音符の頭が飛び出す瞬間に軽い発射音を鳴らす。
                if (role == SCORE_NOTE_HEAD) {
                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                }
            }
        }

        if (pShot->param_i[2] != 0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体の弾幕パターン
// ============================================================
void EnemyPat_StaffNotation_ChatGPT()
{
    static int direction;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 48.0;
        enemy.maxHp = enemy.hp = 200;
        direction = 1;
    }
    else {
        enemy.x += direction * 0.85;
        if (enemy.x >= 360.0) direction = -1;
        if (enemy.x <= 120.0) direction = 1;
        enemy.y = 48.0 + 8.0 * sin(count * 0.012);
    }

    // 一定間隔で新しい小節を出現させる。交互に譜面の高さを変える。
    if (count % 200 == 1) {
        const int measureIndex = (count - 1) / 200;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ScorePattern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = measureIndex;
        pEnemyShotSet->param_d[0] = 96.0 + (measureIndex % 2) * 24.0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
