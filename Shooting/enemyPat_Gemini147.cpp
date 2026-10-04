#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 角度を滑らかに補間するためのヘルパー関数
static double LerpAngle(double a, double b, double t) {
    double d = b - a;
    while (d > DX_PI)  d -= 2 * DX_PI;
    while (d < -DX_PI) d += 2 * DX_PI;
    return a + d * t;
}

// 弾幕：三節連環・蛇行撃（左右反転対応版）
static void ShotSansetsukon(sEnemyShotSet* pEnemyShotSet)
{
    int c = pEnemyShotSet->count;
    int dir = pEnemyShotSet->param_i[15]; // 1:通常(右構えから左へスイング), -1:反転(左構えから右へスイング)

    // 初期化（Phase 1開始）
    if (c == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        double aim = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        // 最初は大きく折りたたむように構える (dirで反転)
        pEnemyShotSet->param_d[0] = aim + DX_PI * 0.8 * dir;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->param_d[0] - DX_PI * 0.6 * dir;
        pEnemyShotSet->param_d[2] = pEnemyShotSet->param_d[1] + DX_PI * 0.6 * dir;
        // 初期の節の長さ
        pEnemyShotSet->param_d[3] = 40.0;
        pEnemyShotSet->param_d[4] = 40.0;
        pEnemyShotSet->param_d[5] = 40.0;

        // 弾生成用ヘルパーラムダ
        auto add_shot = [&](int type, int idx, int kind) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->kind = kind;
            pShot->param_i[0] = type;
            pShot->param_i[1] = idx;
            pShot->margin = 240;
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        };

        // 各部位の構成
        add_shot(0, 0, img_enemyShotLargeBall[1]);  // 先端 (黄・大玉)
        add_shot(1, 0, img_enemyShotMediumBall[0]); // 関節1 (赤・中玉)
        add_shot(2, 0, img_enemyShotMediumBall[0]); // 関節2 (赤・中玉)

        for (int i = 0; i < 8; i++) add_shot(3, i, img_enemyShotDiamond[4]); // 第1節 (青・菱形)
        for (int i = 0; i < 8; i++) add_shot(4, i, img_enemyShotDiamond[4]); // 第2節 (青・菱形)
        for (int i = 0; i < 8; i++) add_shot(5, i, img_enemyShotDiamond[4]); // 第3節 (青・菱形)
    }

    // Phase 1: 溜めと自機への追従 (0 ~ 59フレーム)
    if (c < 60) {
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        double aim = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->param_d[0] = LerpAngle(pEnemyShotSet->param_d[0], aim + DX_PI * 0.8 * dir, 0.05);
        pEnemyShotSet->param_d[1] = LerpAngle(pEnemyShotSet->param_d[1], pEnemyShotSet->param_d[0] - DX_PI * 0.6 * dir, 0.05);
        pEnemyShotSet->param_d[2] = LerpAngle(pEnemyShotSet->param_d[2], pEnemyShotSet->param_d[1] + DX_PI * 0.6 * dir, 0.05);
    }
    // Phase 2: スイングなぎ払い (60 ~ 119フレーム)
    else if (c < 120) {
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        if (c == 60) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        double t = (c - 60) / 60.0;
        // 第1関節を逆サイドへ大きく振り下ろす (dirで回転方向を反転)
        pEnemyShotSet->param_d[0] -= (DX_PI * 1.5 * dir) / 60.0;
        // 第2、第3関節は慣性のように遅れて追従する
        pEnemyShotSet->param_d[1] = LerpAngle(pEnemyShotSet->param_d[1], pEnemyShotSet->param_d[0] - DX_PI * 0.5 * dir * (1.0 - t), 0.2);
        pEnemyShotSet->param_d[2] = LerpAngle(pEnemyShotSet->param_d[2], pEnemyShotSet->param_d[1] - DX_PI * 0.5 * dir * (1.0 - t), 0.2);

        // 遠心力で節が徐々に伸びる
        pEnemyShotSet->param_d[3] += (80.0 - 40.0) / 60.0;
        pEnemyShotSet->param_d[4] += (80.0 - 40.0) / 60.0;
        pEnemyShotSet->param_d[5] += (80.0 - 40.0) / 60.0;
    }
    // Phase 3: 直線延伸と打撃 (120 ~ 149フレーム)
    else if (c < 150) {
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        if (c == 120) {
            // 突き刺しのために自機位置をロックオン
            pEnemyShotSet->param_d[6] = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }

        double target = pEnemyShotSet->param_d[6];
        // 3つの関節の角度が急速に揃い、一直線になる
        pEnemyShotSet->param_d[0] = LerpAngle(pEnemyShotSet->param_d[0], target, 0.2);
        pEnemyShotSet->param_d[1] = LerpAngle(pEnemyShotSet->param_d[1], target, 0.25);
        pEnemyShotSet->param_d[2] = LerpAngle(pEnemyShotSet->param_d[2], target, 0.3);

        // リーチが一気に伸びる
        double t = (c - 120) / 30.0;
        double ease = t * (2.0 - t); // easeOut
        pEnemyShotSet->param_d[3] = 80.0 + 50.0 * ease; // max 130
        pEnemyShotSet->param_d[4] = 80.0 + 50.0 * ease; // max 130
        pEnemyShotSet->param_d[5] = 80.0 + 60.0 * ease; // max 140
    }
    // Phase 4: 衝撃波拡散・解体 (150フレーム)
    else if (c == 150) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            int type = pShot->param_i[0];
            
            // 現在の座標を基準に、外側へ弾け飛ぶベクトルを付与
            if (type != 99) {
                pShot->muki = atan2(pShot->y - pEnemyShotSet->y, pShot->x - pEnemyShotSet->x) + (GetRand(20) - 10) / 100.0 * 4;
                pShot->speed = 2.0 + GetRand(200) / 100.0;
            }

            if (type == 0 || type == 1 || type == 2) {
                // 関節と先端の部分からさらに破裂弾（小玉）を追加でばら撒く
                for (int i = 0; i < 8*2; i++) {
                    sEnemyShot* pNew = new sEnemyShot;
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    pNew->muki = (DX_PI * 2.0 / 8/2) * i + (GetRand(100) / 100.0) * 0.5;
                    pNew->speed = 3.0 + GetRand(100) / 100.0;
                    pNew->kind = (type == 0) ? img_enemyShotSmallBall[1] : img_enemyShotSmallBall[0];
                    pNew->param_i[0] = 99; // 追加弾は最初から自律移動扱い

                    pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNew->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                    pEnemyShotSet->pEnemyShotHead->prev = pNew;
                }
            }
            pShot = pShot->next;
        }
    }

    // --- 座標・挙動の適用ループ ---
    if (c < 150) {
        // 現在の角度と長さから各関節の座標を算出
        double th1 = pEnemyShotSet->param_d[0];
        double th2 = pEnemyShotSet->param_d[1];
        double th3 = pEnemyShotSet->param_d[2];
        double L1 = pEnemyShotSet->param_d[3];
        double L2 = pEnemyShotSet->param_d[4];
        double L3 = pEnemyShotSet->param_d[5];

        double base_x = pEnemyShotSet->x;
        double base_y = pEnemyShotSet->y;
        double p1_x = base_x + L1 * cos(th1);
        double p1_y = base_y + L1 * sin(th1);
        double p2_x = p1_x + L2 * cos(th2);
        double p2_y = p1_y + L2 * sin(th2);
        double p3_x = p2_x + L3 * cos(th3);
        double p3_y = p2_y + L3 * sin(th3);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            int type = pShot->param_i[0];
            int idx = pShot->param_i[1];

            if (type == 0) {
                pShot->x = p3_x; pShot->y = p3_y;
                pShot->muki = th3;
            }
            else if (type == 1) {
                pShot->x = p1_x; pShot->y = p1_y;
                pShot->muki = th1;
            }
            else if (type == 2) {
                pShot->x = p2_x; pShot->y = p2_y;
                pShot->muki = th2;
            }
            else if (type == 3) {
                double r = (idx + 1.0) / 9.0;
                pShot->x = base_x + (p1_x - base_x) * r;
                pShot->y = base_y + (p1_y - base_y) * r;
                pShot->muki = th1;
            }
            else if (type == 4) {
                double r = (idx + 1.0) / 9.0;
                pShot->x = p1_x + (p2_x - p1_x) * r;
                pShot->y = p1_y + (p2_y - p1_y) * r;
                pShot->muki = th2;
            }
            else if (type == 5) {
                double r = (idx + 1.0) / 9.0;
                pShot->x = p2_x + (p3_x - p2_x) * r;
                pShot->y = p2_y + (p3_y - p2_y) * r;
                pShot->muki = th3;
            }
            pShot = pShot->next;
        }
    }
    else {
        // Phase 4 (150フレーム)以降は固定配置を解除し、各弾の速度と向きによる自律移動を行う
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_ThreeSectionStaff_Gemini()
{
    static int muki = 1;
    static int attack_dir = 1; // 1と-1を交互に切り替えるフラグ

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0; // 長い三節棍を振り回すため、初期位置を少し下げておく
        enemy.maxHp = enemy.hp = 200; // 変更不可
        muki = 1;
        attack_dir = 1; // 初期化
    }
    else {
        enemy.x += 0.5 * (double)muki;
        if (enemy.x > 340.0 || enemy.x < 140.0) muki *= -1;
    }

    // 240フレームごとに三節棍攻撃を発動
    if (count % 120 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSansetsukon;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        // 回転方向のフラグを弾幕に渡す
        pEnemyShotSet->param_i[15] = attack_dir;
        attack_dir *= -1; // 次回用に反転させる

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
