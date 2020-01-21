SET font=%1
SET nopause=%2
cd fnt
fnt_dec.exe system%font%.fnt
del system%font%.fnt_INDEX
cd ..
fnt_make_ft.exe tbl_chs.txt %font%
move /Y tbl_fnt\* fnt\system%font%.fnt_unpack\
move /Y tbl_cell.txt fnt\
move /Y tbl_xy.txt fnt\
del /S /Q tbl_fnt\*.*
rd /S /Q tbl_fnt
cd fnt
fnt_build.exe system%font%.fnt
del /S /Q system%font%.fnt_unpack\*.*
rd /S /Q system%font%.fnt_unpack
del /S /Q tbl_cell.txt
del /S /Q tbl_xy.txt
del /S /Q system%font%.fnt_new_INDEX
cd ..
move /Y fnt\system%font%.fnt_new UpdateCHS\
if exist UpdateCHS\system%font%.fnt (del UpdateCHS\system%font%.fnt)
cd UpdateCHS
ren system%font%.fnt_new system%font%.fnt
cd ..
if "%nopause%" == "" (pause)
