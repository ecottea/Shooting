#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：プリズム・レインボウ・スイープ
static void ShotPrismRainbowSweep(sEnemyShotSet* pEnemyShotSet)
{
    // param_i[0]: 状態管理 (0:初期, 1:プリズム配置済, 2:入射光発射済)
    // param_d[0]: 現在の分光発射の基準角度
    // param_d[1]: スイープ用の時間経過パラメータ

    if (pEnemyShotSet->count == 0) {
        // 【フェーズ1：プリズムの展開】
        // 予告音を鳴らす
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // プリズム本体の生成（白色の中楕円弾を「透明なプリズム」に見立てる）
        sEnemyShot* pPrism = new sEnemyShot;
        pPrism->x = pEnemyShotSet->x;
        pPrism->y = pEnemyShotSet->y;
        pPrism->muki = DX_PI / 2.0; // 下向き
        pPrism->speed = 0.0;        // 固定
        pPrism->kind = img_enemyShotMediumOval[6]; // 6:白
        pPrism->param_i[0] = 1;     // プリズム識別フラグ

        pPrism->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pPrism->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pPrism;
        pEnemyShotSet->pEnemyShotHead->prev = pPrism;

        pEnemyShotSet->param_i[0] = 1;
        pEnemyShotSet->param_d[0] = DX_PI / 2.0; // 初期基準角度（真下）
        pEnemyShotSet->param_d[1] = 0.0;         // スイープ時間初期化
    }
    else if (pEnemyShotSet->count == 60) {
        // 【フェーズ1継続：入射光の発射】
        // 画面左端からプリズムに向かって飛ぶ白い弾列（光線）を生成
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pIncident = new sEnemyShot;
            pIncident->x = 0.0;
            pIncident->y = pEnemyShotSet->y + (GetRand(20) - 10); // わずかな揺らぎ
            double angle = atan2(pEnemyShotSet->y - pIncident->y, pEnemyShotSet->x - pIncident->x);
            pIncident->muki = angle;
            pIncident->speed = 6.0;
            pIncident->kind = img_enemyShotMediumBall[6]; // 6:白
            pIncident->param_i[0] = 2; // 入射光識別フラグ

            pIncident->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pIncident->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pIncident;
            pEnemyShotSet->pEnemyShotHead->prev = pIncident;
        }
    }
    else if (pEnemyShotSet->count == 90) {
        // 【フェーズ2：分光（ファン展開）】
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 7色の定義: 0:赤, 8:橙, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ
        int colors[7] = { 0, 8, 1, 2, 3, 4, 5 };
        double base_angle = pEnemyShotSet->param_d[0];

        for (int i = 0; i < 7; i++) {
            sEnemyShot* pRefracted = new sEnemyShot;
            pRefracted->x = pEnemyShotSet->x;
            pRefracted->y = pEnemyShotSet->y;

            // 扇状に広げる：-30度(-PI/6) から +30度(+PI/6) を6等分
            double angle_offset = (i - 3) * (DX_PI / 9.0);
            pRefracted->muki = base_angle + angle_offset;

            // 【ギミック：色ごとの速度差】
            // 屈折率の違いを再現し、紫(マゼンタ)は速く、赤は遅くする
            pRefracted->speed = 2.0 + (i / 6.0) * 2.5;

            // 鱗弾を使用（光の粒や輝きを表現するのに最適）
            pRefracted->kind = img_enemyShotScale[colors[i]];
            pRefracted->param_i[0] = 3; // 分光弾識別フラグ

            pRefracted->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRefracted->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRefracted;
            pEnemyShotSet->pEnemyShotHead->prev = pRefracted;
        }

        // 【フェーズ3：スイープ準備】
        // 次回発射のために基準角度を変化させる（sin波で左右に揺らす）
        pEnemyShotSet->param_d[1] += 0.15;
        pEnemyShotSet->param_d[0] = (DX_PI / 2.0) + sin(pEnemyShotSet->param_d[1]) * (DX_PI / 4.0);
    }

    // 既存の弾の移動処理
    // ※ countのインクリメントと画面外消去はメインルーチンで行われる仕様
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // プリズム本体(param_i[0]==1)は移動しない
        if (pShot->param_i[0] != 1) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    // 【フェーズ3継続：スイープ（薙ぎ払い）】
    // 初回発射(count==90)以降、30フレームごとに分光発射を繰り返し、角度をずらす
    if (pEnemyShotSet->count >= 90 && pEnemyShotSet->count % 30 == 0 && pEnemyShotSet->count < 300*3) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int colors[7] = { 0, 8, 1, 2, 3, 4, 5 };
        double base_angle = pEnemyShotSet->param_d[0];

        for (int i = 0; i < 7; i++) {
            sEnemyShot* pRefracted = new sEnemyShot;
            pRefracted->x = pEnemyShotSet->x;
            pRefracted->y = pEnemyShotSet->y;

            double angle_offset = (i - 3) * (DX_PI / 9.0);
            pRefracted->muki = base_angle + angle_offset;
            pRefracted->speed = 2.0 + (i / 6.0) * 2.5;
            pRefracted->kind = img_enemyShotScale[colors[i]];
            pRefracted->param_i[0] = 3;

            pRefracted->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pRefracted->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pRefracted;
            pEnemyShotSet->pEnemyShotHead->prev = pRefracted;
        }

        // 次のスイープ角度を更新
        pEnemyShotSet->param_d[1] += 0.15;
        pEnemyShotSet->param_d[0] = (DX_PI / 2.0) + sin(pEnemyShotSet->param_d[1]) * (DX_PI / 4.0);
    }
}

// 敵本体のパターン
void EnemyPat_Prism_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0; // プリズムが画面内に収まりやすい位置
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右への緩やかな移動
        enemy.x += 0.8 * (double)muki;

        // 画面端付近で折り返し
        if (enemy.x < 120.0 || enemy.x > 360.0) {
            muki *= -1;
        }
    }

    // 120フレーム(約2秒)ごとにプリズム弾幕セットを生成
    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPrismRainbowSweep;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = shot_count++;

        // ヘッダーノードの初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに連結
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}