// enemyPat_SuikaFusion.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 2点間の距離の2乗を返すヘルパー関数
static double GetDistSq(double x1, double y1, double x2, double y2) {
    return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

// 弾幕：シンカの連鎖弾（くだものフュージョン）
static void ShotFusion(sEnemyShotSet* pEnemyShotSet)
{
    // スイカ状になった巨大弾のグループを管理するための一意なID
    static int suikaIdCounter = 0;

    // --- 1. 散布フェーズ（小弾のばら撒き） ---
    // カウント800まで、ボス本体の位置から小弾を継続的に撒き続ける
    if (pEnemyShotSet->count % 4 == 0) {
        // 効果音はうるさくならないよう間引いて再生
        if (pEnemyShotSet->count % 12 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        for (int i = 0; i < 2; i++) {
            sEnemyShot* pNew = new sEnemyShot;
            // 敵の座標を直接参照して発射点を動かす
            pNew->x = enemy.x + (GetRand(60) - 30);
            pNew->y = enemy.y + 10.0;
            // 下方向を中心にランダムに扇状に発射
            pNew->muki = DX_PI / 2.0 + (GetRand(100) - 50) / 100.0 * (DX_PI / 2.0);
            pNew->speed = (100 + GetRand(150)) / 100.0; // 1.0 ~ 2.5 の速度
            pNew->speed *= 3;

            pNew->kind = img_enemyShotSmallBall[0]; // サクランボ相当（赤小玉）

            // param_i を状態管理に利用
            pNew->param_i[0] = 0; // 進化レベル (0:小, 1:中, 2:大, 3:特大コア, 4:特大外殻, 5:破裂後)
            pNew->param_i[1] = 0; // 状態フラグ (0:通常, -1:削除予定)

            pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNew->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
            pEnemyShotSet->pEnemyShotHead->prev = pNew;
        }
    }

    // --- 2. 弾の移動と減速処理 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[1] == -1) {
            pShot = pShot->next;
            continue; // 削除予定の弾は動かさない
        }

        // スイカ状態（レベル3コア・レベル4外殻）は急激に空気抵抗を受けて停止する
        if (pShot->param_i[0] == 3 || pShot->param_i[0] == 4) {
            pShot->speed *= 0.94;
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }

    // --- 3. 合体判定フェーズ（総当たりチェック） ---
    for (sEnemyShot* p1 = pEnemyShotSet->pEnemyShotHead->next; p1 != pEnemyShotSet->pEnemyShotHead; p1 = p1->next) {
        if (p1->param_i[1] == -1 || p1->param_i[0] >= 3) continue; // 合体済・特大以上はスルー

        for (sEnemyShot* p2 = p1->next; p2 != pEnemyShotSet->pEnemyShotHead; p2 = p2->next) {
            if (p2->param_i[1] == -1 || p2->param_i[0] >= 3) continue;

            // 同じ進化レベル同士が触れ合った場合のみ合体
            if (p1->param_i[0] == p2->param_i[0]) {
                double rSq = 0.0;
                if (p1->param_i[0] == 0) rSq = 8.0 * 8.0;   // 小玉同士の判定
                else if (p1->param_i[0] == 1) rSq = 18.0 * 18.0; // 中玉同士の判定
                else if (p1->param_i[0] == 2) rSq = 36.0 * 36.0; // 大玉同士の判定

                if (GetDistSq(p1->x, p1->y, p2->x, p2->y) < rSq) {
                    // 合体成立：p1を進化させ、p2を削除フラグにする
                    p1->param_i[0]++;
                    p1->x = (p1->x + p2->x) / 2.0; // 中間地点に移動
                    p1->y = (p1->y + p2->y) / 2.0;
                    p1->speed *= 0.6; // 進化するごとに重くなり速度低下

                    if (p1->param_i[0] == 1) {
                        // イチゴ相当
                        p1->kind = img_enemyShotMediumBall[5]; // マゼンタ中玉
                        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                    }
                    else if (p1->param_i[0] == 2) {
                        // メロン相当
                        p1->kind = img_enemyShotLargeBall[1]; // 黄色大玉
                        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                    }
                    else if (p1->param_i[0] == 3) {
                        // スイカ相当（最大サイズ）爆誕
                        p1->kind = img_enemyShotLargeBall[0]; // 中心コア（赤大玉）
                        p1->margin = 60.0; // 巨大なので画面外判定を広くする
                        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

                        suikaIdCounter++;
                        p1->param_i[2] = suikaIdCounter; // 識別IDを付与
                        p1->param_i[3] = 0; // 破裂までのカウントダウン用タイマー

                        // 大きさを表現するため、周囲に6つの大玉（外殻）をくっつける
                        for (int i = 0; i < 6; i++) {
                            sEnemyShot* pOuter = new sEnemyShot;
                            double ang = i * (DX_PI / 3.0);
                            pOuter->x = p1->x + 20.0 * cos(ang);
                            pOuter->y = p1->y + 20.0 * sin(ang);
                            pOuter->muki = p1->muki;     // コアと同じベクトルを持たせることで
                            pOuter->speed = p1->speed;   // 減速処理が同期し、相対位置が維持される
                            pOuter->kind = img_enemyShotLargeBall[2]; // 外殻は緑の大玉
                            pOuter->margin = 60.0;

                            pOuter->param_i[0] = 4; // 特大外殻
                            pOuter->param_i[2] = suikaIdCounter; // コアと同じID

                            pOuter->prev = pEnemyShotSet->pEnemyShotHead->prev;
                            pOuter->next = pEnemyShotSet->pEnemyShotHead;
                            pEnemyShotSet->pEnemyShotHead->prev->next = pOuter;
                            pEnemyShotSet->pEnemyShotHead->prev = pOuter;
                        }
                    }

                    p2->param_i[1] = -1; // 相手の弾は役目を終えて削除へ
                    break; // p1は進化したため、このフレームでの別弾との合体判定は打ち切る
                }
            }
        }
    }

    // --- 4. 臨界タイマー処理とリストのクリーンアップ ---
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        // スイカの中心コアによるタイマー制御
        if (pShot->param_i[0] == 3) {
            pShot->param_i[3]++; // タイマー進行

            // タイマー後半（120F以降）は激しく明滅して爆発を予告
            if (pShot->param_i[3] > 120) {
                bool flash = (pShot->param_i[3] % 8 < 4);
                pShot->kind = flash ? img_enemyShotLargeBall[6] : img_enemyShotLargeBall[0]; // 白と赤で明滅

                // 同じIDを持つ外殻も連動して明滅させる
                int targetId = pShot->param_i[2];
                for (sEnemyShot* pSearch = pEnemyShotSet->pEnemyShotHead->next; pSearch != pEnemyShotSet->pEnemyShotHead; pSearch = pSearch->next) {
                    if (pSearch->param_i[0] == 4 && pSearch->param_i[2] == targetId) {
                        pSearch->kind = flash ? img_enemyShotLargeBall[6] : img_enemyShotLargeBall[2]; // 白と緑で明滅
                    }
                }
            }

            // 180F（3秒経過）で大爆発
            if (pShot->param_i[3] >= 180) {
                PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

                // 全方位へ「スイカの種」を高速で撒き散らす
                int way = 45;
                for (int i = 0; i < way; i++) {
                    sEnemyShot* pExp = new sEnemyShot;
                    pExp->x = pShot->x;
                    pExp->y = pShot->y;
                    pExp->muki = i * DX_PI * 2.0 / way;
                    pExp->speed = 4.5; // 避けにくい超高速
                    pExp->kind = img_enemyShotScale[7]; // スイカの種（黒の鱗弾）

                    pExp->param_i[0] = 5; // 破裂後の弾（これ以上合体しない）
                    pExp->param_i[1] = 0;

                    pExp->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pExp->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pExp;
                    pEnemyShotSet->pEnemyShotHead->prev = pExp;
                }

                // コア自身と、周囲の殻をすべて削除フラグにする
                int targetId = pShot->param_i[2];
                for (sEnemyShot* pSearch = pEnemyShotSet->pEnemyShotHead->next; pSearch != pEnemyShotSet->pEnemyShotHead; pSearch = pSearch->next) {
                    if ((pSearch->param_i[0] == 3 || pSearch->param_i[0] == 4) && pSearch->param_i[2] == targetId) {
                        pSearch->param_i[1] = -1;
                    }
                }
            }
        }

        // 削除フラグ(-1)が立っている弾をリストから外して消去
        if (pShot->param_i[1] == -1) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }

        pShot = pNext;
    }
}

// 敵本体のパターン（指定名で作成）
void EnemyPat_SuikaGame_Gemini()
{
    static int muki;

    // 出現時の初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // ボスを想定したHP
        muki = 1;

        // 全ての弾が同じリストに入って合体できるように、弾幕セットは1つだけ登録する
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFusion; // 上で作成したフュージョン弾幕を設定
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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
    else {
        // ボス本体は画面上部をゆっくり左右に揺れ動く
        enemy.x += 1.2 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;

        // 少しフワフワと上下させる演出
        enemy.y = 70.0 + 8.0 * sin(count / 30.0);
    }
}