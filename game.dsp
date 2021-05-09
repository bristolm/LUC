# Microsoft Developer Studio Project File - Name="game" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=game - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "game.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "game.mak" CFG="game - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "game - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "game - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "game - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GAME_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GAME_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"\Games\Quake2\demo\gamex86.dll"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GAME_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GAME_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"\Games\Quake2\demo\gamex86.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "game - Win32 Release"
# Name "game - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\g_ai.c
# End Source File
# Begin Source File

SOURCE=.\g_chase.c
# End Source File
# Begin Source File

SOURCE=.\g_cmds.c
# End Source File
# Begin Source File

SOURCE=.\g_combat.c
# End Source File
# Begin Source File

SOURCE=.\g_func.c
# End Source File
# Begin Source File

SOURCE=.\g_items.c
# End Source File
# Begin Source File

SOURCE=.\g_main.c
# End Source File
# Begin Source File

SOURCE=.\g_misc.c
# End Source File
# Begin Source File

SOURCE=.\g_monster.c
# End Source File
# Begin Source File

SOURCE=.\g_phys.c
# End Source File
# Begin Source File

SOURCE=.\g_save.c
# End Source File
# Begin Source File

SOURCE=.\g_spawn.c
# End Source File
# Begin Source File

SOURCE=.\g_svcmds.c
# End Source File
# Begin Source File

SOURCE=.\g_target.c
# End Source File
# Begin Source File

SOURCE=.\g_trigger.c
# End Source File
# Begin Source File

SOURCE=.\g_utils.c
# End Source File
# Begin Source File

SOURCE=.\g_weapon.c
# End Source File
# Begin Source File

SOURCE=.\game.def
# End Source File
# Begin Source File

SOURCE=.\luc_beetle.c
# End Source File
# Begin Source File

SOURCE=.\luc_blastdamage.c
# End Source File
# Begin Source File

SOURCE=.\luc_decoy.c
# End Source File
# Begin Source File

SOURCE=.\luc_g_bubbles.c
# End Source File
# Begin Source File

SOURCE=.\luc_g_func.c
# End Source File
# Begin Source File

SOURCE=.\luc_g_local.c
# End Source File
# Begin Source File

SOURCE=.\luc_g_misc.c
# End Source File
# Begin Source File

SOURCE=.\luc_g_miscbots.c
# End Source File
# Begin Source File

SOURCE=.\luc_g_sprite.c
# End Source File
# Begin Source File

SOURCE=.\luc_g_weapon.c
# End Source File
# Begin Source File

SOURCE=.\luc_hbot.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_ambush.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_battle.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_bot.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_bot_wbeam.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_bot_wclaw.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_bot_wcoil.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_fatman.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_gbot.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_gwhite.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_gwhite_ext.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\luc_m_jelly.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_kelp.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_littleboy.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_repeater.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_supervisor.c
# End Source File
# Begin Source File

SOURCE=.\luc_m_turret_s.c
# End Source File
# Begin Source File

SOURCE=.\luc_powerups.c
# End Source File
# Begin Source File

SOURCE=.\luc_teleporter.c
# End Source File
# Begin Source File

SOURCE=.\luc_utils.c
# End Source File
# Begin Source File

SOURCE=.\luc_warp.c
# End Source File
# Begin Source File

SOURCE=.\m_flash.c
# End Source File
# Begin Source File

SOURCE=.\m_move.c
# End Source File
# Begin Source File

SOURCE=.\p_client.c
# End Source File
# Begin Source File

SOURCE=.\p_hud.c
# End Source File
# Begin Source File

SOURCE=.\p_trail.c
# End Source File
# Begin Source File

SOURCE=.\p_view.c
# End Source File
# Begin Source File

SOURCE=.\p_weapon.c
# End Source File
# Begin Source File

SOURCE=.\q_shared.c
# End Source File
# Begin Source File

SOURCE=.\resource.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\g_local.h
# End Source File
# Begin Source File

SOURCE=.\game.h
# End Source File
# Begin Source File

SOURCE=.\luc_blastdamage.h
# End Source File
# Begin Source File

SOURCE=.\luc_decoy.h
# End Source File
# Begin Source File

SOURCE=.\luc_g_local.h
# End Source File
# Begin Source File

SOURCE=.\luc_hbot.h
# End Source File
# Begin Source File

SOURCE=.\luc_m_ambush.h
# End Source File
# Begin Source File

SOURCE=.\luc_m_battle.h
# End Source File
# Begin Source File

SOURCE=.\luc_m_botframes.h
# End Source File
# Begin Source File

SOURCE=.\luc_m_bots.h
# End Source File
# Begin Source File

SOURCE=.\luc_m_fatman.h
# End Source File
# Begin Source File

SOURCE=.\luc_m_gbot.h
# End Source File
# Begin Source File

SOURCE=.\luc_m_gwhite.h
# End Source File
# Begin Source File

SOURCE=.\luc_m_gwhite_ext.h

!IF  "$(CFG)" == "game - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\luc_m_repeater.h
# End Source File
# Begin Source File

SOURCE=.\luc_m_supervisor.h
# End Source File
# Begin Source File

SOURCE=.\luc_m_turret_s.h
# End Source File
# Begin Source File

SOURCE=.\luc_powerups.h
# End Source File
# Begin Source File

SOURCE=.\luc_teleporter.h
# End Source File
# Begin Source File

SOURCE=.\luc_warp.h
# End Source File
# Begin Source File

SOURCE=.\m_player.h
# End Source File
# Begin Source File

SOURCE=.\q_shared.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\game.dsp
# End Source File
# Begin Source File

SOURCE=.\game.dsw
# End Source File
# Begin Source File

SOURCE=.\game.opt
# End Source File
# Begin Source File

SOURCE=.\resource.aps
# End Source File
# End Target
# End Project
