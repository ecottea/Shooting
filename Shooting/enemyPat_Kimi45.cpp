// enemyPat_dnaHelix.cpp
// DNAダブルヘリックス弾幕（改修版）
// - 螺旋が開いていく（半径拡大）
// - 塩基対が分離・散開
// - 遺伝子発現（放射噴出）
// - 突然変異（HARD：軌道分岐）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕パターン：DNAダブルヘリックス（改修版）
// ============================================================

static void ShotDNAHelix(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double PI = DX_PI;
    const double DEG2RAD = PI / 180.0;

    // --- 初回生成 ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        int phase = pEnemyShotSet->kind % 3;

        // param_d[0] = 螺旋中心X
        // param_d[1] = 螺旋中心Y（初期）
        // param_d[2] = 螺旋半径（初期）
        // param_d[3] = 回転速度（度/フレーム）
        // param_d[4] = 下降速度
        // param_d[5] = 半径拡大速度（開いていく速度）
        // param_d[6] = 塩基対分離までの時間（フレーム）
        // param_d[7] = 遺伝子発現間隔（フレーム）
        // param_i[0] = 塩基対の出現間隔（フレーム）
        // param_i[1] = 現在の塩基対インデックス
        // param_i[2] = 螺旋方向（1=右巻き, -1=左巻き）
        // param_i[3] = 遺伝子発現カウンタ
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y;
        pEnemyShotSet->param_d[4] = 1.2; // 下降速度

        if (phase == 0) { // EASY
            pEnemyShotSet->param_d[2] = 40.0;    // 初期半径
            pEnemyShotSet->param_d[3] = 3.0;      // 回転速度
            pEnemyShotSet->param_d[5] = 0.15;    // 半径拡大速度
            pEnemyShotSet->param_d[6] = 90.0;    // 1.5秒後に分離
            pEnemyShotSet->param_d[7] = 120.0;   // 2秒ごとに発現
            pEnemyShotSet->param_i[0] = 12;       // 塩基対間隔
            pEnemyShotSet->param_i[2] = 1;
        }
        else if (phase == 1) { // NORMAL
            pEnemyShotSet->param_d[2] = 50.0;
            pEnemyShotSet->param_d[3] = 4.5;
            pEnemyShotSet->param_d[5] = 0.25;
            pEnemyShotSet->param_d[6] = 60.0;    // 1秒後に分離
            pEnemyShotSet->param_d[7] = 90.0;    // 1.5秒ごとに発現
            pEnemyShotSet->param_i[0] = 10;
            pEnemyShotSet->param_i[2] = 1;
        }
        else { // HARD
            pEnemyShotSet->param_d[2] = 60.0;
            pEnemyShotSet->param_d[3] = 6.0;
            pEnemyShotSet->param_d[5] = 0.4;
            pEnemyShotSet->param_d[6] = 45.0;    // 0.75秒後に分離
            pEnemyShotSet->param_d[7] = 60.0;    // 1秒ごとに発現
            pEnemyShotSet->param_i[0] = 8;
            pEnemyShotSet->param_i[2] = (GetRand(1) == 0) ? 1 : -1;
        }

        pEnemyShotSet->param_i[1] = 0;
        pEnemyShotSet->param_i[3] = 0;

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // --- パラメータ取得 ---
    double centerX = pEnemyShotSet->param_d[0];
    double centerY = pEnemyShotSet->param_d[1];
    double baseRadius = pEnemyShotSet->param_d[2];
    double rotSpeed = pEnemyShotSet->param_d[3];
    double descentSpeed = pEnemyShotSet->param_d[4];
    double expandSpeed = pEnemyShotSet->param_d[5];
    double splitTime = pEnemyShotSet->param_d[6];
    double expressInterval = pEnemyShotSet->param_d[7];
    int basePairInterval = pEnemyShotSet->param_i[0];
    int helixDir = pEnemyShotSet->param_i[2];
    int phase = pEnemyShotSet->kind % 3;

    // 現在の半径（時間と共に広がる）
    double currentRadius = baseRadius + pEnemyShotSet->count * expandSpeed;
    if (currentRadius > 180.0) currentRadius = 180.0; // 最大半径制限

    // 螺旋中心の移動（ゆっくり下降）
    centerY += descentSpeed;
    pEnemyShotSet->param_d[1] = centerY;

    // 現在の回転角度
    double currentAngle = pEnemyShotSet->count * rotSpeed * helixDir;

    // --- 新しい主鎖弾を生成 ---
    for (int strand = 0; strand < 2; strand++) if (pEnemyShotSet->count % 2 == 0) {
        double angleOffset = strand * 180.0;
        double angle = (currentAngle + angleOffset) * DEG2RAD;

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = centerX + currentRadius * cos(angle);
        pEnemyShot->y = centerY + currentRadius * sin(angle) * 0.35;

        // 主鎖弾は螺旋に沿った接線方向に初速を持たせる
        double tangentAngle = angle + (helixDir > 0 ? PI / 2.0 : -PI / 2.0);
        pEnemyShot->muki = tangentAngle;
        // 初速は小さく、螺旋の形を保つ
        pEnemyShot->speed = 0.3 + phase * 0.2;

        pEnemyShot->count = 0;
        int colorIdx = 6; // 白
        pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];
        pEnemyShot->param_i[0] = 0; // 0=主鎖
        pEnemyShot->param_i[1] = strand;
        // 生成時の螺旋情報を記録（後の分離用）
        pEnemyShot->param_d[0] = pEnemyShot->x; // 生成時X
        pEnemyShot->param_d[1] = pEnemyShot->y; // 生成時Y
        pEnemyShot->param_d[2] = angle;         // 生成時角度
        pEnemyShot->param_d[3] = currentRadius; // 生成時半径

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // --- 塩基対の生成 ---
    if (pEnemyShotSet->count % basePairInterval == 0) {
        int basePairIdx = pEnemyShotSet->param_i[1]++;

        // DNA塩基対の色パターン
        int baseColors[4][2] = {
            {0, 4}, // A(赤)-T(青)
            {4, 0}, // T(青)-A(赤)
            {2, 1}, // G(緑)-C(黄)
            {1, 2}  // C(黄)-G(緑)
        };
        int colorPair = basePairIdx % 4;

        for (int strand = 0; strand < 2; strand++) {
            double angleOffset = strand * 180.0;
            double angle = (currentAngle + angleOffset) * DEG2RAD;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = centerX + currentRadius * cos(angle);
            pEnemyShot->y = centerY + currentRadius * sin(angle) * 0.35;

            // 塩基対は螺旋に沿った方向に少し速め
            double tangentAngle = angle + (helixDir > 0 ? PI / 2.0 : -PI / 2.0);
            pEnemyShot->muki = tangentAngle;
            pEnemyShot->speed = 0.5 + phase * 0.3;

            pEnemyShot->count = 0;
            int color = baseColors[colorPair][strand];
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            pEnemyShot->param_i[0] = 1; // 1=塩基対
            pEnemyShot->param_i[1] = strand;
            pEnemyShot->param_i[2] = basePairIdx; // ペアID
            pEnemyShot->param_i[3] = pEnemyShotSet->count; // 生成時の親count

            // 分離後の方向を予め決定（ランダム）
            double splitAngle = (GetRand(360)) * DEG2RAD;
            pEnemyShot->param_d[0] = splitAngle; // 分離方向
            pEnemyShot->param_d[1] = 1.5 + phase * 0.5; // 分離後の速度
            pEnemyShot->param_d[2] = angle;
            pEnemyShot->param_d[3] = currentRadius;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 連結レーザー（塩基対同士を結ぶ）
        double angle0 = currentAngle * DEG2RAD;
        double angle1 = (currentAngle + 180.0) * DEG2RAD;
        double x0 = centerX + currentRadius * cos(angle0);
        double y0 = centerY + currentRadius * sin(angle0) * 0.35;
        double x1 = centerX + currentRadius * cos(angle1);
        double y1 = centerY + currentRadius * sin(angle1) * 0.35;

        double midX = (x0 + x1) / 2.0;
        double midY = (y0 + y1) / 2.0;
        double connectAngle = atan2(y1 - y0, x1 - x0);
        double connectLen = sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = midX;
        pEnemyShot->y = midY;
        pEnemyShot->muki = connectAngle;
        pEnemyShot->speed = 0.0;
        pEnemyShot->count = 0;
        pEnemyShot->kind = img_enemyShotLaser[6]; // 白
        pEnemyShot->param_d[0] = connectLen;
        pEnemyShot->param_i[0] = 2; // 2=連結レーザー
        pEnemyShot->param_i[2] = basePairIdx;
        pEnemyShot->param_i[3] = pEnemyShotSet->count;

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // --- 遺伝子発現（放射噴出） ---
    if (pEnemyShotSet->count > 0 &&
        (int)(pEnemyShotSet->count % (int)expressInterval) == 0) {

        int expressCount = 8 + phase * 4; // EASY:8, NORMAL:12, HARD:16
        double expressSpeed = 2.0 + phase * 0.8;

        for (int i = 0; i < expressCount; i++) {
            pEnemyShot = new sEnemyShot;
            // 螺旋中心から放射状
            double expressAngle = (360.0 / expressCount) * i * DEG2RAD;
            pEnemyShot->x = centerX;
            pEnemyShot->y = centerY;
            pEnemyShot->muki = expressAngle;
            pEnemyShot->speed = expressSpeed + GetRand(20) / 10.0;
            pEnemyShot->count = 0;

            // 色はランダム（DNA塩基対の色から）
            int expressColors[4] = { 0, 4, 2, 1 }; // 赤, 青, 緑, 黄
            int color = expressColors[i % 4];
            pEnemyShot->kind = img_enemyShotBullet[color]; // 銃弾で高速感

            pEnemyShot->param_i[0] = 3; // 3=遺伝子発現弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // --- 弾の移動処理 ---
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        int shotType = pEnemyShot->param_i[0];
        int shotAge = pEnemyShot->count;

        if (shotType == 0) {
            // 主鎖弾：螺旋の接線方向に進みつつ、半径が広がっていく
            // 生成時の角度に基づいて、半径を広げる
            double genAngle = pEnemyShot->param_d[2];
            double genRadius = pEnemyShot->param_d[3];
            // 経過時間に応じて半径を広げる
            double nowRadius = genRadius + shotAge * expandSpeed * 0.5;
            // 中心は下降しているので、相対的に追従
            // 実際には接線方向に進みながら、螺旋の開いていく動きを模倣
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            // さらに中心から外へ離れる成分（開いていく）
            double toCenterX = centerX - pEnemyShot->x;
            double toCenterY = centerY - pEnemyShot->y;
            double dist = sqrt(toCenterX * toCenterX + toCenterY * toCenterY);
            if (dist > 1.0) {
                pEnemyShot->x -= (toCenterX / dist) * expandSpeed * 0.3;
                pEnemyShot->y -= (toCenterY / dist) * expandSpeed * 0.3;
            }
        }
        else if (shotType == 1) {
            // 塩基対弾：一定時間後に分離・散開
            int spawnCount = pEnemyShot->param_i[3];
            int ageSinceSpawn = pEnemyShotSet->count - spawnCount;

            if (ageSinceSpawn < splitTime) {
                // 分離前：螺旋に沿って移動
                pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
                pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            }
            else {
                // 分離後：予め決めた方向へ直線的に散開
                // 初回のみ方向を変更
                if (shotAge == 0 || pEnemyShot->param_i[4] == 0) {
                    pEnemyShot->muki = pEnemyShot->param_d[0];
                    pEnemyShot->speed = pEnemyShot->param_d[1];
                    pEnemyShot->param_i[4] = 1; // 分離済みフラグ
                }
                pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
                pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
                // 加速
                pEnemyShot->speed += 0.02;
            }
        }
        else if (shotType == 2) {
            // 連結レーザー：塩基対と同じく分離後は消滅（または追従停止）
            int spawnCount = pEnemyShot->param_i[3];
            int ageSinceSpawn = pEnemyShotSet->count - spawnCount;
            if (ageSinceSpawn < splitTime) {
                // 塩基対の現在位置を再計算して追従（簡易）
                // 実際にはレーザーは位置更新しない（見た目のみ）
                // 中心の下降に追従
                pEnemyShot->y += descentSpeed;
            }
            else {
                // 分離後：レーザーも飛んでいく（危険な演出）
                pEnemyShot->x += 0.5 * cos(pEnemyShot->muki);
                pEnemyShot->y += 0.5 * sin(pEnemyShot->muki);
            }
        }
        else if (shotType == 3) {
            // 遺伝子発現弾：直線的に放射
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            // 少し加速
            pEnemyShot->speed += 0.015;
        }

        // --- HARDフェーズ：突然変異（軌道分岐） ---
        if (phase == 2 && shotType != 3) {
            // 一定時間ごとに方向をランダムに変える
            if (shotAge > 0 && shotAge % 30 == 0 && GetRand(2) == 0) {
                double mutationAngle = (GetRand(120) - 60) * DEG2RAD;
                pEnemyShot->muki += mutationAngle;
                // 速度も変化
                pEnemyShot->speed *= 0.8 + GetRand(40) / 100.0;
            }
        }

        pEnemyShot = pEnemyShot->next;
    }

    // --- フェーズ進行による加速 ---
    if (phase == 2) {
        if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 60 == 0) {
            pEnemyShotSet->param_d[3] += 0.3; // 回転速度上昇
            pEnemyShotSet->param_d[5] += 0.05; // 拡大速度上昇
            if (pEnemyShotSet->param_d[3] > 10.0) pEnemyShotSet->param_d[3] = 10.0;
        }
    }
}


// ============================================================
//  敵本体のパターン：DNAヘリックス
// ============================================================
void EnemyPat_DNA_Kimi()
{
    static int muki;
    static int shot_count;
    static int phase;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
        phase = 0;
    }
    else {
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 120.0 || enemy.x > 360.0) muki *= -1;
    }

    if (enemy.hp > 130) phase = 0;
    else if (enemy.hp > 60) phase = 1;
    else phase = 2;

    int spawnInterval;
    if (phase == 0) spawnInterval = 500;      // 5秒
    else if (phase == 1) spawnInterval = 500;   // 4秒
    else spawnInterval = 500;                   // 3秒

    if (count % spawnInterval == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDNAHelix;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = phase;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        shot_count++;
    }

    if (enemy.x < 40.0) enemy.x = 40.0;
    if (enemy.x > 440.0) enemy.x = 440.0;
    if (enemy.y < 20.0) enemy.y = 20.0;
    if (enemy.y > 100.0) enemy.y = 100.0;
}