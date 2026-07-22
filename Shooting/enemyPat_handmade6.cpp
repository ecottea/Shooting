// enemyPat_Nabeatsu.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <vector>

static int current_num;

// 数字が3の倍数、または3のつく数字かどうかを判定する関数
static bool IsNabeatsu(int num) {
    if (num % 3 == 0) return true;
    while (num > 0) {
        if (num % 10 == 3) return true;
        num /= 10;
    }
    return false;
}

static void ShotScatter(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 1) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < (current_num - 1) * 5; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
            pEnemyShot->speed = (100 + GetRand(300)) / 100.0;
            int col = GetRand(8);

            switch (GetRand(6)) {
            case 0:
                pEnemyShot->kind = img_enemyShotSmallBall[col];
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[col];
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotLargeBall[col];
                break;
            case 3:
                pEnemyShot->kind = img_enemyShotBullet[col];
                break;
            case 4:
                pEnemyShot->kind = img_enemyShotScale[col];
                break;
            case 5:
                pEnemyShot->kind = img_enemyShotDiamond[col];
                break;
            case 6:
                pEnemyShot->kind = img_enemyShotMediumOval[col];
                break;
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 弾幕：7セグ数字描画 ＋ 全方位 or 自機狙い
static void ShotNabeatsu(sEnemyShotSet* pEnemyShotSet)
{
    // 発射フレームのみ弾を生成する
    if (pEnemyShotSet->count == 0) {
        int num = pEnemyShotSet->param_i[0];
        bool isAho = IsNabeatsu(num);

        // 条件に応じて効果音を切り替え
        if (isAho) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        else {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 弾をリストに追加するラムダ式
        auto addShot = [&](double x, double y, double speed, double angle, int kind) {
            sEnemyShot* p = new sEnemyShot;
            p->x = x;
            p->y = y;
            p->speed = speed;
            p->muki = angle;
            p->kind = kind;
            p->margin = 40;

            // 双方向循環リストの末尾（headの直前）に挿入
            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        };

        // 7セグの描画色（3関連の時は赤、通常時は青）
        int color7seg = isAho ? 0 : 4;

        // 指定された線分上に5発の小玉弾を並べるラムダ式
        auto drawLine = [&](double x1, double y1, double x2, double y2) {
            for (int i = 0; i < 5; ++i) {
                double tx = x1 + (x2 - x1) * i / 4.0;
                double ty = y1 + (y2 - y1) * i / 4.0;
                // 7セグを構成する弾は真下にゆっくり落ちるようにする
                addShot(tx, ty, 1.0, DX_PI / 2.0, img_enemyShotSmallBall[color7seg]);
            }
        };

        // 表示する数字を桁ごとに分解（下位桁から格納される）
        std::vector<int> digits;
        int temp = num;
        if (temp == 0) {
            digits.push_back(0);
        }
        else {
            while (temp > 0) {
                digits.push_back(temp % 10);
                temp /= 10;
            }
        }

        // 文字の描画位置の計算
        int count_digits = (int)digits.size();
        double w = 12.0;   // セグメントの半幅
        double h = 16.0;   // セグメントの半高
        double gap = 12.0; // 文字間の隙間
        double total_w = count_digits * (2 * w) + (count_digits - 1) * gap;
        double startX = pEnemyShotSet->x - total_w / 2.0 + w; // 一番左の文字の中心X

        // 7セグメントデータ (ビット割当: 0=上, 1=左上, 2=右上, 3=中, 4=左下, 5=右下, 6=下)
        int segData[10] = {
            0b1110111, // 0
            0b0100100, // 1
            0b1011101, // 2
            0b1101101, // 3
            0b0101110, // 4
            0b1101011, // 5
            0b1111011, // 6
            0b0100101, // 7
            0b1111111, // 8
            0b1101111  // 9
        };

        // 左の桁から順に描画
        for (int i = 0; i < count_digits; i++) {
            int d = digits[count_digits - 1 - i];
            double cx = startX + i * (2 * w + gap);
            double cy = pEnemyShotSet->y - 30.0; // 敵の少し上に描画
            int pattern = segData[d];

            if (pattern & (1 << 0)) drawLine(cx - w, cy - h, cx + w, cy - h); // 上
            if (pattern & (1 << 1)) drawLine(cx - w, cy - h, cx - w, cy);     // 左上
            if (pattern & (1 << 2)) drawLine(cx + w, cy - h, cx + w, cy);     // 右上
            if (pattern & (1 << 3)) drawLine(cx - w, cy, cx + w, cy);     // 中
            if (pattern & (1 << 4)) drawLine(cx - w, cy, cx - w, cy + h); // 左下
            if (pattern & (1 << 5)) drawLine(cx + w, cy, cx + w, cy + h); // 右下
            if (pattern & (1 << 6)) drawLine(cx - w, cy + h, cx + w, cy + h); // 下
        }

        // --- 攻撃弾の生成 ---
        if (isAho) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotScatter;
            pSet->x = pEnemyShotSet->x;
            pSet->y = pEnemyShotSet->y;

            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        else {
            // 通常時は自機狙い弾 (少し避けごたえのある3way)
            double angle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            for (int i = 0; i < current_num - 1; i++) {
                // 黄色の銃弾を発射
                addShot(pEnemyShotSet->x, pEnemyShotSet->y, 3.8, angle + (i - (current_num - 2) / 2.0) * 0.15, img_enemyShotLaser[1]);
            }
        }
    }

    // --- 弾の移動処理 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Nabeatsu()
{
    // 出現時の初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 300;
        current_num = 1;
    }
    else {
        // 画面上部をゆるやかに左右移動させる
        enemy.x = 240.0 + 80.0 * sin(count * DX_PI / 120.0);
    }

    // 60フレームごとに数字を表示して攻撃
    if (count % 60 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotNabeatsu;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->param_i[0] = current_num; // 表示する数字を渡す

        // リスト用ダミーヘッドの生成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // EnemyShotSetのリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        current_num++; // 次回発射のためにカウントアップ
    }
}