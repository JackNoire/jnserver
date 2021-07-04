# 编译方法

```shell
make
```

# 运行方法

程序需要在安装了php-cgi后才能运行，在ubuntu中安装php-cgi的命令为：

```shell
sudo apt install php-cgi
```
安装后需要保证php-cgi在/usr/bin/目录中，可以使用whereis命令查看php-cgi的路径：

```shell
whereis php-cgi
> php-cgi: /usr/bin/php-cgi7.2 /usr/bin/php-cgi /usr/share/man/man1/php-cgi.1.gz

```
如果php-cgi位于其他路径，需要在cgi.c中修改代码：
```c
        argv[0] = "/usr/bin/php-cgi";
        argv[1] = "-f";
        argv[2] = filepath;
```
运行程序的命令为：

```shell
./jnserver [-r 根目录] [-p 端口号]
```

# 参考资料

[shenfeng/tiny-web-server](https://github.com/shenfeng/tiny-web-server)

[Linux下getopt()函数的简单使用](https://www.cnblogs.com/qingergege/p/5914218.html)

[HTTP 响应代码](https://developer.mozilla.org/zh-CN/docs/Web/HTTP/Status)

[Reading directory content in linux](https://stackoverflow.com/questions/5734802/reading-directory-content-in-linux)

[socket - close(2) send RST packet instead of FIN packet](https://stackoverflow.com/questions/54937761/socket-close2-send-rst-packet-instead-of-fin-packet)