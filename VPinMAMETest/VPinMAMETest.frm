VERSION 5.00
Begin VB.Form VPinMAMETest 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "VPinMame Test"
   ClientHeight    =   8475
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   5640
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   8475
   ScaleWidth      =   5640
   Begin VB.CommandButton Command1 
      Caption         =   "Clear"
      Height          =   375
      Left            =   120
      TabIndex        =   58
      Top             =   2160
      Width           =   555
   End
   Begin VB.TextBox GName 
      Height          =   285
      Left            =   1320
      TabIndex        =   56
      Top             =   7440
      Width           =   1815
   End
   Begin VB.TextBox GValue 
      Height          =   285
      Left            =   3240
      TabIndex        =   55
      Top             =   7440
      Width           =   1335
   End
   Begin VB.CommandButton GPut 
      Caption         =   "P"
      Height          =   375
      Left            =   4680
      TabIndex        =   54
      Top             =   7440
      Width           =   375
   End
   Begin VB.CommandButton GGet 
      Caption         =   "G"
      Height          =   375
      Left            =   5160
      TabIndex        =   53
      Top             =   7440
      Width           =   375
   End
   Begin VB.CommandButton GSGet 
      Caption         =   "G"
      Height          =   375
      Left            =   5160
      TabIndex        =   52
      Top             =   6960
      Width           =   375
   End
   Begin VB.CommandButton GSPut 
      Caption         =   "P"
      Height          =   375
      Left            =   4680
      TabIndex        =   51
      Top             =   6960
      Width           =   375
   End
   Begin VB.TextBox GSValue 
      Height          =   285
      Left            =   3240
      TabIndex        =   50
      Top             =   6960
      Width           =   1335
   End
   Begin VB.TextBox GSName 
      Height          =   285
      Left            =   1320
      TabIndex        =   49
      Top             =   6960
      Width           =   1815
   End
   Begin VB.CommandButton GetDIP 
      Caption         =   "G"
      Height          =   375
      Left            =   5160
      TabIndex        =   47
      Top             =   6480
      Width           =   375
   End
   Begin VB.CommandButton PutDIP 
      Caption         =   "P"
      Height          =   375
      Left            =   4680
      TabIndex        =   46
      Top             =   6480
      Width           =   375
   End
   Begin VB.TextBox DipValue 
      Height          =   285
      Left            =   3600
      TabIndex        =   45
      Text            =   "0"
      Top             =   6480
      Width           =   975
   End
   Begin VB.TextBox DipBank 
      Height          =   285
      Left            =   3000
      MaxLength       =   1
      TabIndex        =   44
      Text            =   "0"
      Top             =   6480
      Width           =   495
   End
   Begin VB.CommandButton GetLamp 
      Caption         =   "L?"
      Height          =   375
      Left            =   1920
      TabIndex        =   42
      Top             =   6480
      Width           =   375
   End
   Begin VB.TextBox LampNo 
      Height          =   285
      Left            =   1320
      TabIndex        =   41
      Text            =   "0"
      Top             =   6480
      Width           =   495
   End
   Begin VB.CheckBox DebugBox 
      Caption         =   "MAME Debug"
      Height          =   255
      Left            =   2040
      TabIndex        =   39
      Top             =   5280
      Width           =   1335
   End
   Begin VB.CommandButton GetSwitch 
      Caption         =   "S?"
      Height          =   375
      Left            =   5040
      TabIndex        =   38
      Top             =   6000
      Width           =   375
   End
   Begin VB.CommandButton ClearSwitch 
      Caption         =   "Clear Switch"
      Height          =   375
      Left            =   3480
      TabIndex        =   37
      Top             =   6000
      Width           =   1455
   End
   Begin VB.CommandButton SetSwitch 
      Caption         =   "Set Switch"
      Height          =   375
      Left            =   1920
      TabIndex        =   36
      Top             =   6000
      Width           =   1455
   End
   Begin VB.TextBox SwitchNo 
      Height          =   285
      Left            =   1320
      TabIndex        =   34
      Text            =   "0"
      Top             =   6000
      Width           =   495
   End
   Begin VB.TextBox HandleMech 
      Height          =   285
      Left            =   1320
      TabIndex        =   32
      Text            =   "0"
      Top             =   5640
      Width           =   495
   End
   Begin VB.TextBox ImgDir 
      Enabled         =   0   'False
      Height          =   285
      Left            =   960
      TabIndex        =   30
      Top             =   4080
      Width           =   3375
   End
   Begin VB.CommandButton Pathes 
      Caption         =   "Pathes"
      Height          =   375
      Left            =   4440
      TabIndex        =   29
      Top             =   2640
      Width           =   1095
   End
   Begin VB.CommandButton CheckRoms 
      Caption         =   "Check ROMS"
      Height          =   375
      Left            =   1560
      TabIndex        =   28
      Top             =   7920
      Width           =   1215
   End
   Begin VB.CheckBox DoubleSize 
      Caption         =   "Double Size"
      Height          =   255
      Left            =   120
      TabIndex        =   27
      Top             =   5280
      Width           =   1815
   End
   Begin VB.CheckBox ShowFrame 
      Caption         =   "Show Frame"
      Enabled         =   0   'False
      Height          =   255
      Left            =   2040
      TabIndex        =   26
      Top             =   4800
      Value           =   1  'Checked
      Width           =   1335
   End
   Begin VB.TextBox MechNo 
      BeginProperty DataFormat 
         Type            =   1
         Format          =   "0"
         HaveTrueFalseNull=   0
         FirstDayOfWeek  =   0
         FirstWeekOfYear =   0
         LCID            =   1031
         SubFormatType   =   1
      EndProperty
      Height          =   285
      Left            =   4200
      TabIndex        =   24
      Text            =   "0"
      Top             =   4560
      Width           =   495
   End
   Begin VB.CommandButton GetMech 
      Caption         =   "GetMech"
      Height          =   375
      Left            =   4200
      TabIndex        =   23
      Top             =   4920
      Width           =   1215
   End
   Begin VB.CheckBox ShowDMDOnly 
      Caption         =   "Show DMD Only"
      Height          =   255
      Left            =   120
      TabIndex        =   22
      Top             =   5040
      Width           =   1815
   End
   Begin VB.CheckBox ShowTitle 
      Caption         =   "Show Title"
      Height          =   255
      Left            =   120
      TabIndex        =   21
      Top             =   4800
      Width           =   1815
   End
   Begin VB.CheckBox EnableKeyboard 
      Caption         =   "Enable Keyboard"
      Height          =   255
      Left            =   120
      TabIndex        =   20
      Top             =   4560
      Value           =   1  'Checked
      Width           =   1815
   End
   Begin VB.CommandButton About 
      Caption         =   "About"
      Height          =   375
      Left            =   4320
      TabIndex        =   19
      Top             =   7920
      Width           =   1215
   End
   Begin VB.ComboBox GameNames 
      Height          =   315
      ItemData        =   "VPinMAMETest.frx":0000
      Left            =   120
      List            =   "VPinMAMETest.frx":0002
      Sorted          =   -1  'True
      Style           =   2  'Dropdown List
      TabIndex        =   18
      Top             =   120
      Width           =   1095
   End
   Begin VB.CommandButton Options 
      Caption         =   "Options"
      Height          =   375
      Left            =   2880
      TabIndex        =   17
      Top             =   7920
      Width           =   1335
   End
   Begin VB.CommandButton ResumeButton 
      Caption         =   "Resume"
      Height          =   375
      Left            =   4560
      TabIndex        =   16
      Top             =   120
      Width           =   975
   End
   Begin VB.CommandButton PauseButton 
      Caption         =   "Pause"
      Height          =   375
      Left            =   3480
      TabIndex        =   15
      Top             =   120
      Width           =   975
   End
   Begin VB.TextBox SampleDirs 
      Enabled         =   0   'False
      Height          =   285
      Left            =   945
      TabIndex        =   13
      Top             =   3720
      Width           =   3375
   End
   Begin VB.TextBox NVRamDir 
      Enabled         =   0   'False
      Height          =   285
      Left            =   945
      TabIndex        =   11
      Top             =   3360
      Width           =   3375
   End
   Begin VB.TextBox CfgDir 
      Enabled         =   0   'False
      Height          =   285
      Left            =   945
      TabIndex        =   9
      Top             =   3000
      Width           =   3375
   End
   Begin VB.TextBox RomDirs 
      Enabled         =   0   'False
      Height          =   285
      Left            =   945
      TabIndex        =   7
      Top             =   2640
      Width           =   3375
   End
   Begin VB.ListBox SolenoidList 
      Height          =   1425
      Left            =   120
      TabIndex        =   6
      Top             =   600
      Width           =   5415
   End
   Begin VB.CommandButton EscapeButton 
      Caption         =   "Escape"
      Height          =   375
      Left            =   1065
      TabIndex        =   5
      Top             =   2160
      Width           =   975
   End
   Begin VB.CommandButton DownButton 
      Caption         =   "Down"
      Height          =   375
      Left            =   2145
      TabIndex        =   4
      Top             =   2160
      Width           =   975
   End
   Begin VB.CommandButton UpButton 
      Caption         =   "Up"
      Height          =   375
      Left            =   3225
      TabIndex        =   3
      Top             =   2160
      Width           =   975
   End
   Begin VB.CommandButton StopButton 
      Caption         =   "Stop"
      Height          =   375
      Left            =   2400
      TabIndex        =   2
      Top             =   120
      Width           =   975
   End
   Begin VB.CommandButton StartButton 
      Caption         =   "Start"
      Default         =   -1  'True
      Height          =   375
      Left            =   1320
      TabIndex        =   1
      Top             =   120
      Width           =   975
   End
   Begin VB.CommandButton EnterButton 
      Caption         =   "Enter"
      Height          =   375
      Left            =   4305
      TabIndex        =   0
      Top             =   2160
      Width           =   975
   End
   Begin VB.Label Label3 
      Caption         =   "Global settings"
      Height          =   255
      Left            =   120
      TabIndex        =   57
      Top             =   7440
      Width           =   1095
   End
   Begin VB.Label slsl 
      Caption         =   "Game settings"
      Height          =   255
      Left            =   120
      TabIndex        =   48
      Top             =   6960
      Width           =   1095
   End
   Begin VB.Label Label2 
      Caption         =   "DIP:"
      Height          =   255
      Left            =   2520
      TabIndex        =   43
      Top             =   6480
      Width           =   495
   End
   Begin VB.Label Label1 
      Caption         =   "Lamp"
      Height          =   255
      Left            =   120
      TabIndex        =   40
      Top             =   6480
      Width           =   975
   End
   Begin VB.Label Label7 
      Caption         =   "Switch"
      Height          =   255
      Left            =   120
      TabIndex        =   35
      Top             =   6000
      Width           =   975
   End
   Begin VB.Label Label6 
      AutoSize        =   -1  'True
      Caption         =   "HandleMech"
      Height          =   195
      Left            =   120
      TabIndex        =   33
      Top             =   5640
      Width           =   915
   End
   Begin VB.Label Label5 
      Caption         =   "Images"
      Height          =   255
      Left            =   120
      TabIndex        =   31
      Top             =   4080
      Width           =   735
   End
   Begin VB.Label MechResult 
      BorderStyle     =   1  'Fixed Single
      Height          =   255
      Left            =   4800
      TabIndex        =   25
      Top             =   4560
      Width           =   615
   End
   Begin VB.Label SampleLabel 
      Caption         =   "Samples"
      Height          =   255
      Left            =   105
      TabIndex        =   14
      Top             =   3720
      Width           =   735
   End
   Begin VB.Label NVRamLabel 
      Caption         =   "NVRAM"
      Height          =   255
      Left            =   105
      TabIndex        =   12
      Top             =   3360
      Width           =   735
   End
   Begin VB.Label CfgLabel 
      Caption         =   "Cfg"
      Height          =   255
      Left            =   105
      TabIndex        =   10
      Top             =   3000
      Width           =   735
   End
   Begin VB.Label ROMLabel 
      Caption         =   "ROMs"
      Height          =   255
      Left            =   105
      TabIndex        =   8
      Top             =   2640
      Width           =   735
   End
End
Attribute VB_Name = "VPinMAMETest"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Const S11_SWADVANCE = -1
Const S11_SWUPDN = -2
Const S11_SWCPUDIAG = -3
Const S11_SWSOUNDDIAG = -4
Const WPC_Escape = 5
Const WPC_Down = 6
Const WPC_Up = 7
Const WPC_Enter = 8

Dim WithEvents Controller As Controller
Attribute Controller.VB_VarHelpID = -1

Private Sub About_Click()
    Controller.ShowAboutDialog 1
End Sub

Private Sub BorderSizeX_Change()
    If (BorderSizeX.Text <> "") Then
        Controller.BorderSizeX = CInt(BorderSizeX.Text)
    End If
End Sub

Private Sub BorderSizeY_Change()
    If (BorderSizeY.Text <> "") Then
        Controller.BorderSizeY = CInt(BorderSizeY.Text)
    End If
End Sub

Private Sub CheckRoms_Click()
    If (Not Controller.CheckRoms(0)) Then
        MsgBox "ROM set is invalid"
    End If
End Sub

Private Sub Command1_Click()
Dim x
For x = 1 To SolenoidList.ListCount
    SolenoidList.RemoveItem (0)
Next
End Sub

Private Sub DebugBox_Click()
    If (Controller.Version < "01200000") Then Exit Sub
    Controller.Games(Controller.GameName).Settings.Value("debug") = DebugBox.Value
End Sub

Private Sub DoubleSize_Click()
    Controller.DoubleSize = DoubleSize.Value
End Sub

Private Sub EnableKeyboard_Click()
    Controller.HandleKeyboard = EnableKeyboard.Value
End Sub

Private Sub GameNames_Click()
    If (GameNames.Text = "<Default>") Then
        StartButton.Enabled = False
        Controller.GameName = ""
        UpdateSettings
    Else
        StartButton.Enabled = True
        Controller.GameName = GameNames.Text
        UpdateSettings
    End If
End Sub

Private Sub GetDIP_Click()
    DipValue.Text = "&H" + Hex(Controller.Dip(CInt(DipBank.Text)))
End Sub

Private Sub GetMech_Click()
    MechResult.Caption = Controller.GetMech(MechNo.Text)
End Sub

Private Sub GetSwitch_Click()
    If (Controller.Switch(Int(SwitchNo.Text))) Then
        MsgBox ("Switch is set")
    Else
        MsgBox ("Switch is cleared")
    End If
End Sub

Private Sub GSGet_Click()
    If (Controller.Version < "01200000") Then Exit Sub
    
    If (Controller.GameName <> "") Then
        GSValue.Text = Controller.Games(Controller.GameName).Settings.Value(GSName)
    End If
End Sub

Private Sub GSPut_Click()
    If (Controller.Version < "01200000") Then Exit Sub

    If (Controller.GameName <> "") Then
        Controller.Games(Controller.GameName).Settings.Value(GSName) = GSValue.Text
    End If
End Sub

Private Sub GGet_Click()
    If (Controller.Version < "01200000") Then Exit Sub
    
    GValue.Text = Controller.Settings.Value(GName)
End Sub

Private Sub GPut_Click()
    If (Controller.Version < "01200000") Then Exit Sub
    
    Controller.Settings.Value(GName) = GValue.Text
End Sub

Private Sub Options_Click()
    Controller.ShowOptsDialog 0
End Sub

Private Sub Pathes_Click()
    Controller.ShowPathesDialog 0
End Sub

Private Sub PauseButton_Click()
    Controller.Pause = 1
    ResumeButton.Enabled = True
    PauseButton.Enabled = False
End Sub

Private Sub PutDIP_Click()
    Controller.Dip(CInt(DipBank.Text)) = CInt(DipValue.Text)
End Sub

Private Sub ResumeButton_Click()
    Controller.Pause = 0
    PauseButton.Enabled = True
    ResumeButton.Enabled = False
End Sub

Private Sub EscapeButton_MouseDown(Button As Integer, Shift As Integer, x As Single, Y As Single)
    If Controller.WPCNumbering Then
        Controller.Switch(WPC_Escape) = 1
    Else
        If (Controller.Switch(S11_SWUPDN)) Then
            Controller.Switch(S11_SWUPDN) = 0
        Else
            Controller.Switch(S11_SWUPDN) = 1
        End If
    End If
End Sub

Private Sub EscapeButton_MouseUp(Button As Integer, Shift As Integer, x As Single, Y As Single)
    If Controller.WPCNumbering Then
        Controller.Switch(WPC_Escape) = 0
    Else
         Controller.Switch(S11_SWUPDN) = 0
'        Controller.Switch(-6) = True
    End If
End Sub
Private Sub DownButton_MouseDown(Button As Integer, Shift As Integer, x As Single, Y As Single)
    If Controller.WPCNumbering Then
        Controller.Switch(WPC_Down) = 1
    Else
        Controller.Switch(S11_SWCPUDIAG) = 1
    End If
End Sub

Private Sub DownButton_MouseUp(Button As Integer, Shift As Integer, x As Single, Y As Single)
    If Controller.WPCNumbering Then
        Controller.Switch(WPC_Down) = 0
    Else
        Controller.Switch(S11_SWCPUDIAG) = 0
    End If
End Sub

Private Sub GetLamp_Click()
    If (Controller.Lamp(Int(LampNo.Text))) Then
        MsgBox ("Lamp is on")
    Else
        MsgBox ("Lamp if off")
    End If
End Sub

Private Sub SetSwitch_Click()
    Dim swNo As Integer
    swNo = Int(SwitchNo.Text)
'    swNo = ((swNo \ 10) + 1) + ((swNo Mod 10) + 1) * 10
    
    Controller.Switch(swNo) = True
End Sub

Private Sub ClearSwitch_Click()
    Dim swNo As Integer
    swNo = Int(SwitchNo.Text)
'    swNo = ((swNo \ 10) + 1) + ((swNo Mod 10) + 1) * 10
    
    Controller.Switch(swNo) = False
End Sub

Private Sub ShowDMDOnly_Click()
    Controller.ShowDMDOnly = CBool(ShowDMDOnly.Value)
End Sub

Private Sub ShowFrame_Click()
    Controller.ShowFrame = CBool(ShowFrame.Value)
End Sub

Private Sub ShowTitle_Click()
    Controller.ShowTitle = CBool(ShowTitle.Value)
   
    ShowFrame.Enabled = ShowTitle.Value = 0
End Sub

Private Sub UpButton_MouseDown(Button As Integer, Shift As Integer, x As Single, Y As Single)
    If Controller.WPCNumbering Then
        Controller.Switch(WPC_Up) = 1
    Else
        Controller.Switch(S11_SWSOUNDDIAG) = 1
    End If
End Sub

Private Sub UpButton_MouseUp(Button As Integer, Shift As Integer, x As Single, Y As Single)
    If Controller.WPCNumbering Then
        Controller.Switch(WPC_Up) = 0
    Else
        Controller.Switch(S11_SWSOUNDDIAG) = 0
    End If
End Sub

Private Sub EnterButton_MouseDown(Button As Integer, Shift As Integer, x As Single, Y As Single)
    If Controller.WPCNumbering Then
        Controller.Switch(WPC_Enter) = 1
    Else
        Controller.Switch(S11_SWADVANCE) = 1
    End If
End Sub

Private Sub EnterButton_MouseUp(Button As Integer, Shift As Integer, x As Single, Y As Single)
    If Controller.WPCNumbering Then
        Controller.Switch(WPC_Enter) = 0
    Else
       Controller.Switch(S11_SWADVANCE) = 0
    End If
End Sub
Private Sub StartButton_Click()
    Controller.SplashInfoLine = "Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext" & vbCrLf & "Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext" & vbCrLf & "Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext Blindtext"
    Controller.HandleMechanics = HandleMech.Text
    Controller.Dip(0) = &H40
    Controller.Dip(1) = &HC1
    Controller.Dip(2) = &HCB
    Controller.Dip(3) = &H3F
    
    On Error Resume Next
    Controller.Run 0
    
    If (Err = 0) Then
        SetButtons 1
        'SetButtons 2       -Doesn't work
    Else
        MsgBox Err.Description, vbOKOnly
    End If
End Sub

Private Sub StopButton_Click()
    Controller.Stop
    SetButtons 0
End Sub

Private Sub UpdateSettings()
    If (Controller.HandleKeyboard) Then
        EnableKeyboard.Value = 1
    Else
        EnableKeyboard.Value = 0
    End If
        
    If (Controller.ShowDMDOnly) Then
        ShowDMDOnly.Value = 1
    Else
        ShowDMDOnly.Value = 0
    End If
        
    If (Controller.ShowTitle) Then
        ShowTitle.Value = 1
    Else
        ShowTitle.Value = 0
    End If
        
    If (Controller.DoubleSize) Then
        DoubleSize.Value = 1
    Else
        DoubleSize.Value = 0
    End If
    
    If (Controller.Version >= "01200000") Then
        If (Controller.Games(Controller.GameName).Settings.Value("debug")) Then
            DebugBox.Value = 1
        Else
            DebugBox.Value = 0
        End If
    End If
End Sub

Private Sub Form_Load()
    SetButtons 0
    Set Controller = CreateObject("VPinMAME.Controller")
    
    DebugBox.Enabled = (Controller.Version >= "01200000")
    
    UpdateSettings
    
    EnableKeyboard.Value = 1
    
    HandleMech.Text = 255
        
    Dim Machines, GameName
 
    GameNames.AddItem ("<Default>")
    
    If GetSetting("VPinMAMETest", "Settings", "GameName", "NotFound") = "NotFound" Then
        SaveSetting "VPinMAMETest", "Settings", "GameName", "<Default>"
    End If
    Machines = Controller.Machines
    If (Not IsEmpty(Machines)) Then
        For Each GameName In Machines
            GameNames.AddItem (GameName)
        Next
    End If
'    GameNames.Text = "<Default>"
    GameNames.Text = GetSetting("VPinMAMETest", "Settings", "GameName")
End Sub
Private Sub Form_Unload(Cancel As Integer)
    SaveSetting "VPinMAMETest", "Settings", "GameName", GameNames.Text
    Set Controller = Nothing
End Sub

Private Sub Controller_OnStateChange(ByVal nState As Long)
    SetButtons nState
End Sub

Private Sub Controller_OnSolenoid(ByVal nSolenoid As Long, ByVal IsActive As Long)
    Dim Msg As String
    
    Msg = "Solenoid " & nSolenoid & " is "
    If (IsActive <> 0) Then
        Msg = Msg & "on"
    Else
        Msg = Msg & "off"
    End If
    
    SolenoidList.AddItem (Msg)
    SolenoidList.ListIndex = SolenoidList.ListCount - 1
    SolenoidList.ListIndex = -1
End Sub

Private Sub SetButtons(ByVal i As Long)
    StartButton.Enabled = (GameNames.Text <> "") And (i = 0)
    StopButton.Enabled = (i = 1)
    EscapeButton.Enabled = (i = 1)
    DownButton.Enabled = (i = 1)
    UpButton.Enabled = (i = 1)
    EnterButton.Enabled = (i = 1)
    PauseButton.Enabled = (i = 1)
    ResumeButton.Enabled = (i = 1)
End Sub

Private Sub WindowPosX_Change()
    If (WindowPosX.Text <> "") Then
        Controller.WindowPosX = CInt(WindowPosX.Text)
    End If
End Sub

Private Sub WindowPosY_Change()
    If (WindowPosY.Text <> "") Then
        Controller.WindowPosY = CInt(WindowPosY.Text)
    End If
End Sub


