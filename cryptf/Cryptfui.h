#ifndef CRYPTFUI_H
#define CRYPTFUI_H

//Prototypes
BOOL BrowseForDir(HWND hWnd, UINT ctrlid);
void FormLostFocus(HWND hWnd, UINT nID);
BOOL FormDestroy(HWND hWnd);
BOOL FormInit(HWND hWnd);
void FormButtonsPressed(HWND hWnd, UINT nID);
void FormMenuSelected(HWND hWnd, UINT nID);
void RefreshData(HWND hWnd);

#endif