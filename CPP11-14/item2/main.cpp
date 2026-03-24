// ============================================================
// Effective Modern C++ Item2: auto 类型推导
// ============================================================
// 核心结论：
//   auto 的推导规则和 Item1 模板推导几乎完全一样
//   把 auto 当成模板里的 T 看就行
//   唯一例外：大括号初始化，auto 推导为 initializer_list
// ============================================================

#include <iostream>
#include <vector>
#include <initializer_list>

// ============================================================
// 【Part 1】auto 对应 Item1 的三种 Case
// 把 auto 当 T，auto 左边的修饰符当 ParamType
// ============================================================

void part1_auto_vs_template(){
    int x = 27;
    const int cx = x;
    const int& rx = cx;
    // --- Case1：auto& 或 const auto& ---
    // 对应 ParamType = T& 或 const T&
    // 规则：引用被忽略，const 保留进 T
    auto& a = x;          // auto = int,       a = int&
    auto& b = cx;         // auto = const int, b = const int&  （const保留）
    auto& c = rx;         // auto = const int, c = const int&  （&忽略，const保留）
    const auto& d = x;    // auto = int,       d = const int&
    const auto& e = cx;   // auto = int,       e = const int&  （const不重复进auto）
    const auto& f = rx;   // auto = int,       f = const int&

    // --- Case2：auto&&（通用引用）---
    // 对应 ParamType = T&&
    // 规则：左值 → auto推导为左值引用；右值 → 正常推导
    auto&& g = x;         // x 是左值，auto = int&,  g = int&   （引用折叠）
    auto&& h = cx;        // cx 是左值，auto = const int&, h = const int&
    auto&& i = 27;        // 27 是右值，auto = int,  i = int&&

    // --- Case3：纯 auto（传值）---
    // 对应 ParamType = T
    // 规则：const 和 & 全部剥掉，param 是独立拷贝
    auto j = x;           // auto = int
    auto k = cx;          // auto = int   （const 被剥掉）
    auto l = rx;          // auto = int   （const 和 & 都被剥掉）
}

// ============================================================
// 【Part 2】唯一例外：大括号初始化
// auto 推导为 initializer_list，模板推导直接报错
// ============================================================

void part2_initializer_list(){
    // 四种初始化方式，对 auto 结果不同
    auto a = 27;      // auto = int
    auto b(27);       // auto = int
    auto c = {27};    // auto = std::initializer_list<int>  ← 注意！
    auto d{27};       // C++11: initializer_list<int>，C++14/17: int

    // 多个元素
    auto e = {1, 2, 3};  // auto = std::initializer_list<int>
    // auto f = {1, 2, 3.0};  // 编译报错！类型不一致，initializer_list要求类型相同

    // initializer_list 的基本用法
    for (auto x : e) {
        (void)x;        // 可以遍历
    }

    e.size();           // 返回 3
    // e[0];            // 不合法！initializer_list 没有下标运算符
}

// ============================================================
// 【Part 3】模板推导 vs auto 推导，大括号的区别
// ============================================================

template<typename T>
void f_template(T param) { (void)param; }

void part3_template_vs_auto(){
    // 模板推导：大括号直接报错
    // f_template({1, 2, 3});   // 编译报错！模板无法推导 initializer_list
    // auto 推导：大括号推导为 initializer_list
    auto list = {1, 2, 3};     // OK，std::initializer_list<int>
}

// ============================================================
// 【Part 4】C++14 中 auto 用于函数返回值和 lambda 参数
// 注意：这两种情况用的是模板推导规则，不是 auto 规则
// ============================================================

// 函数返回值 auto 推导（模板规则）
auto add(int a, int b) {
    return a + b;    // 推导为 int
}

// auto return 不能返回大括号（模板规则，不支持 initializer_list）
// auto createList() {
//     return {1, 2, 3};   // 编译报错！
// }

void part4_auto_return_and_lambda(){
     // 函数返回值
    auto result = add(1, 2);   // result = int = 3

    // lambda 参数（C++14，泛型lambda，用模板推导规则）
    auto lambda = [](auto x, auto y) {
        return x + y;
    };

    auto r1 = lambda(1, 2);      // x=int, y=int
    auto r2 = lambda(1.0, 2.0);  // x=double, y=double

    // lambda 参数同样不支持大括号推导
    // lambda({1,2,3}, {4,5,6});  // 编译报错
}

// ============================================================
// 【补充】initializer_list 的实际使用场景
// ============================================================

void part5_initializer_list_usage(){
    // 场景1：初始化 STL 容器（最常见，天天在用）
    std::vector<int> v = {1, 2, 3, 4, 5};   // 触发 initializer_list 构造函数

     // 场景2：自己写接受不定数量同类型参数的函数
    auto sum = [](std::initializer_list<int> list) {
        int result = 0;
        for (auto x : list) result += x;
        return result;
    };
    sum({1, 2, 3});         // 传3个
    sum({1, 2, 3, 4, 5});   // 传5个，随便传多少
}

// ============================================================
// 【核心总结】
// ============================================================
//
// 1. auto 推导规则 = Item1 模板推导规则
//    把 auto 当 T，auto 左边修饰符当 ParamType，规则完全一样
//
// 2. 唯一例外：大括号初始化
//    auto x = {1,2,3}  → std::initializer_list<int>
//    模板 f({1,2,3})    → 编译报错
//
// 3. 实际建议：
//    用 auto 初始化时，除非明确想要 initializer_list，否则不用大括号
//    用小括号或直接赋值：auto x = 27; 或 auto x(27);
//
// 4. C++14 函数返回值和 lambda 参数里的 auto
//    用的是模板推导规则，不是 auto 规则
//    → 同样不支持大括号推导
//
// ============================================================
 
int main()
{
    part1_auto_vs_template();
    part2_initializer_list();
    part3_template_vs_auto();
    part4_auto_return_and_lambda();
    part5_initializer_list_usage();
    return 0;
}
 