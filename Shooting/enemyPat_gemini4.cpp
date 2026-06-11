// 必須インクルード
#include "DxLib.h"
#include "gv.h"
#include <math.h>

// 円周率（math.hにM_PIがない環境を考慮）
constexpr double PI = 3.14159265358979323846;

// 弾幕：桜の花びらが舞う軌道
static void ShotSakura(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射フレーム（count == 0）のみ弾を生成
    if (pEnemyShotSet->count == 0) {
        if (pEnemyShotSet->kind) {
            if (CheckSoundMem(sound_enemyShot_light) == 1)
                StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        int way = 7; // 7方向
        for (int i = 0; i < way; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            
            // 円状に等間隔で配置
            pEnemyShot->muki = pEnemyShotSet->muki + (2.0 * PI / way) * i;
            pEnemyShot->speed = 1.0; // 初速は遅め

            // 桜をイメージしてマゼンタ(4)と赤(0)の鱗弾（Scale）を交互に
            int color = (i % 2 == 0) ? 5 : 0;
            pEnemyShot->kind = img_enemyShotScale[color];

            // リストへ連結
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の軌道更新（ひらひらと舞う表現）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 徐々に加速しつつ、常に同じ方向へ曲がることで螺旋（吹雪）を描く
        pEnemyShot->speed += 0.015;
        pEnemyShot->muki += 0.012; 

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
    
    // ※もしフレームワーク側で自動加算されていない場合は以下のコメントアウトを外してください
    // pEnemyShotSet->count++; 
}

// 敵本体のパターン：桜吹雪
void Pattern_SakuraFubuki()
{
    // 敵の移動ロジック（中央付近で左右に揺れる）
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = 100;
        enemy.hp = enemy.maxHp;
    } else {
        enemy.x = 240.0 + 80.0 * sin(count * PI / 120.0);
    }

    // 8フレームごとに発射（吹雪のように途切れず出す）
    if (count % 4 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSakura;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->kind = (count / 4) & 1;

        // 発射の基準角を少しずつ回転させる
        pEnemyShotSet->muki = (count % 360) * PI / 180.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}