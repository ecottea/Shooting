#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 飛鉢「伝説の飛空円盤」 (Hard/Lunatic 相当)
// 聖白蓮 ラストスペル再現
//
// 仕様概要:
//   - 円状に並んだ札弾が膨張・収縮しながら降ってくる
//   - 軌道サイクル: 固定 → 自機狙い → 自機狙い → 固定 の繰り返し
//   - 体力段階で色と弾数・発射間隔が変化
//     青(300F) → 水色(250F) → 黄(230F) → 紫(200F) → 赤(150F) → 発狂(150F)
//   - ボス本体当たり判定なし想定（本実装ではHP管理のみ）
//   - count / pEnemyShotSet->count / pEnemyShot->count のインクリメント
//     および画面外弾消去はメインルーチン側で行う
// ============================================================

// 段階定義
enum FlyingSaucerPhase {
    PHASE_BLUE = 0,   // 青
    PHASE_CYAN,       // 水色
    PHASE_YELLOW,     // 黄
    PHASE_PURPLE,     // 紫 (マゼンタ)
    PHASE_RED,        // 赤
    PHASE_RAGE        // 発狂
};

// 段階ごとのパラメータ (Lunatic 相当)
// 弾数は1円あたりの枚数、interval は発射間隔フレーム
static const int PHASE_BULLET_NUM[] = { 24, 24, 26, 28, 30, 32 };
static const int PHASE_INTERVAL[] = { 300, 250, 230, 200, 150, 150 };
static const int PHASE_COLOR_INDEX[] = { 4, 3, 1, 5, 0, 0 }; // 青,シアン,黄,マゼンタ,赤,赤

// 体力段階の境界 (maxHp=200 前提、残りHP割合で遷移)
// 青→水色: 80%, 水色→黄: 60%, 黄→紫: 40%, 紫→赤: 20%, 赤→発狂: 10%
static const double PHASE_HP_RATIO[] = { 1.00, 0.80, 0.60, 0.40, 0.20, 0.10 };

// ------------------------------------------------------------
// 1つの円盤弾幕セットのパターン関数
// param_i[0] : 弾数
// param_i[1] : 色インデックス (0:赤〜)
// param_i[2] : 軌道タイプ (0:固定, 1:自機狙い)
// param_d[0] : 基準角度 (固定時は初期角度、自機狙い時は発射時の自機角度)
// param_d[1] : 初期半径
// param_d[2] : 角速度 (rad/frame)
// param_d[3] : 膨張・収縮の振幅
// param_d[4] : 膨張・収縮の角速度
// ------------------------------------------------------------
static void ShotFlyingSaucerRing(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ弾を生成
    if (pEnemyShotSet->count == 0) {
        // 発射音
        if (CheckSoundMem(sound_enemyShot_medium)) {
            StopSoundMem(sound_enemyShot_medium);
        }
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int bulletNum = pEnemyShotSet->param_i[0];
        const int colorIdx = pEnemyShotSet->param_i[1];
        const int aimType = pEnemyShotSet->param_i[2]; // 0:固定 1:自機狙い

        // 基準角度決定
        double baseAngle;
        if (aimType == 1) {
            // 自機狙い: 発射瞬間の自機方向を基準にする
            baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        }
        else {
            // 固定: セット作成時に決めた角度を使用
            baseAngle = pEnemyShotSet->param_d[0];
        }

        const double initRadius = pEnemyShotSet->param_d[1];
        const double angularSpeed = pEnemyShotSet->param_d[2];
        const double expandAmp = pEnemyShotSet->param_d[3];
        const double expandSpeed = pEnemyShotSet->param_d[4];

        for (int i = 0; i < bulletNum; i++) {
            pEnemyShot = new sEnemyShot;

            // 円周上の初期位置
            const double angle = baseAngle + (DX_PI * 2.0 * i) / bulletNum;
            pEnemyShot->x = pEnemyShotSet->x + initRadius * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + initRadius * sin(angle);

            // 向きは円の接線方向（回転しながら外側へ広がる動きを後で与える）
            // ここでは一旦放射方向にしておき、移動処理で調整する
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0; // 速度は移動処理で動的に計算

            // 札弾相当として菱形弾を使用
            pEnemyShot->kind = img_enemyShotDiamond[colorIdx];

            // 弾個別パラメータ
            // param_d[0] : この弾の初期角度（円上の位置角度）
            // param_d[1] : 基準半径
            // param_d[2] : 角速度
            // param_d[3] : 膨張振幅
            // param_d[4] : 膨張角速度
            // param_d[5] : 中心x (発射時の中心を保持)
            // param_d[6] : 中心y
            // param_i[0] : 寿命カウンタ用など自由
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_d[1] = initRadius;
            pEnemyShot->param_d[2] = angularSpeed;
            pEnemyShot->param_d[3] = expandAmp;
            pEnemyShot->param_d[4] = expandSpeed;
            pEnemyShot->param_d[5] = pEnemyShotSet->x;
            pEnemyShot->param_d[6] = pEnemyShotSet->y;
            pEnemyShot->param_i[0] = 0;

            pEnemyShot->margin = 480;

            // リスト連結
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの移動処理
    // 円を描きながら膨張・収縮し、徐々に外側へ広がっていく
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 経過フレーム (弾自身のcountはメインでインクリメントされる前提だが、
        // セットのcountを基準にしても良い。ここでは弾のcountを利用)
        const int t = pShot->count;

        const double baseAngle = pShot->param_d[0];
        const double baseRadius = pShot->param_d[1];
        const double angSpeed = pShot->param_d[2];
        const double expandAmp = pShot->param_d[3];
        const double expandSpeed = pShot->param_d[4];
        const double cx = pShot->param_d[5];
        const double cy = pShot->param_d[6];

        // 回転角度
        const double angle = baseAngle + angSpeed * t;

        // 半径: 基本半径 + 時間経過による外側への広がり + サイン波による膨張収縮
        // 外側への広がり速度は段階によって少し変えても良いが、ここでは固定気味に
        const double expand = expandAmp * sin(expandSpeed * t);
        const double radius = baseRadius + 0.55 * t + expand;

        // 位置更新
        pShot->x = cx + radius * cos(angle);
        pShot->y = cy + radius * sin(angle);

        // 見た目の向き（菱形弾の向き合わせ用）
        // 進行方向の接線に合わせる
        pShot->muki = angle + DX_PI * 0.5;

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// void EnemyPat_HikouEnban_Grok() で実装すること
// ------------------------------------------------------------
void EnemyPat_HikouEnban_Grok()
{
    // 静的変数で状態を保持
    static int  currentPhase;      // 現在の段階
    static int  waveCounter;       // 波カウンタ (固定/自機狙いの切替用)
    static int  nextShotFrame;     // 次の発射フレーム
    static int  ringSerial;        // 円盤の通し番号 (角度オフセット用)
    static double fixedBaseAngle;  // 固定波用の基準角度

    // ゲーム画面は 480x480
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 180.0;            // 少し上めに配置
        enemy.maxHp = enemy.hp = 200; // 変更禁止

        currentPhase = PHASE_BLUE;
        waveCounter = 0;
        nextShotFrame = 30;       // 開幕少し待つ
        ringSerial = 0;
        fixedBaseAngle = 0.0;
    }

    // ボスはほぼ固定位置（軽く左右に揺れる程度）
    // 原作では大きく動かない
    enemy.x = 240.0 + 18.0 * sin(count * 0.02);
    enemy.y = 180.0 + 6.0 * sin(count * 0.015);

    // --- 段階判定 (残りHP割合) ---
    const double hpRatio = (double)enemy.hp / (double)enemy.maxHp;
    int newPhase = PHASE_BLUE;
    for (int i = 5; i >= 0; i--) {
        if (hpRatio <= PHASE_HP_RATIO[i]) {
            newPhase = i;
            break;
        }
    }
    // 発狂は残り10%以下、または十分時間が経った場合に強制遷移しても良い
    if (hpRatio <= 0.10) {
        newPhase = PHASE_RAGE;
    }
    if (newPhase != currentPhase) {
        currentPhase = newPhase;
        // 段階移行時は次の発射を少し早める
        if (nextShotFrame > count + 20) {
            nextShotFrame = count + 20;
        }
    }

    // --- 発射判定 ---
    if (count >= nextShotFrame) {
        const int bulletNum = PHASE_BULLET_NUM[currentPhase];
        const int interval = PHASE_INTERVAL[currentPhase]/2;
        const int colorIdx = PHASE_COLOR_INDEX[currentPhase];

        // 軌道タイプ決定: 固定 → 自機狙い → 自機狙い → 固定 のサイクル
        // waveCounter % 4 で判定
        const int cycle = waveCounter % 4;
        int aimType;
        if (cycle == 0 || cycle == 3) {
            aimType = 0; // 固定
        }
        else {
            aimType = 1; // 自機狙い
        }

        // 固定波の場合は基準角度を更新（徐々に回転させる）
        if (aimType == 0) {
            fixedBaseAngle += 0.35; // 波ごとに少しずらす
            if (fixedBaseAngle > DX_PI * 2.0) {
                fixedBaseAngle -= DX_PI * 2.0;
            }
        }

        // 円盤セットを生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFlyingSaucerRing;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = ringSerial++;

        // パラメータ設定
        pEnemyShotSet->param_i[0] = bulletNum;
        pEnemyShotSet->param_i[1] = colorIdx;
        pEnemyShotSet->param_i[2] = aimType;

        if (aimType == 0) {
            // 固定: 事前に決めた角度
            pEnemyShotSet->param_d[0] = fixedBaseAngle + (ringSerial % 3) * (DX_PI * 2.0 / 3.0);
        }
        else {
            // 自機狙いは patternFunc 内で再計算するのでダミー
            pEnemyShotSet->param_d[0] = 0.0;
        }

        // 初期半径・角速度・膨張パラメータ
        // 原作は比較的ゆったりした膨張収縮
        pEnemyShotSet->param_d[1] = 28.0 + (currentPhase * 2.0);           // 初期半径
        pEnemyShotSet->param_d[2] = 0.018 + currentPhase * 0.0015;         // 角速度 (段階で少し速く)
        pEnemyShotSet->param_d[3] = 10.0 + currentPhase * 1.5;            // 膨張振幅
        pEnemyShotSet->param_d[4] = 0.045;                                 // 膨張角速度

        // 弾リストの番兵ノード
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // セットリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        // 次の発射タイミングと波カウンタ更新
        nextShotFrame = count + interval;
        waveCounter++;
    }
}