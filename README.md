# HD

```shell
mkdir build 
cd build 
cmake -DBUILD_MODE:STRING=LIVE_MODE,DEAD_MODE,SEND_KAFKA,HD_DEBUG,HD_BENCH ..
```

## TODO

- [x] 时间戳解析
  - 解决方案：把packet复制出来而不是用指针
- [x] 回归测试 
- [ ] 重复代码太多，思考如何重构一下
