# mmap基础知识
- 应用程序和驱动程序之间传递数据时，可以通过 read、 write 函数进行。这
涉及在用户态 buffer 和内核态 buffer 之间传数据，如下图所示：
![alt text](../pic/mmap.png)
## 内存映射现象与数据结构
![alt text](../pic/mmap内存映射.png)
- 这就是 pfn。假设每页大小是 4K，那么给定物理地址 phy，它的 pfn = phy /
4096 = phy >> 12。内核的 page 一般是 4K，但是也可以配置内核修改 page
的大小。所以为了通用，pfn = phy >> PAGE_SHIFT。
- APP 调用 mmap 后，会导致驱动程序的 mmap 函数被调用，最终 APP 的虚拟
地址和驱动程序中的物理地址就建立了映射关系。APP 可以直接访问驱动程序的
buffer。