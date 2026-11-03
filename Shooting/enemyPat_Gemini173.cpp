#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：五線譜のプレリュード (Staff Prelude)
static void ShotStaffPrelude(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int cnt = pEnemyShotSet->count;

    if (cnt == 0) {
        // 効果音の再生（音符の出現に合わせて中量級のショット音）
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // kind == 0: 五線譜の5本線（水平な短レーザー群）を形成
        // kind > 0: 楽譜上を流れて中央で弾を拡散させる音符（ノート）
        if (pEnemyShotSet->kind == 0) {
            // 画面中央付近に上下に等間隔（24ピクセル刻み）で5本の平行線を配置
            for (int i = 0; i < 5; i++) {
                // 各ラインを複数の短レーザーで横一文字につなぐように配置
                for (int j = 0; j < 6; j++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = 40.0 + j * 80.0;
                    pEnemyShot->y = 140.0 + (double)(i * 24.0);
                    pEnemyShot->muki = 0.0;   // 水平方向（右向き）
                    pEnemyShot->speed = 0.0;  // ライン自体は静止
                    // 白色の短レーザーを使用
                    pEnemyShot->kind = img_enemyShotLaser[6];
                    pEnemyShot->param_i[0] = i; // ライン番号を保存

                    // リストへの登録
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
        else {
            // 音符（ノート）の生成：左端から流れてくる符頭（中玉）と符幹（銃弾）
            // param_i[0] に割り当てる五線譜のライン番号（0〜4）を格納
            int lineIdx = pEnemyShotSet->param_i[0];

            // 1. 符頭（中玉・黄色）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 20.0;
            pEnemyShot->y = 140.0 + (double)(lineIdx * 24.0);
            pEnemyShot->muki = 0.0;  // 右へスライド移動
            pEnemyShot->speed = 2.0;
            pEnemyShot->kind = img_enemyShotMediumBall[1]; // 黄色の中玉
            pEnemyShot->param_i[1] = 0; // 状態フラグ (0:移動中, 1:拡散済み)

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 2. 符幹（短い棒を表現する銃弾を符頭の上下に添える）
            for (int k = 0; k < 2; k++) {
                sEnemyShot* pStem = new sEnemyShot;
                pStem->x = 20.0 + (k == 0 ? -4.0 : 4.0);
                pStem->y = 140.0 + (double)(lineIdx * 24.0) - 8.0;
                pStem->muki = 0.0;
                pStem->speed = 2.0;
                pStem->kind = img_enemyShotBullet[6]; // 白の銃弾
                pStem->param_i[1] = 0;

                pStem->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pStem->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pStem;
                pEnemyShotSet->pEnemyShotHead->prev = pStem;
            }
        }
    }

    // 弾の更新ループ
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShotSet->kind == 0) {
            // 五線譜のラインを少し上下に波打たせて生きているような演出にする
            int lineId = pShot->param_i[0];
            // 初期Y座標を基準にサイン波で揺らす
            // ※ count はメインルーチンでインクリメントされるグローバル変数
            double baseY = 140.0 + (double)(lineId * 24.0);
            pShot->y = baseY + sin(count * 0.03 + lineId * 0.5) * 3.0;
        }
        else {
            // 音符の移動と演奏（中央付近での拡散ギミック）
            if (pShot->param_i[1] == 0) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);

                // 画面中央（X = 240付近）に到達した瞬間、「音符が演奏（発音）」して全方位へ弾を拡散
                if (pShot->x >= 240.0 && pShot->kind == img_enemyShotMediumBall[1]) {
                    pShot->param_i[1] = 1;

                    // 高音の軽快な効果音
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                    // 符頭の位置からシアン色の小玉を全方位（8方向）にパッとパルス状に散らす
                    double base_angle = GetRand(99999);
                    for (int k = 0; k < 8*10; k++) {
                        sEnemyShot* pSubShot = new sEnemyShot;
                        pSubShot->x = pShot->x;
                        pSubShot->y = pShot->y;
                        pSubShot->muki = base_angle + k * (DX_PI / 4.0/10);
                        pSubShot->speed = 2.2 + k%8*0.3;
                        pSubShot->kind = img_enemyShotSmallBall[3]; // シアンの小玉

                        // リスト末尾に安全に追加
                        pSubShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pSubShot->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pSubShot;
                        pEnemyShotSet->pEnemyShotHead->prev = pSubShot;
                    }

                    // 符頭の音符弾自体はここで消去
                    sEnemyShot* pTarget = pShot;
                    pShot = pShot->next;

                    pTarget->prev->next = pTarget->next;
                    pTarget->next->prev = pTarget->prev;
                    delete pTarget;
                    continue;
                }
            }
            else {
                // 拡散後の小玉の移動
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_StaffNotation_Gemini()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;

        // 戦闘開始時に一度だけ「五線譜のライン（kind = 0）」を生成して固定展開する
        sEnemyShotSet* pStaffSet = new sEnemyShotSet;
        pStaffSet->count = 0;
        pStaffSet->patternFunc = ShotStaffPrelude;
        pStaffSet->x = enemy.x;
        pStaffSet->y = enemy.y;
        pStaffSet->muki = 0.0;
        pStaffSet->kind = 0; // 0番は五線譜ライン構築用
        pStaffSet->pEnemyShotHead = new sEnemyShot;
        pStaffSet->pEnemyShotHead->prev = pStaffSet->pEnemyShotHead;
        pStaffSet->pEnemyShotHead->next = pStaffSet->pEnemyShotHead;
        pStaffSet->prev = enemyShotSetHead.prev;
        pStaffSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pStaffSet;
        enemyShotSetHead.prev = pStaffSet;
    }
    else {
        // ボス本体の左右移動
        enemy.x += 0.8 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 一定間隔（例: 70フレームごと）に、五線譜のどれかのラインに沿って「音符」を流れ込ませる
    if (count > 60 && count % 25 == 0) {
        sEnemyShotSet* pNoteSet = new sEnemyShotSet;
        pNoteSet->count = 0;
        pNoteSet->patternFunc = ShotStaffPrelude;
        pNoteSet->x = enemy.x;
        pNoteSet->y = enemy.y;
        pNoteSet->muki = 0.0;

        // 0〜4のランダムな五線譜のラインを選択（GetRand(4) は 0〜4 の5種類の整数を返す）
        pNoteSet->kind = 1;
        pNoteSet->param_i[0] = GetRand(4);

        pNoteSet->pEnemyShotHead = new sEnemyShot;
        pNoteSet->pEnemyShotHead->prev = pNoteSet->pEnemyShotHead;
        pNoteSet->pEnemyShotHead->next = pNoteSet->pEnemyShotHead;
        pNoteSet->prev = enemyShotSetHead.prev;
        pNoteSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pNoteSet;
        enemyShotSetHead.prev = pNoteSet;
    }
}