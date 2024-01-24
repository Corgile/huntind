# HD

修改 `vendor/libpcap/pcap-linux.c` :

```diff
 if (handle->opt.buffer_size == 0) {
     /* by default request 2M for the ring buffer */
-    handle->opt.buffer_size = 2*1024*1024;
+    handle->opt.buffer_size = 100*1024*1024;
 }
```

```shell
mkdir build 
cd build 
cmake -DBUILD_MODE:STRING=LIVE_MODE,DEAD_MODE,SEND_KAFKA,HD_DEBUG,HD_BENCH ..
```

## TODO

- [x] 时间戳为什么解析突然不正确了
  - 解决方案：把packet复制出来而不是用指针
- [x] synced_stream 只保证在调用一次 << 的情况下输出到文件的正确位置，两次函数调用可能会丢掉锁
- [x] 回归测试 
- [ ] 重复代码太多，思考如何重构一下