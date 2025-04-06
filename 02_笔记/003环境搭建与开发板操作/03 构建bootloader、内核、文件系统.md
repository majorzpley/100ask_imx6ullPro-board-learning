- 编译系统
```shell
bug: arm-buildroot-linux-gnueabihf-g++.br_real: internal compiler error: Killed (program cc1plus)
```
- **(注意，经测试在ubuntu20.04上编译buildroot需要内存在8G左右，否则编译不成功)**
- 下面以 100ask_imx6ull_pro_ddr512m_systemV_qt5_defconfig 配置文 件为例，在 ubuntu 终端上说明 Buildroot 的配置过程：
```shell
cd /home/book/100ask_imx6ull-sdk
book@100ask:~/100ask_imx6ull-sdk$ cd Buildroot_2020.02.x
book@100ask:~/100ask_imx6ull-sdk/Buildroot_2020.02.x$ make clean
book@100ask:~/100ask_imx6ull-sdk/Buildroot_2020.02.x$ make 100ask_imx6ull_pro_ddr512m_systemV_qt5_defconfig
book@100ask:~/100ask_imx6ull-sdk/Buildroot_2020.02.x$ make all -j$(nproc)
```