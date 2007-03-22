                            >  ;�     <  [   	  @^    [ ] use "apFrame.inc"[ ] [ ] //GLOBAL VARIABLES[ ] ARRAY[8] OF ANYTYPE aaData[ ] STRING sTestArea[ ] STRING sTestCoverage[ ] STRING sTestName[ ] STRING sTestFile // file to open for tests[ ] STRING sTestCaption // caption of sTestFile[ ] STRING sReport // name of the report file[ ] STRING MOZILLA_HOME = ReadMozillaHome() // home directory of seamonkey executable (Apprunner)[ ] HFILE hReport,hData[ ] [+] Debug()	[ ] [+] DebugStr(STRING s)	[-] if (bDebug == true)		[ ] Print(s)[+] // INT OpenEditor()	[ ] // Apprunner.Invoke()	[ ] // Apprunner.Tasks.Editor.Pick()	[ ] // return 1[+] // INT DoStyleTest()	[ ] // BOOLEAN bCompare	[ ] // INT iOrigCRC,iNewOrigCRC,iAlteredCRC,iNewAlteredCRC	[ ] // STRING sBaseline = "{MOZILLA_HOME}:tools:tests:mac:Enderb.bmp"	[ ] // STRING sNewBaseline = "{MOZILLA_HOME}:tools:tests:mac:Endernew.bmp"	[ ] // STRING sAltered = "{MOZILLA_HOME}:tools:tests:mac:Ender2.bmp"	[ ] // STRING sNewAltered = "{MOZILLA_HOME}:tools:tests:mac:Ender2new.bmp"	[ ] // STRING sMask = "{MOZILLA_HOME}:tools:tests:mac:Enderb.mask"	[ ] // RECT rTest = {72,175,70,70}	[ ] // iOrigCRC = SYS_GetBitmapCRC(sBaseline,sMask)	[ ] // iAlteredCRC = SYS_GetBitmapCRC(sAltered,sMask)	[ ] // OpenEditor()	[ ] // Apprunner.Tasks.Editor.Pick()	[ ] // Agent.SetOption(OPT_BITMAP_MATCH_COUNT, 1) 	[ ] // Agent.SetOption(OPT_BITMAP_MATCH_INTERVAL, .1)	[ ] // //EnderHTMLTestPage.CaptureBitmap(sBaseline,rTest)	[ ] // sleep(10)	[ ] // EnderHTMLTestPage.Size(560,518)	[ ] // EnderHTMLTestPage.VerifyBitmap(sBaseline,rTest)	[ ] // Agent.SetOption (OPT_MOUSE_DELAY, 1)	[ ] // Mozilla.PressMouse (1, 10, 157)	[ ] // Mozilla.ReleaseMouse (1, 303, 220)	[ ] // EnderHTMLTestPage.Bold.Click ()	[ ] // EnderHTMLTestPage.Italic.Click ()	[ ] // EnderHTMLTestPage.Underline.Click ()	[ ] // Agent.SetOption (OPT_MOUSE_DELAY, 0)	[ ] // EnderHTMLTestPage.Click(1,10,157)	[ ] // sleep(10)	[ ] // //EnderHTMLTestPage.CaptureBitmap(sAltered,rTest)	[ ] // EnderHTMLTestPage.VerifyBitmap(sAltered,rTest)	[ ] // EnderHTMLTestPage.Close()	[ ] // return (1)[+] // INT InsertText()	[ ] // LIST OF STRING ls	[ ] // Agent.SetOption (OPT_MOUSE_DELAY, 1)	[ ] // Apprunner.Tasks.Editor.Pick()	[ ] // sleep(20)	[ ] // EnderHTMLTestPage.Size(560,518)	[ ] // Apprunner.Debug.InsertText.Pick()	[ ] // EnderHTMLTestPage.Click (1, 65, 42)	[ ] // EnderHTMLTestPage.PressMouse (1, 13, 42)	[ ] // EnderHTMLTestPage.ReleaseMouse (1, 259, 41)	[ ] // Apprunner.Edit.Copy.Pick ()	[ ] // ls = Clipboard.GetText()	[ ] // Verify(ls[1],"Once more into the breach, dear friends.")	[ ] // EnderHTMLTestPage.Close()	[ ] // Agent.SetOption (OPT_MOUSE_DELAY, 0)	[ ] // return (1)[+] INT DoBasicTest()	[-] switch (aaData[4])		[-] case "ClickOnBackForward"			[ ] return DoBackClick()		[-] case "OpenNewBrowserWin"			[ ] return OpenNewWin()		[-] case "OpenFile"			[ ] sTestFile = aaData[1]			[ ] sTestCaption = aaData[6]			[ ] return OpenFile(sTestFile,sTestCaption)		[+] // case "InsertText"			[ ] // return InsertText()		[+] // case "StyleText"			[ ] // return DoStyleTest()		[-] default			[ ] return 0[+] INT DoWinScroll()	[ ] // DoWinScroll	[ ] // Opens a window and takes a bitmap of a rectangle within the window, then clicks in the scrollbar. Compares	[ ] // new bitmap of the same rectangle and verifies that it is different. Then clicks in the scrollbar	[ ] // high enough to set the scroll bar to its original (y = 0) position. Verifies that the bitmaps are	[ ] // once again identical.	[ ] 	[ ] //Local Variable declarations	[ ] INT iCRC // checksum for bitmap region	[ ] INT iRes // numerical result for return value	[ ] BOOLEAN bMatch	[ ] STRING sRes // Result of test returned as a string	[ ] STRING sPrintStr // string to print to report file	[ ] STRING sNewWindow // window that actually loads	[ ] STRING sBitmap // File to store bitmap	[ ] RECT rTextRect //a section of text in the window to be scrolled	[ ] RECT rWinRect // rectangle for Mozilla startup window	[ ] 	[ ] //Variable assignment--aaData is a global array that  has been read from data file	[ ] sBitmap = "winscroll.bmp"	[ ] sURL = aaData[1]	[ ] rWinRect = Mozilla.GetRect()	[ ] rTextRect = aaData[5]	[ ] AppRunnerBMP(sBitmap,rTextRect)	[ ] iBaselineCRC = SYS_GetBitmapCRC(sBitmap)	[ ] iCRC = 0	[ ] bMatch = false	[ ] WaitWindowCaption(sWindowName)	[ ] iCRC = Mozilla.GetBitmapCRC(rTextRect)	[ ] Verify(iCRC,iBaselineCRC,"bitmap matches baseline")	[ ] Mozilla.Click (1, rWinRect.xSize-22, 300) //click low in scrollbar	[-] if(iBaselineCRC == Mozilla.GetBitmapCRC(rTextRect))		[ ] bMatch = true	[-] if (bMatch)		[ ] iRes = -1		[ ] return iRes	[ ] Mozilla.Click (1, rWinRect.xSize-22, 100) //click high	[ ] sleep(10)	[-] if (Mozilla.GetBitmapCRC(rTextRect) == iBaselineCRC)		[ ] iRes = 1	[-] else		[ ] iRes = -1	[ ] return iRes[+] INT DoBackClick()	[ ] STRING sWin1,sWin2	[ ] Mozilla.SetActive()	[ ] Mozilla.MyNetscape.Click()	[ ] sWin1 = Mozilla.GetCaption()	[ ] Mozilla.Back.Click()	[ ] sWin2 = Mozilla.GetCaption()	[-] if (sWin1 == sWin2)		[ ] return E_BACK_BUTTON_FAILS	[ ] Mozilla.Forward.Click()	[-] if (sWin1 != Mozilla.GetCaption())		[ ] return E_FORWARD_BUTTON_FAILS	[ ] return 1[+] INT OpenNewWin()	[ ] Mozilla.SetActive()	[ ] Apprunner.File.NewBrowserWindow.Pick()	[ ] sleep(30)	[-] if (Apprunner.ChildWin("#2").Exists())		[-] do			[ ] Verify(Apprunner.ChildWin("#1").GetCaption(),sWindowName)			[ ] Apprunner.ChildWin("#1").Close()			[ ] return 1		[-] except			[ ] Print(ExceptData())			[ ] return E_WIN_NOT_FOUND	[-] else		[ ] Print(ExceptData())		[ ] return E_WIN_NOT_OPEN[+] INT OpenFile(STRING s, STRING sName optional)	[ ] LIST OF STRING ls	[ ] INT iTimes = 0	[ ] Apprunner.File.OpenFile.Pick()	[ ] sleep(5)	[ ] OpenLocation.SelectFile.Click()	[ ] sleep(5)	[ ] ls = String2List(s)	[ ] OpenFileDialog.OpenFile(ls)	[-] if (sName != NULL)		[-] while (Apprunner.ChildWin("#1").GetCaption() != sName)			[ ] sleep(1)			[ ] iTimes++			[-] if (iTimes > 30)				[ ] raise E_WIN_NOT_FOUND, "Window does not have expected caption"	[ ] Apprunner.ChildWin("#1").Close()	[-] if (OpenLocation.Exists())		[ ] OpenLocation.SetActive()		[ ] OpenLocation.Close()	[ ] return 1[+] SetSeamonkeyOptions()	[ ] // QAP options necessary for running these tests	[ ] Agent.SetOption(OPT_VERIFY_EXPOSED, false)	[ ] Agent.SetOption(OPT_VERIFY_UNIQUE, false)	[ ] Agent.SetOption(OPT_VERIFY_ENABLED, false)	[ ] Agent.SetOption(OPT_WINDOW_TIMEOUT, 60) // maximum time QAP will wait for a window	[ ] Agent.SetOption(OPT_BITMAP_MATCH_COUNT, 1) 	[ ] Agent.SetOption(OPT_BITMAP_MATCH_INTERVAL, .1)	[ ] Agent.SetOption(OPT_BITMAP_MATCH_TIMEOUT, 60) // maximum time QAP will wait for a bitmap to match[+] WaitWindowCaption(STRING s,INT iWait optional)	[ ] INT i = 0	[-] if (iWait==NULL)		[ ] iWait = 60	[-] while (StrPos(Mozilla.GetCaption(), s) == 0)		[ ] sleep (1)		[ ] i++		[-] if ( i > iWait)			[ ] raise 1, "Expected window did not load: test {sTestName} FAILS"[+] STRING GetBuildNum()	[ ] HFILE hXUL	[ ] STRING sLine,sBuildID	[-] if (bDebug == true)		[ ] hXUL = FileOpen("MacHDD:mozilla:dist:viewer:Chrome:Navigator:locale:en-US:navigator.dtd",FM_READ)		[-] while (FileReadLine(hXUL,sLine))			[ ] INT iPos = StrPos(sLine,"Build ID:")			[-] if (iPos > 0)				[ ] sBuildID = SubStr(sLine,12,10)				[ ] DebugStr(sBuildID)	[-] else		[ ] hXUL = FileOpen("BigFatBoy:Mozilla Tree:mozilla:config:build_number",FM_READ)		[ ] FileReadLine(hXUL,sLine)		[ ] sBuildID = sLine	[ ] return (sBuildID)[+] AppRunnerBMP(STRING sBMP, RECT rVerify)	[ ] // captures a bitmap	[ ] // this should be repeated before each run	[ ] sleep(2)	[ ] Mozilla.CaptureBitmap(sBMP,rVerify)[+] INT DoMailTest(ARRAY OF STRING aaArray)	[ ] DebugStr("Do Mail Test")	[ ] return(1)[+] INT DoEditTest(ARRAY OF STRING aaArray)	[ ] DebugStr("Do Edit Test")	[ ] return(1)[-] INT DoBrowserTest(ARRAY[6] OF ANYTYPE aaArray)	[ ] //Local Variable declarations	[ ] INT iCRC // checksum for bitmap region	[ ] INT iTimes // loop variable for checksum verification	[ ] INT iExcept // exception number returned by QA Partner	[ ] INT iRes // numerical result for return value	[ ] INT iFound // tests whether search string is found in window caption	[ ] BOOLEAN bDone // Flag for completion of checksum verification	[ ] BOOLEAN bTimedOut // If bitmap doesn't match, also gives information about time	[ ] STRING sRes // Result of test returned as a string	[ ] STRING sPrintStr // string to print to report file	[ ] STRING sNewWindow // window that actually loads	[ ] HTIMER hTimer // timer handle	[ ] NUMBER nSeconds // seconds elapsed since beginning of checksum verification	[ ] RECT rRect // rectangle of bitmap for verification--must include part of throbber	[ ] 	[ ] //Variable assignment--aaArray has been read from data file	[ ] sURL = aaArray[1] // URL to Load	[ ] sTestArea = aaArray[2]	[ ] sTestCoverage = aaArray[3]	[ ] sTestName = aaArray[4]	[ ] rRect = rThrobber	[ ] sWindowName = aaArray[6]	[-] if (sTestArea == "stop")		[ ] FileWriteLine(hReport,"{sTestName} TEST NOT RUN: {sTestCoverage}")		[ ] return -1	[ ] Apprunner.Invoke()	[-] do		[ ] iCRC = 0		[ ] iTimes = 0		[ ] nSeconds = 0		[ ] bTimedOut = false		[ ] bDone = false		[ ] hTimer = TimerCreate()		[ ] Mozilla.SetAddress(sURL)		[ ] Mozilla.TypeKeys("<enter>")		[ ] TimerStart(hTimer)		[-] while (bDone == false)				[-] do					[ ] iCRC = Mozilla.GetBitmapCRC(rThrobber)				[-] except					[-] if (ExceptNum()==E_BITMAP_REGION_INVALID)						[ ] Mozilla.Close()				[-] if (iCRC != iBaselineCRC)					[ ] nSeconds = TimerValue(hTimer)					[ ] iTimes = 0					[-] if (nSeconds > iBitmapWait)						[ ] bDone = true				[-] else					[-] switch (iTimes)						[-] case 0							[ ] nSeconds = TimerValue(hTimer)							[ ] iTimes++						[-] case 1,2,3							[ ] iTimes++						[-] case 4							[ ] bDone = true				[-] if(nSeconds > iBitmapWait)					[ ] bDone = true					[ ] DebugStr("***Error: Time Ran Out -{iBitmapWait}")		[ ] sNewWindow = Mozilla.GetCaption()		[-] if (StrPos(sWindowName,sNewWindow) == 0)			[ ] DebugStr("Expected: {sWindowName} Found {sNewWindow}")			[ ] DebugStr("***May be due to changes on site")		[-] do			[ ] Verify(bTimedOut,false,"time less than {iBitmapWait} seconds")		[-] except			[ ] DebugStr("{sTestName} FAILED-E1")			[ ] reraise		[-] do			[ ] Verify(iCRC,iBaselineCRC,"bitmap matches baseline")		[-] except			[ ] DebugStr("{sTestName} FAILED-E1")			[ ] reraise		[ ] iRes = 1		[ ] TimerDestroy(hTimer)	[-] except		[ ] iExcept = ExceptNum()		[-] switch (iExcept)			[-] case E_VERIFY // iCRC does not equal iBaselineCRC				[ ] iRes = -1				[ ] sRes = "FAIL-E1"			[-] case E_WINDOW_NOT_FOUND // Window not found--often because App has crashed				[ ] iRes = -2				[ ] sRes = "FAIL-E2"			[-] case E_BITMAPS_DIFFERENT // same as E1				[ ] iRes = -3				[ ] sRes = "FAIL-E3"			[-] case E_BITMAP_NOT_STABLE // mainly with waitBitmap--possibly hung				[ ] iRes = -4				[ ] sRes = "FAIL-E4"			[-] case E_BITMAP_REGION_INVALID // bitmap region is not inside window boundaries				[ ] iRes = -5				[ ] sRes = "FAIL-E5"			[-] default				[ ] sRes = "FAIL-UNKNOWN FAILURE"				[ ] iRes = -999		[+] // if (ApprunnerError.Exists()) // uncomment line for windows			[ ] // ApprunnerError.Close.Click()		[ ] FileWriteLine(hReport,"{sTestName} {sRes}")		[ ] DebugStr("{sTestName} {sRes}")		[ ] TimerDestroy(hTimer)	[ ] return iRes[-] SmokeTest()	[ ] //Local Variable declarations	[ ] STRING sTimeStamp	[ ] STRING printStr	[ ] STRING sTestType	[ ] INTEGER i,iLoop,iExcept,iPass,iFail,iRan,iRes	[ ] HTIMER hTimer,hStarter	[ ] INTEGER iFound	[ ] BOOLEAN bPass	[ ] Print("Beginning smoke tests at {TimeStr()}")	[ ] sData = "all_smoke.txt"	[ ] Print("Using data file {sData}")	[ ] sTimeStamp = StrTran(TimeStr(),":","")	[ ] sReport = "SmokeReport{sTimeStamp}.txt"	[ ] Print("Using log file {sReport}")	[ ] Print(GetBuildNum())	[ ] hReport = FileOpen(sReport, FM_WRITE)	[ ] sBitmapFile = "throb.bmp"	[ ] sTestCaption = "Preferences"	[ ] hData = FileOpen(sData,FM_READ)	[ ] FileWriteLine(hReport, "Beginning smoke tests at {TimeStr()}")	[ ] FileWriteLine(hReport, "apprunner located at {MOZILLA_HOME}")	[ ] FileWriteLine(hReport, "Using data file {sData}")	[ ] FileWriteLine(hReport, GetBuildNum())	[ ] Agent.SetOption(OPT_VERIFY_EXPOSED, false)	[ ] Agent.SetOption(OPT_VERIFY_UNIQUE, false)	[ ] Agent.SetOption(OPT_VERIFY_ENABLED, false)	[ ] Agent.SetOption(OPT_BITMAP_MATCH_COUNT, 1)	[ ] Agent.SetOption(OPT_BITMAP_MATCH_INTERVAL, .1)	[ ] Agent.SetOption(OPT_BITMAP_MATCH_TIMEOUT, iBitmapWait)	[ ] Apprunner.Invoke()	[ ] sleep(20)	[ ] AppRunnerBMP(sBitmapFile,rThrobber)	[ ] iBaselineCRC = SYS_GetBitmapCRC(sBitmapFile)	[-] while (FileReadValue(hData,aaData))		[ ] sURL = aaData[1]		[ ] sTestArea = aaData[2]		[ ] sTestType = aaData[3]		[ ] sTestName = aaData[4]		[ ] sWindowName = aaData[6]		[-] switch sTestArea			[-] // case "Mail"				[ ] // DoMailTest(aaData)			[-] // case "Editor"				[ ] // DoEditTest(aaData)			[-] case "Startup" // This test must pass or the others will not be run				[-] do					[ ] Apprunner.Invoke()				[-] except					[ ] reraise				[-] do					[ ] iRes = StrPos("Mozilla",Mozilla.GetCaption())				[-] except					[ ] reraise				[-] if (iRes == 0)					[ ] iRes = -1			[-] case "Exit" 				[-] do					[ ] Apprunner.Invoke()				[-] except					[ ] DebugStr("{sTestName} FAIL {ExceptData()}")					[ ] reraise				[-] do					[ ] Mozilla.Close()					[ ] iRes = 1				[-] except					[ ] iRes = -1			[-] case "Top Site", "Frames", "Tables","Java","JavaScript Applets","PNG Images","JPEG Images","Transparencies","Java Applets","Multilingual UTF-8","Multilingual NCR"				[-] do					[ ] Apprunner.Invoke()					[ ] iRes = DoBrowserTest(aaData)				[-] except					[ ] FileWriteLine(hReport,"{sTestName} FAILED")					[ ] DebugStr("{sTestName} FAILED")			[-] case "Basic Functional Test"				[-] do					[-] if (sTestType == "Browser")						[ ] Apprunner.Invoke()						[ ] Mozilla.SetAddress(sURL)						[ ] Mozilla.TypeKeys("<enter>")					[ ] iRes = DoBasicTest()				[-] except					[ ] Print(ExceptData())					[-] if (sTestType == "Ender")						[ ] EnderHTMLTestPage.Close()					[ ] iRes = -1			[-] case "WindowScrolling"				[-] do					[ ] Apprunner.Invoke()					[ ] Mozilla.SetAddress(sURL)					[ ] Mozilla.TypeKeys("<enter>")					[ ] sleep(10)					[ ] iRes = DoWinScroll()				[-] except					[ ] Print(ExceptData())					[ ] iRes = -1			[-] default				[ ] iRes = 0		[+] if (iRes > 0)			[ ] FileWriteLine (hReport, "{sTestName} PASSED")			[ ] DebugStr("{sTestName} PASS")		[-] else if (iRes < 0)			[ ] FileWriteLine (hReport, "{sTestName} FAILED")			[ ] DebugStr("{sTestName} FAIL")		[+] else if (iRes == 0)			[ ] FileWriteLine(hReport,"{sTestName} NOT RUN")			[ ] DebugStr("{sTestName} NOT RUN")		[-] do			[ ] Mozilla.Close()		[-] except			[-] if (sTestArea == "Exit")				[ ] 			[-] else				[ ] Apprunner.Invoke()	[ ] FileWriteLine(hReport,"Smoke test complete at {TimeStr()}")	[ ] Print("Smoke test complete at {TimeStr()}")[-] main()	[ ] SmokeTest()     �  �   �� �2"}i�D��d�yZER��*x��e�xC]�Mީ5�*�¿�P�O ((���W�W"#I2�����y��^�;����rTe�st�VW�����hrFT��{��J:��J�c,�O��� b������)(�*�ų�ryl���B�V A���T{�,meU�K�,9U�f��9~�~�ԛ֓�ÎF&�战�I*H=GQ����
⫸|8� ��6�fSC~�6   H 	Monaco                              ht�h ht�h                    >��B�?�20         ���ҳ���                amasri%netscape.com 1.7 smoketest.t   �added extra tests for version 2.0added timing changes to help stabilize bitmapadded functional tests for version 2.0added error handling to account for windows not opening completelychanged default URL loading method to Mozilla.LoadURL() NOTE: Smoketest is now dependant only on correct button positions for "OpenFile" dialog.added full path OpenFile() capabilities on mac, plus String2List utility to permit graphic file interface to be manipulatedAdded SetActive() to OpenNewWin()    H��        smoketest.t 1.7   -kb            mozilla/tools/tests/mac     �  �   ���&�    Z MPSR   ckid   &mcvs   2���    �� �     L     �   �    Projector DataMacCVS Version ResourceTEXTQAP2 ����                  