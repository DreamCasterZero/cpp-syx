// ============================================================
// Effective Modern C++ Item31 & Item32: Lambda 捕获
// ============================================================
// Item31 核心：避免默认捕获，尽量显式捕获
// Item32 核心：C++14 广义捕获，支持移动捕获
// ============================================================

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <functional>

// ============================================================
// 【Part 1】默认引用捕获 [&] 的坑：悬空引用
// ============================================================

std::vector<std::function<bool(int)>> filters;

void part1_ref_capture_dangling()
{
    // 危险写法：捕获局部变量的引用
    auto makeFilter_bad = []() {
        int threshold = 10;
        return [&](int val) {         // 捕获 threshold 的引用
            return val > threshold;   // 函数返回后 threshold 已销毁！
        };
    };
    // auto filter = makeFilter_bad();
    // filter(20);   // 未定义行为！threshold 已经不存在了

    // 正确写法：显式按值捕获
    auto makeFilter_good = []() {
        int threshold = 10;
        return [threshold](int val) {  // 按值捕获，安全
            return val > threshold;
        };
    };
    auto filter = makeFilter_good();
    std::cout << "filter(20): " << filter(20) << "\n";  // OK
}

// ============================================================
// 【Part 2】默认值捕获 [=] 的坑：实际捕获的是 this
// ============================================================

class Widget {
public:
    Widget(int threshold) : threshold_(threshold) {}

    void addFilter_bad() {
        // 危险：[=] 捕获成员变量时实际捕获的是 this 指针
        filters.push_back([=](int val) {
            return val > threshold_;  // 看起来捕获了值，实际是 this->threshold_
        });
        // Widget 销毁后 this 悬空，再调用 filter 是未定义行为
    }

    void addFilter_good() {
        // 正确写法1：先拷贝出来再捕获
        auto threshold = threshold_;
        filters.push_back([threshold](int val) {
            return val > threshold;   // 捕获的是局部变量的拷贝，安全
        });
    }

    void addFilter_cpp14() {
        // 正确写法2：C++14 广义捕获，直接捕获成员变量的拷贝
        filters.push_back([threshold = threshold_](int val) {
            return val > threshold;   // 安全
        });
    }

private:
    int threshold_;
};

// ============================================================
// 【Part 3】显式捕获多个变量
// ============================================================

void part3_explicit_multi_capture()
{
    int a = 1, b = 2, c = 3;

    // 多个变量按值捕获，逗号隔开
    auto f1 = [a, b, c](int val) {
        return val > a + b + c;
    };

    // 多个变量按引用捕获
    auto f2 = [&a, &b, &c](int val) {
        a++; b++; c++;   // 可以修改外部变量
        return val > a + b + c;
    };

    // 混合捕获：a 按值，b 按引用
    auto f3 = [a, &b](int val) {
        b++;             // 可以修改 b
        return val > a + b;
    };

    std::cout << "f1(10): " << f1(10) << "\n";
    std::cout << "f2(10): " << f2(10) << "\n";
    std::cout << "f3(10): " << f3(10) << "\n";

    // 变量很多时，打包成结构体捕获
    struct Params {
        int threshold;
        int minValue;
        int maxValue;
    };
    Params p{10, 0, 100};
    auto f4 = [p](int val) {
        return val > p.threshold && val < p.maxValue;
    };
    std::cout << "f4(50): " << f4(50) << "\n";
}

// ============================================================
// 【Part 4】C++14 广义捕获：移动捕获（Item32）
// 语法：[新变量名 = 表达式]
// 主要用途：把只能移动不能拷贝的对象移动进 lambda
// ============================================================

void part4_generalized_capture()
{
    // unique_ptr 不能拷贝，C++11 没办法捕获
    auto ptr = std::make_unique<std::string>("hello");

    // C++14 广义捕获：移动进 lambda
    auto f = [p = std::move(ptr)] {
        std::cout << "lambda 内部: " << *p << "\n";
    };
    // ptr 已经被移走，变为空
    // std::cout << *ptr;  // 未定义行为！

    f();  // OK，p 在 lambda 内部有效

    // 广义捕获：捕获时直接计算
    int x = 10;
    auto f2 = [y = x + 1] {    // y = 11，不是捕获 x
        std::cout << "y = " << y << "\n";
    };
    f2();

    // 广义捕获：捕获时重命名
    std::string longVariableName = "world";
    auto f3 = [s = std::move(longVariableName)] {
        std::cout << "s = " << s << "\n";
    };
    f3();
}

// ============================================================
// 【Part 5】什么时候可以用默认捕获
// lambda 不逃出当前作用域时，[=] 相对安全
// ============================================================

void part5_safe_default_capture()
{
    int threshold = 10;
    int minVal = 0;
    int maxVal = 100;

    // lambda 只在当前作用域内用，不返回出去
    // 所有捕获的变量生命周期都比 lambda 长，相对安全
    auto filter = [=](int val) {
        return val > threshold && val >= minVal && val <= maxVal;
    };

    std::vector<int> data = {-5, 0, 5, 10, 50, 100, 200};
    for (auto v : data) {
        if (filter(v)) {
            std::cout << v << " 通过过滤\n";
        }
    }
}

// ============================================================
// 【核心总结】
// ============================================================
//
// Item31：避免默认捕获
//
// 1. [&] 默认引用捕获的坑
//    捕获局部变量引用，函数返回后引用悬空
//    解决：显式按值捕获，或确保 lambda 不逃出作用域
//
// 2. [=] 默认值捕获的坑
//    捕获成员变量时实际捕获的是 this 指针
//    Widget 销毁后 this 悬空，不是真正的值捕获
//    解决：先把成员变量拷贝到局部变量再捕获
//
// 3. 显式捕获多个变量
//    逗号隔开：[a, b, &c]
//    变量太多时打包成结构体捕获
//
// Item32：广义 lambda 捕获（C++14）
//
// 4. 语法：[新变量名 = 表达式]
//    左边是 lambda 内部用的名字
//    右边是初始化表达式，可以是 std::move
//
// 5. 主要用途
//    把 unique_ptr 等只能移动的对象移动进 lambda
//    捕获时直接计算表达式
//    捕获成员变量的拷贝（不捕获 this）
//
// ============================================================

int main()
{
    part1_ref_capture_dangling();
    part3_explicit_multi_capture();
    part4_generalized_capture();
    part5_safe_default_capture();
    return 0;
}