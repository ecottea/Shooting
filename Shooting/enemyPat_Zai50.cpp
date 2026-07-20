// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：紅き群れと一匹の墨（スイミー）
static void ShotSwimmy(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ==========================================
    // フェーズ0：散開（ふわふわと漂う赤い群れ）
    // ==========================================
    if (pEnemyShotSet->param_i[0] == 0) {
        if (pEnemyShotSet->count == 0) {
            // 予告音を鳴らす
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

            // 赤い小弾を60個生成
            for (int i = 0; i < 180; i++) {
                pEnemyShot = new sEnemyShot;
                // 敵の周辺にランダムに配置
                pEnemyShot->x = pEnemyShotSet->x + (GetRand(300) - 150);
                pEnemyShot->y = pEnemyShotSet->y + 20.0 + GetRand(100);
                pEnemyShot->speed = 0.0;
                pEnemyShot->muki = 0.0;
                pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤い小玉

                // ふわふわ移動用のパラメータを設定
                pEnemyShot->param_i[0] = 0; // 赤弾であることの識別フラグ
                pEnemyShot->param_i[1] = i; // 魚の形を形成するためのインデックス(0〜59)
                pEnemyShot->param_d[0] = GetRand(628) / 100.0; // 基礎となる角度
                pEnemyShot->param_d[1] = 0.5 + GetRand(10) / 10.0; // 基礎となる速度(0.5〜1.5)
                pEnemyShot->param_d[2] = GetRand(628) / 100.0; // 動きの位相(ずれ)
                pEnemyShot->margin = 480;

                // 双方向リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 赤弾をサイン波を使ってふわふわと漂わせる
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            pEnemyShot->param_d[2] += 0.02; // 位相を進める
            double move_muki = pEnemyShot->param_d[0] + sin(pEnemyShot->param_d[2]) * 1.5;
            pEnemyShot->x += pEnemyShot->param_d[1] * cos(move_muki);
            pEnemyShot->y += pEnemyShot->param_d[1] * sin(move_muki);
            pEnemyShot = pEnemyShot->next;
        }

        // 120フレーム経過で結集フェーズへ移行
        if (pEnemyShotSet->count == 120) {
            pEnemyShotSet->param_i[0] = 1;

            // スイミー（黒い中玉）を1個だけ生成
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 30.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->muki = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[7]; // 黒い中玉
            pEnemyShot->param_i[0] = 1; // スイミーであることの識別フラグ

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }
    // ==========================================
    // フェーズ1：結集（巨大な魚の形への変形）
    // ==========================================
    else if (pEnemyShotSet->param_i[0] == 1) {

        // リストからスイミー（黒弾）を探す
        sEnemyShot* pSwimmy = pEnemyShotSet->pEnemyShotHead->next;
        while (pSwimmy != pEnemyShotSet->pEnemyShotHead) {
            if (pSwimmy->param_i[0] == 1) break;
            pSwimmy = pSwimmy->next;
        }

        if (pSwimmy) {
            // スイミーはゆっくり下降する
            pSwimmy->y += 0.8;

            // 赤弾をスイミーを基準とした「魚の形」に整列させる
            pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
                if (pEnemyShot->param_i[0] == 0) {
                    int idx = pEnemyShot->param_i[1];
                    double target_x, target_y;

                    // インデックスに応じて魚のパーツ（頭、背中、尾びれ、腹、内側）の座標を計算
                    if (idx < 20) {
                        // 背中側（頭の上から尾びれの上へ）
                        double t = idx / 19.0;
                        if (t < 0.2) {
                            double tt = (t - 0.2) / 0.2;
                            target_x = pSwimmy->x + t * 40.0;
                            target_y = pSwimmy->y - sqrt(1 - tt * tt) * 30.0;
                        }
                        else if (t > 0.7) {
                            double tt = (t - 0.7) / 0.3;
                            target_x = pSwimmy->x + 80.0 + tt * 40.0;
                            target_y = pSwimmy->y - 30.0 - 30.0 * sin(tt * DX_PI);
                        }
                        else {
                            target_x = pSwimmy->x + 8.0 + (t - 0.2) / 0.5 * 72.0;
                            target_y = pSwimmy->y - 30.0;
                        }
                    }
                    else if (idx < 40) {
                        // 腹側（尾びれの下から頭の下へ戻る）
                        double t = (idx - 20) / 19.0;
                        if (t < 0.3) {
                            double tt = t / 0.3;
                            target_x = pSwimmy->x + 120.0 - tt * 40.0;
                            target_y = pSwimmy->y + 30.0 + 30.0 * sin(tt * DX_PI);
                        }
                        else if (t > 0.8) {
                            double tt = (t - 0.8) / 0.2;
                            target_x = pSwimmy->x + 40.0 * (1.0 - tt);
                            target_y = pSwimmy->y + sqrt(1 - tt * tt) * 30.0;
                        }
                        else {
                            target_x = pSwimmy->x + 80.0 - (t - 0.3) / 0.5 * 72.0;
                            target_y = pSwimmy->y + 30.0;
                        }
                    }
                    else {
                        // 体の内側（黄金角螺旋で綺麗に満たす）
                        double a = (idx - 40) * 137.508 * DX_PI / 180.0;
                        double r = sqrt((idx - 40) / 19.0) * 25.0;
                        target_x = pSwimmy->x + 40.0 + r * cos(a);
                        target_y = pSwimmy->y + r * sin(a) * 0.8;
                    }

                    // 現在位置から目標位置へ滑らかに移動（割合で近づける）
                    pEnemyShot->x += (target_x - pEnemyShot->x) * 0.1;
                    pEnemyShot->y += (target_y - pEnemyShot->y) * 0.1;
                }
                pEnemyShot = pEnemyShot->next;
            }

            // 60フレーム経過（countが180）で遊泳・発射フェーズへ移行
            if (pEnemyShotSet->count >= 180) {
                pEnemyShotSet->param_i[0] = 2;
                // 後の尾びれ発射の基準点として、スイミーの最終位置を保存
                pEnemyShotSet->param_d[0] = pSwimmy->x;
                pEnemyShotSet->param_d[1] = pSwimmy->y;
            }
        }
    }
    // ==========================================
    // フェーズ2：遊泳・発射（魚の形を維持したまま襲いかかる）
    // ==========================================
    else if (pEnemyShotSet->param_i[0] == 2) {

        // フェーズ2に切り替わった瞬間の初回処理
        if (pEnemyShotSet->count == 181) {
            pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
                if (pEnemyShot->param_i[0] == 1) {
                    // スイミー（目）はプレイヤーに向かって狙撃発射
                    pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
                    pEnemyShot->speed = 4.5;
                    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                }
                else {
                    int idx = pEnemyShot->param_i[1];
                    // 尾びれ部分（インデックス14〜25）の弾だけ、後方（右方向）へ扇形に発射
                    if (idx >= 14 && idx <= 25) {
                        pEnemyShot->muki = ((idx - 14) / 11.0 - 0.5) * (DX_PI / 2.5);
                        pEnemyShot->speed = 2.5 + GetRand(15) / 10.0;
                    }
                    else {
                        // それ以外の弾（体の形を構成していた弾）は、中心から外向きにゆっくり散らばる
                        double dx = pEnemyShot->x - (pEnemyShotSet->param_d[0] + 35);
                        double dy = pEnemyShot->y - pEnemyShotSet->param_d[1];
                        pEnemyShot->muki = atan2(dy, dx);
                        pEnemyShot->speed = 0.5 + GetRand(10) / 10.0;
                    }
                }
                pEnemyShot = pEnemyShot->next;
            }
        }

        // 全ての弾を設定された速度と角度で移動させる
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            pEnemyShot = pEnemyShot->next;
        }
    }
}


// 敵本体のパターン
void EnemyPat_Swimmy_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 360フレーム（6秒）ごとに弾幕を発動
    if (count % 150 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSwimmy;
        pEnemyShotSet->x = 100 + GetRand(280);
        pEnemyShotSet->y = 100 + GetRand(180);
        pEnemyShotSet->muki = 0.0; // 今回は直接使わない
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->param_i[0] = 0; // フェーズ管理用変数の初期化

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}