//==========================================================================
//  enemyPat_tmp.cpp
//
//  「ステージ道中」風の高難度弾幕
//  ボスは自機ショットの届かない画面外上部(y = -80.0)に隠れており、
//  大玉のザコ隊列を次々と送り込んでくる。ザコ(大玉)を撃破することでのみ
//  ボスにダメージが通る。
//
//  [動作仕様]
//   ・ボスは自機ショットの届かない画面外上部 enemy.y = -80.0 に配置する。
//   ・大玉をザコ敵代わりに使用し、param_i[15] をHP代わりにする(最大6)。
//   ・自機ショットが大玉に当たるとHPを減らし、残HPに応じた色に変わる。
//       6:赤 → 5:橙 → 4:黄 → 3:緑 → 2:シアン → 1:青 (被弾直後は一瞬白くフラッシュ)
//   ・HPが0になった大玉は消滅し、ボスに1ダメージ入る。
//   ・大玉と自機ショットの当たり判定はフレームワークに用意されていないため
//     ZakoDamageCheck() で自前実装する(当たった自機ショットは消滅する)。
//   ・ザコが撃つ弾は大玉以外(中玉/銃弾/菱形弾/鱗弾/中楕円弾)を使用。
//   ・ザコは3種類の隊列で画面外から飛来する(ローテーション+時間経過で難易度上昇)。
//       0:V字編隊  上方からV字で突入 → 陣形を組んで狙い撃ち → 加速して離脱
//       1:蛇行縦隊 横方向から縦隊で飛来 → サインカーブ上を隊列のまま通過
//       2:両翼包囲 左右から同時突入 → 画面両端に展開し狙い弾と環状弾で圧迫
//   ・ザコとして用いる大玉の margin は 240.0(画面外からの進入・待機を許すため)。
//
//  [素材の選定]
//   ・弾種: 大玉=ザコ本体 / 中玉=3-way狙い弾 / 銃弾=高速狙撃弾 / 菱形弾=蛇行隊の2-way弾 /
//           鱗弾=環状弾と撃破時の散弾 / 中楕円弾=包囲隊の狙い弾。
//           小玉と短レーザーは視認性・判定の煩雑さを考慮して不採用。
//   ・色:   弾は役割ごとに色を固定して見分けやすくする
//           (中玉=マゼンタ / 銃弾=白 / 菱形弾=黄 / 鱗弾=シアンと橙 / 中楕円弾=橙)。
//   ・効果音: sound_enemyCharge=隊列来襲の予告 / sound_enemyShot_medium=V字隊の射撃 /
//            sound_enemyShot_light=蛇行隊の射撃 / sound_enemyShot_heavy=包囲隊の射撃と撃破 /
//            sound_enemyShot_extreme=高速狙撃弾。
//
//  [ザコ(大玉)のパラメータ使用表]
//       param_i[15] : HP(仕様)
//       param_i[14] : 被弾フラッシュの残りフレーム
//       param_i[13] : 隊列スロット番号
//       param_i[12] : 状態 (0:移入中 1:展開・攻撃 2:離脱)
//       param_i[10] : サブ識別子(包囲隊の左右など)
//       param_i[9]  : ザコフラグ (1=ザコ本体 / 0=通常弾)
//       param_d[0]～param_d[2] : 隊列ごとの基準座標・方向などの保持
//       param_d[3]  : 射撃タイマー
//       param_d[4]  : 特殊射撃(狙撃/環状弾)タイマー
//       param_d[5]  : 離脱時の加速量
//
//  ※ count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//    画面外の弾の消去はメインルーチン側で行われる前提で実装している。
//  ※ GetRand(x) は 0～x の x+1 種類の整数を返す。全ての使用箇所はその前提で記述。
//==========================================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const int    ZAKO_HP_MAX = 6;      // ザコ(大玉)のHP
static const double ZAKO_HIT_RANGE = 14.0;   // 大玉(半径10)+自機ショット(半径4想定)の当たり距離
static const double ZAKO_HIT_RANGE_SQ = ZAKO_HIT_RANGE * ZAKO_HIT_RANGE;
static const int    ZAKO_FLASH_FRAMES = 4;      // 被弾時の白フラッシュ長(フレーム)

// 残HP → 大玉の色インデックス (0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙)
// HP6=赤 → 5=橙 → 4=黄 → 3=緑 → 2=シアン → 1=青 と段階的に変化する
static int ZakoColorForHp(int hp)
{
    static const int tbl[ZAKO_HP_MAX] = { 4, 3, 2, 1, 8, 0 }; // 添字は hp-1
    if (hp < 1) hp = 1;
    if (hp > ZAKO_HP_MAX) hp = ZAKO_HP_MAX;
    return tbl[hp - 1];
}

// 効果音再生(鳴っていたら止めてから鳴らす: sampleと同じ手法)
static void PlayEnemySound(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

// 難易度上昇率(0.0～1.0)を得る。countが進むほど1に近づく(約2分で最大)
static double GetRamp()
{
    double r = count / 7200.0;
    if (r > 1.0) r = 1.0;
    if (r < 0.0) r = 0.0;
    return r;
}

// ============================================================
//  生成・共通処理
// ============================================================

// 隊列管理用のShotSetを1つ作ってリストに繋ぐ
static sEnemyShotSet* CreateWaveSet(sEnemyShotSet::PatternFunc patternFunc, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = kind;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// ザコが撃つ通常弾を1発セットに追加する(直線移動)
static void FireWaveShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki,
    double speed, int kindImg)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kindImg;
    pEnemyShot->margin = 20.0;   // 通常弾は既定値(画面外の消去はメインルーチン)
    pEnemyShot->param_i[9] = 0;  // ザコフラグ OFF
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// ザコ(大玉)を1機セットに追加する
static sEnemyShot* SpawnZako(sEnemyShotSet* pEnemyShotSet, double x, double y)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = 0.0;
    pEnemyShot->speed = 0.0;
    pEnemyShot->margin = 240.0;                // 仕様: 画面外でも生存できるようにする
    pEnemyShot->param_i[15] = ZAKO_HP_MAX;     // 仕様: HP代わり
    pEnemyShot->param_i[14] = 0;               // 被弾フラッシュ
    pEnemyShot->param_i[13] = 0;               // 隊列スロット
    pEnemyShot->param_i[12] = 0;               // 状態 0:移入 1:展開 2:離脱
    pEnemyShot->param_i[10] = 0;               // サブ識別子
    pEnemyShot->param_i[9] = 1;               // ザコフラグ ON
    pEnemyShot->kind = img_enemyShotLargeBall[ZakoColorForHp(ZAKO_HP_MAX)];
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// ザコの見た目を更新: 被弾フラッシュ中は白、それ以外は残HPに応じた色
static void ZakoUpdateVisual(sEnemyShot* pZako)
{
    if (pZako->param_i[14] > 0) {
        pZako->param_i[14]--;
        pZako->kind = img_enemyShotLargeBall[6];   // 白フラッシュ(不要ならここを削除)
    }
    else {
        pZako->kind = img_enemyShotLargeBall[ZakoColorForHp(pZako->param_i[15])];
    }
}

// ザコ(大玉)と自機ショットの当たり判定+被弾処理
// フレームワークに当たり判定機能が無いためここで自前実装する。
// 当たった自機ショットは消滅し、ザコのHPを1減らす。HPが0になったら
// 大玉を消してボスに1ダメージ。
// 戻り値: 1=ザコは撃破された(リストから削除済み) / 0=生存
static int ZakoDamageCheck(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pZako)
{
    sPlayerShot* pPlayerShot = playerShotHead.next;
    while (pPlayerShot != &playerShotHead) {
        double dx = pPlayerShot->x - pZako->x;
        double dy = pPlayerShot->y - pZako->y;
        if (dx * dx + dy * dy <= ZAKO_HIT_RANGE_SQ) {
            if (CheckSoundMem(sound_playerShotHit_default)) StopSoundMem(sound_playerShotHit_default);
            PlaySoundMem(sound_playerShotHit_default, DX_PLAYTYPE_BACK);
            
            // 自機ショットをリストから外して消す
            sPlayerShot* pPlayerShotNext = pPlayerShot->next;
            pPlayerShot->prev->next = pPlayerShot->next;
            pPlayerShot->next->prev = pPlayerShot->prev;
            delete pPlayerShot;
            pPlayerShot = pPlayerShotNext;

            // ザコのHPを減らす
            pZako->param_i[15]--;
            pZako->param_i[14] = ZAKO_FLASH_FRAMES;
            if (pZako->param_i[15] <= 0) {
                // 撃破: 周囲へ散弾(鱗弾)をばら撒く
                for (int i = 0; i < 8; i++) {
                    FireWaveShot(pEnemyShotSet, pZako->x, pZako->y,
                        i * DX_PI / 4.0, 1.6, img_enemyShotScale[8]);
                }
                PlayEnemySound(sound_enemyShot_heavy);
                // ボスに1ダメージ
                if (enemy.hp > 0) enemy.hp--;
                // 大玉を消す
                pZako->prev->next = pZako->next;
                pZako->next->prev = pZako->prev;
                delete pZako;
                return 1;
            }
        }
        else {
            pPlayerShot = pPlayerShot->next;
        }
    }
    return 0;
}

// ============================================================
//  隊列その0: V字編隊
//  画面外上部から5機がV字を組んだまま突入し、陣形で停止して
//  狙い撃ちを行い、その後加速して画面下へ離脱する。
// ============================================================
static void ShotWave_VFormation(sEnemyShotSet* pEnemyShotSet)
{
    double ramp = GetRamp();

    if (pEnemyShotSet->count == 0) {
        // 5機がV字を組んで画面外上部から飛来する
        double anchorX = 150.0 + GetRand(180);   // 150～330
        for (int i = -2; i <= 2; i++) {
            int ai = (i < 0) ? -i : i;
            sEnemyShot* pZako = SpawnZako(pEnemyShotSet, anchorX + i * 58.0,
                -45.0 - ai * 36.0);
            pZako->param_i[13] = i;                     // スロット
            pZako->param_d[0] = anchorX + i * 58.0;    // 滞空X座標
            pZako->param_d[1] = 118.0 - ai * 36.0;     // 停止Y座標(先頭が最も深い)
        }
        pEnemyShotSet->param_i[0] = 330 + GetRand(50);  // 離脱開始フレーム(set count)
        return;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pShotNext = pShot->next;
        if (pShot->param_i[9] == 1) {
            if (ZakoDamageCheck(pEnemyShotSet, pShot) == 0) {
                int slot = pShot->param_i[13];
                switch (pShot->param_i[12]) {
                case 0: // 突入中
                    pShot->y += 2.3;
                    if (pShot->y >= pShot->param_d[1]) {
                        pShot->y = pShot->param_d[1];
                        pShot->param_i[12] = 1;
                        pShot->param_d[3] = 26.0 + (slot + 2) * 9.0;  // 射撃タイマー(スロットでずらす)
                        pShot->param_d[4] = 66.0 + GetRand(30);       // 狙撃タイマー
                    }
                    break;
                case 1: // 滞空して攻撃
                    pShot->x = pShot->param_d[0]
                        + sin((pEnemyShotSet->count + slot * 40) * 0.018) * 12.0;
                    // 3-way狙い弾(中玉)
                    pShot->param_d[3] -= 1.0;
                    if (pShot->param_d[3] <= 0.0) {
                        double aim = atan2(player.y - pShot->y, player.x - pShot->x);
                        for (int j = -1; j <= 1; j++) {
                            FireWaveShot(pEnemyShotSet, pShot->x, pShot->y + 8.0,
                                aim + j * 0.21, 2.6 + ramp * 0.4,
                                img_enemyShotMediumBall[5]);   // マゼンタ
                        }
                        if (slot == 0) PlayEnemySound(sound_enemyShot_medium);
                        pShot->param_d[3] = (52.0 - ramp * 16.0) * (0.85 + GetRand(30) / 100.0);
                    }
                    // 先頭機は高速銃弾で狙撃
                    if (slot == 0) {
                        pShot->param_d[4] -= 1.0;
                        if (pShot->param_d[4] <= 0.0) {
                            double aim = atan2(player.y - pShot->y, player.x - pShot->x);
                            FireWaveShot(pEnemyShotSet, pShot->x, pShot->y + 8.0, aim,
                                5.4 + ramp * 0.4, img_enemyShotBullet[6]);   // 白
                            PlayEnemySound(sound_enemyShot_extreme);
                            pShot->param_d[4] = (104.0 - ramp * 30.0) * (0.85 + GetRand(30) / 100.0);
                        }
                    }
                    // 離脱開始
                    if (pEnemyShotSet->count >= pEnemyShotSet->param_i[0]) pShot->param_i[12] = 2;
                    break;
                case 2: // 加速して画面下へ離脱(画面外の消去はメインルーチン)
                    pShot->param_d[5] += 0.07;
                    pShot->y += 1.0 + pShot->param_d[5];
                    pShot->x += slot * 0.4;
                    break;
                }
                ZakoUpdateVisual(pShot);
            }
        }
        else {
            // 通常弾の直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShotNext;
    }
}

// ============================================================
//  隊列その1: 蛇行縦隊
//  画面外の左右どちらかから6機が縦隊で飛来し、全機が同じ
//  サインカーブを通ることで隊列のまま蛇行しながら画面を横断する。
//  画面内にいる間だけ攻撃する。
// ============================================================
static void ShotWave_Column(sEnemyShotSet* pEnemyShotSet)
{
    double ramp = GetRamp();

    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = GetRand(2) * 2 - 1;        // 進行方向 (-1 or +1)
        pEnemyShotSet->param_d[0] = 95.0 + GetRand(75);        // 蛇行の基準Y
        pEnemyShotSet->param_d[1] = 2.4 + GetRand(50) / 100.0; // 移動速度
        for (int k = 0; k < 6; k++) {
            // 先頭から30px間隔で画面外に縦隊を組ませる
            double sx = (pEnemyShotSet->param_i[0] > 0) ? (-35.0 - k * 30.0)
                : (515.0 + k * 30.0);
            sEnemyShot* pZako = SpawnZako(pEnemyShotSet, sx, pEnemyShotSet->param_d[0]);
            pZako->param_i[13] = k;
            pZako->param_i[12] = 1;                    // 最初から行動状態
            pZako->param_d[3] = 34.0 + k * 8.0;       // 射撃タイマー
            pZako->param_d[4] = 100.0 + k * 24.0;     // 狙撃タイマー(先頭機のみ使用)
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pShotNext = pShot->next;
        if (pShot->param_i[9] == 1) {
            if (ZakoDamageCheck(pEnemyShotSet, pShot) == 0) {
                int k = pShot->param_i[13];
                // 全機が同じサインカーブを通ることで縦隊の蛇行になる
                pShot->x += pEnemyShotSet->param_i[0] * pEnemyShotSet->param_d[1];
                pShot->y = pEnemyShotSet->param_d[0] + sin(pShot->x * 0.018) * 44.0;
                // 画面内にいる間だけ攻撃
                if (pShot->x > 15.0 && pShot->x < 465.0) {
                    // 2-way狙い弾(菱形弾)
                    pShot->param_d[3] -= 1.0;
                    if (pShot->param_d[3] <= 0.0) {
                        double aim = atan2(player.y - pShot->y, player.x - pShot->x);
                        FireWaveShot(pEnemyShotSet, pShot->x, pShot->y, aim - 0.22,
                            2.2 + GetRand(50) / 100.0 + ramp * 0.3,
                            img_enemyShotDiamond[1]);   // 黄
                        FireWaveShot(pEnemyShotSet, pShot->x, pShot->y, aim + 0.22,
                            2.2 + GetRand(50) / 100.0 + ramp * 0.3,
                            img_enemyShotDiamond[1]);   // 黄
                        if (k == 0) PlayEnemySound(sound_enemyShot_light);
                        pShot->param_d[3] = (46.0 - ramp * 14.0) * (0.85 + GetRand(30) / 100.0);
                    }
                    // 先頭機は時々高速銃弾
                    if (k == 0) {
                        pShot->param_d[4] -= 1.0;
                        if (pShot->param_d[4] <= 0.0) {
                            double aim = atan2(player.y - pShot->y, player.x - pShot->x);
                            FireWaveShot(pEnemyShotSet, pShot->x, pShot->y, aim,
                                5.4, img_enemyShotBullet[6]);   // 白
                            PlayEnemySound(sound_enemyShot_extreme);
                            pShot->param_d[4] = (138.0 - ramp * 40.0) * (0.85 + GetRand(30) / 100.0);
                        }
                    }
                }
                ZakoUpdateVisual(pShot);
            }
        }
        else {
            // 通常弾の直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShotNext;
    }
}

// ============================================================
//  隊列その2: 両翼包囲
//  左右の画面外から3機ずつ同時突入し、画面両端に縦に展開して
//  ゆっくりした狙い弾と環状弾(鱗弾)で挟み込む。その後画面外へ撤収。
// ============================================================
static void ShotWave_Pincer(sEnemyShotSet* pEnemyShotSet)
{
    double ramp = GetRamp();

    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[0] = 360 + GetRand(50);   // 撤収開始フレーム(set count)
        pEnemyShotSet->param_d[0] = GetRand(40);         // 展開位置のYずらし
        for (int s = 0; s < 2; s++) {
            for (int j = 0; j < 3; j++) {
                double side = (s == 0) ? -1.0 : 1.0;
                double hx = (j == 1) ? 52.0 : 74.0;    // 中央機がやや内側へ
                double hy = 122.0 + j * 62.0 + pEnemyShotSet->param_d[0];
                double sx = (s == 0) ? -45.0 : 525.0;
                sEnemyShot* pZako = SpawnZako(pEnemyShotSet, sx, hy);
                pZako->param_i[13] = j;
                pZako->param_i[10] = s;                          // 0:左翼 1:右翼
                pZako->param_i[12] = 0;                          // 移入状態から開始
                pZako->param_d[0] = (s == 0) ? hx : 480.0 - hx; // 展開X座標
                pZako->param_d[1] = hy;                         // 展開Y座標
                pZako->param_d[2] = side;                       // 撤収方向
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pShotNext = pShot->next;
        if (pShot->param_i[9] == 1) {
            if (ZakoDamageCheck(pEnemyShotSet, pShot) == 0) {
                int j = pShot->param_i[13];
                int s = pShot->param_i[10];
                switch (pShot->param_i[12]) {
                case 0: // 展開位置へ移動
                {
                    double dx = pShot->param_d[0] - pShot->x;
                    double dy = pShot->param_d[1] - pShot->y;
                    double dist = sqrt(dx * dx + dy * dy);
                    double step = 3.1;
                    if (dist <= step) {
                        pShot->x = pShot->param_d[0];
                        pShot->y = pShot->param_d[1];
                        pShot->param_i[12] = 1;
                        pShot->param_d[3] = 44.0 + j * 22.0 + s * 11.0;  // 狙い弾タイマー
                        pShot->param_d[4] = 96.0 + s * 77.0;             // 環状弾タイマー(j==1のみ)
                    }
                    else {
                        pShot->x += dx / dist * step;
                        pShot->y += dy / dist * step;
                    }
                    break;
                }
                case 1: // 展開して攻撃
                    pShot->y = pShot->param_d[1]
                        + sin((pEnemyShotSet->count + j * 50 + s * 25) * 0.02) * 7.0;
                    // ゆっくりした狙い弾(中楕円弾)
                    pShot->param_d[3] -= 1.0;
                    if (pShot->param_d[3] <= 0.0) {
                        double aim = atan2(player.y - pShot->y, player.x - pShot->x);
                        FireWaveShot(pEnemyShotSet, pShot->x, pShot->y, aim,
                            2.1 + ramp * 0.4, img_enemyShotMediumOval[8]);   // 橙
                        if (j == 0 && s == 0) PlayEnemySound(sound_enemyShot_heavy);
                        pShot->param_d[3] = (88.0 - ramp * 24.0) * (0.85 + GetRand(30) / 100.0);
                    }
                    // 中央機は自機位置を通る鱗弾の環
                    if (j == 1) {
                        pShot->param_d[4] -= 1.0;
                        if (pShot->param_d[4] <= 0.0) {
                            for (int i = 0; i < 10; i++) {
                                FireWaveShot(pEnemyShotSet, pShot->x, pShot->y,
                                    i * DX_PI * 2.0 / 10.0
                                    + pEnemyShotSet->count * 0.01,
                                    2.4, img_enemyShotScale[3]);   // シアン
                            }
                            PlayEnemySound(sound_enemyShot_heavy);
                            pShot->param_d[4] = (154.0 - ramp * 40.0) * (0.85 + GetRand(30) / 100.0);
                        }
                    }
                    // 撤収開始
                    if (pEnemyShotSet->count >= pEnemyShotSet->param_i[0]) pShot->param_i[12] = 2;
                    break;
                case 2: // 画面外へ撤収(消去はメインルーチン)
                {
                    double v = 1.2 + (pEnemyShotSet->count - pEnemyShotSet->param_i[0]) * 0.05;
                    pShot->x += pShot->param_d[2] * v;
                    break;
                }
                }
                ZakoUpdateVisual(pShot);
            }
        }
        else {
            // 通常弾の直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShotNext;
    }
}

// ============================================================
//  敵本体のパターン
//  画面外上部に隠れたボスが、3種類のザコ隊列をローテーションで
//  送り込み続ける。ザコを1機撃破するごとにボスが1ダメージを受ける。
// ============================================================
void EnemyPat_OnTheWay_Zai()
{
    static int waveTimer;   // 次の隊列を送るまでのフレーム数
    static int waveNum;     // これまでに送り込んだ隊列の総数

    if (count == 1) {
        // ボスは自機ショットの届かない画面外上部に配置する(直接は倒せない)
        enemy.x = 240.0;
        enemy.y = -80.0;
        enemy.maxHp = enemy.hp = 20; // 200から変更しないこと
        waveTimer = 80;
        waveNum = 0;
    }
    else {
        // 画面外でゆっくり揺れて隊列の出現位置に変化をつける
        enemy.x = 240.0 + sin(count * 0.008) * 60.0;
    }

    // ボス撃破後は新しい隊列を送らない
    if (enemy.hp <= 0) return;

    waveTimer--;
    if (waveTimer <= 0) {
        // 隊列来襲の予告音
        PlayEnemySound(sound_enemyCharge);

        // 隊列をローテーションで選んで生成
        sEnemyShotSet* pEnemyShotSet;
        switch (waveNum % 3) {
        case 0:  pEnemyShotSet = CreateWaveSet(ShotWave_VFormation, 0); break;
        case 1:  pEnemyShotSet = CreateWaveSet(ShotWave_Column, 1); break;
        default: pEnemyShotSet = CreateWaveSet(ShotWave_Pincer, 2); break;
        }
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        waveNum++;

        // 進行に応じて隊列の間隔を詰めていく(難易度上昇)
        int period = 215 - waveNum * 4;
        if (period < 145) period = 145;
        waveTimer = period + GetRand(45);
    }
}
