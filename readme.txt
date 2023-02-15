使用opencv库

mkdir build
cd build
cmake ..
make

运行:
./Label ../tiny.png 
第二个参数是图片相对路径

运行后先点击图片，按s后命令行输入围成多边形内部的label(非0整数如1），回车后结束此多边形的标注
可以画多个多边形

按Esc退出
输出在output.txt，可以直观看出多边形形状，多边形内部是标注的label，具体输出格式可以再调整 .
