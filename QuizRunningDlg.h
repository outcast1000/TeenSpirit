#pragma once


#include "MediaPlayer.h"
#include "MillionaireButton.h"
#include "LabelEx.h"
#include "CaptionBordStatic.h"
#include "GuiControls/BCMenu.h"
#include "CountDownCtrl.h"
#include "XHTMLStatic.h"
//#include "TSTranslatable.h"


class CQuizRunningDlg : public CDialog//, public TSTranslatable
{
	DECLARE_DYNAMIC(CQuizRunningDlg)

public:
	struct GameParameters
	{
		LPCTSTR gameName;
		INT totalTime;
		INT penaltyTime;
		INT bonusTime;
	};

	CQuizRunningDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CQuizRunningDlg();
// Dialog Data
	enum { IDD = IDD_QUIZ };

protected:

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()
private:
//	void ApplyTranslation(ITranslation& translation);
	void EvaluateAnswer(INT qa);

	enum FormStates
	{
		ST_Start,
		ST_RunningQuestions,
		ST_DisplayingResultsAndWait,
		ST_DisplayingQuestionsAndWait,
		ST_GameOver
	};

	//void SetButtonsState(BOOL bEnable, CMillionaireButton::States State = CMillionaireButton::ST_Last);
	void SetState(FormStates st);
private:
	void ConfugureTextControl(CLabelEx& bs, 
		COLORREF bk, COLORREF fg, INT resText);

	MediaPlayer* m_pPlayer;
	CCountDownCtrl m_progress;

	//CMillionaireButton m_X;
	//CMillionaireButton m_btn[4];
	CLabelEx m_title;
	CLabelEx m_subTitle;
	CXHTMLStatic m_info;
	CXHTMLStatic m_result;

	afx_msg void OnDestroy();
	static LPCTSTR SettingsCategory;


	
	GameParameters m_curGame;
	INT m_lastResponseTime;
	INT m_curGameIdx;
	INT m_curGameBestScore;
	std::tstring m_curGameBestScoreDate;
	FormStates m_curState;
	

	INT m_curTime;
	INT m_correctAnswer;
	INT m_correctAnswers;
	INT m_totalAnswers;
	UINT m_LastTickCount;
	UINT m_gameTimeStamp;

	BOOL DisplayNextQuestion();
	void DisplayRunningResult();
	void GameOver();
	void SetCurrentGame(INT gameIdx);
	void HideAll();

	//BCMenu m_options;
	INT m_poolCount;
	CFont m_largeFont;
	CFont m_normalFont;
	CFont m_smallFont;


public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	enum BUTTONS
	{
		BT_Start,
		BT_SelectPrev,
		BT_SelectNext,
		
		BT_Answer1,
		BT_Answer2,
		BT_Answer3,
		BT_Answer4,
		BT_AnswerNo,
		BT_EndGame,
		
		BT_NewGame,
		BT_Submit,

		BT_Exit,
		
		BT_Last
	};
	CMillionaireButton* m_pButtons[BT_Last];
};
