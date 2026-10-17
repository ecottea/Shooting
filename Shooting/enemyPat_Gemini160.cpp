#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕パターン：超空間幾何学：ウラム・スパイラル・カタストロフィ
static void ShotUlamSpiralCatastrophe(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 極限戦闘用の強烈な予告・発射音
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 再現性を担保しつつ、シードに応じた微小な幾何学的ランダムオフセットを付与
        int rand_offset = GetRand(20) - 10;

        // 【レイヤー0】24方向：高密度シアン・ダイヤモンドメッシュ（収縮→反転→超加速の二段構え）
        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = (2.0 * DX_PI / 24.0) * i + (rand_offset * 0.001);
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 4.8;
            pEnemyShot->kind = img_enemyShotDiamond[3]; // シアンの菱形弾
            pEnemyShot->param_i[0] = 0; // レイヤーID
            pEnemyShot->param_d[0] = angle; // 基準角度
            pEnemyShot->param_d[1] = (i % 2 == 0 ? 1.0 : -1.0); // 交互の旋回スピン

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 【レイヤー1】12方向×3深度：ウラム・スパイラル複層螺旋（マゼンタ小玉 ＆ 黄色中玉）
        for (int arm = 0; arm < 12; arm++) {
            for (int depth = 0; depth < 3; depth++) {
                pEnemyShot = new sEnemyShot;
                double angle = (2.0 * DX_PI / 12.0) * arm + depth * 0.12;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = angle;
                pEnemyShot->speed = 2.2 + depth * 0.7;
                pEnemyShot->kind = (depth % 2 == 0) ? img_enemyShotSmallBall[5] : img_enemyShotMediumBall[1]; // マゼンタ小玉 / 黄中玉
                pEnemyShot->param_i[0] = 1; // レイヤーID
                pEnemyShot->param_i[1] = depth;
                pEnemyShot->param_d[0] = angle;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 【レイヤー2】8方向：ホワイト・ショートレーザーによる交差封鎖網
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = (2.0 * DX_PI / 8.0) * i + 0.392;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 4.0;
            pEnemyShot->kind = img_enemyShotLaser[6]; // 白の短レーザー
            pEnemyShot->param_i[0] = 2; // レイヤーID
            pEnemyShot->param_d[0] = angle;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾ごとの動的非線形軌道計算（フレーム毎に空間全体が歪むギミック）
    // ※ count のインクリメントや画面外消去はメインルーチンで行われます
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int c = pShot->count;
        int layer = pShot->param_i[0];

        if (layer == 0) {
            // シアンのダイヤモンド：初め急減速して密集陣形を形成し、特定のフレームで逆向きへ瞬時屈折したのち超加速
            double spinDir = pShot->param_d[1];
            if (c < 40) {
                pShot->speed -= 0.09;
                pShot->muki += 0.035 * spinDir;
            }
            else if (c == 40) {
                pShot->speed = 0.8;
                pShot->muki += DX_PI * 0.85 * spinDir; // ほぼ真後ろへ急反転
            }
            else {
                pShot->speed += 0.14;
                pShot->muki += 0.015 * spinDir * sin(c * 0.12);
            }
        }
        else if (layer == 1) {
            // ウラム・スパイラル螺旋：素数分布の干渉を模した不規則かつ高頻度な位相ゆらぎと呼吸加速
            int depth = pShot->param_i[1];
            double baseAngle = pShot->param_d[0];

            pShot->muki = baseAngle + (c * 0.03 * (depth + 1) * (depth % 2 == 0 ? 1.0 : -1.0)) + sin(c * 0.09) * 0.6;
            pShot->speed = 2.4 + 2.0 * cos(c * 0.06 + depth);
        }
        else if (layer == 2) {
            // 白の短レーザー：完全静止からの超高速スナイピング変異
            if (c == 35) {
                pShot->speed = 0.0; // 一瞬停止して自機位置を完全にロック
            }
            else if (c == 70) {
                pShot->speed = 8.5; // 人間の反応不可能な初速で急襲
            }
            else if (c > 70) {
                pShot->speed *= 0.97;
                pShot->muki += 0.025 * sin(c * 0.25);
            }
            else {
                pShot->speed = 2.2;
            }
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_GeometryTAS_Gemini()
{
    static int muki;
    static int phase_timer;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        phase_timer = 0;
    }
    else {
        // 高速かつ複雑な変則往復機動
        enemy.x += 2.0 * (double)muki + 0.8 * sin(phase_timer * 0.06);
        if (enemy.x < 70.0 || enemy.x > 410.0) {
            muki *= -1;
        }
        phase_timer++;
    }

    // 予告音演出
    if (phase_timer % 110 == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 幾何学弾幕の圧倒的密度での連続投下
    if (phase_timer % 110 == 80) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotUlamSpiralCatastrophe;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}