#include "GUITask.h"
#include "Window.h"
#include "Font.h"

void BaseGUITask(void) {
	QWORD WindowID;
	int MouseX;
	int MouseY;
	int WindowWidth;
	int WindowHeight;
	EVENT ReceivedEvent;
	MOUSEEVENT *MouseEvent;
	KEYEVENT *KeyEvent;
	WINDOWEVENT *WindowEvent;
	if(IsGraphicMode() == FALSE) {
		Printf(0x17 , "Sorry, This task can run only GUI Mode.\n");
		return;
	}
	GetCursorPosition(&MouseX , &MouseY);
	WindowWidth = 200;
	WindowHeight = 100;
	WindowID = CreateWindow(MouseX-10 , MouseY-WINDOW_TITLEBAR_HEIGHT/2 , WindowWidth , WindowHeight , WINDOW_FLAGS_DEFAULT , "Test Demo Window");
	if(WindowID == WINDOW_INVALIDID) {
		return;
	}
	while(1) {
		if(ReceiveEventFromWindowQueue(WindowID , &ReceivedEvent) == FALSE) {
			Sleep(0);
			continue;
		}
		switch(ReceivedEvent.Type) {
			case EVENT_MOUSE_MOVE:
			case EVENT_MOUSE_LBUTTONDOWN:
			case EVENT_MOUSE_LBUTTONUP:
			case EVENT_MOUSE_RBUTTONDOWN:
			case EVENT_MOUSE_RBUTTONUP:
			case EVENT_MOUSE_MBUTTONDOWN:
			case EVENT_MOUSE_MBUTTONUP:
				MouseEvent = &(ReceivedEvent.MouseEvent);
				break;
			case EVENT_KEY_DOWN:
			case EVENT_KEY_UP:
				KeyEvent = &(ReceivedEvent.KeyEvent);
				break;
			case EVENT_WINDOW_SELECT:
			case EVENT_WINDOW_DESELECT:
			case EVENT_WINDOW_MOVE:
			case EVENT_WINDOW_RESIZE:
			case EVENT_WINDOW_CLOSE:
				WindowEvent = &(ReceivedEvent.WindowEvent);
				if(ReceivedEvent.Type == EVENT_WINDOW_CLOSE) {
					DeleteWindow(WindowID);
					return;
				}
				break;
			default:
				break;
		}
	}
}

void DemoGUITask(void) {
	QWORD qwWindowID;
    int iMouseX, iMouseY;
    int iWindowWidth, iWindowHeight;
    EVENT stReceivedEvent;
    MOUSEEVENT* pstMouseEvent;
    KEYEVENT* pstKeyEvent;
    WINDOWEVENT* pstWindowEvent;
    int iY;
    char vcTempBuffer[ 50 ];
    static int s_iWindowCount = 0;
    // �̺�Ʈ Ÿ�� ���ڿ�
    char* vpcEventString[] = { 
            "Unknown",
            "MOUSE_MOVE       ",
            "MOUSE_LBUTTONDOWN",
            "MOUSE_LBUTTONUP  ",
            "MOUSE_RBUTTONDOWN",
            "MOUSE_RBUTTONUP  ",
            "MOUSE_MBUTTONDOWN",
            "MOUSE_MBUTTONUP  ",
            "WINDOW_SELECT    ",
            "WINDOW_DESELECT  ",
            "WINDOW_MOVE      ",
            "WINDOW_RESIZE    ",
            "WINDOW_CLOSE     ",            
            "KEY_DOWN         ",
            "KEY_UP           " };
    RECT stButtonArea;
    QWORD qwFindWindowID;
    EVENT stSendEvent;
    int i;

    //--------------------------------------------------------------------------
    // �׷��� ��� �Ǵ�
    //--------------------------------------------------------------------------
    // MINT64 OS�� �׷��� ���� �����ߴ��� Ȯ��
    if( IsGraphicMode() == FALSE )
    {        
        // MINT64 OS�� �׷��� ���� �������� �ʾҴٸ� ����
        Printf(0x17 , "This task can run only GUI mode~!!!\n" );
        return ;
    }
    
    //--------------------------------------------------------------------------
    // �����츦 ����
    //--------------------------------------------------------------------------
    // ���콺�� ���� ��ġ�� ��ȯ
    GetCursorPosition( &iMouseX, &iMouseY );

    // �������� ũ��� ���� ����
    iWindowWidth = 500;
    iWindowHeight = 200;
    
    // ������ ���� �Լ� ȣ��, ���콺�� �ִ� ��ġ�� �������� �����ϰ� ��ȣ�� �߰��Ͽ�
    // �����츶�� �������� �̸��� �Ҵ�
    SPrintf( vcTempBuffer, "Hello World Window %d", s_iWindowCount++ );
    qwWindowID = CreateWindow( iMouseX - 10, iMouseY - WINDOW_TITLEBAR_HEIGHT / 2,
        iWindowWidth, iWindowHeight, WINDOW_FLAGS_DEFAULT, vcTempBuffer );
    // �����츦 �������� �������� ����
    if( qwWindowID == WINDOW_INVALIDID )
    {
        return ;
    }
    
    //--------------------------------------------------------------------------
    // ������ �Ŵ����� ������� �����ϴ� �̺�Ʈ�� ǥ���ϴ� ������ �׸�
    //--------------------------------------------------------------------------
    // �̺�Ʈ ������ ����� ��ġ ����
    iY = WINDOW_TITLEBAR_HEIGHT + 10;
    
    // �̺�Ʈ ������ ǥ���ϴ� ������ �׵θ��� ������ ID�� ǥ��
    DrawRect( qwWindowID, 10, iY + 8, iWindowWidth - 10, iY + 70, RGB( 0, 0, 0 ),
            FALSE );
    SPrintf( vcTempBuffer, "GUI Event Information[Window ID: 0x%Q]", qwWindowID );
    DrawText( qwWindowID, 20, iY, RGB( 0, 0, 0 ), RGB( 255, 255, 255 ), 
               vcTempBuffer, StrLen( vcTempBuffer ) );    
    
    //--------------------------------------------------------------------------
    // ȭ�� �Ʒ��� �̺�Ʈ ���� ��ư�� �׸�
    //--------------------------------------------------------------------------
    // ��ư ������ ����
    SetRectangleData( 10, iY + 80, iWindowWidth - 10, iWindowHeight - 10, 
            &stButtonArea );
    // ����� �������� �������� �����ϰ� ���ڴ� ���������� �����Ͽ� ��ư�� �׸�
    DrawButton( qwWindowID, &stButtonArea, WINDOW_COLOR_BACKGROUND, 
            "User Message Send Button(Up)", RGB( 0, 0, 0 ) );
    // �����츦 ȭ�鿡 ǥ��
    ShowWindow( qwWindowID, TRUE );
    
    //--------------------------------------------------------------------------
    // GUI �½�ũ�� �̺�Ʈ ó�� ����
    //--------------------------------------------------------------------------
    while( 1 )
    {
        // �̺�Ʈ ť���� �̺�Ʈ�� ����
        if( ReceiveEventFromWindowQueue( qwWindowID, &stReceivedEvent ) == FALSE )
        {
            Sleep( 0 );
            continue;
        }  
        
        // ������ �̺�Ʈ ������ ǥ�õ� ������ �������� ĥ�Ͽ� ������ ǥ���� �����͸�
        // ��� ����
        DrawRect( qwWindowID, 11, iY + 20, iWindowWidth - 11, iY + 69, 
                   WINDOW_COLOR_BACKGROUND, TRUE );  
        
        // ���ŵ� �̺�Ʈ�� Ÿ�Կ� ���� ������ ó��
        switch( stReceivedEvent.Type )
        {
            // ���콺 �̺�Ʈ ó��
        case EVENT_MOUSE_MOVE:
        case EVENT_MOUSE_LBUTTONDOWN:
        case EVENT_MOUSE_LBUTTONUP:            
        case EVENT_MOUSE_RBUTTONDOWN:
        case EVENT_MOUSE_RBUTTONUP:
        case EVENT_MOUSE_MBUTTONDOWN:
        case EVENT_MOUSE_MBUTTONUP:
            Reboot();
            // ���⿡ ���콺 �̺�Ʈ ó�� �ڵ� �ֱ�
            pstMouseEvent = &( stReceivedEvent.MouseEvent );

            // ���콺 �̺�Ʈ�� Ÿ���� ���
            SPrintf( vcTempBuffer, "Mouse Event: %s", 
                      vpcEventString[ stReceivedEvent.Type ] );
            DrawText( qwWindowID, 20, iY + 20, RGB( 0, 0, 0 ), 
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, StrLen( vcTempBuffer ) );
            
            // ���콺 �����͸� ���
            SPrintf( vcTempBuffer, "Data: X = %d, Y = %d, Button = %X", 
                     pstMouseEvent->Point.X, pstMouseEvent->Point.Y,
                     pstMouseEvent->ButtonStatus );
            DrawText( qwWindowID, 20, iY + 40, RGB( 0, 0, 0 ), 
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, StrLen( vcTempBuffer ) );

            //------------------------------------------------------------------
            // ���콺 ���� �Ǵ� ������ �̺�Ʈ�̸� ��ư�� ������ �ٽ� �׸�
            //------------------------------------------------------------------
            // ���콺 ���� ��ư�� ������ �� ��ư ó��
            if( stReceivedEvent.Type == EVENT_MOUSE_LBUTTONDOWN )
            {
                // ��ư ������ ���콺 ���� ��ư�� ���ȴ����� �Ǵ�
                if( IsInRectangle( &stButtonArea, pstMouseEvent->Point.X, 
                                    pstMouseEvent->Point.Y ) == TRUE )
                {
                    // ��ư�� ����� ���� ������� �����Ͽ� �������� ǥ��
                    DrawButton( qwWindowID, &stButtonArea, 
                                 RGB( 79, 204, 11 ), "User Message Send Button(Down)",
                                 RGB( 255, 255, 255 ) );
                    UpdateScreenByID( qwWindowID );
                    
                    //----------------------------------------------------------
                    // �ٸ� ������� ���� �̺�Ʈ�� ����
                    //----------------------------------------------------------
                    // ������ ��� �����츦 ã�Ƽ� �̺�Ʈ�� ����
                    stSendEvent.Type = EVENT_USER_TESTMESSAGE;
                    stSendEvent.Data[ 0 ] = qwWindowID;
                    stSendEvent.Data[ 1 ] = 0x1234;
                    stSendEvent.Data[ 2 ] = 0x5678;
                    
                    // ������ �������� �� ��ŭ ������ �����ϸ鼭 �̺�Ʈ�� ����
                    for( i = 0 ; i < s_iWindowCount ; i++ )
                    {
                        // ������ �������� �����츦 �˻�
                        SPrintf( vcTempBuffer, "Hello World Window %d", i );
                        qwFindWindowID = FindWindowByTitle( vcTempBuffer );
                        // �����찡 �����ϸ� ������ �ڽ��� �ƴ� ���� �̺�Ʈ�� ����
                        if( ( qwFindWindowID != WINDOW_INVALIDID ) &&
                            ( qwFindWindowID != qwWindowID ) )
                        {
                            // ������� �̺�Ʈ ����
                            SendEventToWindow( qwFindWindowID, &stSendEvent );
                        }
                    }
                }
            }
            // ���콺 ���� ��ư�� �������� �� ��ư ó��
            else if( stReceivedEvent.Type == EVENT_MOUSE_LBUTTONUP )
            {
                // ��ư�� ����� ������� �����Ͽ� ������ �ʾ����� ǥ��
                DrawButton( qwWindowID, &stButtonArea, 
                    WINDOW_COLOR_BACKGROUND, "User Message Send Button(Up)", 
                    RGB( 0, 0, 0 ) );
            }            
            break;

            // Ű �̺�Ʈ ó��
        case EVENT_KEY_DOWN:
        case EVENT_KEY_UP:
            // ���⿡ Ű���� �̺�Ʈ ó�� �ڵ� �ֱ�
            pstKeyEvent = &( stReceivedEvent.KeyEvent );

            // Ű �̺�Ʈ�� Ÿ���� ���
            SPrintf( vcTempBuffer, "Key Event: %s", 
                      vpcEventString[ stReceivedEvent.Type ] );
            DrawText( qwWindowID, 20, iY + 20, RGB( 0, 0, 0 ), 
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, StrLen( vcTempBuffer ) );
            
            // Ű �����͸� ���
            SPrintf( vcTempBuffer, "Data: Key = %c, Flag = %X", 
                    pstKeyEvent->ASCIICode, pstKeyEvent->Flags );
            DrawText( qwWindowID, 20, iY + 40, RGB( 0, 0, 0 ), 
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, StrLen( vcTempBuffer ) );
            break;

            // ������ �̺�Ʈ ó��
        case EVENT_WINDOW_SELECT:
        case EVENT_WINDOW_DESELECT:
        case EVENT_WINDOW_MOVE:
        case EVENT_WINDOW_RESIZE:
        case EVENT_WINDOW_CLOSE:
            // ���⿡ ������ �̺�Ʈ ó�� �ڵ� �ֱ�
            pstWindowEvent = &( stReceivedEvent.WindowEvent );

            // ������ �̺�Ʈ�� Ÿ���� ���
            SPrintf( vcTempBuffer, "Window Event: %s", 
                      vpcEventString[ stReceivedEvent.Type ] );
            DrawText( qwWindowID, 20, iY + 20, RGB( 0, 0, 0 ), 
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, StrLen( vcTempBuffer ) );
            
            // ������ �����͸� ���
            SPrintf( vcTempBuffer, "Data: X1 = %d, Y1 = %d, X2 = %d, Y2 = %d", 
                    pstWindowEvent->Area.X1, pstWindowEvent->Area.Y1, 
                    pstWindowEvent->Area.X2, pstWindowEvent->Area.Y2 );
            DrawText( qwWindowID, 20, iY + 40, RGB( 0, 0, 0 ), 
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, StrLen( vcTempBuffer ) );
            
            //------------------------------------------------------------------
            // ������ �ݱ� �̺�Ʈ�̸� �����츦 �����ϰ� ������ �������� �½�ũ�� ����
            //------------------------------------------------------------------
            if( stReceivedEvent.Type == EVENT_WINDOW_CLOSE )
            {
                // ������ ����
                DeleteWindow( qwWindowID );
                return ;
            }
            break;
            
            // �� �� ����
        default:
            // ���⿡ �� �� ���� �̺�Ʈ ó�� �ڵ� �ֱ�
            // �� �� ���� �̺�Ʈ�� ���
            SPrintf( vcTempBuffer, "Unknown Event: 0x%X", stReceivedEvent.Type );
            DrawText( qwWindowID, 20, iY + 20, RGB( 0, 0, 0 ), WINDOW_COLOR_BACKGROUND,
                       vcTempBuffer, StrLen( vcTempBuffer ) );
            
            // �����͸� ���
            SPrintf( vcTempBuffer, "Data0 = 0x%Q, Data1 = 0x%Q, Data2 = 0x%Q",
                      stReceivedEvent.Data[ 0 ], stReceivedEvent.Data[ 1 ], 
                      stReceivedEvent.Data[ 2 ] );
            DrawText( qwWindowID, 20, iY + 40, RGB( 0, 0, 0 ), 
                    WINDOW_COLOR_BACKGROUND, vcTempBuffer, StrLen( vcTempBuffer ) );
            break;
        }

        // �����츦 ȭ�鿡 ������Ʈ
        ShowWindow( qwWindowID, TRUE );
    }
	/*
	QWORD WindowID;
	int MouseX;
	int MouseY;
	int WindowWidth = 500;
	int WindowHeight = 300;
	EVENT ReceivedEvent;
	MOUSEEVENT *MouseEvent;
	KEYEVENT *KeyEvent;
	WINDOWEVENT *WindowEvent;
	int Y;
	char TempBuffer[50];
	static int WindowCount = 0;
	RECT ButtonArea;
	QWORD FindWindowID;
	EVENT SendEvent;
	int i;
	if(IsGraphicMode() == FALSE) {
		Printf(0x17 , "Sorry, This task can run only GUI Mode.\n");
		return;
	}
	GetCursorPosition(&MouseX , &MouseY);
	SPrintf(TempBuffer , "Demo Window - Number %d" , WindowCount++);
	WindowID = CreateWindow(MouseX-10 , MouseY-WINDOW_TITLEBAR_HEIGHT/2 , WindowWidth , WindowHeight , WINDOW_FLAGS_DRAWTITLE|WINDOW_FLAGS_DRAWFRAME|WINDOW_FLAGS_SHOW , TempBuffer);
	if(WindowID == WINDOW_INVALIDID) {
		return;
	}
	Y = WINDOW_TITLEBAR_HEIGHT+10;
	DrawButton(WindowID , &ButtonArea , RGB(0xBB , 0xBB , 0xBB) , "Button-Click Me!!" , RGB(00 , 00 , 00));
	ShowWindow(WindowID , TRUE);
	while(1) {
		if(ReceiveEventFromWindowQueue(WindowID , &ReceivedEvent) == FALSE) {
			Sleep(0);
			continue;
		}
		DrawRect(WindowID , 10 , 10 , FONT_ENGLISHWIDTH+10 , FONT_ENGLISHHEIGHT*20 , WINDOW_COLOR_BACKGROUND , TRUE);
		switch(ReceivedEvent.Type) {
			case EVENT_MOUSE_MOVE:
			case EVENT_MOUSE_LBUTTONDOWN:
			case EVENT_MOUSE_LBUTTONUP:
			case EVENT_MOUSE_RBUTTONDOWN:
			case EVENT_MOUSE_RBUTTONUP:
			case EVENT_MOUSE_MBUTTONDOWN:
			case EVENT_MOUSE_MBUTTONUP:
				MouseEvent = &(ReceivedEvent.MouseEvent);
				if(IsInRectangle(&ButtonArea , MouseEvent->Point.X , MouseEvent->Point.X) == TRUE) {
					DrawButton(WindowID , &ButtonArea , RGB(00 , 0xFF , 00) , "Button-Click Me!!" , RGB(0xFF , 0xFF , 0xFF));
					UpdateScreenByID(WindowID);
					DrawText(WindowID , 10 , 10 , RGB(00 , 00 , 00) , WINDOW_COLOR_BACKGROUND , "Click!!" , 7);
				}
				break;
			case EVENT_KEY_DOWN:
			case EVENT_KEY_UP:
				KeyEvent = &(ReceivedEvent.KeyEvent);
				break;
	        case EVENT_WINDOW_SELECT:
	        case EVENT_WINDOW_DESELECT:
	        case EVENT_WINDOW_MOVE:
	        case EVENT_WINDOW_RESIZE:
	        case EVENT_WINDOW_CLOSE:
	        	WindowEvent = &(ReceivedEvent.WindowEvent);
	        	if(ReceivedEvent.Type == EVENT_WINDOW_CLOSE) {
	        		DeleteWindow(WindowID);
	        		return;
	        	}
	        	break;
	        default:
	        	break;
	    }
	    ShowWindow(WindowID , TRUE);
	}
	*/
}
