// PropertyList.cpp : implementation file
//

#include "stdafx.h"
#include "PropertyList.h"
#include "../cmCacheManager.h"

#define IDC_PROPCMBBOX   712
#define IDC_PROPEDITBOX  713
#define IDC_PROPBTNCTRL  714
#define IDC_PROPCHECKBOXCTRL 715

/////////////////////////////////////////////////////////////////////////////
// CPropertyList

CPropertyList::CPropertyList()
{
  m_Dirty = false;
  m_curSel = -1;
}

CPropertyList::~CPropertyList()
{
  for(std::set<CPropertyItem*>::iterator i = m_PropertyItems.begin();
      i != m_PropertyItems.end(); ++i)
    {
    delete *i;
    }
}


BEGIN_MESSAGE_MAP(CPropertyList, CListBox)
  //{{AFX_MSG_MAP(CPropertyList)
  ON_WM_CREATE()
  ON_CONTROL_REFLECT(LBN_SELCHANGE, OnSelchange)
  ON_WM_LBUTTONUP()
  ON_WM_KILLFOCUS()
  ON_WM_LBUTTONDOWN()
  ON_WM_RBUTTONUP()
  ON_WM_MOUSEMOVE()
  //}}AFX_MSG_MAP
  ON_CBN_KILLFOCUS(IDC_PROPCMBBOX, OnKillfocusCmbBox)
  ON_CBN_SELCHANGE(IDC_PROPCMBBOX, OnSelchangeCmbBox)
  ON_EN_KILLFOCUS(IDC_PROPEDITBOX, OnKillfocusEditBox)
  ON_EN_CHANGE(IDC_PROPEDITBOX, OnChangeEditBox)
  ON_BN_CLICKED(IDC_PROPBTNCTRL, OnButton)
  ON_BN_CLICKED(IDC_PROPCHECKBOXCTRL, OnCheckBox)
  ON_COMMAND(42, OnDelete)
  ON_COMMAND(43, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropertyList message handlers

BOOL CPropertyList::PreCreateWindow(CREATESTRUCT& cs) 
{
  if (!CListBox::PreCreateWindow(cs))
    return FALSE;

  cs.style &= ~(LBS_OWNERDRAWVARIABLE | LBS_SORT);
  cs.style |= LBS_OWNERDRAWFIXED;

  m_bTracking = FALSE;
  m_nDivider = 0;
  m_bDivIsSet = FALSE;

  return TRUE;
}

void CPropertyList::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
  lpMeasureItemStruct->itemHeight = 20; //pixels
}


void CPropertyList::DrawItem(LPDRAWITEMSTRUCT lpDIS) 
{
  CDC dc;
  dc.Attach(lpDIS->hDC);
  CRect rectFull = lpDIS->rcItem;
  CRect rect = rectFull;
  if (m_nDivider==0)
    m_nDivider = rect.Width() / 2;
  rect.left = m_nDivider;
  CRect rect2 = rectFull;
  rect2.right = rect.left - 1;
  UINT nIndex = lpDIS->itemID;

  if (nIndex != (UINT) -1)
    {
    //draw two rectangles, one for each row column
    dc.FillSolidRect(rect2,RGB(192,192,192));
    dc.DrawEdge(rect2,EDGE_SUNKEN,BF_BOTTOMRIGHT);
    dc.DrawEdge(rect,EDGE_SUNKEN,BF_BOTTOM);

    //get the CPropertyItem for the current row
    CPropertyItem* pItem = (CPropertyItem*) GetItemDataPtr(nIndex);

    //write the property name in the first rectangle
    dc.SetBkMode(TRANSPARENT);
    dc.DrawText(pItem->m_propName,CRect(rect2.left+3,rect2.top+3,
                                        rect2.right-3,rect2.bottom+3),
                DT_LEFT | DT_SINGLELINE);

    //write the initial property value in the second rectangle
    dc.DrawText(pItem->m_curValue,CRect(rect.left+3,rect.top+3,
                                        rect.right+3,rect.bottom+3),
                DT_LEFT | DT_SINGLELINE);
    }
  dc.Detach();
}

int CPropertyList::AddItem(CString txt)
{
  int nIndex = AddString(txt);
  return nIndex;
}

int CPropertyList::AddPropItem(CPropertyItem* pItem)
{
  int nIndex = AddString(_T(""));
  SetItemDataPtr(nIndex,pItem);
  m_PropertyItems.insert(pItem);
  return nIndex;
}

int CPropertyList::AddProperty(const char* name,
                               const char* value,
                               const char* helpString,
                               int type,
                               const char* comboItems)
{ 
  CPropertyItem* pItem = 0;
  for(int i =0; i < this->GetCount(); ++i)
    {
    CPropertyItem* item = this->GetItem(i);
    if(item->m_propName == name)
      {
      pItem = item;
      if(pItem->m_curValue != value)
        {
        pItem->m_curValue = value;
        pItem->m_HelpString = helpString;
        m_Dirty = true;
        Invalidate();
        }
      return i;
      }
    }
  // if it is not found, then create a new one
  if(!pItem)
    {
    pItem = new CPropertyItem(name, value, helpString, type, comboItems);
    }
  return this->AddPropItem(pItem);
}

int CPropertyList::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
  if (CListBox::OnCreate(lpCreateStruct) == -1)
    return -1;

  m_bDivIsSet = FALSE;
  m_nDivider = 0;
  m_bTracking = FALSE;

  m_hCursorSize = AfxGetApp()->LoadStandardCursor(IDC_SIZEWE);
  m_hCursorArrow = AfxGetApp()->LoadStandardCursor(IDC_ARROW);

  m_SSerif8Font.CreatePointFont(80,_T("MS Sans Serif"));

  return 0;
}

void CPropertyList::OnSelchange() 
{
  CRect rect;
  CString lBoxSelText;

  GetItemRect(m_curSel,rect);
  rect.left = m_nDivider;

  CPropertyItem* pItem = (CPropertyItem*) GetItemDataPtr(m_curSel);

  if (m_btnCtrl)
    m_btnCtrl.ShowWindow(SW_HIDE);
  if (m_CheckBoxControl)
    m_CheckBoxControl.ShowWindow(SW_HIDE);
  
  if (pItem->m_nItemType==CPropertyList::COMBO)
    {
    //display the combo box.  If the combo box has already been
    //created then simply move it to the new location, else create it
    m_nLastBox = 0;
    if (m_cmbBox)
      m_cmbBox.MoveWindow(rect);
    else
      {	
      rect.bottom += 100;
      m_cmbBox.Create(CBS_DROPDOWNLIST 
                      | CBS_NOINTEGRALHEIGHT | WS_VISIBLE 
                      | WS_CHILD | WS_BORDER,
                      rect,this,IDC_PROPCMBBOX);
      m_cmbBox.SetFont(&m_SSerif8Font);
      }

    //add the choices for this particular property
    CString cmbItems = pItem->m_cmbItems;
    lBoxSelText = pItem->m_curValue;
		
    m_cmbBox.ResetContent();
    int i,i2;
    i=0;
    while ((i2=cmbItems.Find('|',i)) != -1)
      {
      m_cmbBox.AddString(cmbItems.Mid(i,i2-i));
      i=i2+1;
      }
    if(i != 0)
      m_cmbBox.AddString(cmbItems.Mid(i));

    m_cmbBox.ShowWindow(SW_SHOW);
    m_cmbBox.SetFocus();

    //jump to the property's current value in the combo box
    int j = m_cmbBox.FindStringExact(0,lBoxSelText);
    if (j != CB_ERR)
      m_cmbBox.SetCurSel(j);
    else
      m_cmbBox.SetCurSel(0);
    }
  else if (pItem->m_nItemType==CPropertyList::EDIT)
    {
    //display edit box
    m_nLastBox = 1;
    m_prevSel = m_curSel;
    rect.bottom -= 3;
    if (m_editBox)
      m_editBox.MoveWindow(rect);
    else
      {	
      m_editBox.Create(ES_LEFT | ES_AUTOHSCROLL | WS_VISIBLE 
                       | WS_CHILD | WS_BORDER,
                       rect,this,IDC_PROPEDITBOX);
      m_editBox.SetFont(&m_SSerif8Font);
      }

    lBoxSelText = pItem->m_curValue;

    m_editBox.ShowWindow(SW_SHOW);
    m_editBox.SetFocus();
    //set the text in the edit box to the property's current value
    m_editBox.SetWindowText(lBoxSelText);
    }
  else if (pItem->m_nItemType == CPropertyList::CHECKBOX)
    {
    rect.bottom -= 3;
    if (m_CheckBoxControl)
      m_CheckBoxControl.MoveWindow(rect);
    else
      {	
      m_CheckBoxControl.Create("check",BS_CHECKBOX 
                               | BM_SETCHECK |BS_LEFTTEXT 
                               | WS_VISIBLE | WS_CHILD,
                               rect,this,IDC_PROPCHECKBOXCTRL);
      m_CheckBoxControl.SetFont(&m_SSerif8Font);
      }

    lBoxSelText = pItem->m_curValue;

    m_CheckBoxControl.ShowWindow(SW_SHOW);
    m_CheckBoxControl.SetFocus();
    //set the text in the edit box to the property's current value
    if(lBoxSelText == "ON")
      {
      m_CheckBoxControl.SetCheck(1);
      }
    else
      {
      m_CheckBoxControl.SetCheck(0);
      }
    }
        
  else
    DisplayButton(rect);
}

void CPropertyList::DisplayButton(CRect region)
{
  //displays a button if the property is a file/color/font chooser
  m_nLastBox = 2;
  m_prevSel = m_curSel;

  if (region.Width() > 25)
    region.left = region.right - 25;
  region.bottom -= 3;

  if (m_btnCtrl)
    m_btnCtrl.MoveWindow(region);
  else
    {	
    m_btnCtrl.Create("...",BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD,
                     region,this,IDC_PROPBTNCTRL);
    m_btnCtrl.SetFont(&m_SSerif8Font);
    }

  m_btnCtrl.ShowWindow(SW_SHOW);
  m_btnCtrl.SetFocus();
}

void CPropertyList::OnKillFocus(CWnd* pNewWnd) 
{
  //m_btnCtrl.ShowWindow(SW_HIDE);

  CListBox::OnKillFocus(pNewWnd);
}

void CPropertyList::OnKillfocusCmbBox() 
{
  m_cmbBox.ShowWindow(SW_HIDE);

  Invalidate();
}

void CPropertyList::OnKillfocusEditBox()
{
  CString newStr;
  m_editBox.ShowWindow(SW_HIDE);

  Invalidate();
}

void CPropertyList::OnSelchangeCmbBox()
{
  CString selStr;
  if (m_cmbBox)
    {
    m_cmbBox.GetLBText(m_cmbBox.GetCurSel(),selStr);
    CPropertyItem* pItem = (CPropertyItem*) GetItemDataPtr(m_curSel);
    pItem->m_curValue = selStr;
    m_Dirty = true;
    }
}

void CPropertyList::OnChangeEditBox()
{
  CString newStr;
  m_editBox.GetWindowText(newStr);
	
  CPropertyItem* pItem = (CPropertyItem*) GetItemDataPtr(m_curSel);
  pItem->m_curValue = newStr;
  m_Dirty = true;
}

void CPropertyList::OnCheckBox()
{ 
  CPropertyItem* pItem = (CPropertyItem*) GetItemDataPtr(m_curSel);
  if(m_CheckBoxControl.GetCheck())
    {
    pItem->m_curValue = "ON";
    }
  else
    {
    pItem->m_curValue = "OFF";
    }
  m_Dirty = true;
}

// Insane Microsoft way of setting the initial directory
// for the Shbrowseforfolder function...
//  SetSelProc
//  Callback procedure to set the initial selection of the browser.

int CALLBACK SetSelProc( HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM
                         lpData )
{
  if (uMsg==BFFM_INITIALIZED)
    {
    ::SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData );
    }
  return 0;
}

void CPropertyList::OnButton()
{
  CPropertyItem* pItem = (CPropertyItem*) GetItemDataPtr(m_curSel);

  //display the appropriate common dialog depending on what type
  //of chooser is associated with the property
  if (pItem->m_nItemType == CPropertyList::COLOR)
    {
    COLORREF initClr;
    CString currClr = pItem->m_curValue;
    //parse the property's current color value
    if (currClr.Find("RGB") > -1)
      {
      int j = currClr.Find(',',3);
      CString bufr = currClr.Mid(4,j-4);
      int RVal = atoi(bufr);
      int j2 = currClr.Find(',',j+1);
      bufr = currClr.Mid(j+1,j2-(j+1));
      int GVal = atoi(bufr);
      int j3 = currClr.Find(')',j2+1);
      bufr = currClr.Mid(j2+1,j3-(j2+1));
      int BVal = atoi(bufr);
      initClr = RGB(RVal,GVal,BVal);
      }
    else
      initClr = 0;
		
    CColorDialog ClrDlg(initClr);
		
    if (IDOK == ClrDlg.DoModal())
      {
      COLORREF selClr = ClrDlg.GetColor();
      CString clrStr;
      clrStr.Format("RGB(%d,%d,%d)",GetRValue(selClr),
                    GetGValue(selClr),GetBValue(selClr));
      m_btnCtrl.ShowWindow(SW_HIDE);

      pItem->m_curValue = clrStr;
      m_Dirty = true;
      Invalidate();
      }
    }
  else if (pItem->m_nItemType == CPropertyList::FILE)
    {
    CString SelectedFile; 
    CString Filter("All Files (*.*)||");
	
    CFileDialog FileDlg(TRUE, NULL, NULL, NULL,
			Filter);
    CString initialDir;
    CString currPath = pItem->m_curValue;
    if (currPath.GetLength() > 0)
      {
      int endSlash = currPath.ReverseFind('\\');
      if(endSlash == -1)
        {
        endSlash = currPath.ReverseFind('/');
        }
      initialDir = currPath.Left(endSlash);
      }		
    initialDir.Replace("/", "\\");
    FileDlg.m_ofn.lpstrTitle = "Select file";
    if (currPath.GetLength() > 0)
      FileDlg.m_ofn.lpstrInitialDir = initialDir;
    
    if(IDOK == FileDlg.DoModal())
      {
      SelectedFile = FileDlg.GetPathName();
			
      m_btnCtrl.ShowWindow(SW_HIDE);
			
      pItem->m_curValue = SelectedFile;
      m_Dirty = true;
      Invalidate();
      }
    }
   else if (pItem->m_nItemType == CPropertyList::PATH)
    {
    CString initialDir;
    CString currPath = pItem->m_curValue;
    if (currPath.GetLength() > 0)
      {
      int endSlash = currPath.ReverseFind('\\');
      if(endSlash == -1)
        {
        endSlash = currPath.ReverseFind('/');
        }
      initialDir = currPath.Left(endSlash);
      }
    initialDir.Replace("/", "\\");
    char szPathName[4096];
    BROWSEINFO bi;
    bi.lpfn = SetSelProc;
    bi.lParam = (LPARAM)(LPCSTR) initialDir;

    bi.hwndOwner = m_hWnd;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = (LPTSTR)szPathName;
    bi.lpszTitle = "Select Directory";
    bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS;
    
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    
    BOOL bSuccess = SHGetPathFromIDList(pidl, szPathName);
    CString SelectedFile; 
    if(bSuccess)
      {
      SelectedFile = szPathName;
      m_btnCtrl.ShowWindow(SW_HIDE);
      pItem->m_curValue = SelectedFile;
      m_Dirty = true;
      Invalidate();
      }
    }
  else if (pItem->m_nItemType == CPropertyList::FONT)
    {	
    CFontDialog FontDlg(NULL,CF_EFFECTS | CF_SCREENFONTS,NULL,this);
		
    if(IDOK == FontDlg.DoModal())
      {
      CString faceName = FontDlg.GetFaceName();
			
      m_btnCtrl.ShowWindow(SW_HIDE);
			
      pItem->m_curValue = faceName;
      m_Dirty = true;
      Invalidate();
      }
    }
}

void CPropertyList::OnLButtonUp(UINT nFlags, CPoint point) 
{
  if (m_bTracking)
    {
    //if columns were being resized then this indicates
    //that mouse is up so resizing is done.  Need to redraw
    //columns to reflect their new widths.
		
    m_bTracking = FALSE;
    //if mouse was captured then release it
    if (GetCapture()==this)
      ::ReleaseCapture();

    ::ClipCursor(NULL);

    CClientDC dc(this);
    InvertLine(&dc,CPoint(point.x,m_nDivTop),CPoint(point.x,m_nDivBtm));
    //set the divider position to the new value
    m_nDivider = point.x;

    //redraw
    Invalidate();
    }
  else
    {
    BOOL loc;
    int i = ItemFromPoint(point,loc);
    m_curSel = i;
    CListBox::OnLButtonUp(nFlags, point);
    }
}

void CPropertyList::OnLButtonDown(UINT nFlags, CPoint point) 
{
  if ((point.x>=m_nDivider-5) && (point.x<=m_nDivider+5))
    {
    //if mouse clicked on divider line, then start resizing

    ::SetCursor(m_hCursorSize);

    CRect windowRect;
    GetWindowRect(windowRect);
    windowRect.left += 10; windowRect.right -= 10;
    //do not let mouse leave the list box boundary
    ::ClipCursor(windowRect);
		
    if (m_cmbBox)
      m_cmbBox.ShowWindow(SW_HIDE);
    if (m_editBox)
      m_editBox.ShowWindow(SW_HIDE);

    CRect clientRect;
    GetClientRect(clientRect);

    m_bTracking = TRUE;
    m_nDivTop = clientRect.top;
    m_nDivBtm = clientRect.bottom;
    m_nOldDivX = point.x;

    CClientDC dc(this);
    InvertLine(&dc,CPoint(m_nOldDivX,m_nDivTop),CPoint(m_nOldDivX,m_nDivBtm));

    //capture the mouse
    SetCapture();
    }
  else
    {
    m_bTracking = FALSE;
    CListBox::OnLButtonDown(nFlags, point);
    }
}

void CPropertyList::OnMouseMove(UINT nFlags, CPoint point) 
{	
  if (m_bTracking)
    {
    //move divider line to the mouse pos. if columns are
    //currently being resized
    CClientDC dc(this);
    //remove old divider line
    InvertLine(&dc,CPoint(m_nOldDivX,m_nDivTop),CPoint(m_nOldDivX,m_nDivBtm));
    //draw new divider line
    InvertLine(&dc,CPoint(point.x,m_nDivTop),CPoint(point.x,m_nDivBtm));
    m_nOldDivX = point.x;
    }
  else if ((point.x >= m_nDivider-5) && (point.x <= m_nDivider+5))
    //set the cursor to a sizing cursor if the cursor is over the row divider
    ::SetCursor(m_hCursorSize);
  else
    CListBox::OnMouseMove(nFlags, point);
}

void CPropertyList::InvertLine(CDC* pDC,CPoint ptFrom,CPoint ptTo)
{
  int nOldMode = pDC->SetROP2(R2_NOT);
	
  pDC->MoveTo(ptFrom);
  pDC->LineTo(ptTo);

  pDC->SetROP2(nOldMode);
}

void CPropertyList::PreSubclassWindow() 
{
  m_bDivIsSet = FALSE;
  m_nDivider = 0;
  m_bTracking = FALSE;
  m_curSel = 1;

  m_hCursorSize = AfxGetApp()->LoadStandardCursor(IDC_SIZEWE);
  m_hCursorArrow = AfxGetApp()->LoadStandardCursor(IDC_ARROW);

  m_SSerif8Font.CreatePointFont(80,_T("MS Sans Serif"));
}

CPropertyItem* CPropertyList::GetItem(int index)
{
  return (CPropertyItem*)GetItemDataPtr(index);
}

void CPropertyList::OnRButtonUp( UINT nFlags, CPoint point )
{
  CMenu menu;
  CRect rect;
  this->GetWindowRect(&rect);
  BOOL loc;
  m_curSel = ItemFromPoint(point,loc);
  menu.CreatePopupMenu();
  menu.AppendMenu(MF_STRING | MF_ENABLED, 42, "Delete Cache Entry");
  menu.AppendMenu(MF_STRING | MF_ENABLED, 43, "Help For Cache Entry");
  menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
                      rect.TopLeft().x + point.x,
                      rect.TopLeft().y + point.y, this, NULL);
}


void CPropertyList::OnDelete()
{ 
  if(m_curSel == -1 || this->GetCount() <= 0)
    {
    return;
    }
  CPropertyItem* pItem = (CPropertyItem*) GetItemDataPtr(m_curSel);
  cmCacheManager::GetInstance()->RemoveCacheEntry(pItem->m_propName);
  m_PropertyItems.erase(pItem);
  delete pItem;
  this->DeleteString(m_curSel);
  Invalidate();
}

void CPropertyList::OnHelp()
{ 
  if(m_curSel == -1 || this->GetCount() <= 0)
    {
    return;
    }
  CPropertyItem* pItem = (CPropertyItem*) GetItemDataPtr(m_curSel);
  MessageBox(pItem->m_HelpString, pItem->m_propName, MB_OK|MB_ICONINFORMATION);
}

void CPropertyList::RemoveAll()
{
  int c = this->GetCount();
  for(int i =0; i < c; ++i)
    {
    CPropertyItem* pItem = (CPropertyItem*) GetItemDataPtr(0);
    cmCacheManager::GetInstance()->RemoveCacheEntry(pItem->m_propName);
    m_PropertyItems.erase(pItem);
    delete pItem;
    this->DeleteString(0);
    }
  Invalidate();
}
