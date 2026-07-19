// gv.h

#pragma once
#include <vector>
#include <cassert>
#include <cstddef>
#include <cstdint>

//  メモリプール CRTP ベース
template<typename Derived, int N>
struct PoolAllocator {
protected:
    static constexpr int POOL_SIZE = N;

private:
    inline static Derived* pool_ = nullptr;
    inline static Derived* freeList_ = nullptr;
    inline static bool initialized_ = false;

    static void initPool() {
        pool_ = new Derived[N];
        for (int i = 0; i < N - 1; ++i)
            pool_[i].next = &pool_[i + 1];
        pool_[N - 1].next = nullptr;
        freeList_ = pool_;
        initialized_ = true;
    }

public:
    void* operator new(std::size_t) {
        if (!initialized_) initPool();
        assert(freeList_ && "pool exhausted");
        Derived* p = freeList_;
        freeList_ = freeList_->next;
        return p;
    }

    void operator delete(void* ptr) noexcept {
        if (!ptr) return;
        auto* p = static_cast<Derived*>(ptr);
        p->next = freeList_;
        freeList_ = p;
    }
};

// ============================================================
//  共有構造体
// ============================================================
struct sPlayer {
    double x = 0.0, y = 0.0;        // 位置
};

// プール管理付き弾構造体: PoolAllocator を継承するだけで
// new/delete が自動的にプールを使うようになる
struct sPlayerShot : PoolAllocator<sPlayerShot, 64> {
    double       x = 0.0, y = 0.0;  // 位置
    sPlayerShot* prev = nullptr;
    sPlayerShot* next = nullptr;
};

struct sEnemy {
    double x = 0.0, y = 0.0;
    int    hp = 0, maxHp = 0;
};

struct sEnemyShot : PoolAllocator<sEnemyShot, 4096> {
    double      x = 0.0, y = 0.0; // 中心位置
    double      muki = 0.0;       // 向き
    double      speed = 0.0;      // 速さ
    int         count = 0;        // 毎フレーム自動で+1
    int         kind = 0;         // 種類と色
    double      margin = 20.0;    // 画面外へどれだけ出たら弾を自動で消すか（不用意に変更しないこと）
    int         param_i[8]{};     // 自由な目的に使えるパラメータ
    double      param_d[8]{};     // 自由な目的に使えるパラメータ
    sEnemyShot* prev = nullptr;
    sEnemyShot* next = nullptr;
};

struct sEnemyShotSet : PoolAllocator<sEnemyShotSet, 1024> {
    // 関数ポインタ型に名前を付けて宣言を読みやすくする
    using PatternFunc = void(*)(sEnemyShotSet*);

    double         x = 0.0, y = 0.0;
    double         muki = 0.0;
    PatternFunc    patternFunc = nullptr;
    int            count = 0;        // 毎フレーム自動で+1
    int            kind = 0;
    int            param_i[8]{};     // 自由な目的に使えるパラメータ
    double         param_d[8]{};     // 自由な目的に使えるパラメータ
    sEnemyShot*    pEnemyShotHead = nullptr;
    sEnemyShotSet* prev = nullptr;
    sEnemyShotSet* next = nullptr;
};

// ============================================================
//  グローバル変数（extern 宣言のみ）
// ============================================================
extern int count; // 毎フレーム自動で+1
extern int key[256];

extern sPlayer       player;
extern sPlayerShot   playerShotHead;
extern sEnemy        enemy;
extern sEnemyShotSet enemyShotSetHead;
