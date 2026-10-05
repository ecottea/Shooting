#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ガトリング・排莢バースト
static void ShotGatlingBurst(sEnemyShotSet* pEnemyShotSet)
{
    // 敵本体の座標に追従
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // バーストの周期（120フレーム = 2秒周期）
    int t = pEnemyShotSet->count % 120;

    // 射撃フェーズ（最初の30フレーム間で、2フレームごとに発射）
    if (t < 30 && t % 2 == 0) {
        // SE再生（高頻度で鳴らすため軽い音を使用）
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // プレイヤーへの基本角度
        double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // --- ① 曳光弾（6フレームごと = 3発に1発） または ② 実弾 ---
        if (t % 6 == 0) {
            // 曳光弾（赤色・中玉）: ブレがなく、少し遅めでプレイヤーへの視認性を高める
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = base_muki;
            pShot->speed = 8.0;
            pShot->kind = img_enemyShotMediumBall[0]; // 0:赤
            pShot->param_i[0] = 0; // 通常弾フラグ

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        else {
            // 実弾（黄色・銃弾）: 角度と速度にランダムなブレを与え、面で制圧する
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;

            // GetRand(160) は 0～160 の整数。-80 で -80～80。
            // ±約8度のブレ（8度 ≒ 0.14ラジアン）
            double blur = (GetRand(160) - 80) / 1800.0 * DX_PI;
            pShot->muki = base_muki + blur;

            // 速度ブレ: 9.0 ～ 11.0
            pShot->speed = 9.0 + GetRand(20) / 10.0;
            pShot->kind = img_enemyShotBullet[1]; // 1:黄
            pShot->param_i[0] = 0; // 通常弾フラグ

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }

        // --- ③ 薬莢弾（毎回発射） ---
        {
            sEnemyShot* pShot = new sEnemyShot;
            // 敵の右斜め下（排莢口のイメージ）から出るように初期位置をずらす
            pShot->x = pEnemyShotSet->x + 15.0;
            pShot->y = pEnemyShotSet->y + 5.0;

            // 右斜め上方向に打ち出す（-45度近辺）
            double eject_angle = -DX_PI / 4.0 + (GetRand(240) - 120) / 1000.0 * DX_PI; // 角度にブレを加味
            double eject_speed = 3.0 + GetRand(20) / 10.0; // 初速 3.0～5.0

            // X方向とY方向の初速ベクトルをパラメータに格納
            pShot->param_d[0] = eject_speed * cos(eject_angle); // vx
            pShot->param_d[1] = eject_speed * sin(eject_angle); // vy

            pShot->kind = img_enemyShotSmallBall[8]; // 8:橙色・小玉
            pShot->param_i[0] = 1; // 薬莢フラグ

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 弾の移動処理（リスト内の全弾）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 薬莢フラグが立っている場合：重力の影響を受けて放物線を描いて落ちる
            pShot->param_d[1] += 0.2; // 下方向への加速度（重力）を加算
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
        }
        else {
            // 実弾・曳光弾：指定角度へ直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン：ガトリングバースト
void EnemyPat_MachineGun_Gemini()
{
    // 移動の基準となる座標
    static double baseX = 240.0;
    static double baseY = 100.0;

    // 初期化処理
    if (count == 1) {
        baseX = 240.0;
        baseY = 100.0;
        enemy.x = baseX;
        enemy.y = baseY;
        enemy.maxHp = enemy.hp = 200; // HPは変更しない

        // 弾幕セットの初期化（出現時に1つだけ作成し、永続的に管理させる）
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGatlingBurst;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->alive = 9999;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストの末尾に接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // バースト周期の管理（120フレーム = 2秒周期。弾の周期と完全に同期する）
    int t = count % 120;

    if (t < 30) {
        // 射撃フェーズ（0〜29フレーム）：反動でガタガタと震える演出
        // ベース位置は固定し、描画座標にランダムなブレを直接加える
        enemy.x = baseX + (GetRand(40) - 20) / 10.0; // ±2.0ピクセルのブレ
        enemy.y = baseY + (GetRand(40) - 20) / 10.0;
    }
    else {
        // リロード・移動フェーズ（30〜119フレーム）：左右に移動して次の射撃位置へ
        // 滑らかに目標座標（baseX, baseY）を更新
        baseX = 240.0 + sin(count / 40.0) * 120.0;
        baseY = 100.0; // 高さは固定
        enemy.x = baseX;
        enemy.y = baseY;
    }
}