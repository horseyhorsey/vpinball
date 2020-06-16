#include "StdAfx.h"
#include "resource.h"
#include "SoundDialog.h"

typedef struct _tagSORTDATA
{
    HWND hwndList;
    int subItemIndex;
    int sortUpDown;
} SORTDATA;

extern SORTDATA SortData;
extern int CALLBACK MyCompProc( LPARAM lSortParam1, LPARAM lSortParam2, LPARAM lSortOption );
int SoundDialog::m_columnSortOrder;

SoundDialog::SoundDialog() : CDialog( IDD_SOUNDDIALOG )
{
    hSoundList = NULL;
    m_columnSortOrder = 1;
}

SoundDialog::~SoundDialog()
{
}

void SoundDialog::OnDestroy()
{
    CDialog::OnDestroy();
}

void SoundDialog::OnClose()
{
    SavePosition();
    CCO(PinTable) * const pt = g_pvp->GetActiveTable();
    if (pt)
        pt->StopAllSounds(); 
    CDialog::OnClose();
}

long GetSystemDPI()
{
    CClientDC clientDC(NULL);
	const SIZE ret = { clientDC.GetDeviceCaps(LOGPIXELSX), clientDC.GetDeviceCaps(LOGPIXELSY) };
	return ret.cx;
}

long GetDPI()
{
	static const long dpi = GetSystemDPI();
	return dpi;
}

int DPIValue(int value)
{
	return MulDiv(value, GetDPI(), 96);
}

BOOL SoundDialog::OnInitDialog()
{
    CCO( PinTable ) * const pt = g_pvp->GetActiveTable();
    hSoundList = GetDlgItem( IDC_SOUNDLIST ).GetHwnd();

    LoadPosition();

    LVCOLUMN lvcol;
    m_columnSortOrder = 1;

    ListView_SetExtendedListViewStyle( hSoundList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES );
	memset(&lvcol, 0, sizeof(LVCOLUMN));
	lvcol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT; 
	LocalString ls( IDS_NAME );
    lvcol.pszText = ls.m_szbuffer;// = "Name";
    lvcol.cx = DPIValue(150);
    ListView_InsertColumn( hSoundList, 0, &lvcol );

    LocalString ls2( IDS_IMPORTPATH );
    lvcol.pszText = ls2.m_szbuffer; // = "Import Path";
    lvcol.cx = DPIValue(200);
    ListView_InsertColumn( hSoundList, 1, &lvcol );

	lvcol.pszText = "Output";
	lvcol.cx = DPIValue(80);
	ListView_InsertColumn(hSoundList, 2, &lvcol);

	lvcol.pszText = "Pan";
	lvcol.cx = DPIValue(50);
	ListView_InsertColumn(hSoundList, 3, &lvcol);

	lvcol.pszText = "Fade";
	lvcol.cx = DPIValue(50);
	ListView_InsertColumn(hSoundList, 4, &lvcol);

	lvcol.pszText = "Vol";
	lvcol.cx = DPIValue(50);
	ListView_InsertColumn(hSoundList, 5, &lvcol);

	if (pt)
		pt->ListSounds(hSoundList);

   ListView_SetItemState(hSoundList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
   GotoDlgCtrl(hSoundList);
   return FALSE;
}

INT_PTR SoundDialog::DialogProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CCO( PinTable ) * const pt = g_pvp->GetActiveTable();

    switch(uMsg)
    {
        case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = (LPNMHDR)lParam;
            if (wParam == IDC_SOUNDLIST)
            {
                LPNMLISTVIEW lpnmListView = (LPNMLISTVIEW)lParam;
                if (lpnmListView->hdr.code == LVN_COLUMNCLICK)
                {
                    const int columnNumber = lpnmListView->iSubItem;
                    if (m_columnSortOrder == 1)
                       m_columnSortOrder = 0;
                    else
                       m_columnSortOrder = 1;
                    SortData.hwndList = hSoundList;
                    SortData.subItemIndex = columnNumber;
                    SortData.sortUpDown = m_columnSortOrder;
                    ListView_SortItems( SortData.hwndList, MyCompProc, &SortData );
                }
            }
            switch(pnmhdr->code)
            {
                case LVN_ENDLABELEDIT:
                {
                    NMLVDISPINFO *const pinfo = (NMLVDISPINFO *)lParam;
                    if (pinfo->item.pszText == NULL || pinfo->item.pszText[0] == '\0')
                        return FALSE;
                    ListView_SetItemText( hSoundList, pinfo->item.iItem, 0, pinfo->item.pszText );
                    LVITEM lvitem;
                    lvitem.mask = LVIF_PARAM;
                    lvitem.iItem = pinfo->item.iItem;
                    lvitem.iSubItem = 0;
                    ListView_GetItem( hSoundList, &lvitem );
                    PinSound * const pps = (PinSound *)lvitem.lParam;
                    strncpy_s( pps->m_szName, pinfo->item.pszText, MAXTOKEN-1 );
                    strncpy_s( pps->m_szInternalName, pinfo->item.pszText, MAXTOKEN-1 );
                    CharLowerBuff( pps->m_szInternalName, lstrlen( pps->m_szInternalName ) );
                    if (pt)
                        pt->SetNonUndoableDirty( eSaveDirty );
                    return TRUE;
                }
                break;

                case LVN_ITEMCHANGED:
                {
                    const int count = ListView_GetSelectedCount( hSoundList );
                    const BOOL enable = !(count > 1);
                    ::EnableWindow( GetDlgItem(IDC_REIMPORTFROM).GetHwnd(), enable );
                    ::EnableWindow( GetDlgItem(IDC_RENAME).GetHwnd(), enable );
                    ::EnableWindow( GetDlgItem(IDC_PLAY).GetHwnd(), enable );
                    ::EnableWindow( GetDlgItem(IDC_STOP).GetHwnd(), fFalse );
                    if (pt)
                        pt->StopAllSounds(); 
                }
                break;
            }
        }
        break;
    }
    return DialogProcDefault( uMsg, wParam, lParam );
}

BOOL SoundDialog::OnCommand( WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED_PARAMETER( lParam );

    switch(LOWORD( wParam ))
    {
        case IDC_IMPORT: Import(); break;
        case IDC_REIMPORT: ReImport(); break;
        case IDC_REIMPORTFROM: ReImportFrom(); break;
        case IDC_SNDEXPORT: Export(); break;
        case IDC_SNDTOBG: SoundToBG(); break;
        case IDC_SNDPOSITION: SoundPosition(); break;
        case IDC_DELETE_SOUND: DeleteSound(); break;
        case IDC_OK: SavePosition(); CDialog::OnOK(); break;
        case IDC_STOP:
        {
            const int sel = ListView_GetNextItem(hSoundList, -1, LVNI_SELECTED);
            if (sel != -1)
            {
                LVITEM lvitem;
                lvitem.mask = LVIF_PARAM;
                lvitem.iItem = sel;
                lvitem.iSubItem = 0;
                ListView_GetItem( hSoundList, &lvitem );
                PinSound * const pps = (PinSound *)lvitem.lParam;
                pps->Stop();
                ::EnableWindow(GetDlgItem(IDC_STOP).GetHwnd(), fFalse);
            }
            break;
        }
        case IDC_PLAY:
        {
            const int sel = ListView_GetNextItem(hSoundList, -1, LVNI_SELECTED);
            if (sel != -1)
            {
                LVITEM lvitem;
                lvitem.mask = LVIF_PARAM;
                lvitem.iItem = sel;
                lvitem.iSubItem = 0;
                ListView_GetItem( hSoundList, &lvitem );

                PinSound * const pps = (PinSound *)lvitem.lParam;
                pps->TestPlay();
                ::EnableWindow(GetDlgItem(IDC_STOP).GetHwnd(), fTrue);
            }
            break;
        }
        case IDC_RENAME:
        {
            const int sel = ListView_GetNextItem( hSoundList, -1, LVNI_SELECTED );
            if (sel != -1)
            {
                ::SetFocus( hSoundList );
                ListView_EditLabel( hSoundList, sel );
            }
            break;
        }
        default: return FALSE;
    }

    return TRUE;
}

void SoundDialog::OnOK()
{
    // do not call CDialog::OnOk() here because if you rename sounds keys like backspace or escape in rename mode cause an IDOK message and this function is called
}

void SoundDialog::OnCancel()
{
    SavePosition();
    CDialog::OnCancel();
}

void SoundDialog::Import()
{
    CCO( PinTable ) * const pt = g_pvp->GetActiveTable();
    char szFileName[MAXSTRING];
    char szInitialDir[MAXSTRING];
    int  fileOffset;
    szFileName[0] = '\0';

    /*const HRESULT hr =*/ LoadValueString( "RecentDir", "SoundDir", szInitialDir, MAXSTRING);

    if (g_pvp->OpenFileDialog(szInitialDir, szFileName, "Sound Files (.wav/.ogg/.mp3)\0*.wav;*.ogg;*.mp3\0", "mp3", OFN_EXPLORER | OFN_ALLOWMULTISELECT, fileOffset))
    {
        strcpy_s( szInitialDir, sizeof( szInitialDir ), szFileName );

        int len = lstrlen( szFileName );
        if (len < fileOffset)
        {
            // Multi-file select
            char szT[MAXSTRING];
            lstrcpy( szT, szFileName );
            lstrcat( szT, "\\" );
            len++;
            int filenamestart = fileOffset;
            int filenamelen = lstrlen( &szFileName[filenamestart] );
            while(filenamelen > 0)
            {
                lstrcpy( &szT[len], &szFileName[filenamestart] );
                pt->ImportSound( hSoundList, szT );
                filenamestart += filenamelen + 1;
                filenamelen = lstrlen( &szFileName[filenamestart] );
            }
        }
        else
        {
            szInitialDir[fileOffset] = 0;
            if (pt)
               pt->ImportSound( hSoundList, szFileName );
        }
        SaveValueString( "RecentDir", "SoundDir", szInitialDir);
        if (pt)
            pt->SetNonUndoableDirty( eSaveDirty );
    }
    SetFocus();
}

void SoundDialog::ReImport()
{
    CCO( PinTable ) * const pt = g_pvp->GetActiveTable();
    const int count = ListView_GetSelectedCount( hSoundList );
    if (count > 0)
    {
        LocalString ls( IDS_REPLACESOUND );
        const int ans = MessageBox( ls.m_szbuffer/*"Are you sure you want to remove this image?"*/, "Confirm Reimport", MB_YESNO | MB_DEFBUTTON2 );
        if (ans == IDYES)
        {
            int sel = ListView_GetNextItem( hSoundList, -1, LVNI_SELECTED );
            while(sel != -1)
            {
                LVITEM lvitem;
                lvitem.mask = LVIF_PARAM;
                lvitem.iItem = sel;
                lvitem.iSubItem = 0;
                ListView_GetItem( hSoundList, &lvitem );
                PinSound * const pps = (PinSound *)lvitem.lParam;

                const HANDLE hFile = CreateFile( pps->m_szPath, GENERIC_READ, FILE_SHARE_READ,
                                                 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

                if (hFile != INVALID_HANDLE_VALUE)
                {
                    CloseHandle( hFile );

                    pt->ReImportSound( hSoundList, pps, pps->m_szPath );
                    pt->SetNonUndoableDirty( eSaveDirty );
                }
                else
                    MessageBox( pps->m_szPath, "FILE NOT FOUND!", MB_OK );

                sel = ListView_GetNextItem( hSoundList, sel, LVNI_SELECTED );
            }
        }
        //pt->SetNonUndoableDirty(eSaveDirty);
    }
    SetFocus();
}

void SoundDialog::ReImportFrom()
{
    CCO( PinTable ) * const pt = g_pvp->GetActiveTable();
    const int sel = ListView_GetNextItem( hSoundList, -1, LVNI_SELECTED );
    if (sel != -1)
    {
        LocalString ls( IDS_REPLACESOUND );
        const int ans = MessageBox( ls.m_szbuffer/*"Are you sure you want to replace this sound with a new one?"*/, "Confirm Reimport", MB_YESNO | MB_DEFBUTTON2 );
        if (ans == IDYES)
        {
            char szInitialDir[MAXSTRING];
            char szFileName[MAXSTRING];
            szFileName[0] = '\0';
            int fileOffset;

            /*const HRESULT hr =*/ LoadValueString("RecentDir", "SoundDir", szInitialDir, MAXSTRING);

            if (g_pvp->OpenFileDialog(szInitialDir, szFileName, "Sound Files (.wav/.ogg/.mp3)\0*.wav;*.ogg;*.mp3\0", "mp3", 0, fileOffset))
            {
                LVITEM lvitem;
                lvitem.mask = LVIF_PARAM;
                lvitem.iItem = sel;
                lvitem.iSubItem = 0;
                ListView_GetItem( hSoundList, &lvitem );
                PinSound * const pps = (PinSound *)lvitem.lParam;

                pt->ReImportSound( hSoundList, pps, szFileName );
                ListView_SetItemText( hSoundList, sel, 1, szFileName );
                pt->SetNonUndoableDirty( eSaveDirty );
            }
        }
    }
    SetFocus();
}

void SoundDialog::Export()
{
    CCO( PinTable ) * const pt = g_pvp->GetActiveTable();
    const int selectedItemsCount = ListView_GetSelectedCount(hSoundList);
    const size_t renameOnExport = SendMessage(GetDlgItem(IDC_CHECK_RENAME_ON_EXPORT).GetHwnd(), BM_GETCHECK, 0, 0);

    if (selectedItemsCount)
    {
        OPENFILENAME ofn;
        LVITEM lvitem;
        int sel = ListView_GetNextItem( hSoundList, -1, LVNI_SELECTED ); //next selected item 	
        if (sel != -1)
        {
            lvitem.mask = LVIF_PARAM;
            lvitem.iItem = sel;
            lvitem.iSubItem = 0;
            ListView_GetItem( hSoundList, &lvitem );
            PinSound *pps = (PinSound *)lvitem.lParam;

            ZeroMemory( &ofn, sizeof( OPENFILENAME ) );
            ofn.lStructSize = sizeof( OPENFILENAME );
            ofn.hInstance = g_hinst;
            ofn.hwndOwner = g_pvp->GetHwnd();
            //TEXT
            ofn.lpstrFilter = "Sound Files (.wav/.ogg/.mp3)\0*.wav;*.ogg;*.mp3\0";

            int begin;		//select only file name from pathfilename
            int len = lstrlen( pps->m_szPath );
            memset( m_filename, 0, MAX_PATH );
            memset( m_initDir, 0, MAX_PATH );

            if (!renameOnExport)
            {
               for (begin = len; begin >= 0; begin--)
               {
                  if (pps->m_szPath[begin] == '\\')
                  {
                     begin++;
                     break;
                  }
               }
               memcpy(m_filename, &pps->m_szPath[begin], len - begin);
            }
            else
            {
               strcat_s(m_filename, pps->m_szName);
               string ext(pps->m_szPath);
               size_t idx = ext.find_last_of('.');
               strcat_s(m_filename, ext.c_str() + idx);

            }
            ofn.lpstrFile = m_filename;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrDefExt = "mp3";
            const HRESULT hr = LoadValueString( "RecentDir", "SoundDir", m_initDir, MAX_PATH );

            if (hr == S_OK)ofn.lpstrInitialDir = m_initDir;
            else ofn.lpstrInitialDir = NULL;

            ofn.lpstrTitle = "SAVE AS";
            ofn.Flags = OFN_NOREADONLYRETURN | OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_EXPLORER;

            m_initDir[ofn.nFileOffset] = 0;
            if (GetSaveFileName( &ofn ))	//Get filename from user
            {
                len = lstrlen( ofn.lpstrFile );
                for (begin = len; begin >= 0; begin--)
                {
                    if (ofn.lpstrFile[begin] == '\\')
                    {
                        begin++;
                        break;
                    }
                }

                if (begin >= MAX_PATH)
                    begin=MAX_PATH - 1;

                char pathName[MAX_PATH] = { 0 };
                memcpy( pathName, ofn.lpstrFile, begin );
                pathName[begin] = 0;
                while(sel != -1)
                {
                    len = lstrlen( pps->m_szPath );
                    for (begin = len; begin >= 0; begin--)
                    {
                        if (pps->m_szPath[begin] == '\\')
                        {
                            begin++;
                            break;
                        }
                    }
                    if (selectedItemsCount > 1)
                    {
                       memset(m_filename, 0, MAX_PATH);
                       strcpy_s(m_filename, MAX_PATH-1, pathName);
                       if (!renameOnExport)
                          strcat_s(m_filename, MAX_PATH-1, &pps->m_szPath[begin]);
                       else
                       {
                          strcat_s(m_filename, pps->m_szName);
                          string ext(pps->m_szPath);
                          size_t idx = ext.find_last_of('.');
                          strcat_s(m_filename, ext.c_str() + idx);
                       }
                    }

                    pt->ExportSound(pps, m_filename);
                    sel = ListView_GetNextItem( hSoundList, sel, LVNI_SELECTED ); //next selected item
                    lvitem.iItem = sel;
                    lvitem.iSubItem = 0;
                    ListView_GetItem( hSoundList, &lvitem );
                    pps = (PinSound *)lvitem.lParam;

                }
                SaveValueString( "RecentDir", "SoundDir", pathName);
            }
        }
    }
    SetFocus();
}

void SoundDialog::SoundToBG()
{
    CCO( PinTable ) * const pt = g_pvp->GetActiveTable();

    if (ListView_GetSelectedCount( hSoundList ))
    {
        LVITEM lvitem;
        int sel = ListView_GetNextItem( hSoundList, -1, LVNI_SELECTED ); //next selected item 	
        while(sel != -1)
        {
            lvitem.mask = LVIF_PARAM;
            lvitem.iItem = sel;
            lvitem.iSubItem = 0;
            ListView_GetItem( hSoundList, &lvitem );
            PinSound * const pps = (PinSound *)lvitem.lParam;

            pps->m_outputTarget = (pps->m_outputTarget != SNDOUT_BACKGLASS) ? SNDOUT_BACKGLASS : SNDOUT_TABLE;

            switch (pps->m_outputTarget)
            {
               case SNDOUT_BACKGLASS:
                  ListView_SetItemText(hSoundList, sel, 2, "Backglass");
                  break;
               case SNDOUT_TABLE:
               default:
                  ListView_SetItemText(hSoundList, sel, 2, "Table");
                  break;
            }
            pt->SetNonUndoableDirty(eSaveDirty);

            sel = ListView_GetNextItem(hSoundList, sel, LVNI_SELECTED ); //next selected item
        }
    }
    SetFocus();
}

void SoundDialog::SoundPosition()
{
	CCO(PinTable) * const pt = g_pvp->GetActiveTable();

	if (ListView_GetSelectedCount(hSoundList))
	{
		LVITEM lvitem;
		int sel = ListView_GetNextItem(hSoundList, -1, LVNI_SELECTED); //next selected item 	
			
		lvitem.mask = LVIF_PARAM;
		lvitem.iItem = sel;
		lvitem.iSubItem = 0;
		ListView_GetItem(hSoundList, &lvitem);
		PinSound *pps = (PinSound *)lvitem.lParam;
		SoundPositionDialog spd(pps);

		if (spd.DoModal() == IDOK)
		{
			while (sel != -1)
			{
				lvitem.mask = LVIF_PARAM;
				lvitem.iItem = sel;
				lvitem.iSubItem = 0;
				ListView_GetItem(hSoundList, &lvitem);
				pps = (PinSound *)lvitem.lParam;
				pps->m_outputTarget = spd.m_cOutputTarget;
				pps->m_balance = spd.m_balance;
				pps->m_fade = spd.m_fade;
				pps->m_volume = spd.m_volume;
				pps->ReInitialize();

				pt->SetNonUndoableDirty(eSaveDirty);

				sel = ListView_GetNextItem(hSoundList, sel, LVNI_SELECTED); //next selected item
			}
			pt->ListSounds(hSoundList);
			SetFocus();
		}
	}
}


void SoundDialog::DeleteSound()
{
    CCO( PinTable ) * const pt = g_pvp->GetActiveTable();

    const int count = ListView_GetSelectedCount( hSoundList );
    if (count > 0)
    {
        LocalString ls( IDS_REMOVESOUND );
        const int ans = MessageBox( ls.m_szbuffer/*"Are you sure you want to remove this image?"*/, "Confirm Deletion", MB_YESNO | MB_DEFBUTTON2 );
        if (ans == IDYES)
        {
            int sel = ListView_GetNextItem( hSoundList, -1, LVNI_SELECTED );
            while(sel != -1)
            {
                LVITEM lvitem;
                lvitem.mask = LVIF_PARAM;
                lvitem.iItem = sel;
                lvitem.iSubItem = 0;
                ListView_GetItem( hSoundList, &lvitem );
                PinSound * const pps = (PinSound *)lvitem.lParam;
                ListView_DeleteItem( hSoundList, sel );
                pt->RemoveSound(pps);

                // The previous selection is now deleted, so look again from the top of the list
                sel = ListView_GetNextItem( hSoundList, -1, LVNI_SELECTED );
            }
        }
        pt->SetNonUndoableDirty( eSaveDirty );
    }
    SetFocus();
}

void SoundDialog::LoadPosition()
{
    const int x = LoadValueIntWithDefault("Editor", "SoundMngPosX", 0);
    const int y = LoadValueIntWithDefault("Editor", "SoundMngPosY", 0);

    SetWindowPos( NULL, x, y, 0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
}

void SoundDialog::SavePosition()
{
    const CRect rect = GetWindowRect();
    SaveValueInt( "Editor", "SoundMngPosX", rect.left);
    SaveValueInt( "Editor", "SoundMngPosY", rect.top);
}


SoundPositionDialog::SoundPositionDialog(PinSound * const pps) : CDialog(IDD_SOUND_POSITION_DIALOG)
{
	m_balance = pps->m_balance;
	m_fade = pps->m_fade;
	m_volume = pps->m_volume;
	m_cOutputTarget = pps->m_outputTarget;
	m_pps = pps;
}

SoundPositionDialog::~SoundPositionDialog()
{
	m_pps->Stop();
	m_pps->ReInitialize();
}

void SoundPositionDialog::OnDestroy()
{
	CDialog::OnDestroy();
}

void SoundPositionDialog::OnClose()
{
	CDialog::OnClose();
}

BOOL SoundPositionDialog::OnInitDialog()
{
	m_Volume.Attach(GetDlgItem(IDC_AUD_VOLUME));
	m_Volume.SetRangeMin(-100);
	m_Volume.SetRangeMax(100);
	m_Volume.SetTicFreq(25);
	m_Balance.Attach(GetDlgItem(IDC_AUD_BALANCE));
	m_Balance.SetRangeMin(-100);
	m_Balance.SetRangeMax(100);
	m_Balance.SetTicFreq(25);
	m_Fader.Attach(GetDlgItem(IDC_AUD_FADER));
	m_Fader.SetRangeMin(-100);
	m_Fader.SetRangeMax(100);
	m_Fader.SetTicFreq(25);
	SetSliderValues();
	SetTextValues();

	HWND boxtocheck;
	switch (m_cOutputTarget)
	{
	case SNDOUT_BACKGLASS:
		boxtocheck = GetDlgItem(IDC_SPT_BACKGLASS);
		break;
	default:  // SNDOUT_TABLE
		boxtocheck = GetDlgItem(IDC_SPT_TABLE);
		break;
	}
	::SendMessage(boxtocheck, BM_SETCHECK, BST_CHECKED, 0);
	return TRUE;
}

void SoundPositionDialog::SetSliderValues()
{
	m_Volume.SetPos(m_volume, 1);
	m_Balance.SetPos(m_balance, 1);
	m_Fader.SetPos(m_fade, 1);
}

INT_PTR SoundPositionDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//const HWND hwndDlg = GetHwnd();
	//CCO(PinTable) * const pt = g_pvp->GetActiveTable();

	switch (uMsg)
	{
	case WM_HSCROLL:
	case WM_VSCROLL:
		switch (LOWORD(wParam))
		{
		case TB_ENDTRACK:
			ReadValuesFromSliders();
			SetTextValues();
			break;
		default:
			break;
		}
		break;
	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case EN_KILLFOCUS:
			ReadTextValue(IDC_EDIT_BALANCE, m_balance);
			ReadTextValue(IDC_EDIT_FADER, m_fade);
			ReadTextValue(IDC_EDIT_VOL, m_volume);
			SetSliderValues();
			SetTextValues();
			break;
		}
		break;
	}
	return DialogProcDefault(uMsg, wParam, lParam);
}

void SoundPositionDialog::ReadTextValue(int item, int &oValue)
{
	const CString textStr = GetDlgItemText(item);
	float fval;
	const int ret = sscanf_s(textStr.c_str(), "%f", &fval);
	if (ret == 1 && fval >= -1.0f && fval <= 1.0f)
		oValue = quantizeSignedPercent(fval);
}

void SoundPositionDialog::SetTextValues()
{
	SetTextValue(IDC_EDIT_BALANCE, m_balance);
	SetTextValue(IDC_EDIT_FADER, m_fade);
	SetTextValue(IDC_EDIT_VOL, m_volume);
}

void SoundPositionDialog::SetTextValue(int ctl, int val)
{
    char textBuf[MAXNAMEBUFFER];
	sprintf_s(textBuf, "%.03f", dequantizeSignedPercent(val));
	const CString textStr(textBuf);
	SetDlgItemText(ctl, textStr);
}


void SoundPositionDialog::GetDialogValues()
{
	m_cOutputTarget = SNDOUT_TABLE;
	if (SendMessage(GetDlgItem(IDC_SPT_BACKGLASS), BM_GETCHECK, 0, 0))
	{
		m_cOutputTarget = SNDOUT_BACKGLASS;
	}
}

void SoundPositionDialog::ReadValuesFromSliders()
{
	m_volume = m_Volume.GetPos();
	m_fade = m_Fader.GetPos();
	m_balance = m_Balance.GetPos();
}

BOOL SoundPositionDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDC_TEST:
		TestSound();
		break;
	case IDC_OK:
		GetDialogValues();
		CDialog::OnOK(); 
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

void SoundPositionDialog::TestSound()
{
	// Hold the actual output target temporarily and reinitialize.  It could be reset if dialog is canceled.
	const SoundOutTypes iOutputTargetTmp = m_pps->m_outputTarget;
	GetDialogValues();
	m_pps->m_outputTarget = m_cOutputTarget;
	m_pps->ReInitialize();

	const float volume = dequantizeSignedPercent(m_volume);
	const float pan = dequantizeSignedPercent(m_balance);
	const float front_rear_fade = dequantizeSignedPercent(m_fade);

	m_pps->Play((1.0f + volume) * 100.0f, 0.0f, 0, pan, front_rear_fade, 0, false);
	m_pps->m_outputTarget = iOutputTargetTmp;
}

void SoundPositionDialog::OnOK()
{
	// do not call CDialog::OnOk() here because if you rename sounds keys like backspace or escape in rename mode cause an IDOK message and this function is called
}

void SoundPositionDialog::OnCancel()
{
	CDialog::OnCancel();
}

/*int SoundPositionDialog::SliderToValue(const int Slider)
{
	return quantizeSignedPercent(powf(dequantizeSignedPercent(Slider), 10.0f));
}

int SoundPositionDialog::ValueToSlider(const int Value)
{
	return quantizeSignedPercent(powf(dequantizeSignedPercent(Value), (float)(1.0/10.0)));
}*/
