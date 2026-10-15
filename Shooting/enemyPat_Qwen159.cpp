#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 刀専用弾幕セットのパターン関数
// ============================================================
static void Shot_Katana(sEnemyShotSet* pEnemyShotSet)
{
    int phase = pEnemyShotSet->param_i[0];
    int timer = pEnemyShotSet->param_i[1];

    // 刀身となるレーザー弾は、セット内に1つだけ存在し、それを更新する
    sEnemyShot* pKatana = pEnemyShotSet->pEnemyShotHead->next;
    if (pKatana != pEnemyShotSet->pEnemyShotHead) {
        // 刀身の位置をボスに追従させる
        pKatana->x = enemy.x;
        pKatana->y = enemy.y;

        if (phase == 1) { // 【フェーズ1: 構え】
            pKatana->muki = -DX_PI / 2.0; // 上向き
            pKatana->kind = img_enemyShotLaser[6]; // 白レーザー
        }
        else if (phase == 2) { // 【フェーズ2: 斬撃】
            // 0フレームで -90度(上), 30フレームで +90度(下) に回転させる
            double progress = (timer < 30) ? (timer / 30.0) : 1.0;
            pKatana->muki = (-DX_PI / 2.0) + DX_PI * progress;
            pKatana->kind = img_enemyShotLaser[3]; // 斬撃中はシアンレーザーで光らせる

            // 斬撃軌跡に「刃圧(剣気)」を生成 (3フレームごと、30フレーム間)
            if (timer % 2 == 0 && timer <= 30) {
                sEnemyShot* pSlash = new sEnemyShot;
                // 刀の「先端」から出るように座標をオフセット (レーザー長64の半分=32)
                pSlash->x = pKatana->x + cos(pKatana->muki) * 32.0;
                pSlash->y = pKatana->y + sin(pKatana->muki) * 32.0;
                pSlash->muki = pKatana->muki;
                pSlash->speed = 4.0 + progress * 4.0; // 4.0 ～ 8.0 で加速
                pSlash->kind = img_enemyShotDiamond[6]; // 白菱形弾（鋭い刃圧）

                // リストに追加
                pSlash->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSlash->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSlash;
                pEnemyShotSet->pEnemyShotHead->prev = pSlash;
            }
        }
        else if (phase == 3) { // 【フェーズ3: 残心(血振り)】
            pKatana->muki = DX_PI / 2.0; // 下向き
            pKatana->kind = img_enemyShotLaser[6]; // 白レーザーに戻す

            // 刀についたものを払うように火花を散らす
            if (timer % 2 == 0 && timer < 20) {
                for (int i = 0; i < 5+2; i++) {
                    sEnemyShot* pSpark = new sEnemyShot;
                    pSpark->x = pKatana->x;
                    pSpark->y = pKatana->y;
                    // 下向きを基準に扇状にばらまく
                    pSpark->muki = pKatana->muki + (i - 2-1) * 0.25*2;
                    pSpark->speed = 2.0 + GetRand(20) / 10.0; // 2.0 ～ 4.0
                    pSpark->kind = img_enemyShotSmallBall[8]; // 橙小玉（火花）

                    pSpark->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pSpark->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pSpark;
                    pEnemyShotSet->pEnemyShotHead->prev = pSpark;
                }
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 刀身レーザーは speed=0 なので移動しない。それ以外は速度に応じて移動する。
        if (pShot->speed > 0.0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // タイマーを進める (メインのcountとは別に、フェーズごとの経過時間を管理)
    pEnemyShotSet->param_i[1]++;
}

// ============================================================
// 敵本体のパターン：虚空を断つ居合斬り
// ============================================================
void EnemyPat_Sword_Qwen()
{
    static int cycle;
    static sEnemyShotSet* pKatanaSet = nullptr;

    // 240フレーム(約4秒)で1ループ
    cycle = count % 240;

    // --- 初期化 ---
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        pKatanaSet = nullptr;
    }

    // --- ボスの移動制御 ---
    if (cycle >= 1 && cycle < 60) {
        // 待機：上下に少し揺れる
        enemy.y = 80.0 + sin(cycle * 0.1) * 10.0;
    }
    else if (cycle >= 60 && cycle < 120) {
        // 構え：画面左側へ移動
        double t = (cycle - 60) / 60.0;
        enemy.x = 240.0 + (80.0 - 240.0) * t;
        enemy.y = 80.0 + (100.0 - 80.0) * t;
    }
    else if (cycle >= 120 && cycle < 160) {
        // 斬撃：画面中央へ高速突進 (40フレームで到達)
        double t = (cycle - 120) / 40.0;
        enemy.x = 80.0 + (240.0 - 80.0) * t;
        enemy.y = 100.0 + (240.0 - 100.0) * t;
    }
    else if (cycle >= 160 && cycle < 200) {
        // 残心：画面右側へ移動
        double t = (cycle - 160) / 40.0;
        enemy.x = 240.0 + (400.0 - 240.0) * t;
        enemy.y = 240.0 + (100.0 - 240.0) * t;
    }
    else {
        // リセット準備位置
        enemy.x = 400.0;
        enemy.y = 100.0;
    }

    // --- 弾幕・演出のトリガー ---
    if (cycle == 60) {
        // 【構え開始】予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        pKatanaSet = new sEnemyShotSet;
        pKatanaSet->count = 0;
        pKatanaSet->patternFunc = Shot_Katana;
        pKatanaSet->x = enemy.x;
        pKatanaSet->y = enemy.y;
        pKatanaSet->muki = 0;
        pKatanaSet->kind = 0;
        pKatanaSet->param_i[0] = 1; // phase 1: 構え
        pKatanaSet->param_i[1] = 0; // timer

        // ヘッダーノードの初期化
        pKatanaSet->pEnemyShotHead = new sEnemyShot;
        pKatanaSet->pEnemyShotHead->prev = pKatanaSet->pEnemyShotHead;
        pKatanaSet->pEnemyShotHead->next = pKatanaSet->pEnemyShotHead;

        // 刀身レーザーを1つ生成して登録
        sEnemyShot* pKatana = new sEnemyShot;
        pKatana->x = enemy.x;
        pKatana->y = enemy.y;
        pKatana->muki = -DX_PI / 2.0;
        pKatana->speed = 0; // 速度0にすることで、以下の移動処理ループで位置が上書きされず、ボス追従を維持する
        pKatana->kind = img_enemyShotLaser[6]; // 白レーザー
        pKatana->prev = pKatanaSet->pEnemyShotHead->prev;
        pKatana->next = pKatanaSet->pEnemyShotHead;
        pKatanaSet->pEnemyShotHead->prev->next = pKatana;
        pKatanaSet->pEnemyShotHead->prev = pKatana;

        // グローバルリストに登録
        pKatanaSet->prev = enemyShotSetHead.prev;
        pKatanaSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pKatanaSet;
        enemyShotSetHead.prev = pKatanaSet;
    }
    else if (cycle == 120) {
        // 【斬撃開始】極大効果音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        if (pKatanaSet) {
            pKatanaSet->param_i[0] = 2; // phase 2: 斬撃
            pKatanaSet->param_i[1] = 0; // timer リセット
        }
    }
    else if (cycle == 160) {
        // 【残心開始】
        if (pKatanaSet) {
            pKatanaSet->param_i[0] = 3; // phase 3: 残心
            pKatanaSet->param_i[1] = 0; // timer リセット
        }
    }
    else if (cycle == 200) {
        // 片付け：次のループに備えて参照を切る
        // (実際のメモリ解放はメインルーチンの弾消去処理に委ねる仕様)
        pKatanaSet = nullptr;
    }
}