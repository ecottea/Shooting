#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  A: ボス中心・間隔が狭い/広いが交互に並ぶ10way全方位弾
//     時計回りに回転しながら常時撃ち続ける
// ============================================================
static void ShotLunaticGunA(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射間隔（フレーム）
    const int fireInterval = 6;
    // 1回の発射で出すway数
    const int way = 10;
    // 回転速度（ラジアン/フレーム）時計回りなのでマイナス
    const double rotSpeed = -0.035;

    if (pEnemyShotSet->count % fireInterval == 0) {
        // 効果音（軽め）
        if (pEnemyShotSet->count % (fireInterval * 2) == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 基準角度（回転し続ける）
        double base = pEnemyShotSet->param_d[0];

        // 狭い間隔と広い間隔を交互に作る
        // 等間隔だと 2π/10 ≈ 0.628
        // 狭い：0.35、広い：0.906 を交互に配置して合計がほぼ2πになるように調整
        const double narrow = 0.32;
        const double wide = 0.94;

        double angle = base;
        for (int i = 0; i < way; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.4;
            // 青の銃弾
            pEnemyShot->kind = img_enemyShotBullet[4];

            // リスト連結
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 次の角度（狭い/広いを交互）
            if (i % 2 == 0)
                angle += narrow;
            else
                angle += wide;
        }

        // 基準角度を時計回りに進める
        pEnemyShotSet->param_d[0] += rotSpeed;
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  B: 自機方向へ荒く狙う「1辺8発の逆三角形」密弾列を5本
//     僅かな時間差で発射。1セット連続2回撃った後、少し間を空ける
// ============================================================
static void ShotLunaticGunB(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // セット内の進行管理
    // param_i[0] : 現在のセット内ステップ（0〜）
    // param_i[1] : セット番号（偶数=連続発射中、奇数=待機中）
    // param_d[0] : 発射時の基準角度（自機狙い）

    const int sideBullets = 8;          // 1辺の弾数
    const int triangleNum = 5;          // 逆三角形の本数
    const int delayBetweenTri = 3;      // 三角形同士の時間差（フレーム）
    const double speedBase = 3.8;
    const double speedDiff = 0.15;      // 前後の三角形で速度差を少し付ける

    // 1セットの総フレーム数（5本 × 3f + 余裕）
    const int oneSetFrames = triangleNum * delayBetweenTri + 2;
    // 連続2セット撃った後の待機フレーム
    const int waitFrames = 45;

    int step = pEnemyShotSet->param_i[0];
    int setPhase = pEnemyShotSet->param_i[1];  // 0,1 = 連続発射 / 2 = 待機

    if (setPhase < 2) {
        // 発射フェーズ
        if (step % delayBetweenTri == 0) {
            int triIndex = step / delayBetweenTri;
            if (triIndex < triangleNum) {
                // 効果音
                if (triIndex == 0) {
                    if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                    PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                }

                // 基準角度（最初の三角形で自機狙いを取得し、以降はそれを使う）
                if (triIndex == 0) {
                    pEnemyShotSet->param_d[0] = atan2(
                        player.y - pEnemyShotSet->y,
                        player.x - pEnemyShotSet->x
                    );
                }
                double baseAngle = pEnemyShotSet->param_d[0];

                // 逆三角形の広がり角度（荒く狙うため少し広め）
                const double spread = 0.55;  // 全体の開き

                // 逆三角形：頂点が手前（自機側）、底辺が奥側になるように
                // 各「段」で弾数を増やしていくイメージを、左右対称の列で表現
                // ここでは「左右の斜辺 + 底辺」の3列を密に並べる方式
                // 実際には「密な弾列が逆三角形を成す」ので、
                // 中心から左右に広がる複数の直線状弾列を時間差で出す

                double spd = speedBase + triIndex * speedDiff;

                // 逆三角形を構成する弾を生成
                // 上底（狭い）→下底（広い）になるように段を作る
                // 段数 = sideBullets
                for (int row = 0; row < sideBullets; row++) {
                    // この段の横方向の弾数（上ほど少なく、下ほど多く）
                    // 逆三角形なので row=0 が頂点側（弾数少）、row=sideBullets-1 が底辺
                    int bulletsInRow = 1 + row;  // 1,2,3,...,8

                    // この段の左右の広がり
                    double halfWidth = (spread * 0.5) * ((double)row / (sideBullets - 1));

                    for (int j = 0; j < bulletsInRow; j++) {
                        double t = (bulletsInRow == 1) ? 0.0 :
                            (double)j / (bulletsInRow - 1) - 0.5;  // -0.5 〜 0.5
                        double ang = baseAngle + t * 2.0 * halfWidth;

                        // 少し前後にずらして「密な列」感を出す
                        double offset = (j % 2 == 0) ? 0.0 : 0.08;

                        pEnemyShot = new sEnemyShot;
                        pEnemyShot->x = pEnemyShotSet->x;
                        pEnemyShot->y = pEnemyShotSet->y;
                        pEnemyShot->muki = ang + offset;
                        pEnemyShot->speed = spd;
                        // 赤の銃弾
                        pEnemyShot->kind = img_enemyShotBullet[0];

                        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                    }
                }
            }
        }

        pEnemyShotSet->param_i[0]++;  // step++

        // 1セット終了判定
        if (pEnemyShotSet->param_i[0] >= oneSetFrames) {
            pEnemyShotSet->param_i[0] = 0;
            pEnemyShotSet->param_i[1]++;  // 次のフェーズへ
        }
    }
    else {
        // 待機フェーズ
        pEnemyShotSet->param_i[0]++;
        if (pEnemyShotSet->param_i[0] >= waitFrames) {
            // 待機終了 → また連続2セットへ
            pEnemyShotSet->param_i[0] = 0;
            pEnemyShotSet->param_i[1] = 999;
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_LunaticGun_Grok()
{
    static int muki;
    static int b_shot_timer;   // Bの発射タイミング管理用

    if (count == 1) {
        // 初期位置（画面中央上寄り）
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        b_shot_timer = 0;

        // ----- Aパターンを常時登録 -----
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotLunaticGunA;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0.0;
            pSet->kind = 0;
            pSet->param_d[0] = 0.0;          // 回転基準角度
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    else {
        // ボスの軽い左右移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 100.0 || enemy.x > 380.0) muki *= -1;
        if (count % 180 == 90) muki *= -1;

        // Aの発生源をボス位置に追従させる
        // （AのShotSetは1つだけなので、先頭付近を探して更新）
        sEnemyShotSet* p = enemyShotSetHead.next;
        while (p != &enemyShotSetHead) {
            if (p->patternFunc == ShotLunaticGunA) {
                p->x = enemy.x;
                p->y = enemy.y;
                break;
            }
            p = p->next;
        }
    }

    // ----- Bパターンの発射管理 -----
    // 「1セット連続2回 → 少し間」を繰り返すため、
    // 新しいShotSetを一定周期で生成する
    // （ShotLunaticGunB内部で連続2回＋待機を自己管理しているので、
    //   ここでは「新しいセットを開始するタイミング」だけを制御）
    b_shot_timer++;

    // 最初のセット開始、および前のセットが終わった頃に次を投げる
    // 1セット（連続2回＋待機）の目安フレーム ≈ oneSetFrames*2 + waitFrames ≈ 80前後
    if (b_shot_timer == 1 || (b_shot_timer > 90 && (b_shot_timer - 1) % 90 == 0)) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotLunaticGunB;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        pSet->kind = 0;
        pSet->param_i[0] = 0;   // step
        pSet->param_i[1] = 0;   // phase (0,1=発射 / 2=待機)
        pSet->param_d[0] = 0.0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}