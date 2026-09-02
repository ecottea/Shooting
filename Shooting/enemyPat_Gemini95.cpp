#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ====================================================================
// 補助弾幕関数群
// ====================================================================

// HP回復時に「NO」の文字を形成した弾が拡散する動き
static void NoSignPattern(sEnemyShotSet* pSet) {
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pSet->count < 60) {
            // 最初の1秒間はゆっくりと文字の形を保ちながら広がる
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pSet->count == 60) {
            // 1秒経過で怒りのように一気に弾け飛ぶ
            pShot->speed = (200 + GetRand(300)) / 100.0;
        }
        else {
            // 弾け飛んだ後の移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 撃破拒否「NO」文字の生成関数
static void GenerateNoSign() {
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = NoSignPattern;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    // 「N」の文字 (5x5 グリッドベース)
    int nx[13] = { 0,0,0,0,0, 1,2,3, 4,4,4,4,4 };
    int ny[13] = { 0,1,2,3,4, 1,2,3, 0,1,2,3,4 };
    for (int i = 0; i < 13; i++) {
        sEnemyShot* pE = new sEnemyShot;
        pE->x = 180 + nx[i] * 15;
        pE->y = enemy.y + 40 + ny[i] * 15;
        pE->muki = atan2(pE->y - enemy.y, pE->x - enemy.x); // 敵の中心から放射状に
        pE->speed = 0.5; // 初速は遅め
        pE->kind = img_enemyShotLargeBall[0]; // 警告を表す赤大玉
        pE->margin = 480;

        pE->prev = pSet->pEnemyShotHead->prev;
        pE->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pE;
        pSet->pEnemyShotHead->prev = pE;
    }

    // 「O」の文字 (5x5 グリッドベース)
    int ox[12] = { 0,0,0, 1,2,3, 1,2,3, 4,4,4 };
    int oy[12] = { 1,2,3, 0,0,0, 4,4,4, 1,2,3 };
    for (int i = 0; i < 12; i++) {
        sEnemyShot* pE = new sEnemyShot;
        pE->x = 280 + ox[i] * 15;
        pE->y = enemy.y + 40 + oy[i] * 15;
        pE->muki = atan2(pE->y - enemy.y, pE->x - enemy.x);
        pE->speed = 0.5;
        pE->kind = img_enemyShotLargeBall[0]; // 赤大玉
        pE->margin = 480;

        pE->prev = pSet->pEnemyShotHead->prev;
        pE->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pE;
        pSet->pEnemyShotHead->prev = pE;
    }

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// 規則的に降り注ぐ「グリッド弾幕」 (ワープの罠用)
static void GridFallPattern(sEnemyShotSet* pSet) {
    if (pSet->count % 50 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // x座標 30, 90, 150, 210, 270, 330, 390, 450 の8列に弾を降らせる
        for (int i = 0; i < 8; i++) {
            sEnemyShot* pE = new sEnemyShot;
            pE->x = 30.0 + i * 60.0;
            pE->y = -20.0;
            pE->muki = DX_PI / 2.0; // 真下
            pE->speed = 2.0;
            pE->kind = img_enemyShotDiamond[7]; // 無機質な黒い菱形弾
            pE->margin = 120;

            pE->prev = pSet->pEnemyShotHead->prev;
            pE->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pE;
            pSet->pEnemyShotHead->prev = pE;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ====================================================================
// メインの反則ギミック群 (自機ショット強奪 ＆ 自機テレポート)
// ====================================================================
static void RuleBreakPattern(sEnemyShotSet* pSet) {

    // 【反則1】自機ショットの不正徴収（自機弾を奪って敵弾にする）
    sPlayerShot* pPShot = playerShotHead.next;
    while (pPShot != &playerShotHead) {
        // 自機から少し離れた（撃った直後の）弾を問答無用で没収
        if (pPShot->y < 380.0) {

            // 敵弾オブジェクトとして生成して自機へ向けて逆流させる
            sEnemyShot* pE = new sEnemyShot;
            pE->x = pPShot->x;
            pE->y = pPShot->y;
            pE->muki = atan2(player.y - pPShot->y, player.x - pPShot->x); // プレイヤーを狙う
            pE->speed = 3.5;
            pE->kind = img_enemyShotBullet[0]; // 奪われた禍々しさを示す赤い銃弾

            // このセットの弾リストに登録
            pE->prev = pSet->pEnemyShotHead->prev;
            pE->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pE;
            pSet->pEnemyShotHead->prev = pE;

            // 自機ショットオブジェクトをリストから外して消去
            sPlayerShot* pDel = pPShot;
            pPShot = pPShot->next; // 次のノードへ進めておく
            pDel->prev->next = pDel->next;
            pDel->next->prev = pDel->prev;
            delete pDel;
        }
        else {
            pPShot = pPShot->next;
        }
    }

    // 【反則2】自機座標の強制書き換え（テレポート攻撃）
    // 4秒(240フレーム)に1回の周期で発動
    if (pSet->count % 240 == 180) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 予兆音
    }
    if (pSet->count % 240 == 239) {
        // グリッド弾幕の降ってくるX座標（危険地帯）の真上にプレイヤーを強制ワープ
        // GetRand(7) は 0～7 の8通りなので、レーンと完全に一致する
        int lane = GetRand(7);
        player.x = 30.0 + lane * 60.0;
        player.y = 280.0 + GetRand(80); // 画面下半分の適当な高さへ

        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    // 奪った逆流弾の移動処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ====================================================================
// 敵本体のパターン
// ====================================================================
void EnemyPat_Violate_Gemini()
{
    static bool isTimeUp = false;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200; // 削りきれるかどうかの絶妙な数値
        isTimeUp = false;

        // 反則システム管理セットの登録
        sEnemyShotSet* pHackSet = new sEnemyShotSet;
        pHackSet->count = 0;
        pHackSet->patternFunc = RuleBreakPattern;
        pHackSet->pEnemyShotHead = new sEnemyShot;
        pHackSet->pEnemyShotHead->prev = pHackSet->pEnemyShotHead;
        pHackSet->pEnemyShotHead->next = pHackSet->pEnemyShotHead;
        pHackSet->alive = 9999;

        pHackSet->prev = enemyShotSetHead.prev;
        pHackSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pHackSet;
        enemyShotSetHead.prev = pHackSet;

        // グリッド弾幕セットの登録
        sEnemyShotSet* pGridSet = new sEnemyShotSet;
        pGridSet->count = 0;
        pGridSet->patternFunc = GridFallPattern;
        pGridSet->pEnemyShotHead = new sEnemyShot;
        pGridSet->pEnemyShotHead->prev = pGridSet->pEnemyShotHead;
        pGridSet->pEnemyShotHead->next = pGridSet->pEnemyShotHead;
        pGridSet->alive = 9999;

        pGridSet->prev = enemyShotSetHead.prev;
        pGridSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pGridSet;
        enemyShotSetHead.prev = pGridSet;
    }

    // 30秒(1800フレーム) 耐え切ったら裏ルールにより強制クリア（自滅）
    if (count >= 1800) {
        isTimeUp = true;
        enemy.hp = 0;
    }

    // 敵本体の移動（中央付近を不気味にゆっくり揺れるだけ）
    if (!isTimeUp) {
        enemy.x = 240.0 + 30.0 * sin(count * 2.0 * DX_PI / 180.0);
    }

    // 【反則3】HP全回復チート（撃破拒否）
    // HPが10%以下になると「NO」の文字と共に完全回復。
    // （※isTimeUp時はクリアさせるため無効にする）
    if (!isTimeUp && enemy.hp < enemy.maxHp * 0.1) {
        enemy.hp = enemy.maxHp; // 強制上書き

        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK); // 絶望感のある重い音
        GenerateNoSign(); // 巨大な「NO」弾幕を展開
    }
}