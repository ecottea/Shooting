#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 四天王奥義「三歩必殺」用弾幕生成・管理関数
// ============================================================
static void Shot_SanpoHissatsu(sEnemyShotSet* pEnemyShotSet)
{
    // 1. 寿命管理: セット自体の寿命が来たら、所属する弾をすべて消去してセット自身も削除する
    if (pEnemyShotSet->count >= pEnemyShotSet->param_i[0]) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* nextShot = pShot->next;
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
            pShot = nextShot;
        }
        delete pEnemyShotSet->pEnemyShotHead; // ダミーノードの解放
        pEnemyShotSet->prev->next = pEnemyShotSet->next;
        pEnemyShotSet->next->prev = pEnemyShotSet->prev;
        delete pEnemyShotSet;
        return;
    }

    // 2. 弾の生成 (count == 0 のときのみ実行)
    if (pEnemyShotSet->count == 0) {
        int phase = pEnemyShotSet->param_i[1];
        int loop_count = pEnemyShotSet->param_i[2];

        // 予告音の再生 (二段階目終了間際と三段階目開始時)
        if ((phase == 2 && pEnemyShotSet->count == 50) || (phase == 3 && pEnemyShotSet->count == 0)) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        // 発射音 (一段階目、二段階目開始時)
        if (phase == 1 || phase == 2) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        int num = 0;
        double radius = 0.0;
        int life = 0;

        if (phase == 1) {
            // 一段階目: ボス周辺に高密度の動かない弾幕 (中玉・緑)
            num = 24;
            radius = 70.0;
            life = 180;
            for (int i = 0; i < num; i++) {
                double angle = i * (DX_PI * 2.0 / num);
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + cos(angle) * radius;
                pEnemyShot->y = pEnemyShotSet->y + sin(angle) * radius;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = 0.0; // 動かない弾幕
                pEnemyShot->kind = img_enemyShotMediumBall[2]; // 2:緑
                pEnemyShot->param_i[0] = life; // 寿命

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        else if (phase == 2) {
            // 二段階目: 一段階目より外側に、まばらな密度で動かない弾幕 (大玉・黄)
            // この隙間を通って内側へ避難する誘導となる
            num = 12;
            radius = 150.0;
            life = 180;
            for (int i = 0; i < num; i++) {
                double angle = i * (DX_PI * 2.0 / num);
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + cos(angle) * radius;
                pEnemyShot->y = pEnemyShotSet->y + sin(angle) * radius;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = 0.0;
                pEnemyShot->kind = img_enemyShotLargeBall[1]; // 1:黄
                pEnemyShot->param_i[0] = life;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        else if (phase == 3) {
            // 三段階目: さらに外側全体を高密度の弾幕で埋める
            radius = 210.0;
            life = 150;

            if (loop_count == 0) {
                // 1回目: 小玉弾 (青) で、隙間を抜ければ回避可能
                num = 48;
                for (int i = 0; i < num; i++) {
                    double angle = i * (DX_PI * 2.0 / num);
                    sEnemyShot* pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = pEnemyShotSet->x + cos(angle) * radius;
                    pEnemyShot->y = pEnemyShotSet->y + sin(angle) * radius;
                    pEnemyShot->muki = angle;
                    pEnemyShot->speed = 0.0;
                    pEnemyShot->kind = img_enemyShotSmallBall[4]; // 4:青
                    pEnemyShot->param_i[0] = life;

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
            else {
                // 2回目以降: 大玉弾(赤)と中玉弾(橙)をずらして配置し、画面を埋め尽くす
                num = 36;
                for (int i = 0; i < num; i++) {
                    double angle1 = i * (DX_PI * 2.0 / num);
                    double angle2 = angle1 + (DX_PI * 2.0 / num) / 2.0; // 隙間を埋めるようにずらす

                    // 大玉弾
                    sEnemyShot* pEnemyShot1 = new sEnemyShot;
                    pEnemyShot1->x = pEnemyShotSet->x + cos(angle1) * radius;
                    pEnemyShot1->y = pEnemyShotSet->y + sin(angle1) * radius;
                    pEnemyShot1->muki = angle1;
                    pEnemyShot1->speed = 0.0;
                    pEnemyShot1->kind = img_enemyShotLargeBall[0]; // 0:赤
                    pEnemyShot1->param_i[0] = life;
                    pEnemyShot1->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot1->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot1;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot1;

                    // 中玉弾
                    sEnemyShot* pEnemyShot2 = new sEnemyShot;
                    pEnemyShot2->x = pEnemyShotSet->x + cos(angle2) * (radius + 10.0);
                    pEnemyShot2->y = pEnemyShotSet->y + sin(angle2) * (radius + 10.0);
                    pEnemyShot2->muki = angle2;
                    pEnemyShot2->speed = 0.0;
                    pEnemyShot2->kind = img_enemyShotMediumBall[8]; // 8:橙
                    pEnemyShot2->param_i[0] = life;
                    pEnemyShot2->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot2->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot2;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot2;
                }
            }
        }
    }

    // 3. 弾の更新と寿命切れの削除
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;

        // 寿命管理
        if (pShot->count >= pShot->param_i[0]) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        else {
            // 移動処理 (speed = 0.0 なので実際には動かないが、汎用性のため記述)
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = nextShot;
    }
}

// ============================================================
// 敵本体のパターン: 四天王奥義「三歩必殺」
// ============================================================
void EnemyPat_SanpoHissatsu_Qwen()
{
    static int phase = 0;         // 0:待機, 1:一段階目, 2:二段階目, 3:三段階目, 4:休憩
    static int loop_count = 0;    // 発動回数
    static int timer = 0;         // 各フェーズの経過時間

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 240.0; // 少し下げて見栄えを良くする
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        phase = 0;
        loop_count = 0;
        timer = 0;
    }
    else {
        // 敵の移動: ゆっくりと左右に揺れる
        enemy.x = 240.0 + sin(count * 0.02) * 120.0;
        enemy.y = 240.0 + sin(count * 0.03) * 20.0;

        timer++;

        // 状態遷移管理
        if (phase == 0) {
            if (timer >= 60-20) {
                phase = 1;
                timer = 0;
                // 一段階目セット作成
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = Shot_SanpoHissatsu;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->param_i[1] = 1;       // phase 1
                pSet->param_i[2] = loop_count;
                pSet->param_i[0] = 200;     // セット自体の寿命
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
        else if (phase == 1) {
            if (timer >= 60-20) {
                phase = 2;
                timer = 0;
                // 二段階目セット作成
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = Shot_SanpoHissatsu;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->param_i[1] = 2;       // phase 2
                pSet->param_i[2] = loop_count;
                pSet->param_i[0] = 200;
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
        else if (phase == 2) {
            if (timer >= 60-20) {
                phase = 3;
                timer = 0;
                // 三段階目セット作成
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = Shot_SanpoHissatsu;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->param_i[1] = 3;       // phase 3
                pSet->param_i[2] = loop_count;
                pSet->param_i[0] = 180;
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
        else if (phase == 3) {
            if (timer >= 90-30) {
                phase = 4;
                timer = 0;
            }
        }
        else if (phase == 4) {
            if (timer >= 60-20) {
                loop_count++;
                if (loop_count >= 3) {
                    // 3回繰り返したらパターン終了（必要に応じてここで敵を消す等の処理を入れる）
                    // 現状はループを継続させる
                    loop_count = 0;
                }
                phase = 0;
                timer = 0;
            }
        }
    }
}