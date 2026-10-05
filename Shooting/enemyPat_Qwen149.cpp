#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：オーバーヒート・リコイル（マシンガン）
static void ShotMachineGun(sEnemyShotSet* pEnemyShotSet)
{
    // param_i[0]: state (0:待機, 1:連射中, 2:オーバーヒート)
    // param_i[1]: shot_count (連射中の発射数カウント)
    // param_i[2]: recoil (現在のリコイル蓄積値: 0〜15)
    
    // 1. 既存の弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 薬莢の挙動：横向きの初速 + 重力落下
            pShot->param_d[0] += 0.15; // 擬似的な重力加速度
            pShot->y += pShot->param_d[0];
            pShot->x += pShot->speed * cos(pShot->muki);
        } else {
            // 通常の弾（メイン弾、オーバーヒート時のばら撒き）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // 2. 状態遷移と弾生成
    if (pEnemyShotSet->param_i[0] == 1) {
        // 連射中：3フレームに1発発射（約20FPS）
        if (pEnemyShotSet->count % 3 == 0) {
            // リコイルの蓄積（最大15度程度まで増加）
            if (pEnemyShotSet->param_i[2] < 15) {
                pEnemyShotSet->param_i[2] += 1;
            }
            
            // --- メイン弾（銃弾）の生成 ---
            sEnemyShot* pMainShot = new sEnemyShot;
            double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            
            // GetRand(30) は 0〜30 を返す。-15 すれば -15〜+15度のランダムブレ
            int recoil_rand = GetRand(30) - 15;
            // 蓄積リコイルを加味（偶数フレームは右、奇数フレームは左に偏らせて「震え」を表現）
            int bias = (pEnemyShotSet->count % 6 == 0) ? pEnemyShotSet->param_i[2] : -pEnemyShotSet->param_i[2];
            
            pMainShot->muki = base_muki + (recoil_rand + bias) * DX_PI / 180.0;
            pMainShot->x = pEnemyShotSet->x;
            pMainShot->y = pEnemyShotSet->y + 10.0;
            pMainShot->speed = 7.0 + GetRand(100) / 100.0; // 7.0 〜 8.0 の高速
            pMainShot->kind = img_enemyShotBullet[8];       // 銃弾・橙（火薬の閃光イメージ）
            pMainShot->param_i[0] = 0; // 通常弾フラグ
            
            // リストに追加
            pMainShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pMainShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pMainShot;
            pEnemyShotSet->pEnemyShotHead->prev = pMainShot;

            // --- 薬莢の生成 ---
            sEnemyShot* pCaseShot = new sEnemyShot;
            pCaseShot->muki = pMainShot->muki - DX_PI / 2.0; // メイン弾に対して横向き（右側）
            pCaseShot->x = pEnemyShotSet->x;
            pCaseShot->y = pEnemyShotSet->y;
            pCaseShot->speed = 2.0 + GetRand(100) / 100.0;   // 横向きの初速
            pCaseShot->param_d[0] = -1.5;                    // 初期は少し上に跳ね上がる
            pCaseShot->param_i[0] = 1;                       // 薬莢フラグ（重力処理用）
            pCaseShot->kind = img_enemyShotSmallBall[1];     // 小玉・黄（真鍮色イメージ）
            
            pCaseShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pCaseShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pCaseShot;
            pEnemyShotSet->pEnemyShotHead->prev = pCaseShot;

            pEnemyShotSet->param_i[1]++; // 連射カウントアップ
            
            // --- オーバーヒート判定 ---
            // 90発（約3秒）撃ったらオーバーヒート状態へ
            if (pEnemyShotSet->param_i[1] >= 90) {
                pEnemyShotSet->param_i[0] = 2; // オーバーヒート状態へ
                pEnemyShotSet->param_i[1] = 0; // リロードカウント用にリセット
                
                // 演出：空薬莢の山が崩れるように、黒い小玉を周囲にばら撒く
                for (int i = 0; i < 12; i++) {
                    sEnemyShot* pOverheatShot = new sEnemyShot;
                    pOverheatShot->muki = GetRand(360) * DX_PI / 180.0;
                    pOverheatShot->x = pEnemyShotSet->x;
                    pOverheatShot->y = pEnemyShotSet->y;
                    pOverheatShot->speed = 1.0 + GetRand(200) / 100.0;
                    pOverheatShot->kind = img_enemyShotSmallBall[7]; // 小玉・黒（熱を持った薬莢や煙イメージ）
                    pOverheatShot->param_i[0] = 0;
                    
                    pOverheatShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pOverheatShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pOverheatShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pOverheatShot;
                }
                
                // オーバーヒート時の重厚な効果音
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            }
        }
    }
    else if (pEnemyShotSet->param_i[0] == 2) {
        // オーバーヒート中 (リロード待機)
        pEnemyShotSet->param_i[1]++;
        
        // 180フレーム（約3秒）待機したら再開
        if (pEnemyShotSet->param_i[1] >= 180) {
            pEnemyShotSet->param_i[0] = 1; // 連射再開
            pEnemyShotSet->param_i[1] = 0;
            pEnemyShotSet->param_i[2] = 0; // リコイル蓄積をリセット
            
            // 再開時の予告音
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
    }
}

// 敵本体のパターン：マシンガン（オーバーヒート・リコイル）
void EnemyPat_MachineGun_Qwen()
{
    static int muki;
    static bool is_initialized = false;
    static sEnemyShotSet* pMachineGunSet = nullptr;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        is_initialized = false;
        pMachineGunSet = nullptr;
    }
    else {
        // 敵の移動：ゆっくり左右に往復（掃射の動きを補助）
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;

        // ShotSetの初期化（出現後、count=60まで待ってから開始）
        if (!is_initialized && count == 60) {
            pMachineGunSet = new sEnemyShotSet;
            pMachineGunSet->count = 0;
            pMachineGunSet->patternFunc = ShotMachineGun;
            pMachineGunSet->x = enemy.x;
            pMachineGunSet->y = enemy.y;
            pMachineGunSet->muki = 0;
            pMachineGunSet->kind = 0;
            
            // 状態初期化
            pMachineGunSet->param_i[0] = 1; // state: 連射中から開始
            pMachineGunSet->param_i[1] = 0; // shot_count: 0
            pMachineGunSet->param_i[2] = 0; // recoil: 0
            
            // 弾リストのヘッド初期化
            pMachineGunSet->pEnemyShotHead = new sEnemyShot;
            pMachineGunSet->pEnemyShotHead->prev = pMachineGunSet->pEnemyShotHead;
            pMachineGunSet->pEnemyShotHead->next = pMachineGunSet->pEnemyShotHead;
            
            // グローバルリストに接続
            pMachineGunSet->prev = enemyShotSetHead.prev;
            pMachineGunSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pMachineGunSet;
            enemyShotSetHead.prev = pMachineGunSet;
            
            is_initialized = true;
            
            // 射撃開始時の効果音
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 既存のShotSetの位置を敵に追従させる（発射基準点を敵の現在位置にする）
        if (pMachineGunSet != nullptr) {
            pMachineGunSet->x = enemy.x;
            pMachineGunSet->y = enemy.y;
        }
    }
}