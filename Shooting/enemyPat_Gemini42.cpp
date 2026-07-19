// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================================
// 【選定した素材の一覧と採用理由】
//
// 1. 効果音 (SE)
//  - sound_enemyCharge        : サイクル開始時、ピラミッド構築の予兆（エネルギー充填音）として採用。
//  - sound_enemyShot_medium   : 斜辺（稜線）を描きながら進む「親弾」の発射音として採用。
//  - sound_enemyShot_extreme  : 頂点から放たれる「黄金の夜明け（全方位リング弾）」の破裂音として採用。
//
// 2. 弾の種類と色
//  - img_enemyShotBullet[6] (白の銃弾) : ピラミッドのシャープな外枠（稜線）を高速で描く親弾として採用。
//  - img_enemyShotDiamond[1] (黄の菱形弾) / [8] (橙の菱形弾) : 
//    親弾の軌跡から内側へ滑り込み、ピラミッドの「黄金の石壁（ブロック）」を形成する子弾として採用。階層ごとに交互に色を変えて美しく魅せます。
//  - img_enemyShotLargeBall[1] (黄の大玉) / img_enemyShotMediumBall[8] (橙の中玉) : 
//    「黄金の夜明け」を表現するため、輝かしい2色のリングが重なりながら追い抜いていく複合全方位弾幕として採用。
// ============================================================================

// 弾幕パターン：ピラミッドモチーフ『ファラオ・コア：黄金の階層構築』
static void ShotPyramid(sEnemyShotSet* pEnemyShotSet)
{
    // 常に最新のボスの座標に同期（ボスが微細に揺れてもピラミッドの頂点がズレないようにする）
    //pEnemyShotSet->x = enemy.x;
    //pEnemyShotSet->y = enemy.y;

    int c = pEnemyShotSet->count;

    // ------------------------------------------------------------------------
    // 【1】 タイムラインに基づく新規弾の生成処理 (pEnemyShotSet->count による制御)
    // ------------------------------------------------------------------------

    // [0フレーム目] 階層構築の予兆 (予告音)
    if (c == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // [20〜140フレームの間] 15フレーム間隔でピラミッドの稜線を描く親弾を左右交互（同時）に発射 (計9回)
    if (c >= 20 && c <= 140 && (c - 20) % 15 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 左下方向への親弾 (角度: 130度付近 = DX_PI * 0.72)
        sEnemyShot* pLeft = new sEnemyShot;
        pLeft->x = pEnemyShotSet->x;
        pLeft->y = pEnemyShotSet->y;
        pLeft->muki = DX_PI * 0.72;
        pLeft->speed = 3.2;
        pLeft->kind = img_enemyShotBullet[6]; // 白い銃弾（光の線の目印）
        pLeft->param_i[0] = 1;                // 1: 左親弾の識別フラグ
        pLeft->count = 0;
        pLeft->margin = 100;

        pLeft->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pLeft->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pLeft;
        pEnemyShotSet->pEnemyShotHead->prev = pLeft;

        // 右下方向への親弾 (角度: 50度付近 = DX_PI * 0.28)
        sEnemyShot* pRight = new sEnemyShot;
        pRight->x = pEnemyShotSet->x;
        pRight->y = pEnemyShotSet->y + 10;
        pRight->muki = DX_PI * 0.28;
        pRight->speed = 3.2;
        pRight->kind = img_enemyShotBullet[6]; // 白い銃弾
        pRight->param_i[0] = 2;                // 2: 右親弾の識別フラグ
        pRight->count = 0;
        pRight->margin = 100;

        pRight->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pRight->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pRight;
        pEnemyShotSet->pEnemyShotHead->prev = pRight;
    }

    // [180フレーム目] 「黄金の夜明け」 (ピラミッド完成直後の高密度全方位複合リング)
    if (c == 155) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        double base_angle = GetRand(359) / 360.0 * 2 * DX_PI;
        int way = 36; // 36方向
        for (int i = 0; i < way; i++) {
            double angle = (2.0 * DX_PI / way) * i;

            // 黄金の大玉 (黄 = 1)
            sEnemyShot* pLarge = new sEnemyShot;
            pLarge->x = pEnemyShotSet->x;
            pLarge->y = pEnemyShotSet->y;
            pLarge->muki = base_angle + angle;
            pLarge->speed = 1.6;
            pLarge->kind = img_enemyShotLargeBall[1];
            pLarge->param_i[0] = 0;
            pLarge->count = 0;
            pLarge->margin = 100;

            pLarge->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pLarge->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pLarge;
            pEnemyShotSet->pEnemyShotHead->prev = pLarge;

            // 黄金の中玉 (橙 = 8) : 少し速度を速くし、大玉の隙間を埋めるように半位相(DX_PI / way)ずらす
            sEnemyShot* pMedium = new sEnemyShot;
            pMedium->x = pEnemyShotSet->x;
            pMedium->y = pEnemyShotSet->y;
            pMedium->muki = base_angle + angle + (DX_PI / way);
            pMedium->speed = 2.2;
            pMedium->kind = img_enemyShotMediumBall[8];
            pMedium->param_i[0] = 0;
            pMedium->count = 0;
            pMedium->margin = 100;

            pMedium->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pMedium->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pMedium;
            pEnemyShotSet->pEnemyShotHead->prev = pMedium;
        }
    }

    // ------------------------------------------------------------------------
    // 【2】 既存の弾の移動処理 ＆ 親弾からのブロック弾（子弾）配置処理
    // ------------------------------------------------------------------------
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    sEnemyShot* pOldLast = pEnemyShotSet->pEnemyShotHead->prev; // ループ開始時の「最後の弾」を記憶しておく

    if (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        while (true) {
            // ループ内でリストの末尾にブロック弾が動的追加されても安全に進めるよう、次のポインタを先取り保持
            sEnemyShot* pNext = pEnemyShot->next;

            // 稜線を描く親弾（識別フラグ1または2）の特殊挙動
            if (pEnemyShot->param_i[0] == 1 || pEnemyShot->param_i[0] == 2) {
                // 親弾が生まれてから 8フレームごとに、その場にブロック弾（子弾）をポロポロと残していく
                if (pEnemyShot->count > 0 && pEnemyShot->count % 8 == 0) {
                    sEnemyShot* pBlock = new sEnemyShot;
                    pBlock->x = pEnemyShot->x;
                    pBlock->y = pEnemyShot->y;
                    pBlock->speed = 0.9;    // 低速で重厚に滑るように移動
                    pBlock->count = 0;
                    pBlock->param_i[0] = 0; // 生成されたブロック弾は特殊処理を行わない通常の子弾

                    // 生成タイミングによって黄(1)と橙(8)のレンガを交互に配置し、綺麗な縞模様の階層にする
                    int colorIdx = ((pEnemyShot->count / 8) % 2 == 0) ? 1 : 8;
                    pBlock->kind = img_enemyShotDiamond[colorIdx];

                    if (pEnemyShot->param_i[0] == 1) {
                        pBlock->muki = 0.0;    // 左斜辺の親弾からは「真右（内側）」へスライド
                    }
                    else {
                        pBlock->muki = DX_PI;  // 右斜辺の親弾からは「真左（内側）」へスライド
                    }

                    pBlock->margin = 100;

                    // リストの末尾（pEnemyShotHead->prev）に子弾を追加
                    pBlock->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pBlock->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pBlock;
                    pEnemyShotSet->pEnemyShotHead->prev = pBlock;
                }
            }

            // 全ての弾の毎フレーム共通の移動処理（※インクリメントはメインルーチンで行われます）
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

            // ループ開始時点での末尾の弾を処理し終えたらループを抜ける
            // （これにより、このフレーム内で新しく末尾に追加された子弾をこのフレーム内で二重移動させるのを防ぐ）
            if (pEnemyShot == pOldLast) {
                break;
            }
            pEnemyShot = pNext;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Pyramid_Gemini()
{
    static int shot_count;

    if (count == 1) {
        // 初期化処理（ゲーム画面サイズは 480x480）
        enemy.x = 240.0;
        enemy.y = 70.0;               // ピラミッドの頂点として機能させるため、少し高めの位置
        enemy.maxHp = enemy.hp = 200; // HPは200で固定
        shot_count = 0;
    }
    else {
        // ピラミッドの「堅牢・不変」のイメージに合わせ、ボスは中央上部から大きくは動きません。
        // ただし、完全に静止していると不自然なため、厳かに優しく左右に揺らします（sin波）
        enemy.x = 240.0 + sin(count / 60.0) * 8.0;
        enemy.y = 60.0;
    }

    // 1サイクルを300フレーム（60fps換算で5秒間）として管理。
    // countは1から始まるため、各サイクル開始直後（count % 300 == 2）に1つの弾幕セットを生成します。
    // ※仕様上、enemyShotSetHeadに繋がったsEnemyShotSetのcountやpatternFuncはメインルーチンで
    // 自動更新・自動クリーンアップされるため、300フレームに1回だけ大元をnewして繋げばタイムラインが正しく機能します。
    if (count % 220 == 2) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPyramid;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + GetRand(10);
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        // 弾リストのヘッドノード初期化（双方向循環リスト）
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルの弾幕セットリスト（enemyShotSetHead）の末尾にリンク
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}