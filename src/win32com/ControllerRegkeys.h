#ifndef CONTROLLERREGKEYS_H
#define CONTROLLERREGKEYS_H

#define REG_BASEKEY		"Software\\Freeware\\Visual PinMame"
#define REG_DEFAULT		"default"
#define REG_GLOBALS		"globals"

// these values are handle by the ISettings object and the ShowSettingsDlg

// game specific values
#define REG_DWUSESAMPLES				"UseSamples"
#define REG_DWUSESAMPLESDEF				1
#define REG_DWUSECHEAT					"UseCheat"
#define REG_DWUSECHEATDEF				1
#define REG_DWUSESOUND					"UseSound"
#define REG_DWUSESOUNDDEF				1
#define REG_DWSAMPLERATE				"SampleRate"
#define REG_DWSAMPLERATEDEF				44100
#define REG_DWSPEEDLIMIT				"SpeedLimit"
#define REG_DWSPEEDLIMITDEF				1
#define REG_DWDMDRED					"DMDRed"
#define REG_DWDMDREDDEF					255
#define REG_DWDMDGREEN					"DMDGreen"
#define REG_DWDMDGREENDEF				88
#define REG_DWDMDBLUE					"DMDBlue"
#define REG_DWDMDBLUEDEF				32
#define REG_DWDMDPERC66					"DMDPerc66"
#define REG_DWDMDPERC66DEF				80
#define REG_DWDMDPERC33					"DMDPerc33"
#define REG_DWDMDPERC33DEF				60
#define REG_DWDMDPERC0					"DMDPerc0"
#define REG_DWDMDPERC0DEF				20
#define REG_DWANTIALIAS					"AntiAlias"
#define REG_DWANTIALIASDEF				50
#define REG_DWTITLE						"Title"
#define REG_DWTITLEDEF					1
#define REG_DWBORDER					"Border"
#define REG_DWBORDERDEF					1
#define REG_DWDISPLAYONLY				"DisplayOnly"
#define REG_DWDISPLAYONLYDEF			0
#define REG_DWCOMPACTSIZE				"CompactSize"
#define REG_DWCOMPACTSIZEDEF			0
#define REG_DWDOUBLESIZE				"DoubleSize"
#define REG_DWDOUBLESIZEDEF				0
#define REG_DWWINDOWPOSX				"WindowPosX"
#define REG_DWWINDOWPOSXDEF				0
#define REG_DWWINDOWPOSY				"WindowPosY"
#define REG_DWWINDOWPOSYDEF				0

// this one is the only which can't set by a property
#define REG_DWALLOWWRITEACCESS			"AllowWriteAccess"
#define REG_DWALLOWWRITEACCESSDEF		1

#endif // CONTROLLERREGKEYS_H
