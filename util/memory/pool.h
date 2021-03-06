#pragma once

#include "alloc.h"

#include <util/system/align.h>
#include <util/generic/bitops.h>
#include <util/system/yassert.h>
#include <util/generic/utility.h>
#include <util/generic/intrlist.h>
#include <util/generic/chartraits.h>

class TMemoryPool {
    private:
        typedef IAllocator::TBlock TBlock;

        class TChunk: public TIntrusiveListItem<TChunk> {
            public:
                inline TChunk(size_t len = 0) throw ()
                    : Cur_((char*)(this + 1))
                    , Left_(len)
                {
                    YASSERT((((size_t)Cur_) % PLATFORM_DATA_ALIGN) == 0);
                }

                inline void* Allocate(size_t len) throw () {
                    if (Left_ >= len) {
                        char* ret = Cur_;

                        Cur_ += len;
                        Left_ -= len;

                        return ret;
                    }

                    return 0;
                }

                inline size_t BlockLength() const throw () {
                    return (Cur_ + Left_) - (char*)this;
                }

                inline size_t Used() const throw () {
                    return Cur_ - (const char*)this;
                }

                inline size_t Left() const throw () {
                    return Left_;
                }

            private:
                char* Cur_;
                size_t Left_;
        };

    public:
        class IGrowPolicy {
            public:
                virtual ~IGrowPolicy() {
                }

                virtual size_t Next(size_t prev) const throw () = 0;
        };

        class TLinearGrow: public IGrowPolicy {
            public:
                virtual size_t Next(size_t prev) const throw () {
                    return prev;
                }

                static IGrowPolicy* Instance() throw ();
        };

        class TExpGrow: public IGrowPolicy {
            public:
                virtual size_t Next(size_t prev) const throw () {
                    return prev * 2;
                }

                static IGrowPolicy* Instance() throw ();
        };

        inline TMemoryPool(size_t initial, IGrowPolicy* grow = TExpGrow::Instance(), IAllocator* alloc = TDefaultAllocator::Instance())
            : Current_(&Empty_)
            , BlockSize_(initial)
            , GrowPolicy_(grow)
            , Alloc_(alloc)
            , Origin_(initial)
        {
        }

        inline ~TMemoryPool() throw () {
            Clear();
        }

        inline void* Allocate(size_t len) {
            return RawAllocate(AlignUp<size_t>(len, PLATFORM_DATA_ALIGN));
        }

        template <typename T>
        inline T* Allocate() {
            return (T*)this->Allocate(sizeof(T));
        }

        template <typename T>
        T* New() {
            T* ptr = Allocate<T>();
            return new (ptr) T;
        }

        template <typename T>
        T* NewArray(size_t count) {
            T* arr = (T*)Allocate(sizeof(T) * count);
            for (T *ptr = arr, *end = arr + count; ptr != end; ++ptr)
                new (ptr) T;
            return arr;
        }

        template <typename TChar>
        inline TChar* Append(const TChar* str) {
            return Append(str, TCharTraits<TChar>::GetLength(str) + 1); // include terminating zero byte
        }

        template <typename TChar>
        inline TChar* Append(const TChar* str, size_t len) {
            TChar* ret = static_cast<TChar*>(Allocate(len * sizeof(TChar)));

            TCharTraits<TChar>::Copy(ret, str, len);

            return ret;
        }

        inline size_t Available() const throw () {
            if (Current_)
                return Current_->BlockLength() - Current_->Used();
            return 0;
        }

        inline void Clear() throw () {
            while (!Chunks_.Empty()) {
                TChunk* c = Chunks_.PopBack();
                TBlock b = {c, c->BlockLength()};

                c->~TChunk();
                Alloc_->Release(b);
            }

            Current_ = &Empty_;
            BlockSize_ = Origin_;
        }

        inline size_t MemoryAllocated() const throw () {
            size_t acc = 0;

            for (TIntrusiveList<TChunk>::TConstIterator i = Chunks_.Begin(); i != Chunks_.End(); ++i) {
                acc += i->Used();
            }

            return acc;
        }

        inline size_t MemoryWaste() const throw () {
            size_t wst = 0;

            for (TIntrusiveList<TChunk>::TConstIterator i = Chunks_.Begin(); i != Chunks_.End(); ++i) {
                wst += i->Left();
            }

            return wst;
        }

    protected:
        inline void* RawAllocate(size_t len) {
            void* ret = Current_->Allocate(len);

            if (ret) {
                return ret;
            }

            AddChunk(len);

            return Current_->Allocate(len);
        }

    private:
        inline void AddChunk(size_t hint) {
            const size_t dataLen = Max(BlockSize_, hint);
            TBlock nb = Alloc_->Allocate(FastClp2(dataLen + sizeof(TChunk)));

            BlockSize_ = GrowPolicy_->Next(dataLen);
            Current_ = new (nb.Data) TChunk(nb.Len - sizeof(TChunk));
            Chunks_.PushBack(Current_);
        }

    private:
        TChunk Empty_;
        TChunk* Current_;
        size_t BlockSize_;
        IGrowPolicy* GrowPolicy_;
        IAllocator* Alloc_;
        TIntrusiveList<TChunk> Chunks_;
        const size_t Origin_;
};

struct TPoolable {
    inline void* operator new(size_t bytes, TMemoryPool& pool) {
        return pool.Allocate(bytes);
    }

    inline void operator delete(void*, size_t) throw () {
    }

    inline void operator delete(void*, TMemoryPool&) throw () {
    }
};

class TMemoryPoolAllocator: public IAllocator {
    public:
        inline TMemoryPoolAllocator(TMemoryPool* pool)
            : Pool_(pool)
        {
        }

        virtual TBlock Allocate(size_t len) {
            TBlock ret = {Pool_->Allocate(len), len};

            return ret;
        }

        virtual void Release(const TBlock& block) {
            (void)block;
        }

    private:
        TMemoryPool* Pool_;
};

template <class T>
class TPoolAlloc {
    public:
        typedef T* pointer;
        typedef const T* const_pointer;
        typedef T& reference;
        typedef const T& const_reference;
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef void value_type;

        inline TPoolAlloc(TMemoryPool* pool)
            : Pool_(pool)
        {
        }

        template <typename TOther>
        inline TPoolAlloc(const TPoolAlloc<TOther>& o)
            : Pool_(o.GetPool())
        {
        }

        inline T* allocate(size_t n) {
            return (T*)Pool_->Allocate(n * sizeof(T));
        }

        inline void deallocate(pointer /*p*/, size_t /*n*/) {
        }

        template <class T1>
        struct rebind {
            typedef TPoolAlloc<T1> other;
        };

        inline size_type max_size() const throw () {
            return size_type(-1) / sizeof(T);
        }

        inline void construct(pointer p, const T& val) {
            new (p) T(val);
        }

        inline void destroy(pointer p) throw () {
            p->~T();
        }

        inline TMemoryPool* GetPool() const {
            return Pool_;
        }

    private:
        TMemoryPool* Pool_;
};

struct TPoolAllocator {
    template <class T>
    struct rebind {
        typedef TPoolAlloc<T> other;
    };
};

template <class T>
inline bool operator==(const TPoolAlloc<T>&, const TPoolAlloc<T>&) throw () {
    return true;
}

template <class T>
inline bool operator!=(const TPoolAlloc<T>&, const TPoolAlloc<T>&) throw () {
    return false;
}
