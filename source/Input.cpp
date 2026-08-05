//*****************************************************************************
// File: Input.cpp
//*****************************************************************************

#include "stdafx.h"
#include "Input.h"
#include "UsefulDef.h"
#include "./Time/Timer.h"
#if	defined WINDOWMODE
#include "UIManager.h"
extern bool g_bWndActive;
#endif
extern CTimer*	g_pTimer;

CInput::CInput()
{

}

CInput::~CInput()
{

}

CInput& CInput::Instance()
{
	static CInput s_Input;                  
    return s_Input;
}

bool CInput::Create(Platform::NativeWindowHandle hWnd, long lScreenWidth, long lScreenHeight)
{
	if (NULL == hWnd)
		return false;

	m_hWnd = hWnd;
	if (!Platform::GetInputBackend().BindWindow(m_hWnd))
		return false;

	m_lScreenWidth = lScreenWidth;
	m_lScreenHeight = lScreenHeight;

	Platform::PointerSnapshot pointer;
	Platform::GetInputBackend().ReadPointer(pointer);
	m_ptCursor.x = pointer.x;
	m_ptCursor.y = pointer.y;
	m_ptFormerCursor = m_ptCursor;

	m_bLeftHand = false;

	m_dDoubleClickTime = (double)Platform::GetInputBackend().GetDoubleClickTimeMilliseconds();

	m_dBtn0LastClickTime = m_dBtn1LastClickTime = m_dBtn2LastClickTime = 0.0;

	m_bFormerBtn0Dn = m_bFormerBtn1Dn = m_bFormerBtn2Dn = false;

	m_bLBtnHeldDn = m_bRBtnHeldDn = m_bMBtnHeldDn = false;

	m_bTextEditMode	= false;

	return true;
}

void CInput::Update()
{
	Platform::PointerSnapshot pointer;
	Platform::GetInputBackend().ReadPointer(pointer);


	m_lDX = m_lDY = 0L;
	m_bLBtnUp = m_bRBtnUp = m_bMBtnUp
		= m_bLBtnDn = m_bRBtnDn = m_bMBtnDn
		= m_bLBtnDbl = m_bRBtnDbl = m_bMBtnDbl = false;

	m_ptCursor.x = pointer.x;
	m_ptCursor.y = pointer.y;

	m_ptCursor.x = LIMIT(m_ptCursor.x, 0, m_lScreenWidth - 1);
	m_ptCursor.y = LIMIT(m_ptCursor.y, 0, m_lScreenHeight - 1);

	m_lDX = m_ptCursor.x - m_ptFormerCursor.x;
	m_lDY = m_ptCursor.y - m_ptFormerCursor.y;

	if (m_lDX || m_lDY)
		m_bFormerBtn0Dn = m_bFormerBtn1Dn = m_bFormerBtn2Dn = false;

	m_ptFormerCursor = m_ptCursor;

	double dAbsTime;

	if (pointer.leftButtonDown)
	{
		dAbsTime = g_pTimer->GetAbsTime();

		if (dAbsTime - m_dBtn0LastClickTime <= m_dDoubleClickTime
			&& m_bFormerBtn0Dn)
		{
			if (m_bLeftHand)
			{
				m_bRBtnDn =	m_bRBtnHeldDn = m_bRBtnDbl = true;
				m_bRBtnUp = false;
			}
			else
			{
				m_bLBtnDn = m_bLBtnHeldDn = m_bLBtnDbl = true;
				m_bLBtnUp = false;
			}
			m_bFormerBtn0Dn = false;
		}
		else
		{
			if (m_bLeftHand)
			{
				m_bRBtnDn =	m_bRBtnHeldDn =	true;
				m_bRBtnUp = m_bRBtnDbl = false;
			}
			else
			{
				m_bLBtnDn = m_bLBtnHeldDn = true;
				m_bLBtnUp = m_bLBtnDbl = false;
			}
			m_bFormerBtn0Dn = true;
		}
		m_dBtn0LastClickTime = dAbsTime;
	}
	else
	{
		if (m_bLeftHand)
		{
			if (m_bRBtnHeldDn)
			{
				m_bRBtnDn = m_bRBtnHeldDn = m_bRBtnDbl = false;
				m_bRBtnUp = true;
			}
		}
		else
		{
			if (m_bLBtnHeldDn)
			{
				m_bLBtnDn = m_bLBtnHeldDn = m_bLBtnDbl = false;
				m_bLBtnUp = true;
			}
		}
	}

	if (pointer.rightButtonDown)
	{
		dAbsTime = g_pTimer->GetAbsTime();

		if (dAbsTime - m_dBtn1LastClickTime <= m_dDoubleClickTime
			&& m_bFormerBtn1Dn)
		{
			if (m_bLeftHand)
			{
				m_bLBtnDn =	m_bLBtnHeldDn = m_bLBtnDbl = true;
				m_bLBtnUp = false;
			}
			else
			{
				m_bRBtnDn = m_bRBtnHeldDn = m_bRBtnDbl = true;
				m_bRBtnUp = false;
			}
			m_bFormerBtn1Dn = false;
		}
		else
		{
			if (m_bLeftHand)
			{
				m_bLBtnDn =	m_bLBtnHeldDn = true;
				m_bLBtnUp = m_bLBtnDbl = false;
			}
			else
			{
				m_bRBtnDn = m_bRBtnHeldDn = true;
				m_bRBtnUp = m_bRBtnDbl = false;
			}
			m_bFormerBtn1Dn = true;
		}
		m_dBtn1LastClickTime = dAbsTime;
	}
	else
	{
		if (m_bLeftHand)
		{
			if (m_bLBtnHeldDn)
			{
				m_bLBtnDn = m_bLBtnHeldDn = m_bLBtnDbl = false;
				m_bLBtnUp = true;
			}
		}
		else
		{
			if (m_bRBtnHeldDn)
			{
				m_bRBtnDn = m_bRBtnHeldDn = m_bRBtnDbl = false;
				m_bRBtnUp = true;
			}
		}
	}

	if (pointer.middleButtonDown)
	{
		dAbsTime = g_pTimer->GetAbsTime();

		if (dAbsTime - m_dBtn2LastClickTime <= m_dDoubleClickTime
			&& m_bFormerBtn2Dn)
		{
			m_bMBtnDbl = true;
			m_bFormerBtn2Dn = false;
		}
		else
		{
			m_bFormerBtn2Dn = true;
			m_bMBtnDbl = false;
		}
		m_bMBtnDn = m_bMBtnHeldDn = true;
		m_bMBtnUp = false;
		m_dBtn2LastClickTime = dAbsTime;
	}
	else
	{
		if (m_bMBtnHeldDn)
		{
			m_bMBtnDn = m_bMBtnHeldDn = m_bMBtnDbl = false;
			m_bMBtnUp = true;
		}
	}
#if	defined WINDOWMODE
	if(GetActiveWindow() == NULL)
	{
		m_lDX = m_lDY = 0L;

		m_bLBtnUp = m_bRBtnUp = m_bMBtnUp
			= m_bLBtnDn = m_bRBtnDn = m_bMBtnDn
			= m_bLBtnDbl = m_bRBtnDbl = m_bMBtnDbl = false;

		m_ptCursor.x = m_ptCursor.y = 0;
		return;
	}
#endif
}
