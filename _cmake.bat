set DirExport=%CD%

rmdir /S /Q "%DirExport%/_Build_wnd"
mkdir "%DirExport%/_Build_wnd"

cd "%DirExport%/_Build_wnd"

call cmake -G "Visual Studio 17 2022" -A x64 "%DirExport%"

cd ..