#ifndef __TC_ATOMIC_H
#define __TC_ATOMIC_H

#include <stdint.h>
#include "atomic_asm.h"

//#if __GNUC__ < 4
//#include "../src/module/spp_incl/atomic_asm.h"
//#else
//#include "../src/module/spp_incl/atomic_gcc.h"
//#endif

//#if __WORDSIZE==64
//#define HAS_ATOMIC8	1
//#include "../src/module/spp_incl/atomic_gcc8.h"
//#include "../src/module/spp_incl/myatomic_gcc8.h"
//#else
//#endif
namespace taf
{

/////////////////////////////////////////////////
/** 
 * @file  tc_atomic.h 
 * @brief  原子计数类. 
 *  
 * @author  jarodruan@tencent.com 
 */           
/////////////////////////////////////////////////

/**
 * @brief 原子操作类,对int做原子操作
 */
class TC_Atomic
{
public:

    /**
     * 原子类型
     */
    typedef int atomic_type;

    /**
	 * @brief 构造函数,初始化为0 
     */
    TC_Atomic(atomic_type at = 0)
    {
        set(at);
    }

    TC_Atomic& operator++()
    {
        inc();
        return *this;
    }

    TC_Atomic& operator--()
    {
        dec();
        return *this;
    }

    operator atomic_type() const
    {
        return get();
    }

    TC_Atomic& operator+=(atomic_type n)
    {
        add(n);
        return *this;
    }

    TC_Atomic& operator-=(atomic_type n)
    {
        sub(n);
        return *this;
    }

    TC_Atomic& operator=(atomic_type n)
    {
        set(n);
        return *this;
    }

    /**
     * @brief 获取值
     *
     * @return int
     */
    atomic_type get() const         { return atomic_read(&_value); }

    /**
     * @brief 添加
     * @param i
     *
     * @return int
     */
    atomic_type add(atomic_type i)  { return atomic_add_return(i, &_value); }

    /**
     * @brief 减少
     * @param i
     *
     * @return int
     */
    atomic_type sub(atomic_type i)  { return atomic_sub_return(i, &_value); }

    /**
     * @brief 自加1
     *
     * @return int
     */
    atomic_type inc()               { return add(1); }

    /**
     * @brief 自减1
     */
    atomic_type dec()               { return sub(1); }

    /**
     * @brief 自加1
     *
     * @return void
     */
    void inc_fast()               { return atomic_inc(&_value); }

    /**
     * @brief 自减1
     * Atomically decrements @_value by 1 and returns true if the
     * result is 0, or false for all other
     */
    bool dec_and_test()               { return atomic_dec_and_test(&_value); }

    /**
     * @brief 设置值
     */
    atomic_type set(atomic_type i)  { atomic_set(&_value, i); return i; }

protected:

    /**
     * 值
     */
    atomic_t    _value;
};

}

#endif
