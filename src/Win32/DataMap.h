
#ifndef DATAMAP_H
#define DATAMAP_H

typedef enum 
{
    DM_NONE = 0,
    DM_BOOL,
    DM_INT
} DataMapType;

typedef enum
{
    CT_NONE = 0,
    CT_BUTTON,
    CT_COMBOBOX,
    CT_SLIDER
}  CtrlType;

typedef struct {
    DWORD dwCtrlId;
    DataMapType dmType;
    CtrlType    cType;
    BOOL bValue;
    BOOL *bVar;
    int nValue;
    int nMin;
    int nMax;
    int *nVar;
    void (*func)(HWND hWnd);
} DATA_MAP;

void InitDataMap(void);
void DataMapAdd(DWORD dwCtrlId, DataMapType dmType, CtrlType cType,
                void *var, int min, int max, void (*func)(HWND hWnd));
void PopulateControls(HWND hWnd);
void ReadControls(HWND hWnd);
BOOL ReadControl(HWND hWnd, DWORD dwCtrlId);

#endif /* DATAMAP_H */