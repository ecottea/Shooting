#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：リコイル・バースト（マシンガン）
//
// マシンガンの「連射 → 反動 → 排莢 → リロード」を既存の敵弾で表現する。
//
// 【使用する既存素材】
//   小弾 (img_enemyShotSmallBall) : 通常のマシンガン弾（黄）
//   中弾 (img_enemyShotMediumBall): 曳光弾（白）、薬莢（橙）
//   大弾 (img_enemyShotLargeBall) : マズルフラッシュ（黄・一瞬だけ）
//
// 【弾幕の内容】
//   1バースト = 8連射（3フレーム間隔 ≈ 0.05秒）
//   4発ごとに曳光弾（中弾・白）
//   2発ごとに薬莢（中弾・橙）を左右交互に低速で排出
//   各発射で反動により発射角が1.2°ずつ上へずれる
//   バースト後 0.5秒停止、3バーストごとに 1.2秒リロード
// ============================================================

// 弾幕：リコイル・バースト
static void ShotRecoilBurst(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // param_i[0] : フェーズ (0=待機, 1=発射中, 2=バースト後停止, 3=リロード)
    // param_i[1] : バースト内の発射番号 (0〜7)
    // param_i[2] : バーストカウンタ（リロード判定用）
    // param_i[3] : 現在フェーズ内のタイマー
    // param_d[0] : バースト開始時の基準角度（プレイヤー方向）
    // param_d[1] : 反動による角度オフセット（度）
    int& phase = pEnemyShotSet->param_i[0];
    int& shotIdx = pEnemyShotSet->param_i[1];
    int& burstCnt = pEnemyShotSet->param_i[2];
    int& timer = pEnemyShotSet->param_i[3];
    double& baseMuki = pEnemyShotSet->param_d[0];
    double& recoil = pEnemyShotSet->param_d[1];

    const int FIRE_INTERVAL = 3-1;  // 発射間隔（フレーム）
    const int BURST_SHOTS = 8*2;  // 1バーストの発射数
    const int PAUSE_FRAMES = 30; // バースト後の停止（約0.5秒）
    const int RELOAD_FRAMES = 72; // リロード（約1.2秒）
    const int BURSTS_PER_RELOAD = 3;  // 何バーストごとにリロードするか

    // 銃口位置を敵の現在位置に追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;

    // 初回初期化
    if (pEnemyShotSet->count == 0) {
        phase = 0;
        shotIdx = 0;
        burstCnt = 0;
        timer = 0;
        baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        recoil = 0.0;
    }

    timer++;

    switch (phase) {
    case 0: // 待機 → バースト開始
    {
        baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        recoil = 0.0;
        shotIdx = 0;
        timer = 0;
        phase = 1;

        // マズルフラッシュ（大弾を一瞬だけ銃口に表示）
        {
            double mx = pEnemyShotSet->x + cos(baseMuki) * 12.0;
            double my = pEnemyShotSet->y + sin(baseMuki) * 12.0;
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = mx;
            pEnemyShot->y = my;
            pEnemyShot->muki = baseMuki;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLargeBall[1]; // 黄
            pEnemyShot->param_i[0] = 2; // マズルフラッシュ
            pEnemyShot->param_i[1] = 2; // 寿命（フレーム）
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 発射音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }
    break;

    case 1: // 発射中
    {
        if (timer % FIRE_INTERVAL == 1 && shotIdx < BURST_SHOTS) {
            double mx = pEnemyShotSet->x + cos(baseMuki) * 12.0;
            double my = pEnemyShotSet->y + sin(baseMuki) * 12.0;

            // 拡散 ±2° と反動を加えた発射角度
            double angle = baseMuki + (recoil + (GetRand(4) - 2)) * DX_PI / 180.0;

            int isTracer = (shotIdx % 4 == 3);

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = mx;
            pEnemyShot->y = my;
            pEnemyShot->muki = angle;
            pEnemyShot->param_i[0] = 0; // 通常弾

            if (isTracer) {
                // 曳光弾（中弾・白）
                pEnemyShot->speed = 6.5;
                pEnemyShot->kind = img_enemyShotMediumBall[6];
            }
            else {
                // 通常弾（小弾・黄）
                pEnemyShot->speed = 7.0;
                pEnemyShot->kind = img_enemyShotSmallBall[1];
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 反動：1発ごとに角度を上へ（DxLibでは角度減少＝画面上方向）
            recoil -= 1.2;

            shotIdx++;

            // 薬莢（2発ごとに左右交互、中弾・橙）
            if (shotIdx % 2 == 0) {
                int side = ((shotIdx / 2) % 2 == 0) ? 1 : -1;
                double casingAngle = baseMuki + side * DX_PI / 2.0;
                casingAngle += (GetRand(40) - 20) * DX_PI / 180.0;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = mx;
                pEnemyShot->y = my;
                pEnemyShot->muki = casingAngle;
                pEnemyShot->speed = 2.5;
                pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙
                pEnemyShot->param_i[0] = 1;  // 薬莢
                pEnemyShot->param_i[1] = 48; // 寿命（約0.8秒）
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            if (shotIdx >= BURST_SHOTS) {
                burstCnt++;
                timer = 0;
                if (burstCnt >= BURSTS_PER_RELOAD) {
                    phase = 3; // リロード
                    burstCnt = 0;
                    // リロード音（予告音を代用）
                    if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                    PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
                }
                else {
                    phase = 2; // バースト後停止
                }
            }
        }
    }
    break;

    case 2: // バースト後の短い停止
        if (timer >= PAUSE_FRAMES) {
            timer = 0;
            phase = 0;
        }
        break;

    case 3: // リロード
        if (timer >= RELOAD_FRAMES) {
            timer = 0;
            phase = 0;
        }
        break;
    }

    // 弾の更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next;

        // マズルフラッシュと薬莢の寿命処理
        // param_i[0]: 0=通常弾, 1=薬莢, 2=マズルフラッシュ
        if (pShot->param_i[0] == 1 || pShot->param_i[0] == 2) {
            pShot->param_i[1]--;
            if (pShot->param_i[1] <= 0) {
                // 画面外へ飛ばしてメインルーチンに消去させる
                pShot->x = -1000.0;
                pShot->y = -1000.0;
                pShot->param_i[0] = 0;
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = nextShot;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_MachineGun_DeepSeek()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;

        // リコイル・バースト用のセットを1つ生成
        // （このセットが連続して発射・停止・リロードを制御する）
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRecoilBurst;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->alive = 99999;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 140 == 70) muki *= -1;
    }
}