.NAME                        := xflash.exe

VCLIBPath                     := $(VCPath)\lib
SDKLIBPath                    := $(SDKPath)\Lib
VC90PDBName                   := vc90.pdb
VC100PDBName                  := vc100.pdb
VCPDBName                     := $(if $(VC2010),$(VC100PDBName),$(VC90PDBName))


XflashExe.CCFLAGS.Debug   := $(INCLUDE_PATH) /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Gm /EHsc /RTC1 /MTd  /Fd"$(OUTPUT_PATH)\vc90.pdb" /W3 /nologo /c /ZI /TP /errorReport:prompt
XflashExe.CCFLAGS.Release := $(INCLUDE_PATH) /O2 /Oi /GL /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FD /EHsc /MTd /Gy  /Fd"$(OUTPUT_PATH)\vc90.pdb" /W3 /nologo /c /Zi /TP /errorReport:prompt


XflashExe.CCFLAGS.Debug := $(XflashExe.CCFLAGS.Debug)
XflashExe.CCFLAGS.Release := $(XflashExe.CCFLAGS.Release)

BOOST_LIB_PATH	:= D:\home\boost157\lib
QT4_LIB_PATH	:= C:\QtSDK\Desktop\Qt\4.7.4\msvc2008\lib

XflashExe.DLLFLAGS.Release:= /LIBPATH:"$(SDKLIBPath)" /LIBPATH:"$(VCLIBPath)"  /LIBPATH:"$(BOOST_LIB_PATH)"  /LIBPATH:"$(QT4_LIB_PATH)"  /INCREMENTAL:NO /NOLOGO /DLL /MANIFEST /MANIFESTFILE:"$(OUTPUT_PATH)\xflash.intermediate.manifest"  /MAP:"$(OUTPUT_PATH)/xflash.map" /MAPINFO:EXPORTS /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /NODEFAULTLIB:"LIBCMT.lib" /DEBUG /PDB:"$(OUTPUT_PATH)\xflash.pdb" /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /LTCG /DYNAMICBASE /NXCOMPAT /MACHINE:X86 /ERRORREPORT:PROMPT  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib xflash-lib.lib QtCore4.lib

XflashExe.DLLFLAGS.Debug  := /LIBPATH:"$(SDKLIBPath)" /LIBPATH:"$(VCLIBPath)"  /LIBPATH:"$(BOOST_LIB_PATH)"  /LIBPATH:"$(QT4_LIB_PATH)"  /INCREMENTAL /NOLOGO /DLL /MANIFEST /MANIFESTFILE:"$(OUTPUT_PATH)\xflash.intermediate.manifest" /MAP:"$(OUTPUT_PATH)/xflash.map" /MAPINFO:EXPORTS /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /NODEFAULTLIB:"LIBCMT.lib" /DEBUG /PDB:"$(OUTPUT_PATH)\xflash.pdb" /SUBSYSTEM:WINDOWS /DYNAMICBASE /NXCOMPAT /MACHINE:X86 /ERRORREPORT:PROMPT  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib xflash-lib.lib QtCore4.lib



XflashExe.DLLFLAGS.Debug := $(XflashExe.DLLFLAGS.Debug)
XflashExe.DLLFLAGS.Release:= $(XflashExe.DLLFLAGS.Release)


rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

XflashExe.SRCLIST := \
    $(call rwildcard,xmain/,*.cpp)	\
	  $(call rwildcard,core/,*.cpp)
	