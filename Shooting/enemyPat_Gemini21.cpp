// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：火山噴火（放物線落下 ＆ 空中分裂 ＆ ひらひら火の粉）
static void ShotVolcano(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 1. 初回フレームで火山弾を上空へ向けて一斉に噴出
    if (pEnemyShotSet->count == 0) {
        // 火山らしい重みのある発射音
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 1回の噴火で噴き出す弾数（8〜14発のランダム）
        int shotNum = 8 + GetRand(6);

        for (int i = 0; i < shotNum; i++) {
            pEnemyShot = new sEnemyShot;

            // 初期位置はボスの火口付近（わずかに左右に散らす）
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(20) - 10);
            pEnemyShot->y = pEnemyShotSet->y;

            // 真上（-DX_PI / 2.0）を中心に、左右45度の範囲へランダムに噴射
            double baseMuki = -DX_PI / 2.0;
            double offsetMuki = ((GetRand(90) - 45) / 180.0) * DX_PI;
            pEnemyShot->muki = baseMuki + offsetMuki;

            // 初速（上方向への勢い: 4.0 〜 7.0）
            pEnemyShot->speed = 4.0 + (GetRand(300) / 100.0);

            // 火山らしい色をチョイス（0:赤、1:黄、8:橙）
            int colorRand = GetRand(2);
            int color = (colorRand == 0) ? 0 : ((colorRand == 1) ? 1 : 8);

            // 弾の種類をランダムに決定
            int type = GetRand(3); // 0:大玉(溶岩塊), 1:中玉, 2:小玉, 3:鱗弾(火の粉)
            switch (type) {
            case 0:
                pEnemyShot->kind = img_enemyShotLargeBall[color];
                pEnemyShot->speed *= 0.8;       // 大玉は重いので初速を少し遅く
                pEnemyShot->param_i[0] = 1;     // 特殊フラグ1: 空中爆発する溶岩塊
                pEnemyShot->param_i[1] = 25 + GetRand(20); // 爆発までのタイマー(25〜45F)
                break;
            case 1:
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                pEnemyShot->param_i[0] = 0;     // 通常の放物線弾
                break;
            case 2:
                pEnemyShot->kind = img_enemyShotSmallBall[color];
                pEnemyShot->param_i[0] = 0;     // 通常の放物線弾
                break;
            case 3:
                pEnemyShot->kind = img_enemyShotScale[color];
                pEnemyShot->speed *= 1.1;       // 鱗弾は軽いので初速を少し速く
                pEnemyShot->param_i[0] = 2;     // 特殊フラグ2: ひらひら落下する火の粉
                break;
            }

            pEnemyShot->margin = 960;

            // 双方向リストの末尾に安全に接続
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 2. 全ての弾の全フレーム移動・更新処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {

        // 極座標（speed, muki）から速度ベクトル（vx, vy）に分解
        double vx = pEnemyShot->speed * cos(pEnemyShot->muki);
        double vy = pEnemyShot->speed * sin(pEnemyShot->muki);

        // 重力加速度を適用（vyを増加させることで、上向きの勢いが減速し、やがて落下に転じる）
        vy += 0.025;

        // 火の粉（鱗弾）の場合は、風に煽られるように左右にひらひらと揺らす
        if (pEnemyShot->param_i[0] == 2) {
            vx += sin((double)count * 0.15 + pEnemyShot->speed) * 0.08;
        }

        // ベクトルから新しい speed と muki を再計算して格納（弾の向きも自動で落下方向を向く）
        pEnemyShot->speed = sqrt(vx * vx + vy * vy);
        pEnemyShot->muki = atan2(vy, vx);

        // 仕様に則り、更新された speed と muki を使って座標を加算
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // --- 特殊処理：大玉（溶岩塊）の空中爆発 ---
        if (pEnemyShot->param_i[0] == 1) {
            pEnemyShot->param_i[1]--; // タイマー減少
            if (pEnemyShot->param_i[1] <= 0) {
                // 軽快な破裂音
                if (CheckSoundMem(sound_enemyShot_light) == 1) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                // 周囲6方向に火の粉（小玉）を撒き散らす
                int burstNum = 6;
                for (int j = 0; j < burstNum; j++) {
                    sEnemyShot* pNewShot = new sEnemyShot;
                    pNewShot->x = pEnemyShot->x;
                    pNewShot->y = pEnemyShot->y;

                    // 全方位全角度へ均等に拡散（わずかにランダム性を加える）
                    pNewShot->muki = ((double)j * (360.0 / burstNum) + GetRand(20)) / 180.0 * DX_PI;
                    pNewShot->speed = 1.0 + (GetRand(100) / 100.0); // 初速 1.0 〜 2.0

                    // 飛び散る火の粉は黄(1)か橙(8)
                    pNewShot->kind = img_enemyShotSmallBall[(GetRand(1) == 0) ? 1 : 8];
                    pNewShot->param_i[0] = 0; // 分裂後は通常の放物線弾になる

                    // リストの末尾（Headの直前）に挿入。
                    // 現在のwhileループの走査位置より後ろに追加されるため、ポインタが壊れず安全です。
                    pNewShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pNewShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pNewShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pNewShot;
                }

                // 親である大玉自身をリストから安全に除外して削除
                sEnemyShot* pNextTmp = pEnemyShot->next;
                pEnemyShot->prev->next = pEnemyShot->next;
                pEnemyShot->next->prev = pEnemyShot->prev;
                delete pEnemyShot;

                // 次の要素へポインタをスキップさせて continue
                pEnemyShot = pNextTmp;
                continue;
            }
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Volcano_Gemini()
{
    static int muki;

    // 初回フレームの初期化
    if (count == 1) {
        // ゲーム画面中央上部に配置
        enemy.x = 240.0;
        enemy.y = 170.0;
        enemy.maxHp = enemy.hp = 200; // ボスを想定したHP
        muki = 1;
    }
    else {
        // 火山の「地鳴り」を表現するために、左右に細かく振動しつつ緩やかに移動
        enemy.x += 0.4 * (double)muki + (GetRand(20) - 10) * 0.1;
        if (count % 120 == 0) muki *= -1;

        // 画面外にハミ出さないようにガード
        if (enemy.x < 60.0) { enemy.x = 60.0;  muki = 1; }
        if (enemy.x > 420.0) { enemy.x = 420.0; muki = -1; }
    }

    // 6フレーム周期（約0.1秒ごと）で細かく噴火の弾セットを生成し、密度の高い噴火を維持
    if (count % 12 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotVolcano;

        // 火口をイメージして、敵本体のやや上から弾を噴出させる
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y - 12.0;

        // 初期方向は念のためプレイヤー方向をセット（ShotVolcano側は真上基準で上書きします）
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 弾を管理する双方向リストのダミーヘッドを構築
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体の弾セットリスト（enemyShotSetHead）へ接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}