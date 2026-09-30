#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  台風の眼パターン
//  内周：時計回り密集渦（小玉・シアン/白）
//  外周：反時計回り渦（中玉・青）
//  雨帯：放射状の鱗弾
//  中心は空洞（眼）を維持し、徐々に回転加速
// ============================================================
static void ShotTyphoonEye(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ---- 初期化（count==0 のときだけ） ----
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // param_d[0] : 現在の回転角度（共通）
        // param_d[1] : 回転角速度（徐々に加速）
        // param_i[0] : フェーズ管理用
        pEnemyShotSet->param_d[0] = 0.0;
        pEnemyShotSet->param_d[1] = 0.04;   // 初期角速度
        pEnemyShotSet->param_i[0] = 0;
    }

    // 回転角度・速度を更新（加速）
    pEnemyShotSet->param_d[1] += 0.00012;          // 徐々に加速
    if (pEnemyShotSet->param_d[1] > 0.12) pEnemyShotSet->param_d[1] = 0.12;
    pEnemyShotSet->param_d[0] += pEnemyShotSet->param_d[1];

    double baseAng = pEnemyShotSet->param_d[0];
    double cx = pEnemyShotSet->x;
    double cy = pEnemyShotSet->y;

    // ---- 弾生成フェーズ（約0〜480フレーム） ----
    if (pEnemyShotSet->count < 480) {

        // 内周渦（時計回り・小玉・密集） 4フレームごと
        if (pEnemyShotSet->count % 4 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            const int NUM = 10;
            const double R_INNER = 55.0 + (pEnemyShotSet->count % 40) * 0.6; // 少し呼吸するように
            for (int i = 0; i < NUM; i++) {
                double ang = baseAng + (DX_PI * 2.0 * i / NUM);
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx + R_INNER * cos(ang);
                pEnemyShot->y = cy + R_INNER * sin(ang);
                // 接線方向 + わずかに外向き
                pEnemyShot->muki = ang + DX_PI * 0.5 + 0.15;
                pEnemyShot->speed = 1.6 + (pEnemyShotSet->count / 400.0) * 0.8;
                // シアン or 白
                int col = (i % 2 == 0) ? 3 : 6;
                pEnemyShot->kind = img_enemyShotSmallBall[col];
                // リンク
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 外周渦（反時計回り・中玉） 6フレームごと
        if (pEnemyShotSet->count % 6 == 0) {
            const int NUM = 8;
            const double R_OUTER = 110.0 + (pEnemyShotSet->count % 50) * 0.8;
            for (int i = 0; i < NUM; i++) {
                double ang = -baseAng * 0.85 + (DX_PI * 2.0 * i / NUM); // 逆回転
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx + R_OUTER * cos(ang);
                pEnemyShot->y = cy + R_OUTER * sin(ang);
                pEnemyShot->muki = ang - DX_PI * 0.5 - 0.12; // 逆接線
                pEnemyShot->speed = 1.9 + (pEnemyShotSet->count / 350.0) * 1.0;
                int col = (i % 3 == 0) ? 4 : 3; // 青 / シアン
                pEnemyShot->kind = img_enemyShotMediumBall[col];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 雨帯（放射状の鱗弾） 20フレームごと
        if (pEnemyShotSet->count % 20 == 10 && pEnemyShotSet->count > 40) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            const int NUM = 12;
            for (int i = 0; i < NUM; i++) {
                double ang = baseAng * 0.3 + (DX_PI * 2.0 * i / NUM) + (GetRand(20) - 10) * 0.01;
                pEnemyShot = new sEnemyShot;
                // 外側から発生
                double r = 140.0 + GetRand(30);
                pEnemyShot->x = cx + r * cos(ang);
                pEnemyShot->y = cy + r * sin(ang);
                // 外向きに飛ばす（台風の雨帯）
                pEnemyShot->muki = ang + (GetRand(30) - 15) * 0.02;
                pEnemyShot->speed = 2.4 + GetRand(80) / 100.0;
                int col = (i % 4 == 0) ? 8 : 4; // 橙アクセント or 青
                pEnemyShot->kind = img_enemyShotScale[col];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    // ---- 終盤：収縮→拡散（480〜560） ----
    else if (pEnemyShotSet->count < 560) {
        if (pEnemyShotSet->count == 480) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        // 中心付近に密集してから外側へ一気に
        if (pEnemyShotSet->count % 3 == 0) {
            const int NUM = 16;
            double t = (pEnemyShotSet->count - 480) / 80.0; // 0→1
            double r = 30.0 + t * 160.0;
            for (int i = 0; i < NUM; i++) {
                double ang = baseAng * 2.0 + (DX_PI * 2.0 * i / NUM);
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = cx + r * 0.3 * cos(ang);
                pEnemyShot->y = cy + r * 0.3 * sin(ang);
                pEnemyShot->muki = ang;
                pEnemyShot->speed = 1.2 + t * 3.5;
                pEnemyShot->kind = img_enemyShotSmallBall[(i % 2 == 0) ? 6 : 3];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ---- 全弾の移動（直線移動） ----
    // ※画面外消去と count インクリメントはメインルーチン側
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
void EnemyPat_Typhoon_Grok()
{
    static int moveDir;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        moveDir = 1;
    }
    else {
        // ゆるやかに左右移動
        enemy.x += 0.6 * (double)moveDir;
        if (enemy.x < 160.0) moveDir = 1;
        if (enemy.x > 320.0) moveDir = -1;
    }

    // パターン開始（一度だけショットセットを生成）
    if (count % 660 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotTyphoonEye;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ショットセットの中心を敵に追従させる（渦の中心が敵についてくる）
    // ※既に生成済みのセットを探す簡易処理
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        if (pSet->patternFunc == ShotTyphoonEye) {
            pSet->x = enemy.x;
            pSet->y = enemy.y + 20.0;
            break;
        }
        pSet = pSet->next;
    }
}