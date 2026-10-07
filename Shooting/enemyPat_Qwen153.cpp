#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 相転移弾幕パターン：『熱力学の三態』
// ============================================================
static void ShotPhaseTransition(sEnemyShotSet* pEnemyShotSet)
{
    // pEnemyShotSet->kind でフェーズを管理 (1:固体, 2:液体, 3:気体)
    // pEnemyShotSet->count, pShot->count はメインルーチンで自動+1される仕様

    // --------------------------------------------------------
    // 【弾の生成処理】 (固体フェーズのみで結晶を生成)
    // --------------------------------------------------------
    if (pEnemyShotSet->kind == 1 && pEnemyShotSet->count % 40 == 1) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        int num_shots = 16;
        for (int i = 0; i < num_shots; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            double angle = (DX_PI * 2.0 / num_shots) * i;

            // 敵の周囲に半径 120 の円周上に配置
            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * 120.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * 120.0;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.0; // 固体は基本静止

            // 素材: 小玉(2.5x2.5) の 白(6) または 青(4)
            // 冷たく硬い結晶のイメージ
            pEnemyShot->kind = img_enemyShotSmallBall[6];

            // param_d[0] に初期角度を保存し、振動計算の基準にする
            pEnemyShot->param_d[0] = angle;
            pEnemyShot->param_i[0] = 0; // 状態変化フラグ初期化

            // 双方向リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --------------------------------------------------------
    // 【弾の更新処理】 (フェーズごとの分子運動の表現)
    // --------------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        if (pEnemyShotSet->kind == 1) {
            // 【固体】クリスタル・ケージ (凝固・結晶)
            // 配置位置を基準に、微小な振幅で振動させる（熱振動の表現）
            double vibration = sin(pShot->count * 0.15) * 2.5;
            pShot->x = pEnemyShotSet->x + cos(pShot->param_d[0]) * (120.0 + vibration);
            pShot->y = pEnemyShotSet->y + sin(pShot->param_d[0]) * (120.0 + vibration);
        }
        else if (pEnemyShotSet->kind == 2) {
            // 【液体】メルティング・フロー (融解・流動)
            // フェーズ移行時に一度だけ画像と初期設定を変更
            if (pShot->param_i[0] == 0) {
                // 素材: 中楕円弾(10.5x7.0) の シアン(3)
                // 粘性があり、流れるようなイメージ
                pShot->kind = img_enemyShotMediumOval[3];
                pShot->param_i[0] = 1;
            }

            // 自機方向への緩やかな誘導
            double target_muki = atan2(player.y - pShot->y, player.x - pShot->x);
            double diff = target_muki - pShot->muki;
            // 角度の最短補正
            while (diff > DX_PI) diff -= DX_PI * 2.0;
            while (diff < -DX_PI) diff += DX_PI * 2.0;

            // 向きを徐々に変化させつつ、sin波で粘性のある蛇行運動を加える
            pShot->muki += diff * 0.04 + sin(pShot->count * 0.08) * 0.03;
            pShot->speed = 1.8;

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pEnemyShotSet->kind == 3) {
            // 【気体】ボイル・バースト (蒸発・拡散)
            // フェーズ移行時に一度だけ画像と運動エネルギーを変更
            if (pShot->param_i[0] == 1) {
                // 素材: 菱形弾(4.5x2.5) の 赤(0) または 橙(8)
                // 激しく拡散する高エネルギー状態のイメージ
                pShot->kind = img_enemyShotDiamond[0];
                pShot->param_i[0] = 2;

                // ランダムな方向へ爆発的に拡散 (GetRand(360)は0〜360を返す)
                pShot->muki = GetRand(360) / 360.0 * DX_PI * 2.0;
                pShot->speed = 4.5 + GetRand(30) / 10.0; // 4.5 〜 7.5 の高速
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 簡易的な画面端バウンド処理 (気体分子の跳ね返り表現)
            // メインルーチンでの消去処理と競合しないよう、画面内ギリギリで反転させる
            if (pShot->x <= 2.0 || pShot->x >= 478.0) {
                pShot->muki = DX_PI - pShot->muki;
                pShot->x = (pShot->x <= 2.0) ? 3.0 : 477.0;
            }
            if (pShot->y <= 2.0 || pShot->y >= 478.0) {
                pShot->muki = -pShot->muki;
                pShot->y = (pShot->y <= 2.0) ? 3.0 : 477.0;
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン制御
// ============================================================
void EnemyPat_PhaseTransition_Qwen()
{
    static int phase = 1;
    static sEnemyShotSet* pTransitionSet = nullptr;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0; // やや中央寄りに配置
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        phase = 1;

        // 弾幕セットの初期化 (このパターンでは1つのセットで状態遷移を管理する)
        pTransitionSet = new sEnemyShotSet;
        pTransitionSet->count = 0;
        pTransitionSet->patternFunc = ShotPhaseTransition;
        pTransitionSet->x = enemy.x;
        pTransitionSet->y = enemy.y;
        pTransitionSet->muki = 0.0;
        pTransitionSet->kind = 1; // Phase 1: 固体

        pTransitionSet->pEnemyShotHead = new sEnemyShot;
        pTransitionSet->pEnemyShotHead->prev = pTransitionSet->pEnemyShotHead;
        pTransitionSet->pEnemyShotHead->next = pTransitionSet->pEnemyShotHead;

        pTransitionSet->prev = enemyShotSetHead.prev;
        pTransitionSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pTransitionSet;
        enemyShotSetHead.prev = pTransitionSet;
    }
    else {
        // 敵の動き：ゆっくりと左右に正弦波移動
        enemy.x = 240.0 + sin(count * 0.015) * 100.0;

        // 弾幕セットの基準位置も敵に追従させる
        pTransitionSet->x = enemy.x;
        pTransitionSet->y = enemy.y;

        // ====================================================
        // フェーズ遷移制御 (count はメインルーチンで自動+1される)
        // ====================================================
        int cycle_count = count % 360; // 360フレーム(約6秒)で1サイクル

        if (cycle_count == 1) {
            // サイクルリセット：固体フェーズへ戻り、既存の弾を全削除
            phase = 1;
            pTransitionSet->kind = 1;
            pTransitionSet->count = 0;

            sEnemyShot* pShot = pTransitionSet->pEnemyShotHead->next;
            while (pShot != pTransitionSet->pEnemyShotHead) {
                sEnemyShot* next = pShot->next;
                delete pShot;
                pShot = next;
            }
            pTransitionSet->pEnemyShotHead->prev = pTransitionSet->pEnemyShotHead;
            pTransitionSet->pEnemyShotHead->next = pTransitionSet->pEnemyShotHead;
        }
        else if (cycle_count == 120) {
            // 固体 → 液体 へ相転移
            phase = 2;
            pTransitionSet->kind = 2;
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        else if (cycle_count == 240) {
            // 液体 → 気体 へ相転移
            phase = 3;
            pTransitionSet->kind = 3;
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
    }
}