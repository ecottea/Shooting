// EnemyPat_Irairabou_Gemini.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// イライラ・コリドー（迷路＆電撃ギミック）のメイン制御
static void ShotMazeCore(sEnemyShotSet* pSet)
{
    // === フェーズ管理と難易度パラメータ ===
    int phase = 1;
    if (pSet->count > 500)  phase = 2; // スパーク追加
    if (pSet->count > 1000) phase = 3; // 発狂（加速・幅狭化）

    double speed = (phase == 3) ? 3.5 : 2.0;       // 弾の落下速度
    double gap_width = (phase == 3) ? 55.0 : 85.0; // 安全地帯（通路）の幅（片側）
    double freq = (phase == 3) ? 0.025 : 0.015;    // うねりの周期
    int spawn_interval = (phase == 3) ? 3 : 5;     // 壁の生成間隔

    // 通路の中心座標（波の位相を連続的に加算して、フェーズ移行時のワープを防ぐ）
    pSet->param_d[0] += freq;
    double center_x = 240.0 + 130.0 * sin(pSet->param_d[0]);

    // === 壁とギミックの生成 ===
    if (pSet->count % spawn_interval == 0) {

        // 壁生成時の効果音（鳴りすぎ防止のため間引く）
        if (pSet->count % (spawn_interval * 4) == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 壁弾の生成（X座標を16ピクセル刻みで敷き詰める）
        for (double x = 0; x <= 480; x += 16.0) {
            // 通路の幅の中には壁を作らない（安全地帯）
            if (fabs(x - center_x) < gap_width) continue;

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = x;
            pShot->y = pSet->y;
            pShot->muki = DX_PI / 2.0; // 下向き（描画用、実際の移動は独自計算）
            pShot->speed = speed;
            pShot->kind = img_enemyShotMediumBall[4]; // 青玉（壁）
            pShot->param_i[0] = 0; // 0 = 壁フラグ

            // リストに繋ぐ
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // スパーク弾（障害物）の生成（Phase 2以降）
        // 壁の生成タイミングに合わせて、通路のド真ん中に配置する
        if (phase >= 2 && pSet->count % (spawn_interval * 15) == 0) {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            sEnemyShot* pSpark = new sEnemyShot;
            pSpark->x = center_x;
            pSpark->y = pSet->y;
            pSpark->muki = DX_PI / 2.0;
            pSpark->speed = speed;
            pSpark->kind = img_enemyShotMediumOval[0]; // 赤楕円（スパーク）

            pSpark->param_i[0] = 1;                    // 1 = スパークフラグ
            pSpark->param_d[0] = center_x;             // 振動の中心軸（生成時の通路の中心）
            pSpark->param_d[1] = gap_width - 10.0;     // 振動の振幅（壁に少し被らない程度）
            pSpark->param_i[1] = (GetRand(1) == 0) ? 1 : -1; // 最初の移動方向（左右ランダム）

            pSpark->prev = pSet->pEnemyShotHead->prev;
            pSpark->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pSpark;
            pSet->pEnemyShotHead->prev = pSpark;
        }
    }

    // === 弾の移動処理（メインルーチン任せにせず独自軌道を描かせる） ===
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 壁弾：完全な真下へのスクロール
            pShot->y += pShot->speed;
        }
        else if (pShot->param_i[0] == 1) {
            // スパーク弾：壁と同じ速度で落下しつつ、生成時の通路幅に合わせて左右に往復する
            pShot->y += pShot->speed;

            double c_x = pShot->param_d[0]; // 中心軸
            double amp = pShot->param_d[1]; // 振幅
            // countを用いて左右にサイン波で振動させる
            pShot->x = c_x + amp * sin(pShot->count * 0.08 * pShot->param_i[1]);

            // スパークらしく見せるため、描画角度(muki)を回転させる
            pShot->muki += 0.3;
        }

        pShot = pShot->next;
    }
}


// 敵本体のパターン
void EnemyPat_Irairabou_Gemini()
{
    // 初期化処理
    if (count == 1) {
        // ボスは画面上部中央から動かず、迷路の生成口となる
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 250; // ギミックを見せるためHPは高めに設定
    }

    // 敵の位置は固定
    enemy.x = 240.0;
    enemy.y = 40.0;

    // ゲーム開始直後に、迷路生成を管理する ShotSet を1つだけ登録する
    if (count == 10) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotMazeCore;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0; // 今回は使用しない

        pSet->param_d[0] = 0.0; // 迷路のうねりの位相トラッキング用

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}