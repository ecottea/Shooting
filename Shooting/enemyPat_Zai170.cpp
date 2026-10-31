// ============================================================
//  転世「一条戻り橋」(Tensei "Ichijou Modoribashi") 再現
//  東方永夜抄 Extra面 中BOSS 上白沢慧音 スペルカード No.193
// ------------------------------------------------------------
//  【原作の構造】(Web調査に基づく)
//   1) 「完全な戻り弾幕」: 弾が画面端(版端)で生成され、逆向きに
//      慧音(ボス)のもとへ収束していく固定弾幕。全弾が必ず回収される
//      (ZUN符卡説明「完全な戻り弾幕はこうである。なんか生理的に避け難い」)。
//   2) 紫系の大玉を極低速で自機狙い発射 (攻略記録: 「固定弾幕+紫色大円
//      極低速自機狙い」)。開口部を流れるプレイヤーを押し動かす「時計」。
//   3) ウェーブごとに湧き位置が回転するため弾の流れの「開口部」が規則的
//      に回る → 原作の攻略法は「開口部を見て常にゆっくり左右へ流れる」。
//   4) 原作は完全固定弾幕+自機狙いなので乱数(GetRand)は不使用。
//      (GetRand(x) は 0〜x の x+1 種を返す点に注意。使う場合は
//       リプレイ用に seed 保存される仕様を崩さないこと)
// ------------------------------------------------------------
//  【仕様上の約束】
//   - count / pEnemyShotSet->count / pEnemyShot->count のインクリメント
//     と、画面外の弾の消去はメインルーチンが行う(本ファイルでは行わない)。
//     → 敵に到達した弾は自分で delete せず、座標を画面外へ退避させて
//       メインルーチンの「画面外弾の消去」に委譲する。
//     → pEnemyShotSet->count==0 は「生成直後の最初のフレーム」の
//       判定に利用できる(サンプル ShotScatter と同じ用法)。
//     → pEnemyShot->count も毎フレーム+1なので「生成からの経過
//       フレーム」として利用できる(回収誤爆防止に使用)。
// ------------------------------------------------------------
//  【素材の選定】(enemyPat_sampleForAI.cpp の素材一覧より)
//   使える弾: 小玉(2.5) 中玉(7.0) 大玉(20.0) 銃弾(5x2) 鱗弾(4x3)
//             菱形弾(4.5x2.5) 中楕円弾(10.5x7) 短レーザー(64x4)
//   使える色: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
//   使える音: sound_enemyShot_light / medium / heavy / extreme /
//             sound_enemyCharge(予告音)
//
//   ■ 戻り弾幕: 中楕円弾 img_enemyShotMediumOval[3] (シアン)
//       原作の版端から湧く淡色の楕円弾に近い。小玉・鱗弾では版端で
//       見分けにくく、銃弾・短レーザーは方向性が強すぎて「収束して
//       戻る」見た目が出ないため中楕円弾を採用。
//   ■ 自機狙い: 大玉 img_enemyShotLargeBall[5] (マゼンタ)
//       原作は「紫色の大玉」だが色パレットに紫が無いため最も近い
//       マゼンタで代用。極低速の目立つ弾が必要なので大玉一択。
//   ■ 効果音: sound_enemyShot_light  = ウェーブ生成(弾の湧き)
//             sound_enemyCharge      = 大玉発射の予告
//             sound_enemyShot_heavy  = 大玉発射
//   ■ 未使用: 小玉 / 中玉 / 銃弾 / 鱗弾 / 菱形弾 / 短レーザー / 他の色
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  戻り弾幕 (1ウェーブ = 1弾セット)
//  画面中心(240,240)・半径230の内接円上=版端のすぐ内側に12発を円形
//  配置し、セット生成時に決めた敵の現位置(収束点)へ一直線に戻る。
//  ウェーブ番号(kind)ごとに基準角が7.5度ずつ回転する(12発の隣接間隔は
//  30度なので4ウェーブで湧き位置が一周) → 開口部が規則的に回る。
// ------------------------------------------------------------
static void ShotReturnBridge(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 基準角: ウェーブ番号 × 7.5度。原作どおり固定弾幕なので乱数なし。
        double baseAng = pEnemyShotSet->kind * (DX_PI / 24.0);
        for (int i = 0; i < 12; i++) {
            double a = baseAng + DX_PI * 2.0 * (double)i / 12.0;
            // 版端(画面の縁のすぐ内側)に生成。全点が画面内(10..470)なので
            // 生成直後にメインルーチンの画面外消去で消されることはない。
            double sx = 240.0 + 230.0 * cos(a)*1.5;
            double sy = 240.0 + 230.0 * sin(a)*1.5;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = sx;
            pEnemyShot->y = sy;
            // 生成位置 → 収束点(=セット生成時の敵位置)への直進。
            // 途中で追尾しないのが原作どおり(完全固定弾幕)。
            pEnemyShot->muki = atan2(pEnemyShotSet->y - sy, pEnemyShotSet->x - sx);
            pEnemyShot->speed = 1.6;
            pEnemyShot->kind = img_enemyShotMediumOval[3]; // 中楕円弾・シアン
            pEnemyShot->margin = 480;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 敵(収束点)に十分近づいたら回収完了。消去は自分では行わず、
        // 座標を画面外へ退避させてメインルーチンの「画面外弾の消去」に
        // 委ねる。全弾が必ず敵まで戻る=「完全な戻り弾幕」の再現。
        // (pShot->count > 10: 生成直後の近距離誤回収を防ぐ経過フレーム条件)
        double dx = pShot->x - pEnemyShotSet->x;
        double dy = pShot->y - pEnemyShotSet->y;
        if (pShot->count > 10 && dx * dx + dy * dy < 400.0) { // 半径20px以内
            pShot->x = -999.0;
            pShot->y = -999.0;
            pShot->speed = 0.0;
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  極低速自機狙い大玉 (1発 = 1弾セット)
//  原作: 「紫色大円・極低速自機狙い」。等速直進のみで追尾なし。
// ------------------------------------------------------------
static void ShotSlowAimedLarge(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;       // 自機狙い角(敵本体側で計算済み)
        pEnemyShot->speed = 0.8;                       // 極低速
        pEnemyShot->kind = img_enemyShotLargeBall[5]; // 大玉・マゼンタ(紫の代用)
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  敵本体のパターン (転世「一条戻り橋」)
// ------------------------------------------------------------
void EnemyPat_Modoribashi_Zai()
{
    static int muki;
    static int wave_count;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0+100;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        wave_count = 0;
    }
    else {
        // 慧音は2符中、画面上段をゆっくり漂う。収束点はウェーブ生成時の
        // 敵位置なので、敵の漂いに合わせてリングの中心もずれていく。
        enemy.x += 0.75 * (double)muki*1.5;
        if (count % 150 == 75) muki *= -1;
    }
    // ① 戻り弾幕: 2秒後に開始、以降 0.7秒ごとに1ウェーブ(12発)
    if (count % 14 == 0 && count % 120 <= 105) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotReturnBridge;
        pEnemyShotSet->x = enemy.x;         // 収束点 = その瞬間の敵位置
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;          // 未使用
        pEnemyShotSet->kind = wave_count++; // ウェーブ番号 → 湧き位置の回転に使用
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    // ② 極低速自機狙い大玉: 予告音 → 2秒後に発射 (4秒周期)
    if (count % 240 == 60) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    if (count % 240 == 180) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSlowAimedLarge;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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
