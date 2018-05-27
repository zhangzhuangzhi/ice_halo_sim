# C++ 代码

[English version](README.md)

matlab 代码只是一个原型, 并没有充分考虑性能, 因此我专门写了一个 C++ 项目, 以期获得高性能表现.
目前 C++ 版本没有用户界面, 只能从命令行启动.

代码使用了 [Halide](http://halide-lang.org/) 库, 以获得并行加速.

## 构建和运行

本项目使用 [CMake](https://cmake.org/) 构建. 简单的构建过程如下:

1. `cd cpp`
2. `mkdir build && cd build`
3. `cmake .. && make -j4`, 或者设置 `CMAKE_BUILD_TYPE` 为 `release` 以获得高性能可执行程序.

生成的可执行程序位于 `build/bin`. 在命令行输入 
`./bin/IceHaloSim <path-to-your-config-file>` 即可运行.
这里有一个 [`cpp/config.json`](./config.json) 文件作为输入配置的样本, 可供参考.

## 配置文件

配置文件中包含了用于模拟的所有参数, 配置文件使用 JSON 格式, 这里进行简单介绍. 
配置文件第一层是一个对象, 必须包含 `sun`, `ray_number`,
`max_recursion`, `crystal` 这几项.

### 模拟的基本设置

* `sun`:
只有一个变量, `altitude`, 用于定义太阳的地平高度.

* `ray_number`:
定义了用于模拟的总光线数量. 请注意, 这里光线数量并不是最终输出的光线数量, 而是输入光线数量. 由于光线在晶体内部进行折射和反射,
在模拟中对所有的折射和反射光线都进行记录, 因此最终的输出光线数量将大于这里定义的值.

* `max_recursion`:
定义了在模拟中光线与晶体表面相交的最多次数. 如果模拟中光线与晶体表面相交次数超过这个值, 而仍然没有离开晶体,
那么对这条光线的模拟将终止, 这条光线的结果将被舍弃.

### 晶体设置

* `type` 和 `parameter`:
用于设置晶体的类型和形状参数. 目前我支持 5 中晶体,
`HexCylinder`, `HexPyramid`, `HexPyramidStackHalf`, `TriPyramid`, `CubicPyramid`.
每种晶体都有自己的形状参数.

  * `HexCylinder`: 六棱柱形冰晶.
  只有 1 个参数, `h`, 定义为 `h / a`, 其中 `h` 是柱体的高, `a` 是底面直径.
  * `HexPyramid`: 六棱锥形冰晶.
  可能有 3, 5, 或者 7 个参数. (具体含义等我有空了补充)
  * `HexPyramidStackHalf`:
  有 7 个参数. (具体含义等我有空了补充)
  * `TriPyramid`:
  有 5 个参数. (具体含义等我有空了补充)
  * `CubicPyramid`:
  有 2 个参数. (具体含义等我有空了补充)

* `axis` and `roll`:
这两个参数定义了晶体的朝向, 其中 `axis` 定义了 c-轴 的指向,`roll` 定义了晶体自身绕 c-轴 的旋转.

  这两个参数都有 3 个属性, `mean`, `std`, `type`. 其中 `type` 定义了随机分布的类型. 要么是 `Gauss`, 
代表高斯分布, 要么是 `Uniform`, 代表平均分布. `mean` 定义了随机分布的均值. 对 `axis` 来说, 
这个值代表天顶角, 从天顶开始度量.
`std` 定义了随机分布的范围. 对高斯分布来说, 这个值就是高斯分布的标准差,
对平均分布来说, 这个值定义了取值范围的大小.

  所有角度单位都是度.

* `population`:
这个参数定义了用于模拟的晶体数量. 请注意, 这里并不是实际数量, 而是一个比例. 比如两种晶体一种是 2.0 一种是 3.0,
那么这等价于一种是 20 另一种是 30.

## 未来的工作

* 使用 OpenCL / OpenGL / CUDA 等对程序进行加速. 不过, 目前使用 Halide 加速的结果已经很好, 进一步加速的必要性并不大.
* 写一个用户界面.