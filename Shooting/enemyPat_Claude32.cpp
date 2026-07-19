// enemyPat_Tmp.cpp
//
// ボンバーマン弾幕：爆弾全設置一斉起爆（ボムカーペット）
//
// ＜動作サイクル：240f ≒ 4秒＞
//   ShotBombArray->count:
//     == 0         : 爆弾弾 9 個を生成し、格子スナップした目標位置へ向けて放つ
//     1  〜  60    : ease-out cubic で目標格子点へ飛行
//     61 〜  90    : 目標位置で上下に揺れる（設置感）
//     91 〜 120    : 赤↔黒を 4f 周期で点滅（カウントダウン警告）
//     == 121       : 一斉起爆！ 9個×4方向=36発の炎弾 ShotSet を生成し爆弾弾を削除
//     122〜        : 炎弾飛翔中。240f 後に次サイクル開始
//
// ＜使用素材＞
//   弾画像 : img_enemyShotLargeBall[7]（黒）, [0]（赤）, img_enemyShotSmallBall[8]（橙）
//   SE     : sound_enemyShot_light（放出）, sound_enemyShot_heavy（起爆）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ─── 前方宣言 ───────────────────────────────────────────────────────────────
static void ShotBlast(sEnemyShotSet* pEnemyShotSet);


// ============================================================================
// ShotBlast  炎弾パターン：一方向に 6 発を時間差で発射（ボンバーマンの爆風）
//   pEnemyShotSet->x, y  : 発射位置（爆弾の設置マス）
//   pEnemyShotSet->muki  : 発射方向（0:右 / π/2:下 / π:左 / -π/2:上）
//
//   sEnemyShot->param_i[0] : 発射ディレイ（フレーム数）
// ============================================================================
static void ShotBlast(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 1) {
        // 6 発を 4f 間隔のディレイで生成
        for (int k = 0; k < 12; k++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(100) - 50) * 0.003;
            pEnemyShot->speed = 5.2;
            pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙（炎色）
            pEnemyShot->param_i[0] = k * 2; // k 番目の弾を k*4 フレーム後に発射

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // pEnemyShot->count が param_i[0] を超えたら移動開始
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->count >= pEnemyShot->param_i[0]) {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        pEnemyShot = pEnemyShot->next;
    }
}


// ============================================================================
// ShotBombArray  爆弾弾パターン：9 個の爆弾弾を一括管理
//   pEnemyShotSet->x, y   : 発射起点（敵の位置）
//
//   各 sEnemyShot 内:
//     param_d[0,1]  : 飛行開始位置 XY（count==0 時点の敵座標を保存）
//     param_d[2,3]  : 格子スナップされた目標位置 XY
// ============================================================================
static void ShotBombArray(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // フェーズ境界（pEnemyShotSet->count 基準）
    const int FLY_END = 60;  // 飛行終了フレーム
    const int BOB_END = 90;  // 揺れ終了フレーム
    const int BLINK_END = 120; // 点滅終了フレーム（次フレームで起爆）
    const int BLAST_F = 121; // 起爆フレーム

    // ────────────────────────────────────────────────────────
    // 初期化：爆弾弾 9 個を円形に生成して目標格子点へ向かわせる
    // ────────────────────────────────────────────────────────
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        const int    BOMB_N = 20;
        const double G = 40.0;  // グリッドサイズ（px）
        const double R = 250.0; // 爆弾配置半径（px）

        for (int i = 0; i < BOMB_N; i++) {
            // 等間隔で円周上に並べ、最寄りの格子点にスナップ
            double angle = (double)i / (double)BOMB_N * 2.0 * DX_PI - DX_PI / 2.0;
            double tx = floor((pEnemyShotSet->x + cos(angle) * R) / G + 0.5) * G;
            double ty = floor((pEnemyShotSet->y + sin(angle) * R) / G + 0.5) * G;

            // 画面端から 1 格子分の余白を確保
            if (tx < G)         tx = G;
            if (tx > 480.0 - G) tx = 480.0 - G;
            if (ty < G)         ty = G;
            if (ty > 480.0 - G) ty = 480.0 - G;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;          // 飛行開始 X（敵の位置）
            pEnemyShot->y = pEnemyShotSet->y;          // 飛行開始 Y
            pEnemyShot->speed = 0.0;                       // 位置は手動計算で更新
            pEnemyShot->kind = img_enemyShotLargeBall[7]; // 黒い大玉（爆弾）
            pEnemyShot->margin = 100.0;                     // 画面外判定を緩く
            pEnemyShot->param_d[0] = pEnemyShotSet->x;         // 飛行開始 X（保存用）
            pEnemyShot->param_d[1] = pEnemyShotSet->y;         // 飛行開始 Y（保存用）
            pEnemyShot->param_d[2] = tx;                        // 目標 X（格子点）
            pEnemyShot->param_d[3] = ty;                        // 目標 Y（格子点）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        return; // count==0 はここで終了（毎フレーム更新は次フレームから）
    }

    int c = pEnemyShotSet->count;

    // ────────────────────────────────────────────────────────
    // 一斉起爆：全爆弾位置から 4 方向の炎弾 ShotSet を生成し、爆弾弾を削除
    // ────────────────────────────────────────────────────────
    if (c == BLAST_F) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 4 方向の発射角（上・下・左・右）
        const double blastMuki[4] = {
            -DX_PI / 2.0, // 上（Y 減少方向）
             DX_PI / 2.0, // 下
             DX_PI,        // 左
             0.0           // 右
        };

        // 全爆弾の設置位置から炎弾 ShotSet を 4 方向に生成
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            for (int d = 0; d < 4; d++) {
                sEnemyShotSet* pBlastSet = new sEnemyShotSet;
                pBlastSet->count = 0;
                pBlastSet->patternFunc = ShotBlast;
                pBlastSet->x = pEnemyShot->param_d[2]; // 爆弾の目標 X
                pBlastSet->y = pEnemyShot->param_d[3]; // 爆弾の目標 Y
                pBlastSet->muki = blastMuki[d];

                pBlastSet->pEnemyShotHead = new sEnemyShot;
                pBlastSet->pEnemyShotHead->prev = pBlastSet->pEnemyShotHead;
                pBlastSet->pEnemyShotHead->next = pBlastSet->pEnemyShotHead;

                pBlastSet->prev = enemyShotSetHead.prev;
                pBlastSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pBlastSet;
                enemyShotSetHead.prev = pBlastSet;
            }
            pEnemyShot->x = 9999;
            pEnemyShot = pEnemyShot->next;
        }

        // 爆弾弾をリストから外して解放
        //pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        //while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        //    sEnemyShot* pNext = pEnemyShot->next;
        //    pEnemyShot->prev->next = pNext;
        //    pNext->prev = pEnemyShot->prev;
        //    delete pEnemyShot;
        //    pEnemyShot = pNext;
        //}
        return;
    }

    // ────────────────────────────────────────────────────────
    // 毎フレーム：フェーズに応じて位置・外観を更新
    // ────────────────────────────────────────────────────────
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        if (c <= FLY_END) {
            // 飛行フェーズ：ease-out cubic で格子点へ滑らかに移動
            double t = (double)c / (double)FLY_END;
            double e = 1.0 - (1.0 - t) * (1.0 - t) * (1.0 - t);
            pEnemyShot->x = pEnemyShot->param_d[0] + (pEnemyShot->param_d[2] - pEnemyShot->param_d[0]) * e;
            pEnemyShot->y = pEnemyShot->param_d[1] + (pEnemyShot->param_d[3] - pEnemyShot->param_d[1]) * e;
            pEnemyShot->kind = img_enemyShotLargeBall[7]; // 黒
        }
        else if (c <= BOB_END) {
            // 揺れフェーズ：X 固定、Y を正弦波で微動（設置された感を演出）
            // param_d[2] を位相オフセットに流用して爆弾ごとに揺れをずらす
            double bob = sin((double)c * 0.25 + pEnemyShot->param_d[2] * 0.05) * 2.0;
            pEnemyShot->x = pEnemyShot->param_d[2];
            pEnemyShot->y = pEnemyShot->param_d[3] + bob;
            pEnemyShot->kind = img_enemyShotLargeBall[7]; // 黒
        }
        else if (c <= BLINK_END) {
            // 点滅フェーズ：赤↔黒 を 4f 周期で切り替え（カウントダウン警告）
            pEnemyShot->kind = ((c / 4) % 2 == 0)
                ? img_enemyShotLargeBall[0]  // 赤（点灯）
                : img_enemyShotLargeBall[7]; // 黒（消灯）
            pEnemyShot->x = pEnemyShot->param_d[2];
            pEnemyShot->y = pEnemyShot->param_d[3];
        }
        // c == BLAST_F（121）以上は起爆済みで弾が存在しないため到達しない

        pEnemyShot = pEnemyShot->next;
    }
}


// ============================================================================
// EnemyPat_Bomberman_Claude  敵本体パターン
// ============================================================================
void EnemyPat_Bomberman_Claude()
{
    static int muki;       // 移動方向（+1: 右 / -1: 左）
    static int next_cycle; // 次に爆弾 ShotSet を生成する count 値

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 120.0; // 画面上部 120px（爆弾が上方にも配置できる余裕）
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        next_cycle = 1; // count==1 で即座にサイクル開始
    }
    else {
        // ゆっくり左右に往復
        enemy.x += 0.5 * (double)muki;
        if (enemy.x >= 420.0) { enemy.x = 420.0; muki = -1; }
        if (enemy.x <= 60.0) { enemy.x = 60.0; muki = 1; }
    }

    // サイクル開始：爆弾配置 ShotSet を 1 個生成
    if (count == next_cycle) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBombArray;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        next_cycle = count + 180; // 次のサイクルまで 240f（約 4 秒）
    }
}