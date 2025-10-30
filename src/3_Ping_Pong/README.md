# Ping Pong
## Table tennis but surreal. <br />
![](https://github.com/zinzin-101/OpenGL_Real-Time-Rendering/blob/main/src/3_Ping_Pong/gif/pongshowcase1.gif) <br />
![](https://github.com/zinzin-101/OpenGL_Real-Time-Rendering/blob/main/src/3_Ping_Pong/gif/pongshowcase2.gif) <br />
![](https://github.com/zinzin-101/OpenGL_Real-Time-Rendering/blob/main/src/3_Ping_Pong/gif/pongshowcase3.gif) <br />
![](https://github.com/zinzin-101/OpenGL_Real-Time-Rendering/blob/main/src/3_Ping_Pong/gif/pongshowcase3.gif) <br />
![](https://github.com/zinzin-101/OpenGL_Real-Time-Rendering/blob/main/src/3_Ping_Pong/gif/pongshowcase4.gif) <br />
## Features:<br />
-Player controlled paddle <br />
-Simple opponent AI <br />
-Auto mode<br />
-Collision detection using AABB<br />
-Verlet integration for physics<br />
-4 different camera views that are adjustable <br />
-Gravity toggle<br />
-Pause game<br />
-Rotating the table axis and the gravity<br />
## Controls:<br />
Hold-left-click -> move paddle<br />
Hold-right-click -> change look angle<br />
Hold-left-and-right-click -> adjust camera height<br />
Z/X -> rotate axis<br />
V -> switch camera<br />
B -> toggle camera lerping<br />
4 cameras: Static, Follow (paddle), Follow (ball), Free<br />
WASD/QE -> move in free camera mode<br />
LShift -> increase speed in free camera mode<br />
O -> toggle auto mode (for player paddle)<br />
P / Enter -> toggle pause<br />
G -> toggle gravity<br />
C -> toggle show collider<br />
R -> reset<br />
Up/Down arrows -> increase/decrease mouse sensitivity<br />

## Credits
Some code are modified from [https://learnopengl.com/](https://learnopengl.com/) <br />

The [table](https://free3d.com/3d-model/pingpongtable-v2--618009.html), [ball](https://free3d.com/3d-model/tennis-ball-v1--806429.html) and [paddle](https://free3d.com/3d-model/pingpong-paddle-v1--337871.html) models are from [free3d.com](https://free3d.com/)

## Additional Info
CMake is required to build the project <br />
When starting the program it may take some time to load. <br />