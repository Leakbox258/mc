| 目标                     | 命令                                 | 说明                 |
| ------------------------ | ------------------------------------ | -------------------- |
| ELF 头信息               | `readelf -h foo.o`                   | 文件基本结构         |
| 节表                     | `readelf -S foo.o`                   | 所有节及其偏移、大小 |
| 符号表                   | `readelf -s foo.o`                   | 所有符号定义与引用   |
| 重定位表                 | `readelf -r foo.o`                   | 哪些地方引用符号     |
| 节内容                   | `readelf -x .text foo.o`             | 查看机器码           |
| 所有信息                 | `readelf -a foo.o`                   | 一键全景视图         |
| 汇编+机器码              | `riscv64-linux-gnu-objdump -d foo.o` | 反汇编               |
| 节与符号信息（人类友好） | `riscv64-linux-gnu-objdump -x foo.o` | 类似 readelf -a      |
| Python 脚本读取          | `pyelftools`                         | 程序化分析           |
