// enemyPat_fruitEvolution.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 無効化されたセット用ダミー関数（メインルーチンが空セットを検出・削除するまでの繋ぎ）
static void ShotDummy(sEnemyShotSet* pEnemyShotSet)
{
    // 何もしない
}

// 段階に応じたフルーツ弾の見た目を生成（複数弾を組み合わせて大きさを表現）
static void CreateFruitVisual(sEnemyShotSet* pSet, int stage, double baseX, double baseY)
{
    int numShots = 1;
    int mainColor = 0;

    switch (stage) {
    case 0: // チェリー：小玉 赤
    case 1: // イチゴ：小玉 赤
        mainColor = 0;
        numShots = 1;
        break;
    case 2: // ブドウ：中玉 マゼンタ
        mainColor = 5;
        numShots = 1;
        break;
    case 3: // オレンジ：中玉 橙
        mainColor = 8;
        numShots = 1;
        break;
    case 4: // リンゴ：中玉 赤
        mainColor = 0;
        numShots = 1;
        break;
    case 5: // 梨：中玉 緑
        mainColor = 2;
        numShots = 1;
        break;
    case 6: // 桃：大玉 マゼンタ
        mainColor = 5;
        numShots = 1;
        break;
    case 7: // パイナップル：大玉 黄
        mainColor = 1;
        numShots = 1;
        break;
    case 8: // メロン：大玉 緑を7個（中心1＋周囲6）で表現
        mainColor = 2;
        numShots = 7;
        break;
    case 9: // スイカ：大玉を17個（中心緑1＋内周赤8＋外周緑8）で表現
        mainColor = 2;
        numShots = 17;
        break;
    }

    for (int i = 0; i < numShots; i++) {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->speed = 0.0;
        pShot->muki = 0.0;
        pShot->count = 0;
        pShot->param_i[0] = stage;
        pShot->param_i[1] = i;
        pShot->param_i[2] = 5;      // 合成クールタイム（生成直後は合成しない）
        pShot->x = baseX;
        pShot->y = baseY;

        int color = mainColor;
        if (stage == 9) {
            if (i == 0)       color = 2;  // 中心：緑
            else if (i <= 8)  color = 0;  // 内周：赤
            else              color = 2;  // 外周：緑
        }

        if (stage <= 1) {
            pShot->kind = img_enemyShotSmallBall[color];
        }
        else if (stage <= 5) {
            pShot->kind = img_enemyShotMediumBall[color];
        }
        else {
            pShot->kind = img_enemyShotLargeBall[color];
        }

        // 相対位置（大きな弾は複数の大玉を組み合わせて表現）
        if (stage == 8) {
            if (i == 0) {
                pShot->param_d[0] = 0.0;
                pShot->param_d[1] = 0.0;
            }
            else {
                double angle = (i - 1) * DX_PI / 3.0;
                pShot->param_d[0] = cos(angle) * 14.0;
                pShot->param_d[1] = sin(angle) * 14.0;
            }
        }
        else if (stage == 9) {
            if (i == 0) {
                pShot->param_d[0] = 0.0;
                pShot->param_d[1] = 0.0;
            }
            else if (i <= 8) {
                double angle = (i - 1) * DX_PI / 4.0;
                pShot->param_d[0] = cos(angle) * 16.0;
                pShot->param_d[1] = sin(angle) * 16.0;
            }
            else {
                double angle = (i - 9) * DX_PI / 4.0 + DX_PI / 8.0;
                pShot->param_d[0] = cos(angle) * 28.0;
                pShot->param_d[1] = sin(angle) * 28.0;
            }
        }
        else {
            pShot->param_d[0] = (GetRand(10) - 5) / 5.0;
            pShot->param_d[1] = (GetRand(10) - 5) / 5.0;
        }

        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
    }
}

// 弾幕：進化の果実列（スイカゲーム風合成進化弾幕）
static void ShotFruit(sEnemyShotSet* pSet)
{
    const int stage = pSet->param_i[0];
    const double gravity = 0.06;
    const double maxFallSpeed = 2.5;

    // 無効セットは無視
    if (pSet->patternFunc == nullptr || pSet->patternFunc == ShotDummy) {
        return;
    }

    // 初回生成
    if (pSet->count == 0) {
        int se;
        if (stage <= 2)       se = sound_enemyShot_light;
        else if (stage <= 5)  se = sound_enemyShot_medium;
        else if (stage <= 7)  se = sound_enemyShot_heavy;
        else                  se = sound_enemyShot_extreme;

        if (count % 10 == 0) {
            if (CheckSoundMem(se)) StopSoundMem(se);
            PlaySoundMem(se, DX_PLAYTYPE_BACK);
        }

        // 論理座標・速度の初期化
        pSet->param_d[0] = pSet->x;
        pSet->param_d[1] = pSet->y;

        // スイカ爆発からの生成時は param_d[4], param_d[5] に初速が入っている
        if (pSet->param_d[4] != 0.0 || pSet->param_d[5] != 0.0) {
            pSet->param_d[2] = pSet->param_d[5]; // Y速度
            pSet->param_d[3] = pSet->param_d[4]; // X速度
            pSet->param_d[4] = 0.0;
            pSet->param_d[5] = 0.0;
        }
        else {
            pSet->param_d[2] = 0.3 + GetRand(100) / 300.0;
            pSet->param_d[3] = (GetRand(40) - 20) / 100.0;
        }

        CreateFruitVisual(pSet, stage, pSet->param_d[0], pSet->param_d[1]);
    }

    // 重力と移動
    pSet->param_d[2] += gravity;
    if (pSet->param_d[2] > maxFallSpeed) {
        pSet->param_d[2] = maxFallSpeed;
    }

    pSet->param_d[0] += pSet->param_d[3];
    pSet->param_d[1] += pSet->param_d[2];

    // 画面端処理（下端でバウンド、左右端で反射）
    if (pSet->param_d[1] > 470.0) {
        pSet->param_d[1] = 470.0;
        pSet->param_d[2] *= -0.4;
        pSet->param_d[3] += (GetRand(60) - 30) / 100.0;
        if (pSet->param_d[2] > -0.3) pSet->param_d[2] = -0.3;
    }
    if (pSet->param_d[0] < 15.0) {
        pSet->param_d[0] = 15.0;
        pSet->param_d[3] *= -0.8;
    }
    if (pSet->param_d[0] > 465.0) {
        pSet->param_d[0] = 465.0;
        pSet->param_d[3] *= -0.8;
    }

    // 物理弾の位置を論理座標に同期
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x = pSet->param_d[0] + pShot->param_d[0];
        pShot->y = pSet->param_d[1] + pShot->param_d[1];
        pShot = pShot->next;
    }

    // 最上位段階は合成しない
    if (stage >= 9) return;

    // クールタイム減少（先頭弾の param_i[2] を代表として使う）
    pShot = pSet->pEnemyShotHead->next;
    if (pShot != pSet->pEnemyShotHead && pShot->param_i[2] > 0) {
        pShot->param_i[2]--;
    }
    if (pShot != pSet->pEnemyShotHead && pShot->param_i[2] > 0) return;

    // 他のフルーツセットと距離判定
    sEnemyShotSet* pOther = enemyShotSetHead.next;
    while (pOther != &enemyShotSetHead) {
        sEnemyShotSet* pNextOther = pOther->next;

        if (pOther == pSet || pOther->patternFunc != ShotFruit) {
            pOther = pNextOther;
            continue;
        }
        if (pOther->param_i[0] != stage) {
            pOther = pNextOther;
            continue;
        }

        // 相手のクールタイム確認
        sEnemyShot* pOtherShot = pOther->pEnemyShotHead->next;
        if (pOtherShot != pOther->pEnemyShotHead && pOtherShot->param_i[2] > 0) {
            pOther = pNextOther;
            continue;
        }

        double dx = pSet->param_d[0] - pOther->param_d[0];
        double dy = pSet->param_d[1] - pOther->param_d[1];
        double dist = sqrt(dx * dx + dy * dy);

        // 合成距離閾値（段階が上がるごとに大きくなる）
        double mergeDist = 10.0 + stage * 3.0;
        if (stage >= 6) mergeDist = 22.0;
        if (stage >= 8) mergeDist = 32.0;

        if (dist < mergeDist) {
            // 合成！1つ上の段階を生成
            sEnemyShotSet* pNew = new sEnemyShotSet;
            pNew->count = 0;
            pNew->patternFunc = ShotFruit;
            pNew->x = (pSet->param_d[0] + pOther->param_d[0]) * 0.5;
            pNew->y = (pSet->param_d[1] + pOther->param_d[1]) * 0.5;
            pNew->muki = 0.0;
            pNew->kind = 0;
            pNew->param_i[0] = stage + 1;

            pNew->pEnemyShotHead = new sEnemyShot;
            pNew->pEnemyShotHead->prev = pNew->pEnemyShotHead;
            pNew->pEnemyShotHead->next = pNew->pEnemyShotHead;

            pNew->prev = enemyShotSetHead.prev;
            pNew->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pNew;
            enemyShotSetHead.prev = pNew;

            // 衝撃波：周囲の弾を弾き飛ばす
            sEnemyShotSet* pWave = enemyShotSetHead.next;
            while (pWave != &enemyShotSetHead) {
                if (pWave != pNew && pWave != pSet && pWave != pOther && pWave->patternFunc == ShotFruit) {
                    double wdx = pWave->param_d[0] - pNew->x;
                    double wdy = pWave->param_d[1] - pNew->y;
                    double wdist = sqrt(wdx * wdx + wdy * wdy);
                    if (wdist < 50.0 && wdist > 0.1) {
                        pWave->param_d[3] += (wdx / wdist) * 1.2;
                        pWave->param_d[2] -= 0.3;
                    }
                }
                pWave = pWave->next;
            }

            // スイカ（段階9）完成時：全方位に種弾を撒き散らす
            if (stage + 1 == 9) {
                if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

                for (int i = 0; i < 12; i++) {
                    sEnemyShotSet* pSeed = new sEnemyShotSet;
                    pSeed->count = -1;
                    pSeed->patternFunc = ShotFruit;
                    pSeed->x = pNew->x;
                    pSeed->y = pNew->y;
                    pSeed->muki = 0.0;
                    pSeed->kind = 0;
                    pSeed->param_i[0] = 0; // チェリーに戻る
                    double angle = i * DX_PI / 6.0;
                    pSeed->param_d[4] = cos(angle) * 2.5; // X初速
                    pSeed->param_d[5] = sin(angle) * 2.5; // Y初速

                    pSeed->pEnemyShotHead = new sEnemyShot;
                    pSeed->pEnemyShotHead->prev = pSeed->pEnemyShotHead;
                    pSeed->pEnemyShotHead->next = pSeed->pEnemyShotHead;

                    pSeed->prev = enemyShotSetHead.prev;
                    pSeed->next = &enemyShotSetHead;
                    enemyShotSetHead.prev->next = pSeed;
                    enemyShotSetHead.prev = pSeed;
                }
            }

            // pOther（相手セット）を無効化：弾を画面外に飛ばし、ダミー化
            // メインルーチンが次フレームで弾を画面外判定→削除→空セットを削除してくれる
            sEnemyShot* ps = pOther->pEnemyShotHead->next;
            while (ps != pOther->pEnemyShotHead) {
                ps->x = -9999.0;
                ps->y = -9999.0;
                ps = ps->next;
            }
            pOther->patternFunc = ShotDummy;
            pOther->param_d[0] = -9999.0;
            pOther->param_d[1] = -9999.0;

            // pSet（自分）を無効化：同様に画面外飛ばし＋ダミー化
            ps = pSet->pEnemyShotHead->next;
            while (ps != pSet->pEnemyShotHead) {
                ps->x = -9999.0;
                ps->y = -9999.0;
                ps = ps->next;
            }
            pSet->patternFunc = ShotDummy;
            pSet->param_d[0] = -9999.0;
            pSet->param_d[1] = -9999.0;

            return;
        }
        pOther = pNextOther;
    }
}

// 敵本体のパターン
void EnemyPat_SuikaGame_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 40フレームごとに新しいフルーツ弾を生成（チェリーから開始）
    if (count % 1 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotFruit;
        pSet->x = enemy.x + GetRand(200) - 100;
        pSet->y = enemy.y + 15.0;
        pSet->muki = DX_PI / 2.0;
        pSet->kind = shot_count++;
        pSet->param_i[0] = 0; // チェリー

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}