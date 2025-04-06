# POLL机制
## 驱动编程
- 使用poll机制时，驱动程序的核心就是提供对应的drv_poll函数。
- 在drv_poll函数中要做两件事:
    - 1.把当前线程挂入队列wq:poll_wait
    APP调用一次poll，可能导致drv_poll被调用两次，但是我们不需要把当前线程挂入队列两次。可以使用内核的函数poll_wait把线程挂入队列，如果线程已经在队列中，它就不会再次挂入。
    - 2.返回设备状态：
        APP调用poll函数时，有可能查询“有没有数据可以读”：POLLIN，也有可能是查询“你有没有空间给我写数据”：POLLOUT。
        所以drv_poll要返回自己的当前状态:(POLLIN|POLLOUT|POLLRDNORM) 或 (POLLOUT|POLLWRNORM)。
        - a. POLLRDNORM 等同于 POLLIN，为了兼容某些 APP 把它们一起返回。
        - b. POLLWRNORM 等同于 POLLOUT ，为了兼容某些 APP 把它们一起返回。
- APP 调用 poll 后，很有可能会休眠。对应的，在按键驱动的中断服务程序中，也要有唤醒操作。
驱动程序中 poll 的代码如下：
```c
static unsigned int gpio_key_drv_poll(struct file *fp, poll_table * wait)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    poll_wait(fp, &gpio_key_wait, wait);
    return is_key_buf_empty() ? 0 : POLLIN | POLLRDNORM;
}
```
## 应用编程
- poll/select 函数可以监测多个文件，可以监测多种事件：

|事件类型|说明|
|---------------|-----------------|
|POLLIN|有数据可读|
|POLLRDNORM|等同于POLLIN|
|POLLRDBAND|Priority band data can be read，有较高优先级的"band data"可读，Linux系统中很少使用这个事件|
|POLLPRI|高优先级数据可读|
|POLLOUT|可以写数据|
|POLLWRNORM|等同于POLLOUT|
|POLLWRBAND|Priority data may be written|
|POLLERR|发生了错误|
|POLLHUP|挂起|
|POLLNVAL|无效的请求，一般是fd未open|

在调用poll函数时，要指明：
- 1.你要监测哪一个文件：哪一个fd
- 2.你想监测这个文件的哪种事件：是POLLIN、还是POLLOUT


        