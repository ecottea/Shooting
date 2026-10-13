#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：超新星爆発（重力崩壊 -> 衝撃波 -> 元素散乱とパルサー）
static void ShotSupernova(sEnemyShotSet* pEnemyShotSet)
{
    // ============================================================
    // フェーズ1：重力崩壊（収束） [0 <= count < 180]
    // ============================================================
    if (pEnemyShotSet->count < 180) {
        // 5フレームごとに弾を生成
        if (pEnemyShotSet->count % 5 == 0) {
            if (pEnemyShotSet->count == 0) {
                if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            }

            for (int i = 0; i < 6; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;

                // 外周から発生させ、発生角度を徐々にずらすことで螺旋状の収束を表現
                double spawn_angle = (pEnemyShotSet->count * 0.08) + (i * (DX_PI * 2.0 / 6.0));
                double spawn_r = 350.0;
                pEnemyShot->x = pEnemyShotSet->x + cos(spawn_angle) * spawn_r;
                pEnemyShot->y = pEnemyShotSet->y + sin(spawn_angle) * spawn_r;

                // 中心に向かう角度
                pEnemyShot->muki = atan2(pEnemyShotSet->y - pEnemyShot->y, pEnemyShotSet->x - pEnemyShot->x);

                // 時間経過で加速（重力崩壊の加速表現）
                pEnemyShot->speed = 1.5 + (pEnemyShotSet->count / 50.0);

                // 色: 赤(0), 橙(8), 黄(1) からランダム
                int color_rand = GetRand(2);
                int color = (color_rand == 0) ? 0 : ((color_rand == 1) ? 8 : 1);
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                pEnemyShot->margin = 480;

                // リストに追加
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }
    // ============================================================
    // フェーズ2：超新星爆発（衝撃波の拡散） [count == 180]
    // ============================================================
    else if (pEnemyShotSet->count == 180) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 衝撃波のリング（中玉：白）
        for (int i = 0; i < 24; i++) {
            sEnemyShot* pRing = new sEnemyShot;
            pRing->x = pEnemyShotSet->x;
            pRing->y = pEnemyShotSet->y;
            pRing->muki = i * (DX_PI * 2.0 / 24.0);
            pRing->speed = 4.5;
            pRing->kind = img_enemyShotMediumBall[6]; // 6:白
            pRing->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRing->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRing;
            pEnemyShotSet->pEnemyShotHead->prev = pRing;
        }

        // リングの隙間を縫う高速弾（銃弾：赤と青）
        for (int i = 0; i < 12; i++) {
            sEnemyShot* pSpiral = new sEnemyShot;
            pSpiral->x = pEnemyShotSet->x;
            pSpiral->y = pEnemyShotSet->y;
            // リングの弾の間隔(15度)の半分(7.5度)ずらす
            pSpiral->muki = i * (DX_PI * 2.0 / 12.0) + (DX_PI * 2.0 / 48.0);
            pSpiral->speed = 7.0;
            int color = (i % 2 == 0) ? 0 : 4; // 0:赤, 4:青
            pSpiral->kind = img_enemyShotBullet[color];
            pSpiral->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pSpiral->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pSpiral;
            pEnemyShotSet->pEnemyShotHead->prev = pSpiral;
        }
    }
    // ============================================================
    // フェーズ3：元素の散乱とパルサー（残骸） [count > 180]
    // ============================================================
    else if (pEnemyShotSet->count <= 400) {
        int phase3_count = pEnemyShotSet->count - 180;

        // 元素の散乱：ランダムな方向・速度で多色の小弾を飛ばす
        if (phase3_count % 8 == 0) {
            for (int i = 0; i < 3; i++) {
                sEnemyShot* pScatter = new sEnemyShot;
                pScatter->x = pEnemyShotSet->x;
                pScatter->y = pEnemyShotSet->y;
                pScatter->muki = GetRand(360) / 360.0 * DX_PI * 2.0;
                pScatter->speed = 1.5 + GetRand(300) / 100.0; // 1.5 ~ 4.5

                // 色: 緑(2), シアン(3), マゼンタ(5), 黄(1) からランダム
                int colors[] = { 2, 3, 5, 1 };
                int color = colors[GetRand(3)];
                pScatter->kind = img_enemyShotSmallBall[color];

                pScatter->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pScatter->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pScatter;
                pEnemyShotSet->pEnemyShotHead->prev = pScatter;
            }
        }

        // パルサー：回転しながら放射される短レーザー
        if (phase3_count % 12 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            for (int i = 0; i < 8; i++) {
                sEnemyShot* pLaser = new sEnemyShot;
                pLaser->x = pEnemyShotSet->x;
                pLaser->y = pEnemyShotSet->y;
                // フレーム経過で角度を回転させる + 8方向配置
                pLaser->muki = (phase3_count * 0.06) + (i * (DX_PI * 2.0 / 8.0));
                pLaser->speed = 9.0; // 高速で飛んで画面外で自動消去される
                pLaser->kind = img_enemyShotLaser[6]; // 6:白
                pLaser->margin = 80;

                pLaser->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pLaser->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pLaser;
                pEnemyShotSet->pEnemyShotHead->prev = pLaser;
            }
        }
    }

    // 既存の弾の移動処理（メインルーチンがcountを管理するため、位置更新はここで行う）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Supernova_Qwen()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 180.0; // やや上寄りに配置し、下方への展開を広く取る
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        // 敵の動き：ゆっくりと左右に移動
        enemy.x += 0.4 * (double)muki;
        if (count % 180 == 90) muki *= -1;

        // 弾幕セットの基準位置を敵の現在位置に追従させる
        sEnemyShotSet* pEnemyShotSet = enemyShotSetHead.next;
        if (pEnemyShotSet != &enemyShotSetHead) {
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;
        }
    }

    if (count % 460 == 1) {
        // 超新星爆発パターン用の弾幕セットを1つだけ生成し、使い回す
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSupernova;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0;
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