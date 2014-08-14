//分形动画屏保  HouSisong 2008.01.15

//#define WIN32_LEAN_AND_MEAN  // 不使用 MFC

#include <windows.h>
#include "resource.h"
#include "GameSys.h"
#include "MMTime.h"
#include "stdlib.h"

#include "SceneKing.h"
#include "SceneMira.h"
#include "SceneSpotFormat.h"
#include "SceneJulia.h"
#include "SceneNewton.h"

void AddScenes(CGameSys& GameSys)
{
    //添加场景
#ifdef OUT_SCR_Newton
    GameSys.AddGameScene(new CSceneNewton());
#else
    GameSys.AddGameScene(new CSceneKing());
    GameSys.AddGameScene(new CSceneMira());
    GameSys.AddGameScene(new CSceneSpotFormat());
    GameSys.AddGameScene(new CSceneJulia());
    GameSys.AddGameScene(new CSceneNewton());
#endif
}


bool g_WindowProc_isRunAsDebug=false;

//系统消息处理函数
LRESULT CALLBACK WindowProc(HWND main_window_handle, UINT msg,  WPARAM wparam,  LPARAM lparam)
{
    switch(msg)
	{	
	case WM_CREATE: 
        {
		    //初始化
            //todo: 
		    return 0;
		} break;
	case WM_PAINT: 
		{
            /*PAINTSTRUCT	ps;		// WM_PAINT消息用
   	        HDC	hdc = BeginPaint(main_window_handle,&ps);	 // 绘图句柄
            TextOut(hdc,rand()*800/RAND_MAX,rand()*600/RAND_MAX,"afsdgfergdgdf",10);
            EndPaint(main_window_handle,&ps);*/
		    return 0;
   		} break;
	case WM_DESTROY: 
		{
		    PostQuitMessage(0);// 结束程序，发送 WM_QUIT 消息
		    return 0;
		} break;
    case WM_KILLFOCUS: 
        {
            if (!g_WindowProc_isRunAsDebug)
                PostMessage(main_window_handle,WM_SYSCOMMAND,SC_MAXIMIZE,0);//阻止被最小化
        } break;
	default:
        break;
    } 

    //分派未处理消息
    return (DefWindowProc(main_window_handle, msg, wparam, lparam));
} 


///////////////////////////////////////////////////////////

bool DisposeCommonline(HINSTANCE hinstance,LPSTR lpcmdline,const char* Caption,bool& out_IsRunAsDebug)
{
    //处理运行参数
    //MessageBox(0,lpcmdline,"",0);
    //return false;

    out_IsRunAsDebug=false;
    if (strlen(lpcmdline)>0)
    {
	    
	    if (strcmp(lpcmdline,"/debug")==0)
	    {
		    //调试运行
            out_IsRunAsDebug=true;
            return true;
	    }
        else if (strcmp(lpcmdline,"/S")==0)
	    {
		    //正常运行\测试运行
            return true;
	    }
	    else if (strcmp(lpcmdline,"/s")==0)
	    {
		    //预览运行\屏保启动
		    //按默认值运行
            return true;
	    }
	    else if ((strcmp(lpcmdline,"/c    ")>0)&&
		    (strcmp(lpcmdline,"/czzzz")<0))
	    {
		    //设置
		    MessageBox(0,"\n no setting !\n",Caption,MB_OK+MB_ICONINFORMATION);
		    return false;
	    }
	    else if ((strcmp(lpcmdline,"/p   ")>0)&&
		         (strcmp(lpcmdline,"/pzzz")<0))
	    {
		    //安装\小屏幕预览
		    char * str=lpcmdline+2;
		    //MessageBox(0,str,Caption,MB_OK+MB_ICONINFORMATION);
		    HWND shwnd=(HWND)atoi(str);
		    HDC shdc,phdc;
		    shdc=::GetWindowDC(shwnd);
		    phdc=::GetWindowDC(GetDesktopWindow());

		    RECT rect;
		    ::GetWindowRect(shwnd,&rect);
		    RECT rectp;
		    ::GetWindowRect(GetDesktopWindow(),&rectp);
		    ::StretchBlt(shdc,0,0,rect.right-rect.left,rect.bottom-rect.top,
			             phdc,0,0,rectp.right-rectp.left,rectp.bottom-rectp.top,
					     SRCCOPY);
    		
		    ::DrawIcon(shdc,(rect.right-rect.left-32)/2,(rect.bottom-rect.top-32)/2,
			    ::LoadIcon(hinstance,MAKEINTRESOURCE(IDI_MAINICON)) );
		    ::TextOut(shdc,0,0,"KingBlizzard",12);
		    ::ReleaseDC(GetDesktopWindow(),phdc);	
		    ::ReleaseDC(shwnd,shdc);	
		    return false;
	    }
	    else  
	    {
		    // 命令行参数错误
		    MessageBox(0,"\nThe Command Line is Error!\n",Caption,MB_OK+MB_ICONERROR);
		    return false;
	    }
    }
    else
    {
        return true;
    }
}



inline bool KEYDOWN(int vk_code)  {  return ( 0!=(GetAsyncKeyState(vk_code) & 0x8000) );  }

inline long GetScreenColorBit()
{
    HWND dHWND=GetDesktopWindow();
    HDC dc=GetDC(dHWND);
    long result=GetDeviceCaps(dc,BITSPIXEL);
    ReleaseDC(dHWND,dc);
    return result;
}

// WINMAIN 函数////////////////////////////////////////////////

int WINAPI WinMain(	HINSTANCE hinstance,
					HINSTANCE hprevinstance,
					LPSTR lpcmdline,
					int ncmdshow)
{
    const char* Caption="King Blizzard";  //窗口标题

    bool isRunAsDebug=false;
    if (!DisposeCommonline(hinstance,lpcmdline,Caption,isRunAsDebug)) return 1;

    g_WindowProc_isRunAsDebug=isRunAsDebug;
    
    //获得屏幕默认设置  也可以默认 1024x768 32bitRGB;
    long SCREEN_WIDTH=GetSystemMetrics(SM_CXSCREEN);   
    long SCREEN_HEIGHT=GetSystemMetrics(SM_CYSCREEN); 
    long SCREEN_COLORBIT=GetScreenColorBit(); 

    //根据fps自动调整运行质量
    //todo:
    /*long MIN_FPS=20;
    long RUN_Quality=100;
    */

    const char* const WINDOW_CLASS_NAME="WINCLASS_King_Blizzard";

    WNDCLASSEX winclass;
    //填充WNDCLASSEX结构
    memset(&winclass,0,sizeof(winclass)); 
    winclass.cbSize         = sizeof(WNDCLASSEX);
    winclass.style			= CS_DBLCLKS | CS_OWNDC | 
                              CS_HREDRAW | CS_VREDRAW;
    winclass.lpfnWndProc	= WindowProc;
    winclass.cbClsExtra		= 0;
    winclass.cbWndExtra		= 0;
    winclass.hInstance		= hinstance;
    winclass.hIcon			= LoadIcon(hinstance, MAKEINTRESOURCE(IDI_MAINICON));
    winclass.hCursor		= LoadCursor(hinstance, MAKEINTRESOURCE(IDC_ARROW)); 
    winclass.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
    winclass.lpszMenuName	= 0;
    winclass.lpszClassName	= WINDOW_CLASS_NAME;
    winclass.hIconSm        = LoadIcon(hinstance, MAKEINTRESOURCE(IDI_MAINICON));

    // 注册窗体类
    if (!RegisterClassEx(&winclass))
	    return 1;

    HWND	   main_window_handle=0;	 // 主窗体句柄
    // 创建窗体
    if (!(main_window_handle = CreateWindowEx(0,                     // 扩展风格
                                WINDOW_CLASS_NAME,     // 类名
						        Caption,               // 标题
						        WS_POPUP | WS_VISIBLE,
					 	        0,0,	               // 初始坐标
						        SCREEN_WIDTH,SCREEN_HEIGHT,  // 初始大小
						        0,	                   // 父窗口句柄 
						        0,	                   // 菜单句柄
						        hinstance,             // 主程序句柄
						        0)))	               // 扩展创建参数
    {
        return 1;
    }


    //隐藏鼠标
	ShowCursor(false);

    CGameSys GameSys;

    // 初始化游戏
    if (!GameSys.Game_Init(SCREEN_WIDTH,SCREEN_HEIGHT,SCREEN_COLORBIT,main_window_handle,isRunAsDebug)) return 1;
    AddScenes(GameSys);//添加不同的场景 然后随机启动一个和切换

    GameSys.Game_Loop();//预先执行一次游戏循环
    unsigned long Game_runStartTime=mmGetTime();

    //进入主事件循环
    while(true)
    {
        MSG	msg; // 消息
        // 检测消息队列，有消息则得到它
        if (PeekMessage(&msg,0,0,0,PM_REMOVE))
        { 
            if (msg.message == WM_QUIT)  break; // 处理退出消息
            
            if (!isRunAsDebug)
            {   //检测鼠标和键盘事件
                unsigned long Game_runNowTime=mmGetTime();
                if ((msg.message>= WM_MOUSEFIRST)&&(msg.message <= WM_MOUSELAST))
                {
                    //捕获到鼠标消息并且已经运行了一定时间后 退出程序
                    if (Game_runNowTime-Game_runStartTime>500) 
                    {
                        static POINT msPos0;
                        static bool IsInti_msPos0=false;
                        if (!IsInti_msPos0)
                        {
                            GetCursorPos(&msPos0);
                            IsInti_msPos0=true;
                        }
                        else
                        {
                            if (msg.message==WM_MOUSEMOVE)
                            {
                                POINT msPos;
                                GetCursorPos(&msPos);
                                if (abs(msPos.x-msPos0.x)+abs(msPos.y-msPos0.y)>10)
                                    break;
                            }
                            else
                                break;
                        }
                    }
                }
                else if ((msg.message >= WM_KEYFIRST)&&(msg.message <= WM_KEYLAST))
                {
                    //按屏保运行,捕获键盘消息时退出程序
                    unsigned long Game_runNowTime=mmGetTime();
                        //捕获到鼠标消息并且已经运行了一定时间后 退出程序
                    if (Game_runNowTime-Game_runStartTime>500) 
                        break; 
                }
            }

            // 转化处理加速键
            TranslateMessage(&msg);

            // 发送消息到 window proc
            DispatchMessage(&msg);
        } // end if


        static bool is_window_closed=false;
        if (!is_window_closed)
        {
            //处理 压 ESC 发送 WM_CLOSE 消息，使程序退出
            if (KEYDOWN(VK_ESCAPE))
            {
                is_window_closed = true;
                PostMessage(main_window_handle,WM_CLOSE,0,0);
            }

            GameSys.Game_Loop();//游戏循环
        }
        mmSleep(0);
    }

    GameSys.Game_Shutdown();// 退出游戏
    
    return 0;// 退出
}

///////////////////////////////////////////////////////////





