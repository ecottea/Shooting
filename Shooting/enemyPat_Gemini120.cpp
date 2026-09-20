#include <cmath> // hypot, sin, cos, atan2

// ============================================================
// ボス1の弾幕（感情的な赤・拡散弾）
// ============================================================
static void ShotRedSpread(sEnemyShotSet* pEnemyShotSet)
{
    // ボス1(enemy.x, enemy.y)の位置に追従
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // 4フレームごとにボス2へ向けて5WAYの波状弾を発射
    if (pEnemyShotSet->count % 4 == 0) {
        if (pEnemyShotSet->count % 8 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        double targetAngle = atan2(enemy.y2 - enemy.y, enemy.x2 - enemy.x);
        for (int i = -2; i <= 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = targetAngle + (i * 12.0) / 180.0 * DX_PI; // 角度をずらして扇状に
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotScale[0]; // 赤・鱗弾

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot->count++;
        pShot = pShot->next;
    }
}

// ============================================================
// ボス2の弾幕（理知的な青・貫通弾）＆ 衝突エフェクト
// ============================================================
static void ShotBluePierce(sEnemyShotSet* pEnemyShotSet)
{
    // ボス2(enemy.x2, enemy.y2)の位置に追従
    pEnemyShotSet->x = enemy.x2;
    pEnemyShotSet->y = enemy.y2;

    // 10フレームごとにボス1へ向けて高速貫通弾を発射
    if (pEnemyShotSet->count % 10 == 0) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        double targetAngle = atan2(enemy.y - enemy.y2, enemy.x - enemy.x2);
        pEnemyShot->muki = targetAngle;
        pEnemyShot->speed = 5.0; // 赤弾より速い
        pEnemyShot->kind = img_enemyShotMediumOval[4]; // 青・中楕円弾

        // 撃った瞬間の2人の距離をもとに、赤弾(速さ3.0)と青弾(速さ5.0)が交差するフレームを計算
        // 相対速度 = 8.0 として、衝突までの時間をparam_i[0]に記憶しておく
        double dist = hypot(enemy.x - enemy.x2, enemy.y - enemy.y2);
        pEnemyShot->param_i[0] = (int)(dist / 8.0);

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 弾の移動・破片発生処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot->count++;

        // 青弾のみ、計算上の衝突タイミングで紫色の破片を撒き散らす
        if (pShot->kind == img_enemyShotMediumOval[4]) {
            if (pShot->count == pShot->param_i[0]) {
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                // 衝突地点から紫色の破片を全方位にランダムに散らす
                for (int i = 0; i < 16; i++) {
                    sEnemyShot* pFrag = new sEnemyShot;
                    pFrag->x = pShot->x;
                    pFrag->y = pShot->y;
                    pFrag->muki = (i * DX_PI * 2.0 / 16.0) + (GetRand(314) / 100.0);
                    pFrag->speed = 1.0 + GetRand(150) / 100.0;
                    pFrag->kind = img_enemyShotSmallBall[5]; // マゼンタ(紫)・小玉
                    pFrag->count = 0;

                    pFrag->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pFrag->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pFrag;
                    pEnemyShotSet->pEnemyShotHead->prev = pFrag;
                }
            }
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン関数
// ============================================================
void EnemyPat_FallOut_Gemini()
{
    if (count == 1) {
        enemy.maxHp = enemy.hp = 200;

        // ボス1(赤弾)の弾幕セットを登録
        sEnemyShotSet* pSet1 = new sEnemyShotSet;
        pSet1->count = 0;
        pSet1->patternFunc = ShotRedSpread;
        pSet1->pEnemyShotHead = new sEnemyShot;
        pSet1->pEnemyShotHead->prev = pSet1->pEnemyShotHead;
        pSet1->pEnemyShotHead->next = pSet1->pEnemyShotHead;

        pSet1->prev = enemyShotSetHead.prev;
        pSet1->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet1;
        enemyShotSetHead.prev = pSet1;

        // ボス2(青弾)の弾幕セットを登録
        sEnemyShotSet* pSet2 = new sEnemyShotSet;
        pSet2->count = 0;
        pSet2->patternFunc = ShotBluePierce;
        pSet2->pEnemyShotHead = new sEnemyShot;
        pSet2->pEnemyShotHead->prev = pSet2->pEnemyShotHead;
        pSet2->pEnemyShotHead->next = pSet2->pEnemyShotHead;

        pSet2->prev = enemyShotSetHead.prev;
        pSet2->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet2;
        enemyShotSetHead.prev = pSet2;
    }

    // ボス同士は複雑なsin軌道で画面を動き回る
    // ボス1は画面左側をうねる
    enemy.x = 120.0 + 80.0 * sin(count * DX_PI / 180.0);
    enemy.y = 240.0 + 160.0 * sin(count * DX_PI / 130.0);

    // ボス2は画面右側をうねる（位相をずらして対角線上になりやすくする）
    enemy.x2 = 360.0 + 80.0 * sin(count * DX_PI / 160.0 + DX_PI);
    enemy.y2 = 240.0 + 160.0 * sin(count * DX_PI / 110.0 + DX_PI);
}