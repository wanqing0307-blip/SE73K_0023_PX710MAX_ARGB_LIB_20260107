@echo off
cd /d "%~dp0"
set /p REPO=Paste GitHub repo URL (e.g. https://github.com/user/repo.git): 
if "%REPO%"=="" (echo No URL given. & pause & exit /b 1)
git add -A
git commit -m "Initial commit: SE73K_0023_PX710MAX_ARGB_LIB_20260107"
git branch -M main
git remote remove origin 2>nul
git remote add origin %REPO%
git push -u origin main
echo.
echo Done. Check the output above for errors.
pause
