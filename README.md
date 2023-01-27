# 期末项目之冬津羽戏
## 1. 操作系统: Win11 x64
## 2. C语言环境: GCC11.2.0
## 3. 编译选项: CMakeLists
## 4. 第三方库: winsock2, pthread, SDL, SDL_ttf, SDL_image, SDL_mixer
## 5. 游戏方式:
### 单人游戏
#### 1. 双击client.exe或start.bat打开客户端程序
#### 2. 单击"帮助"按钮查看帮助文档(可以跳过)
#### 3. 单击"单人"按钮开始单人游戏
### 双人游戏
#### 1. 双击server.exe打开**服务端**程序
#### 2. 检查"cfg/cfg.txt"中ServerIP,ServerPort的值与服务端显示的是否一致
#### 3. 如果不一致，将"cfg/cfg.txt"中的值改为服务端显示的值
#### 4. 双击client.exe或start.bat打开**客户端**程序
#### 5. 单击"帮助"按钮查看帮助文档(可以跳过)
#### 6. 单击"双人"按钮开始双人游戏
#### 7. 等待好友加入(窗口中出现第二个挡板以及砖块时就代表好友已加入)
## 6. 操作方式:
### 1. 发射前，鼠标单击窗口中的某个点，该点将会成为小球移动的方向
### 2. 确定方向以后，按下Enter(回车)或Space(空格)以发射小球
### 3. 如果小球死亡(碰到窗口底端)，将会重置球和挡板的位置，按照步骤1,2操作以再次发射小球
### 2. 游戏过程的任意时刻都可以单击"返回"按钮以返回主界面
## 6. 注意事项:
### 1. 本游戏只能局域网联机，不支持掉线重连
### 2. client.exe和start.bat两种打开方式区别不大，不影响正常游戏
### 3. 确保程序运行时使用英文输入法，否则不能使用WASD移动小球
### 4. 挡板只有顶端能碰撞