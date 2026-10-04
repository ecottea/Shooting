#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 四天王奥義「三歩必殺」再現
// 原作: 東方地霊殿 3面 星熊勇儀 Spell 39-42
// 仕様:
// - 中央で3歩踏む(小揺れ、小揺れ、大揺れ) → 不可視リング3重に弾出現
// - 奇数ウェーブと偶数ウェーブで色とカーブ方向が反転
// - 出現後しばらく停止 → 同時拡散
// - 外周白粒/バブルはプレイヤー位置に重なると出現抑制(原作Easy-Normal仕様)
// - count, pEnemyShotSet->count, pEnemyShot->count のインクリメントと
//   画面外削除はメイン側で行うため、ここでは行わない
// ============================================================

// リングスポーン用ヘルパー
static void SpawnRingBullets(sEnemyShotSet* pSet, double baseRadius, int dirNum, int lineLen,
    double spacing, int imgKind, double curve, double moveSpeed, int waitFrames,
    int ringId, bool checkPlayerOverlap)
{
    for (int d = 0; d < dirNum; d++) {
        double baseAngle = (DX_TWO_PI * d / dirNum);
        // Hard風のランダム塊を偶数ウェーブで混ぜる場合に備えて少し散らすフラグ
        bool isRandomClump = (pSet->kind % 2 == 1); // 偶数ウェーブはHard準拠ランダム塊

        for (int j = 0; j < lineLen; j++) {
            double r = baseRadius + j * spacing;
            double ang = baseAngle;
            double x, y;
            if (isRandomClump && ringId != 2) {
                // Hard仕様: 線ではなくランダム塊
                ang += (GetRand(60) - 30) / 180.0 * DX_PI * 0.3; // ±30度の30%程度のブレ
                r += GetRand(10) - 5;
            }
            x = pSet->x + cos(ang) * r;
            y = pSet->y + sin(ang) * r;

            if (checkPlayerOverlap) {
                double dx = x - player.x;
                double dy = y - player.y;
                if (dx * dx + dy * dy < 24.0 * 24.0) {
                    continue; // プレイヤーに被る白粒/バブルは出現させない(原作仕様)
                }
            }

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = x;
            pShot->y = y;
            pShot->muki = ang; // 外側へ向かう向き
            pShot->speed = 0.0; // 最初は停止
            pShot->kind = imgKind;
            pShot->count = 0;
            // param_d[0]: カーブ量(毎フレームmukiに加算)
            // param_d[1]: 移動開始後の速度
            // param_d[2]: 初期角度保持(拡散時に使う場合)
            pShot->param_d[0] = curve;
            pShot->param_d[1] = moveSpeed;
            pShot->param_d[2] = ang;
            // param_i[0]: リングID 0=内 1=中 2=外
            // param_i[1]: 移動開始までの待機フレーム(弾個別)
            // param_i[2]: スピン方向など
            pShot->param_i[0] = ringId;
            pShot->param_i[1] = waitFrames + j * 2; // 線の先端ほど少し遅れて動く
            pShot->param_i[2] = 0;
            pShot->margin = 480;
            pShot->param_d[10] = x;
            pShot->param_d[11] = y;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }
}

static void Shot_SanpoHissatsu(sEnemyShotSet* pSet)
{
    // pSet->kind 偶奇で挙動を分ける
    // 0,2,4... = 奇数ウェーブ原作(青/シアン/白)
    // 1,3,5... = 偶数ウェーブ原作(赤/紫/バブル)
    bool isOddWave = (pSet->kind % 2 == 0);
    int setCount = pSet->count;

    // --- 3歩の演出 ---
    if (setCount == 0) {
        // 予告音: 足を上げる
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        pSet->param_d[0] = pSet->x; // 中心記憶
        pSet->param_d[1] = pSet->y;
    }

    if (setCount == 15) {
        // 一歩目 小揺れ + 内側リング生成
        enemy.x += (GetRand(6) - 3); // 小揺れ
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        if (isOddWave) {
            // 青の短い線 右カーブ
            SpawnRingBullets(pSet, 60.0, 16, 4, 10.0,
                img_enemyShotSmallBall[4], // 青 小玉(2.5x2.5) 高密度向け
                0.015, // 右カーブ
                2.2, 65,
                0, false);
        }
        else {
            // 赤の短い線 左カーブ (偶数)
            SpawnRingBullets(pSet, 60.0, 16, 4, 10.0,
                img_enemyShotSmallBall[0], // 赤 小玉
                -0.015, // 左カーブ
                2.2, 65,
                0, false);
        }
    }

    if (setCount == 35) {
        // 二歩目 小揺れ + 中間リング
        enemy.x += (GetRand(6) - 3);
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        if (isOddWave) {
            // シアンの長い線 左カーブ
            SpawnRingBullets(pSet, 120.0, 16, 6, 12.0,
                img_enemyShotMediumBall[3], // シアン 中玉(7.0x7.0) 視認性重視
                -0.012,
                1.8, 85,
                1, false);
        }
        else {
            // 紫の線 右カーブ
            SpawnRingBullets(pSet, 120.0, 16, 6, 12.0,
                img_enemyShotMediumBall[5], // マゼンタ 中玉
                0.012,
                1.8, 85,
                1, false);
        }
    }

    if (setCount == 55) {
        // 三歩目 大揺れ + 外側リング
        enemy.x = pSet->param_d[0] + (GetRand(16) - 8);
        enemy.y = pSet->param_d[1] + (GetRand(8) - 4);
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        if (isOddWave) {
            // 白い小粒 48個 円形 左右に分かれて回転
            for (int i = 0; i < 48; i++) {
                double ang = DX_TWO_PI * i / 48.0;
                double r = 180.0 + (GetRand(10) - 5);
                double x = pSet->x + cos(ang) * r;
                double y = pSet->y + sin(ang) * r;
                // プレイヤー重なり抑制
                double dx = x - player.x;
                double dy = y - player.y;
                if (dx * dx + dy * dy < 28.0 * 28.0) continue;

                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = x; pShot->y = y;
                pShot->muki = ang;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotSmallBall[6]; // 白 小玉
                pShot->count = 0;
                // 偶数インデックスは右回転、奇数は左回転に分岐(原作の2グループ分離)
                pShot->param_d[0] = (i % 2 == 0) ? 0.018 : -0.018;
                pShot->param_d[1] = 1.6 + GetRand(40) / 100.0; // 1.6-2.0
                pShot->param_i[0] = 2;
                pShot->param_i[1] = 45; // 外周は早めに動き出す
                pShot->margin = 480;
                pShot->param_d[10] = x;
                pShot->param_d[11] = y;
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
        else {
            // 偶数ウェーブ: 紫バブル(大玉) Lunaticでは重なり無視だがここではEasy準拠で抑制あり
            // Hard再現なら抑制を外すので、kindが3以上なら抑制なしにする
            bool checkOverlap = (pSet->kind < 3);
            for (int i = 0; i < 32; i++) {
                double ang = DX_TWO_PI * i / 32.0 + GetRand(20) / 100.0;
                double r = 180.0;
                double x = pSet->x + cos(ang) * r;
                double y = pSet->y + sin(ang) * r;
                if (checkOverlap) {
                    double dx = x - player.x;
                    double dy = y - player.y;
                    if (dx * dx + dy * dy < 36.0 * 36.0) continue;
                }
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = x; pShot->y = y;
                pShot->muki = ang;
                pShot->speed = 0.0;
                // 原作のバブル弾 → 大玉で再現(20.0x20.0) 存在感と当たり判定が大きいのが特徴
                pShot->kind = img_enemyShotLargeBall[5]; // マゼンタ 大玉
                pShot->count = 0;
                pShot->param_d[0] = 0.008; // 全体右回転
                pShot->param_d[1] = 1.2;
                pShot->param_i[0] = 2;
                pShot->param_i[1] = 50;
                pShot->margin = 480;
                pShot->param_d[10] = x;
                pShot->param_d[11] = y;
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    if (setCount == 75) {
        // 揺れを戻す
        enemy.x = pSet->param_d[0];
        enemy.y = pSet->param_d[1];
    }

    // --- 弾の移動更新 ---
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // 待機時間までは停止(リング上で静止)
        if (pShot->count < pShot->param_i[1]) {
            // 停止中は速度0のまま
        }
        else {
            if (pShot->speed == 0.0) {
                pShot->speed = pShot->param_d[1]; // 移動開始
            }
            // カーブ適用
            pShot->muki += pShot->param_d[0];

            // 内側リング(0)は広がる挙動: 少しずつ速度上昇
            if (pShot->param_i[0] == 0 && isOddWave) {
                // 青は拡散: 外側へ広がるようにカーブを徐々に弱める演出
                // 簡易的に速度を微増
                pShot->speed += 0.005*2;
            }
            // 中間リングは隊列維持のためカーブのみ
            // 外周はそのまま回転しながら離脱

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        if (pShot->count >= 120 && hypot(pShot->x - pShot->param_d[10], pShot->y - pShot->param_d[11]) < 40) pShot->margin = -9999;

        pShot = pShot->next;
    }
}

// 敵本体
void EnemyPat_SanpoHissatsu_MetaAI()
{
    static int waveCount;
    static double baseX, baseY;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0; // 画面中央やや上 原作の勇儀位置
        enemy.maxHp = enemy.hp = 200;
        waveCount = 0;
        baseX = enemy.x;
        baseY = enemy.y;
    }
    else {
        // 基本は中央でホバリング、揺れはShot側で上書きされるのでここでは緩やかな揺らぎのみ
        // 3歩の間(ウェーブ開始後75F)はホバリングを止める
        int inWave = count % 180;
        if (inWave > 80 || inWave < 5) {
            enemy.x = baseX + sin(count * 0.02) * 8.0;
            enemy.y = baseY + cos(count * 0.015) * 4.0;
        }
    }

    // 180Fごとに新しい三歩必殺ウェーブを開始
    if (count % 320 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = Shot_SanpoHissatsu;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = 0.0;
        pSet->kind = waveCount++; // 0,1,2...で奇数/偶数を切り替え
        pSet->param_d[0] = 0.0;
        pSet->param_d[1] = 0.0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}