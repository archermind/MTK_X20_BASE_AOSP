.NAME                        := XFlashLib

VCLIBPath                     := $(VCPath)\lib
SDKLIBPath                    := $(SDKPath)\Lib
VC90PDBName                   := vc90.pdb
VC100PDBName                  := vc100.pdb
VCPDBName                     := $(if $(VC2010),$(VC100PDBName),$(VC90PDBName))

	   
XFlashLib.CCFLAGS.Debug.Dy   := $(INCLUDE_PATH) /Od  /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "XFLASH_EXPORTS" /D "_WINDLL" /D "_MBCS" /Gm /EHsc /RTC1 /MTd  /Fd"$(OUTPUT_PATH)\vc90.pdb" /W3 /nologo /c /ZI /TP /errorReport:prompt  
XFlashLib.CCFLAGS.Release.Dy := $(INCLUDE_PATH) /O2 /Oi /GL  /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "XFLASH_EXPORTS" /D "_WINDLL" /D "_MBCS" /FD /EHsc /MTd /Gy  /Fd"$(OUTPUT_PATH)\vc90.pdb" /W3 /nologo /c /Zi /TP /errorReport:prompt
 

XFlashLib.CCFLAGS.Debug.St   := $(INCLUDE_PATH) /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "XFLASH_EXPORTS" /D "_MBCS" /Gm /EHsc /RTC1 /MTd  /Fd"$(OUTPUT_PATH)\vc90.pdb" /W3 /nologo /c /ZI /TP /errorReport:prompt
XFlashLib.CCFLAGS.Release.St := $(INCLUDE_PATH) /O2 /Oi /GL /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "XFLASH_EXPORTS" /D "_MBCS" /FD /EHsc /MTd /Gy  /Fd"$(OUTPUT_PATH)\vc90.pdb" /W3 /nologo /c /Zi /TP /errorReport:prompt


XFlashLib.CCFLAGS.Debug := $(if $(findstring $(LIB_DEFAULT_TYPE),$(LIB_TYPE)),$(XFlashLib.CCFLAGS.Debug.Dy),$(XFlashLib.CCFLAGS.Debug.St))
XFlashLib.CCFLAGS.Release := $(if $(findstring $(LIB_DEFAULT_TYPE),$(LIB_TYPE)),$(XFlashLib.CCFLAGS.Release.Dy),$(XFlashLib.CCFLAGS.Release.St))

BOOST_LIB_PATH	:= D:\home\boost157\lib

#dynamic library
XFlashLib.DLLFLAGS.Release.Dy:= /LIBPATH:"$(SDKLIBPath)" /LIBPATH:"$(VCLIBPath)"  /LIBPATH:"$(BOOST_LIB_PATH)"  /INCREMENTAL:NO /NOLOGO /DLL /MANIFEST /MANIFESTFILE:"$(OUTPUT_PATH)\xflash-lib.dll.intermediate.manifest"  /MAP:"$(OUTPUT_PATH)/xflash-lib.map" /MAPINFO:EXPORTS /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /NODEFAULTLIB:"LIBCMT.lib" /DEBUG /PDB:"$(OUTPUT_PATH)\xflash-lib.pdb" /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /LTCG /DYNAMICBASE /NXCOMPAT /MACHINE:X86 /ERRORREPORT:PROMPT ./lib/win/libyaml-cppmdd.lib ./lib/win/iconv-MT.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

XFlashLib.DLLFLAGS.Debug.Dy  := /LIBPATH:"$(SDKLIBPath)" /LIBPATH:"$(VCLIBPath)"  /LIBPATH:"$(BOOST_LIB_PATH)"  /INCREMENTAL /NOLOGO /DLL /MANIFEST /MANIFESTFILE:"$(OUTPUT_PATH)\xflash-lib.dll.intermediate.manifest" /MAP:"$(OUTPUT_PATH)/xflash-lib.map" /MAPINFO:EXPORTS /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /NODEFAULTLIB:"LIBCMT.lib" /DEBUG /PDB:"$(OUTPUT_PATH)\xflash-lib.pdb" /SUBSYSTEM:WINDOWS /DYNAMICBASE /NXCOMPAT /MACHINE:X86 /ERRORREPORT:PROMPT ./lib/win/libyaml-cppmdd.lib ./lib/win/iconv-MTd.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

		
#static library
XFlashLib.DLLFLAGS.St  :=    /LIBPATH:"$(SDKLIBPath)" \
                               /LIBPATH:"$(VCLIBPath)"  \
                               /LIBPATH:"$(LIB_PATH)"  \
							   /LIBPATH:"$(BOOST_LIB_PATH)" \
							    /NODEFAULTLIB:"libc.lib" \
                                /NODEFAULTLIB:"libcd.lib" \
                                /NODEFAULTLIB:"msvcrt.lib" \
                                /NODEFAULTLIB:"msvcrtd.lib" \
								/NODEFAULTLIB:"libcpmdt.lib" \
								./lib/libyaml-cppmdd.lib 

XFlashLib.DLLFLAGS.Debug.St  :=  /NODEFAULTLIB:"LIBCMT.lib" $(XFlashLib.DLLFLAGS.St)
XFlashLib.DLLFLAGS.Release.St:=  /LTCG /NODEFAULTLIB:"LIBCMT.lib" $(XFlashLib.DLLFLAGS.St)

XFlashLib.DLLFLAGS.Debug := $(if $(findstring $(LIB_DEFAULT_TYPE),$(LIB_TYPE)),$(XFlashLib.DLLFLAGS.Debug.Dy),$(XFlashLib.DLLFLAGS.Debug.St))
XFlashLib.DLLFLAGS.Release:=$(if $(findstring $(LIB_DEFAULT_TYPE),$(LIB_TYPE)),$(XFlashLib.DLLFLAGS.Release.Dy),$(XFlashLib.DLLFLAGS.Release.St))


rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

XFlashLib.SRCLIST := \
	$(call rwildcard,arch/win,*.cpp)	\
	$(call rwildcard,brom/,*.cpp)	\
	$(call rwildcard,common/,*.cpp)	\
	$(call rwildcard,config/,*.cpp)	\
	$(call rwildcard,loader/,*.cpp)	\
	$(call rwildcard,functions/,*.cpp)	\
	$(call rwildcard,interface/,*.cpp)	\
	$(call rwildcard,lib/,*.cpp)	\
	$(call rwildcard,logic/,*.cpp)	\
	$(call rwildcard,transfer/,*.cpp)
	