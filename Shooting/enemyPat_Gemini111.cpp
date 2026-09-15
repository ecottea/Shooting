// enemyPat_Tmp.cpp

#include <cmath>

// ※ DxLib.h や gv.h はプロジェクトの環境に合わせてインクルードしてください
// #include "gv.h"

// ---------------------------------------------------------------------------
// 弾幕パターン：純白の開花と重力落果（ケントの花 ＋ リンゴ落果）
// ---------------------------------------------------------------------------
static void ShotKentApple(sEnemyShotSet* pEnemyShotSet)
{
    // =======================================================================
    // 1. 開花フェーズ（生成時 count == 0）
    // =======================================================================
    if (pEnemyShotSet->count == 0) {
        // 蕾・開花の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // --- 花芯：黄色の中玉(1) 1発 ＋ 周囲に黄緑(緑2)の小玉 8発 ---
        sEnemyShot* pCenter = new sEnemyShot;
        pCenter->x = pEnemyShotSet->x;
        pCenter->y = pEnemyShotSet->y;
        pCenter->muki = 0.0;
        pCenter->speed = 0.0;
        pCenter->kind = img_enemyShotMediumBall[1]; // 黄色の中玉
        pCenter->param_i[0] = 2; // 種別：花芯
        pCenter->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pCenter->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pCenter;
        pEnemyShotSet->pEnemyShotHead->prev = pCenter;

        for (int i = 0; i < 8; i++) {
            sEnemyShot* pCore = new sEnemyShot;
            pCore->x = pEnemyShotSet->x;
            pCore->y = pEnemyShotSet->y;
            pCore->muki = i * (DX_PI * 2.0 / 8.0);
            pCore->speed = 0.3; // 固さを出すため極低速で展開
            pCore->kind = img_enemyShotSmallBall[2]; // 緑色の小玉（花芯周り）
            pCore->param_i[0] = 2; // 種別：花芯
            pCore->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pCore->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pCore;
            pEnemyShotSet->pEnemyShotHead->prev = pCore;
        }

        // --- 内側の花弁：白(6)の菱形弾（5方向へ重なるV字状に展開）---
        for (int petal = 0; petal < 5; petal++) {
            double baseAngle = pEnemyShotSet->muki + petal * (DX_PI * 2.0 / 5.0);
            for (int j = -2; j <= 2; j++) {
                sEnemyShot* pPetal = new sEnemyShot;
                pPetal->x = pEnemyShotSet->x;
                pPetal->y = pEnemyShotSet->y;
                pPetal->muki = baseAngle + j * 0.08;
                pPetal->speed = 1.2 + (2 - abs(j)) * 0.2; // 花弁の丸みを帯びた形状を作る
                pPetal->kind = img_enemyShotDiamond[6]; // 白の菱形弾（重ね花弁）
                pPetal->param_i[0] = 0; // 種別：花弁弾
                pPetal->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pPetal->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pPetal;
                pEnemyShotSet->pEnemyShotHead->prev = pPetal;
            }
        }

        // --- 外側の花弁：白(6)の中楕円弾（花弁の隙間を埋めるリング）---
        for (int i = 0; i < 10; i++) {
            sEnemyShot* pOuter = new sEnemyShot;
            pOuter->x = pEnemyShotSet->x;
            pOuter->y = pEnemyShotSet->y;
            pOuter->muki = pEnemyShotSet->muki + (i + 0.5) * (DX_PI * 2.0 / 10.0);
            pOuter->speed = 1.8;
            pOuter->kind = img_enemyShotMediumOval[6]; // 白の中楕円弾（八重咲きのボリューム）
            pOuter->param_i[0] = 0; // 種別：花弁弾
            pOuter->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pOuter->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pOuter;
            pEnemyShotSet->pEnemyShotHead->prev = pOuter;
        }
    }

    // =======================================================================
    // 2. 結実フェーズ（満開と同時にリンゴの実が形成される count == 40）
    // =======================================================================
    if (pEnemyShotSet->count == 40) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 赤の大玉（リンゴ本体）
        sEnemyShot* pApple = new sEnemyShot;
        pApple->x = pEnemyShotSet->x;
        pApple->y = pEnemyShotSet->y;
        pApple->muki = DX_PI / 2.0; // 下向き
        pApple->speed = 0.0;
        pApple->kind = img_enemyShotLargeBall[0]; // 赤の大玉
        pApple->param_i[0] = 3; // 種別：リンゴ本体
        pApple->param_d[0] = 0.0; // 下方向への落下加速度(vy)
        pApple->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pApple->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pApple;
        pEnemyShotSet->pEnemyShotHead->prev = pApple;

        // 緑の小玉（リンゴの葉・ヘタ）
        sEnemyShot* pLeaf = new sEnemyShot;
        pLeaf->x = pEnemyShotSet->x;
        pLeaf->y = pEnemyShotSet->y - 12.0; // 大玉の少し上
        pLeaf->muki = DX_PI / 2.0;
        pLeaf->speed = 0.0;
        pLeaf->kind = img_enemyShotSmallBall[2]; // 緑の小玉
        pLeaf->param_i[0] = 4; // 種別：リンゴの葉
        pLeaf->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pLeaf->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pLeaf;
        pEnemyShotSet->pEnemyShotHead->prev = pLeaf;
    }

    // =======================================================================
    // 3. リンゴの破裂処理（落果して着弾した瞬間 count == 110）
    // =======================================================================
    if (pEnemyShotSet->count == 110) {
        // 重い破裂音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // リンゴが落ちた現在位置を特定するため、リストから探す
        double explodeX = pEnemyShotSet->x;
        double explodeY = pEnemyShotSet->y + 200.0;

        sEnemyShot* pSearch = pEnemyShotSet->pEnemyShotHead->next;
        while (pSearch != pEnemyShotSet->pEnemyShotHead) {
            if (pSearch->param_i[0] == 3) { // リンゴ本体
                explodeX = pSearch->x;
                explodeY = pSearch->y;
                break;
            }
            pSearch = pSearch->next;
        }

        // 果汁・果肉の飛び散り：赤(0)の菱形弾を36方向全方位に拡散破裂
        for (int i = 0; i < 36; i++) {
            sEnemyShot* pBurst = new sEnemyShot;
            pBurst->x = explodeX;
            pBurst->y = explodeY;
            pBurst->muki = i * (DX_PI * 2.0 / 36.0);
            pBurst->speed = 2.5 + (GetRand(100) / 100.0); // 弾速にバラつきを持たせる
            pBurst->kind = img_enemyShotDiamond[0]; // 赤の菱形弾
            pBurst->param_i[0] = 5; // 種別：破裂弾
            pBurst->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBurst->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBurst;
            pEnemyShotSet->pEnemyShotHead->prev = pBurst;
        }
    }

    // =======================================================================
    // 4. 全弾の更新・挙動制御（フレーム毎の挙動変化）
    // =======================================================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // --- 種別 0：白い花弁弾（咲いてから回転し、風に舞うように落下）---
        if (pShot->param_i[0] == 0) {
            if (pEnemyShotSet->count < 40) {
                // 初期広がり（徐々に減速）
                pShot->speed *= 0.95;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else if (pEnemyShotSet->count < 110) {
                // 花の回転挙動（優雅に旋回）
                pShot->muki += 0.008;
                pShot->x += 0.5 * cos(pShot->muki);
                pShot->y += 0.5 * sin(pShot->muki);
            }
            else {
                // 舞い散りフェーズ：リンゴ破裂後、ゆらゆら揺れながら下へ落果・舞い散る
                pShot->x += sin((double)pShot->count * 0.1) * 0.8;
                pShot->y += 1.2;
            }
        }
        // --- 種別 2：花芯（蕾から中心にとどまり、破裂時に消散）---
        else if (pShot->param_i[0] == 2) {
            if (pEnemyShotSet->count < 110) {
                pShot->speed *= 0.90;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            else {
                // 破裂後は外側へ逃げて消散
                pShot->speed = 2.0;
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        // --- 種別 3 & 4：リンゴ本体と葉（重力加速度で落下）---
        else if (pShot->param_i[0] == 3 || pShot->param_i[0] == 4) {
            if (pEnemyShotSet->count >= 40 && pEnemyShotSet->count < 110) {
                // 重力加速度を加算 (y方向に加速)
                pShot->param_d[0] += 0.12;
                pShot->y += pShot->param_d[0];
            }
            else if (pEnemyShotSet->count >= 110) {
                // 破裂したため、実と葉は画面外へ消去（画面外へ大きく飛ばしてメインの消去処理に任せる）
                pShot->x = -9999.0;
            }
        }
        // --- 種別 5：破裂弾（直線移動）---
        else if (pShot->param_i[0] == 5) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------------------
// 敵本体のパターン関数
// ---------------------------------------------------------------------------
void EnemyPat_FlowerOfKent_Gemini()
{
    static int muki;

    if (count == 1) {
        // 出現位置と基本ステータス
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 敵本体のゆるやかな左右移動
        enemy.x += 0.8 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 200フレーム周期（約3.3秒周期）でケントの花とリンゴ落果弾幕を展開
    if (count % 200 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotKentApple;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;

        // 自機の方向を基準角にしつつ、少しゆらぎ（ランダム性）を持たせる
        // GetRand(60) で -30～+30 度のゆらぎを付加
        double aimAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double randOffset = ((int)GetRand(60) - 30) / 180.0 * DX_PI;
        pEnemyShotSet->muki = aimAngle + randOffset;

        // リストの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体リストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}