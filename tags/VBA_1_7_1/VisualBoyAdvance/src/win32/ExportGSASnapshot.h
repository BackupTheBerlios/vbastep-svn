/*
 * VisualBoyAdvanced - Nintendo Gameboy/GameboyAdvance (TM) emulator
 * Copyrigh(c) 1999-2003 Forgotten (vb@emuhq.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#if !defined(AFX_EXPORTGSASNAPSHOT_H__ADF8566A_C64D_43CF_9CD2_A290370BA4F1__INCLUDED_)
#define AFX_EXPORTGSASNAPSHOT_H__ADF8566A_C64D_43CF_9CD2_A290370BA4F1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ExportGSASnapshot.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// ExportGSASnapshot dialog

class ExportGSASnapshot : public CDialog
{
  // Construction
 public:
  ExportGSASnapshot(CString filename, CString title,CWnd* pParent = NULL);   // standard constructor

  // Dialog Data
  //{{AFX_DATA(ExportGSASnapshot)
  enum { IDD = IDD_EXPORT_SPS };
  CString  m_desc;
  CString  m_notes;
  CString  m_title;
  //}}AFX_DATA
  CString m_filename;


  // Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(ExportGSASnapshot)
 protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

  // Implementation
 protected:

  // Generated message map functions
  //{{AFX_MSG(ExportGSASnapshot)
  virtual BOOL OnInitDialog();
  afx_msg void OnCancel();
  afx_msg void OnOk();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
    };

    //{{AFX_INSERT_LOCATION}}
    // Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPORTGSASNAPSHOT_H__ADF8566A_C64D_43CF_9CD2_A290370BA4F1__INCLUDED_)
