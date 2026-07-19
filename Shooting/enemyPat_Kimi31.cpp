// enemyPat_tmp.cpp
// 戻り弾幕「蜃気楼（ミラージュ）」の実装

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾が反転するまでの距離（発射点からの距離）
static constexpr double MIRROR_TURN_DISTANCE = 500.0;
// 反転後の速度倍率
static constexpr double MIRROR_RETURN_SPEED_MUL = 1.3;
// 反転地点に到達したと判定する距離の閾値
static constexpr double MIRROR_TURN_THRESHOLD = 5.0;

// 弾幕：蜃気楼（ミラージュ）
// 自機に向かって飛び、一定距離で反転して戻ってくる弾
static void ShotMirage(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 発射時の効果音
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 弾を5発生成
        for (int i = 0; i < 15; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 自機狙いの基本角度に少しばらつきを加える
            double baseMuki = pEnemyShotSet->muki;
            double spread = (GetRand(60) - 30) / 180.0 * DX_PI; // ±30度
            pEnemyShot->muki = baseMuki + spread;
            pEnemyShot->speed = (180 + GetRand(120)) / 100.0; // 1.8〜3.0

            // 反転地点を計算（発射点から自機方向へMIRROR_TURN_DISTANCEの距離）
            // 各弾ごとに少し距離を変える
            double turnDist = MIRROR_TURN_DISTANCE + GetRand(40) - 20; // 80〜120
            pEnemyShot->param_d[0] = pEnemyShot->x + turnDist * cos(pEnemyShot->muki);
            pEnemyShot->param_d[1] = pEnemyShot->y + turnDist * sin(pEnemyShot->muki);

            // 反転フラグ（0:未反転、1:反転済み）
            pEnemyShot->param_i[0] = 0;

            // 反転前の色（シアンまたは青）
            pEnemyShot->param_i[1] = (GetRand(1) == 0) ? 3 : 4; // 3:シアン、4:青
            // 反転後の色（赤または黄）
            pEnemyShot->param_i[2] = (GetRand(1) == 0) ? 0 : 1; // 0:赤、1:黄

            pEnemyShot->margin = 480;

            // 弾の種類と色を設定（反転前の色を使用）
            int colorIndex = pEnemyShot->param_i[1];
            switch (pEnemyShotSet->kind % 6) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[colorIndex];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[colorIndex];
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotLargeBall[colorIndex];
                break;
            case 3:
                pEnemyShot->kind = img_enemyShotBullet[colorIndex];
                break;
            case 4:
                pEnemyShot->kind = img_enemyShotScale[colorIndex];
                break;
            case 5:
                pEnemyShot->kind = img_enemyShotDiamond[colorIndex];
                break;
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 未反転の弾は反転地点チェック
        if (pEnemyShot->param_i[0] == 0) {
            double dx = pEnemyShot->x - pEnemyShot->param_d[0];
            double dy = pEnemyShot->y - pEnemyShot->param_d[1];
            double dist = sqrt(dx * dx + dy * dy);

            // 反転地点に到達したら反転
            if (dist < MIRROR_TURN_THRESHOLD) {
                pEnemyShot->param_i[0] = 1; // 反転済みフラグ
                pEnemyShot->muki += DX_PI; // 180度反転
                pEnemyShot->speed *= MIRROR_RETURN_SPEED_MUL; // 加速

                // 反転時の効果音（軽めの音）
                if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                // 色を変更（反転後の色に）
                int newColor = pEnemyShot->param_i[2];
                // kindの色部分を変更（kindの値は画像ハンドルなので、
                // 同じ種類・異なる色の画像に差し替える）
                // kindの下位ビットに色情報が含まれていると仮定し、
                // 色の差分を計算して更新
                int currentColor = pEnemyShot->param_i[1];
                int colorDiff = newColor - currentColor;
                pEnemyShot->kind += colorDiff;
                pEnemyShot->param_i[1] = newColor; // 現在の色を更新
            }
        }

        // 移動
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン：蜃気楼（ミラージュ）
void EnemyPat_Reverse_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.8 * (double)muki;
        // 画面端で方向転換
        if (enemy.x < 60.0) muki = 1;
        if (enemy.x > 420.0) muki = -1;
    }

    // 60フレームごとに弾幕を発射
    if (count % 60 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMirage;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        // 自機を狙う角度を計算
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}