// enemyPat_kinkakuji.cpp
// 東方文花帖「新難題「金閣寺の一枚天井」」再現
// 敵本体関数: void EnemyPat_Kinkakuji_Grok()

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ======================
// 金閣寺の一枚天井 用 弾幕関数
// ======================
static void ShotKinkakuji(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int totalShots = 7;  // 文花帖準拠: 7枚撮影でクリア
    int phase = totalShots - pEnemyShotSet->count;  // 残り撮影枚数風 (1が最終)

    if (pEnemyShotSet->count == 0) {
        // 初回のみ音と初期弾生成
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);  // 中くらいの音

        // 黄色の「一枚天井」横一列弾 (左右交互っぽく複数列生成)
        // 黄色 = 色1 (img_enemyShotLargeBall などを使う)
        for (int side = 0; side < 2; side++) {  // 左右
            double baseX = pEnemyShotSet->x + (side == 0 ? -120 : 120);
            for (int i = 0; i < 5; i++) {  // 1列に5個程度の横並び
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = baseX + (i - 2) * 50.0 + GetRand(20) - 10;
                pEnemyShot->y = pEnemyShotSet->y - 30.0 + GetRand(20) - 10;
                pEnemyShot->muki = DX_PI / 2.0;  // 下方向
                pEnemyShot->speed = 1.2 + (GetRand(40) / 100.0);  // ゆっくり下降
                pEnemyShot->kind = img_enemyShotLargeBall[1];  // 黄色 大玉 (色1)
                // リンク挿入
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 回転散弾の初期セット (X字4方向)
        double rotSpeed = 0.025;  // 回転速度ベース
        for (int dir = 0; dir < 4; dir++) {
            double baseAngle = dir * (DX_PI / 2.0);
            for (int k = 0; k < 3; k++) {  // 各方向に3弾程度
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = baseAngle + (k - 1) * 0.3;
                pEnemyShot->speed = 2.0 + k * 0.4;
                pEnemyShot->kind = img_enemyShotMediumBall[2];  // 初期緑 (色2)
                pEnemyShot->param_d[0] = rotSpeed * (dir % 2 == 0 ? 1.0 : -1.0);  // 回転方向 param
                pEnemyShot->param_i[0] = 0;  // フェーズ管理用
                // リンク
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレーム更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 天井弾 (下方向直進)
        if (pEnemyShot->kind == img_enemyShotLargeBall[1] ||
            pEnemyShot->kind == img_enemyShotMediumBall[1]) {
            pEnemyShot->y += pEnemyShot->speed;
        }
        // 回転散弾
        else {
            // 回転適用
            pEnemyShot->muki += pEnemyShot->param_d[0];

            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

            // フェーズに応じて色・速度変化 (残り撮影枚数風)
            int currentPhase = phase;
            if (currentPhase <= 4) {  // 水色追加
                if (pEnemyShot->param_i[0] == 0) {
                    pEnemyShot->kind = img_enemyShotMediumBall[3];  // シアン/水色 色3
                    pEnemyShot->param_d[0] *= -1.2;  // 反時計+速め
                    pEnemyShot->param_i[0] = 1;
                }
            }
            if (currentPhase <= 2) {  // 青追加
                if (pEnemyShot->param_i[0] <= 1) {
                    pEnemyShot->kind = img_enemyShotMediumBall[4];  // 青 色4
                    pEnemyShot->speed *= 0.95;  // やや遅め
                    pEnemyShot->param_d[0] *= (currentPhase % 2 == 0 ? 1 : -1);
                    pEnemyShot->param_i[0] = 2;
                }
            }
            if (currentPhase <= 1) {  // 紫追加 (最終地獄)
                if (pEnemyShot->param_i[0] <= 2) {
                    pEnemyShot->kind = img_enemyShotMediumBall[5];  // マゼンタ/紫 色5
                    pEnemyShot->speed = 2.8;  // 密度高
                    pEnemyShot->param_d[0] *= -1.0;
                    pEnemyShot->param_i[0] = 3;
                }
            }
        }

        pEnemyShot = pEnemyShot->next;
    }

    pEnemyShotSet->count++;  // メインルーチンで自動+1だが、ここでも管理用に
}

// ======================
// 敵本体パターン (Tmp)
// ======================
void EnemyPat_Kinkakuji_Grok()
{
    static int muki = 1;
    static int shotTimer = 0;

    if (count == 1) {
        // 初期位置 (ゲーム画面 480x480)
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotTimer = 0;
    }
    else {
        // 左右往復移動 (サンプル準拠)
        enemy.x += 1.2 * (double)muki;
        if (count % 150 == 75) muki *= -1;

        // 少し上下も
        enemy.y = 60.0 + sin(count / 30.0) * 15.0;
    }

    // 定期的に金閣寺弾幕発動
    if (count % 55 == 0 && count > 30) {  // 間隔調整
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKinkakuji;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        // 初期向き (プレイヤー方向参考)
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;  // 未使用
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リスト連結
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        shotTimer++;
    }
}