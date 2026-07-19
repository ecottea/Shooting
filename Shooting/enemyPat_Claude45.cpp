// enemyPat_dna.cpp
// DNAモチーフ弾幕: 二重螺旋(ストランド) + 塩基対(ラング) + 解離(アンジップ)
//
// フェーズ構成:
//   ①螺旋成長フェーズ: ストランドA/B(位相180°ずれ)の針弾が周期的に生成され、
//                       パラメトリックならせん軌道を描きながら下降する。
//   ②塩基対生成      : ストランドが最大幅になる位相と同じタイミングで、
//                       中心の左右にラング(塩基対)マーカー弾を静止生成する。
//   ③変異(揺らぎ)    : 半径をサイン波でゆっくり揺らし、単調な螺旋にならないようにする。
//   ④解離(アンジップ): 一定フレーム経過後、ストランドの回転を凍結して左右に開かせつつ、
//                       蓄積したラング弾を全て自機狙いで一斉発射する。
//                       その後、螺旋成長フェーズへ戻り周期的に繰り返す。
//
// 周期の管理について:
//   「今どの周期の何フレーム目か」「アンジップは発生済みか」は、状態を変数として
//   保持するのではなく、グローバルなcount(このパターン開始からの経過フレーム)から
//   毎フレーム計算で求めている(DnaCyclePos/DnaIsUnzipTriggered)。
//   状態を持たないため、ゲームを再開してcountが1からやり直されさえすれば、
//   古い状態が残って挙動やFPSがおかしくなる心配がない。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  調整用定数
// ============================================================
static const double DNA_CENTER_X = 240.0; // 敵の初期X（画面は480x480想定）
static const double DNA_ENEMY_Y = 50.0;
static const int    DNA_STRAND_INTERVAL = 2;    // ストランド弾ペアを生成する間隔(フレーム)
static const int    DNA_RUNG_INTERVAL = 40;   // 塩基対(ラング)弾を生成する間隔(フレーム)
// ※ DNA_ANGULAR_SPEED の半回転周期と一致させてある
static const int    DNA_HELIX_DURATION = 300;  // 螺旋成長フェーズの長さ(フレーム)。経過後アンジップへ
static const int    DNA_UNZIP_DURATION = 150;  // アンジップ演出の長さ(フレーム)。経過後、螺旋成長フェーズへ戻り周期的に繰り返す
static const double DNA_ANGULAR_SPEED = DX_PI / 40.0; // 螺旋の回転速度(半回転=40フレーム)
static const double DNA_BASE_RADIUS = 70.0; // 螺旋の基本半径
static const double DNA_RADIUS_AMPLITUDE = 14.0; // 変異(ミューテーション)揺らぎの振幅
static const double DNA_RADIUS_WOBBLE_SPD = 0.015;// 揺らぎの速さ
static const double DNA_FALL_SPEED = 1.5;  // 下降速度
static const double DNA_UNZIP_SPEED = 2.4;  // アンジップ後、ラング弾が自機狙いで飛ぶ速さ
static const double DNA_UNZIP_EXPAND_SPD = 0.5;  // アンジップ後、ストランドが左右に開いていく半径増加量/フレーム

// ============================================================
//  周期管理ヘルパー(状態を持たず、countだけから毎フレーム計算する)
// ============================================================

// 現在の周期内での経過フレーム(0 〜 HELIX_DURATION+UNZIP_DURATION-1)。
// count==1(パターン開始)のときに必ず0になる。
static int DnaCyclePos()
{
    const int cycleLength = DNA_HELIX_DURATION + DNA_UNZIP_DURATION;
    return (count - 1) % cycleLength;
}

// アンジップフェーズに入っているかどうか。
static bool DnaIsUnzipTriggered()
{
    return DnaCyclePos() >= DNA_HELIX_DURATION;
}

// ============================================================
//  弾幕：DNAストランド（バックボーン）ペア
//  1回の生成で位相が180°ずれた2本(ストランドA・B)の針弾を1本ずつ生成し、
//  以降は各弾が自身の経過フレーム(pEnemyShot->count)を使ったパラメトリック計算で
//  螺旋軌道を描きながら下降する。
// ============================================================
static void ShotDnaStrandPair(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 2; i++) {
            double phaseOffset = (i == 0) ? 0.0 : DX_PI; // ストランドA=0°, ストランドB=180°(常に反対側)

            pEnemyShot = new sEnemyShot;

            pEnemyShot->param_d[0] = phaseOffset;
            pEnemyShot->param_d[1] = pEnemyShotSet->y; // 生成時のY(螺旋の起点)
            pEnemyShot->param_d[2] = 0.0;               // アンジップ時に回転を止めた深さ(t)。使用時に設定
            pEnemyShot->param_d[3] = pEnemyShotSet->x;  // 螺旋の中心X(生成時に固定)
            pEnemyShot->param_i[0] = 0;                 // 0:回転中 1:アンジップにより回転停止済み

            pEnemyShot->x = pEnemyShotSet->x + DNA_BASE_RADIUS * cos(phaseOffset);
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 0.0; // 未使用（移動はparam_dから直接計算するため）
            pEnemyShot->speed = 0.0; // 未使用

            // 弾の種類一覧: 小玉、中玉、大玉、銃弾、鱗弾、菱形弾、中楕円弾、レーザー
            // 針状で当たり判定が小さい img_enemyShotBullet を採用(レーザーは判定が大きすぎるため不可)。
            // 色一覧: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
            // 2本のバックボーンは同じ分子なので同系色(シアン/青)でまとめ、塩基対(ラング)側で色分けする。
            pEnemyShot->kind = (i == 0) ? img_enemyShotBullet[3] : img_enemyShotBullet[4];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    bool unzipTriggered = DnaIsUnzipTriggered();

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pEnemyShot->count;
        double phaseOffset = pEnemyShot->param_d[0];
        double spawnY = pEnemyShot->param_d[1];
        double centerX = pEnemyShot->param_d[3];

        // アンジップが発生した瞬間、そのときの深さ(t)で回転を凍結する
        if (unzipTriggered && pEnemyShot->param_i[0] == 0) {
            pEnemyShot->param_d[2] = t;
            pEnemyShot->param_i[0] = 1;
        }

        double tRot = (pEnemyShot->param_i[0] == 1) ? pEnemyShot->param_d[2] : t;
        double angle = phaseOffset + DNA_ANGULAR_SPEED * tRot;
        double wobble = DNA_RADIUS_AMPLITUDE * sin(DNA_RADIUS_WOBBLE_SPD * tRot);
        double radius = DNA_BASE_RADIUS + wobble;

        if (pEnemyShot->param_i[0] == 1) {
            double tOpen = t - pEnemyShot->param_d[2]; // アンジップしてからの経過フレーム
            radius += DNA_UNZIP_EXPAND_SPD * tOpen;    // 左右に開いていく
        }

        pEnemyShot->x = centerX + radius * cos(angle);
        pEnemyShot->y = spawnY + DNA_FALL_SPEED * t;

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  弾幕：塩基対(ラング)マーカー
//  螺旋が最大幅になる位置(中心の左右)に静止した弾を2発生成し、
//  アンジップが起きるまではその場に留まる(速度0のマーカー弾)。
//  アンジップ発生後、自機狙いで発射される。
// ============================================================
static void ShotDnaRung(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 2; i++) for (int j = -5; j <= 5; j++) {
            pEnemyShot = new sEnemyShot;

            double side = (i == 0) ? -1.0 : 1.0;
            pEnemyShot->x = pEnemyShotSet->x + side * DNA_BASE_RADIUS;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0; // 速度0のマーカー弾として、アンジップまで画面上に留める
            pEnemyShot->param_i[0] = 0; // 0:未発射 1:発射済み

            // 塩基対の色分け(A-T / G-C 風に2種類のペア配色を交互に使用)
            // 色一覧: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
            bool schemeA = (pEnemyShotSet->kind % 2 == 0);
            int color;
            if (schemeA) color = (i == 0) ? 0 : 2; // 赤・緑ペア
            else         color = (i == 0) ? 5 : 1; // マゼンタ・黄ペア
            pEnemyShot->kind = img_enemyShotDiamond[color];
            pEnemyShot->param_d[0] = j * 0.1;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    bool unzipTriggered = DnaIsUnzipTriggered();

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (unzipTriggered && pEnemyShot->param_i[0] == 0) {
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x) + pEnemyShot->param_d[0];
            pEnemyShot->speed = DNA_UNZIP_SPEED;
            pEnemyShot->param_i[0] = 1;
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体のパターン: DNA二重螺旋
// ============================================================
void EnemyPat_DNA_Claude()
{
    static int hoverDir;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = DNA_CENTER_X;
        enemy.y = DNA_ENEMY_Y;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        hoverDir = 1;
    }
    else {
        // ゆるやかな左右ホバー(周期に関わらず継続)
        enemy.x += 0.25 * (double)hoverDir;
        if (count % 200 == 100) hoverDir *= -1;
    }

    int  cyclePos = DnaCyclePos();          // 周期内での経過フレーム(0始まり)
    int  localCount = cyclePos + 1;           // 螺旋成長フェーズ開始時の判定をcount==1相当に揃えるためのオフセット
    bool unzipTriggered = (cyclePos >= DNA_HELIX_DURATION);

    // 周期の先頭(初回のゲーム開始時も、2周期目以降の螺旋再生成時も、ここで必ず1回だけ真になる)
    if (cyclePos == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 螺旋成長フェーズの終了 → アンジップ発生の瞬間
    if (cyclePos == DNA_HELIX_DURATION) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // ストランド(バックボーン)弾ペアの生成
    if (!unzipTriggered && localCount % DNA_STRAND_INTERVAL == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDnaStrandPair;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 塩基対(ラング)弾の生成 ※ストランドが最大幅になる位相と一致するタイミング
    if (!unzipTriggered && localCount % DNA_RUNG_INTERVAL == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDnaRung;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->kind = (localCount / DNA_RUNG_INTERVAL) % 2; // 塩基対配色を交互に切替(周期ごとに同じ配色から開始)

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}