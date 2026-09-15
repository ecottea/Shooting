// enemyPat_QRCodeScan.cpp

#include "gv.h"
#include <cmath>

#ifndef DX_PI
#define DX_PI 3.14159265358979323846
#endif

// 弾幕：ファインダー・スキャン（QRコードモチーフ）
static void ShotQRCode(sEnemyShotSet* pEnemyShotSet)
{
    int wait_time = 60;   // 展開後の静止時間（1秒）
    int charge_time = 30; // スキャンラインに触れてから弾が動き出すまでの溜め時間（0.5秒）
    double scan_speed = 2.0;

    // 展開時の初期化処理
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int cell_size = 20;
        int grid_size = 17;
        // 画面中央に配置するための開始座標計算
        double start_x = 240.0 - (cell_size * grid_size) / 2.0 + cell_size / 2.0;
        double start_y = 50.0;

        // 1. QRコード弾の配置
        for (int cy = 0; cy < grid_size; cy++) {
            for (int cx = 0; cx < grid_size; cx++) {
                int is_finder = 0;
                double finder_cx = 0, finder_cy = 0;

                // 左上、右上、左下の位置検出パターン（7x7マス）の判定
                if (cx < 7 && cy < 7) {
                    is_finder = 1;
                    finder_cx = start_x + 3 * cell_size;
                    finder_cy = start_y + 3 * cell_size;
                }
                else if (cx >= 10 && cy < 7) {
                    is_finder = 1;
                    finder_cx = start_x + 13 * cell_size;
                    finder_cy = start_y + 3 * cell_size;
                }
                else if (cx < 7 && cy >= 10) {
                    is_finder = 1;
                    finder_cx = start_x + 3 * cell_size;
                    finder_cy = start_y + 13 * cell_size;
                }

                bool place_shot = false;
                if (is_finder) {
                    // ファインダー内部のローカル座標(0〜6)
                    int lx = (cx < 7) ? cx : cx - 10;
                    int ly = (cy < 7) ? cy : cy - 10;

                    // 外枠の四角
                    if (lx == 0 || lx == 6 || ly == 0 || ly == 6) {
                        place_shot = true;
                    }
                    // 中心の四角
                    else if (lx >= 2 && lx <= 4 && ly >= 2 && ly <= 4) {
                        place_shot = true;
                    }
                }
                else {
                    // データ領域はランダムなモザイク状に配置 (約40%の確率)
                    if (GetRand(100) < 40) {
                        place_shot = true;
                    }
                }

                // 弾を置く条件を満たしたら生成
                if (place_shot) {
                    sEnemyShot* pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = start_x + cx * cell_size;
                    pEnemyShot->y = start_y + cy * cell_size;
                    pEnemyShot->speed = 0.0;
                    pEnemyShot->muki = 0.0;
                    pEnemyShot->kind = img_enemyShotMediumBall[3]; // 初期色はサイバーなシアン

                    pEnemyShot->param_i[0] = 0; // 状態：0(未スキャン), 1(スキャン済・待機), 2(起動)
                    pEnemyShot->param_i[1] = is_finder ? 1 : 0; // 属性：0(データ), 1(ファインダー)

                    // ファインダーパターンの場合、中心からの放射角度をあらかじめ計算しておく
                    if (is_finder) {
                        pEnemyShot->param_d[0] = atan2(pEnemyShot->y - finder_cy, pEnemyShot->x - finder_cx);
                    }

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }

        // 2. スキャンラインの配置（赤い中玉を横一列に隙間なく敷き詰める）
        for (int i = 0; i <= 48; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = i * 10.0; // 画面幅480をカバー
            pEnemyShot->y = 10.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->muki = DX_PI / 2.0; // 下向き
            pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤

            pEnemyShot->param_i[0] = 0;
            pEnemyShot->param_i[1] = 2; // 属性：2(スキャンライン)

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    bool played_shot_sound = false;

    // スキャン中の読み取り効果音（断続的に鳴らす）
    if (pEnemyShotSet->count > wait_time && pEnemyShotSet->count % 8 == 0) {
        if (10.0 + (pEnemyShotSet->count - wait_time) * scan_speed < 480.0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // 各弾の更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[1] == 2) {
            // ■ スキャンラインの処理
            if (pEnemyShotSet->count > wait_time) {
                pShot->speed = scan_speed; // 時間が来たら降り始める
            }
            if (pShot->y >= 450) {
                pShot->margin = -999;
            }
        }
        else {
            // ■ QRコード弾の処理
            if (pShot->param_i[0] == 0) {
                // スキャンラインのY座標を仮想的に計算して判定
                double scan_y = 10.0;
                if (pEnemyShotSet->count > wait_time) {
                    scan_y += (pEnemyShotSet->count - wait_time) * scan_speed;
                }

                // スキャンラインが通過した瞬間
                if (scan_y >= pShot->y) {
                    pShot->param_i[0] = 1; // スキャン完了・待機状態へ
                    pShot->param_i[2] = pEnemyShotSet->count; // スキャンされた時刻を保存
                    pShot->kind = img_enemyShotMediumBall[0]; // 警告の赤色へ変化
                }
            }
            else if (pShot->param_i[0] == 1) {
                // スキャンされてから charge_time 経過後に起動
                if (pEnemyShotSet->count - pShot->param_i[2] >= charge_time) {
                    pShot->param_i[0] = 2; // 起動・移動状態へ

                    if (pShot->param_i[1] == 0) {
                        // データ領域：自機へ向けてランダムな速度で動き出す
                        pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                        pShot->speed = 1.0 + GetRand(150) / 100.0;
                    }
                    else if (pShot->param_i[1] == 1) {
                        // ファインダー：生成時に計算した角度へ向けて放射状に弾け飛ぶ
                        pShot->muki = pShot->param_d[0];
                        pShot->speed = 2.0 + GetRand(100) / 100.0;
                    }

                    // 起動音はフレームごとに1回だけ鳴らす（音が割れるのを防ぐ）
                    if (!played_shot_sound) {
                        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                        played_shot_sound = true;
                    }
                }
            }
        }

        // 座標移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_QRCode_Gemini()
{
    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 420フレーム周期（約7秒ごと）でQRコード弾幕を展開
    if (count % 420 == 60) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotQRCode;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}