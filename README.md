# 文件管理器

<img src=".\image\filemanager.png" alt="filemanager" style="zoom: 80%;" />



## 简介

本工程为运行在Windows系统上的文件管理器Demo（Qt Widgets应用），通过动态链接的方式调用Qt库；目前完全通过Qt提供的接口实现，未调用Windows库。

控件/功能如下：

1. 菜单栏-退出/开关导航栏/开关附加文件界面/添加标签页/语言切换（中/英文）/关于
2. 导航栏
3. 附加文件界面
4. 路径栏
5. 历史路径-记录浏览路径，提供历史路径切换功能
6. 文件界面-右键菜单/多标签页
7. 快捷键-剪切/复制/粘贴/删除/刷新/展开文件夹/折叠文件夹
8. 信息栏（状态栏）
9. 自动保存加载应用信息



### 演示视频

[https://www.bilibili.com/video/BV1ng411L7gx](https://www.bilibili.com/video/BV1ng411L7gx)



## 使用须知

1. 不显示隐藏文件；
2. 剪切、复制、粘贴及粘贴快捷方式操作与系统文件管理器的操作不互通；
3. 移动或复制文件时，可以通过进度框结束当前操作，但已执行的操作不会撤回；
4. 删除的文件和粘贴时替换的文件会移动到系统回收站中；
5. 移动文件或文件夹时，指向它的快捷方式将不会被修改，即文件或文件夹移动后原来的快捷方式将失效；
6. 粘贴快捷方式的快捷方式将会创建原快捷方式的目标文件的快捷方式；
7. 快捷方式的目标文件不存在时，不允许剪切或复制，只提供删除操作；
8. 不能重命名快捷方式（重命名快捷方式时会提示"Invalis filename"；当快捷方式的目标文件不存在时，快捷键无法触发重命名操作）；
9. 无法批量重命名；
10. 状态栏的项目数量信息仅显示当前文件夹下的项目数，不包括".."和展开的文件夹下的项目；已选择项目数量包括所有选中的项（会出现文件夹全选时，选中项比当前文件夹项目数量多1的情况）；
11. 每个文件界面最多允许开启10个标签页；
12. 历史记录最大数量为20条记录；
13. 拔插U盘或硬盘时无法动态检测更新盘符信息，需要重启应用。



### **重要提示**

1. 由于暂不支持撤销操作，请尽量避免对重要文件进行重命名、拖放、及剪切粘贴操作，防止文件被误删无法找回；可以先将文件复制到目标路径，确认复制无误后再删除原始文件。



### 快捷键

| 按键   | 操作                                        |
| ------ | ------------------------------------------- |
| Ctrl+X | 剪切                                        |
| Ctrl+C | 复制                                        |
| Ctrl+V | 粘贴                                        |
| Ctrl+E | 展开/折叠选中文件夹                         |
| Ctrl+R | 展开/折叠选中路径（默认当前路径）所有文件夹 |
| Ctrl+G | 显示/隐藏导航栏                             |
| Ctrl+I | 显示/隐藏附加文件界面                       |
| Ctrl+T | 添加标签页                                  |
| Ctrl+Q | 退出应用                                    |



## 工程说明

Qt版本：Qt 5.15.2

构建套件：Desktop Qt 5.15.2 MinGW 64-bit

在Qt Creator导入工程文件，选择5.15.2版本构建套件编译运行。



### 注意事项

1. 编译前需要生成翻译文件（.qm），Qt Creator菜单栏->工具->外部->Qt语言家->发布翻译(lrelease)；确认生成的.qm文件在src\translations目录下且名称为FileManager_zh_CN.qm，保证资源文件（.qrc）能够正常进行调用。

2. 工程文件已关闭qDebug()输出，启用输出需要删除或注释.pro中的QT_NO_DEBUG_OUTPUT定义。

   ```
   DEFINES += QT_NO_DEBUG_OUTPUT
   ```

3. Qt中不同接口对快捷方式的处理方式存在差异，某些接口处理快捷方式时会直接操作快捷方式的目标文件，调用文件操作类接口时需要注意处理目标为快捷方式的情况。



## 设计信息

### 文件界面

```c++
void FileWidget::widgetLayoutInit()
```

<img src=".\image\layout.png" alt="layout" style="zoom:80%;" />



### 主菜单

```c++
void MainWindow::setupMenus()
```

<img src=".\image\pull-down menu.png" alt="pull-down menu" style="zoom: 50%;" />



### 上下文菜单（右键菜单）

```c++
void FileWidget::contextMenu(const QPoint &pos)
```

<img src=".\image\context menu.png" alt="context menu" style="zoom:50%;" />



### 路径切换（文件界面）控制

<img src=".\image\control.png" alt="control" style="zoom:50%;" />



## License

MIT license，参见 [LICENSE](LICENSE.md).
