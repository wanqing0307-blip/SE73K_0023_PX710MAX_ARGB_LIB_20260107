@echo off
cd /d "%~dp0"
for %%I in ("%CD%") do set NAME=%%~nxI
set /p REPO=Paste GitHub repo URL (e.g. https://github.com/user/repo.git): 
if "%REPO%"=="" (echo No URL given. & pause & exit /b 1)
if not exist .git git init
git add -A
git commit -m "Initial commit: %NAME%"
git branch -M main
git remote remove origin 2>nul
git remote add origin %REPO%
git push -u origin main
echo.
echo Done. Check the output above for errors.
pause
